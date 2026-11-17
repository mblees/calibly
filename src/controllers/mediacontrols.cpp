// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

#include "mediacontrols.h"

#include <QBuffer>
#include <QFile>
#include <QImage>
#include <QPointer>
#include <QWindow>

#include <functional>
#include <optional>

#include "librarycontroller.h"
#include "playercontroller.h"
#include "workmodel.h"

#ifdef Q_OS_WIN
#include <windows.h>
#include <roapi.h>
#include <shcore.h>
#include <shlwapi.h>
#include <systemmediatransportcontrolsinterop.h>
#include <windows.media.h>
#include <windows.storage.streams.h>
#include <winstring.h>
#include <wrl/client.h>
#endif

namespace {

// The cover for the overlay: small, as PNG.
QByteArray thumbnailData(const QString &coverPath)
{
    if (coverPath.isEmpty())
        return QByteArray();
    QFile file(coverPath);
    if (!file.open(QIODevice::ReadOnly))
        return QByteArray();
    QImage image = QImage::fromData(file.readAll());
    if (image.isNull())
        return QByteArray();
    constexpr int maxSide = 512;
    if (image.width() > maxSide || image.height() > maxSide)
        image = image.scaled(maxSide, maxSide, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    QByteArray data;
    QBuffer buffer(&data);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    return data;
}

} // namespace

#ifdef Q_OS_WIN

using Microsoft::WRL::ComPtr;
namespace Media = ABI::Windows::Media;
namespace Streams = ABI::Windows::Storage::Streams;

namespace {

// Windows.Storage.Streams.IRandomAccessStreamReferenceStatics: only
// forward-declared in the MinGW headers.
struct DECLSPEC_NOVTABLE StreamReferenceStatics : public IInspectable
{
    // The results are IRandomAccessStreamReference, also only forward-declared:
    // kept as IInspectable.
    virtual HRESULT STDMETHODCALLTYPE CreateFromFile(IInspectable *file, IInspectable **result) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateFromUri(IInspectable *uri, IInspectable **result) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateFromStream(Streams::IRandomAccessStream *stream, IInspectable **result) = 0;
};
constexpr IID kStreamReferenceStaticsIid = { 0x857309dc, 0x3fbf, 0x4e7d, { 0x98, 0x6f, 0xef, 0x3b, 0x1a, 0x07, 0xa9, 0x64 } };

class HString
{
public:
    explicit HString(const QString &text)
    {
        WindowsCreateString(reinterpret_cast<const wchar_t *>(text.utf16()), UINT32(text.size()), &m_string);
    }
    ~HString() { WindowsDeleteString(m_string); }
    HString(const HString &) = delete;
    HString &operator=(const HString &) = delete;
    operator HSTRING() const { return m_string; }

private:
    HSTRING m_string = nullptr;
};

using ButtonHandler = ABI::Windows::Foundation::ITypedEventHandler<Media::SystemMediaTransportControls *,
                                                                    Media::SystemMediaTransportControlsButtonPressedEventArgs *>;

// Receives the button presses; Windows calls it on a thread of its own.
class ButtonCallback final : public ButtonHandler
{
public:
    explicit ButtonCallback(std::function<void(Media::SystemMediaTransportControlsButton)> callback)
        : m_callback(std::move(callback))
    {
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **object) override
    {
        if (!object)
            return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ButtonHandler) || riid == __uuidof(IAgileObject)) {
            *object = static_cast<ButtonHandler *>(this);
            AddRef();
            return S_OK;
        }
        *object = nullptr;
        return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return ULONG(InterlockedIncrement(&m_refs)); }
    ULONG STDMETHODCALLTYPE Release() override
    {
        const LONG refs = InterlockedDecrement(&m_refs);
        if (refs == 0)
            delete this;
        return ULONG(refs);
    }
    HRESULT STDMETHODCALLTYPE Invoke(Media::ISystemMediaTransportControls *,
                                     Media::ISystemMediaTransportControlsButtonPressedEventArgs *args) override
    {
        Media::SystemMediaTransportControlsButton button;
        if (args && SUCCEEDED(args->get_Button(&button)))
            m_callback(button);
        return S_OK;
    }

private:
    ~ButtonCallback() = default;

    LONG m_refs = 1;
    std::function<void(Media::SystemMediaTransportControlsButton)> m_callback;
};

std::optional<MediaControls::Button> buttonOf(Media::SystemMediaTransportControlsButton button)
{
    switch (button) {
    case Media::SystemMediaTransportControlsButton_Play: return MediaControls::Button::Play;
    case Media::SystemMediaTransportControlsButton_Pause: return MediaControls::Button::Pause;
    case Media::SystemMediaTransportControlsButton_Stop: return MediaControls::Button::Stop;
    case Media::SystemMediaTransportControlsButton_Next: return MediaControls::Button::Next;
    case Media::SystemMediaTransportControlsButton_Previous: return MediaControls::Button::Previous;
    case Media::SystemMediaTransportControlsButton_FastForward: return MediaControls::Button::FastForward;
    case Media::SystemMediaTransportControlsButton_Rewind: return MediaControls::Button::Rewind;
    default: return std::nullopt;
    }
}

} // namespace

class MediaControls::Platform
{
public:
    explicit Platform(MediaControls *owner)
        : m_owner(owner)
    {
    }

    ~Platform()
    {
        if (m_controls) {
            if (m_token.value)
                m_controls->remove_ButtonPressed(m_token);
            m_controls->put_IsEnabled(false);
        }
    }

    bool attach(HWND window)
    {
        ComPtr<ISystemMediaTransportControlsInterop> interop;
        HRESULT result = RoGetActivationFactory(HString(QString::fromWCharArray(RuntimeClass_Windows_Media_SystemMediaTransportControls)),
                                                IID_PPV_ARGS(&interop));
        if (SUCCEEDED(result))
            result = interop->GetForWindow(window, IID_PPV_ARGS(&m_controls));
        if (FAILED(result) || !m_controls) {
            qWarning("Media controls: not available (0x%08lx)", static_cast<unsigned long>(result));
            m_controls.Reset();
            return false;
        }
        m_controls->get_DisplayUpdater(&m_updater);

        // Back to the thread of the owner; it may be gone by then.
        const QPointer<MediaControls> owner(m_owner);
        auto *callback = new ButtonCallback([owner](Media::SystemMediaTransportControlsButton pressed) {
            const std::optional<Button> button = buttonOf(pressed);
            if (!button)
                return;
            QMetaObject::invokeMethod(owner, [owner, button] {
                if (owner)
                    owner->handleButton(*button);
            }, Qt::QueuedConnection);
        });
        m_controls->add_ButtonPressed(callback, &m_token);
        callback->Release();

        m_controls->put_IsPlayEnabled(true);
        m_controls->put_IsPauseEnabled(true);
        m_controls->put_IsStopEnabled(true);
        m_controls->put_IsNextEnabled(true);
        m_controls->put_IsPreviousEnabled(true);
        m_controls->put_IsFastForwardEnabled(true);
        m_controls->put_IsRewindEnabled(true);
        m_controls->put_IsEnabled(false);
        return true;
    }

    void show(const Display &display, const Display &before)
    {
        if (!m_controls)
            return;
        if (!display.enabled) {
            m_controls->put_PlaybackStatus(Media::MediaPlaybackStatus_Closed);
            m_controls->put_IsEnabled(false);
            return;
        }
        m_controls->put_IsEnabled(true);
        m_controls->put_PlaybackStatus(display.status == Status::Playing ? Media::MediaPlaybackStatus_Playing
                                                                         : Media::MediaPlaybackStatus_Paused);
        if (!m_updater || (before.enabled && display.title == before.title && display.artist == before.artist
                           && display.album == before.album && display.coverPath == before.coverPath))
            return;

        m_updater->put_Type(Media::MediaPlaybackType_Music);
        ComPtr<Media::IMusicDisplayProperties> music;
        if (SUCCEEDED(m_updater->get_MusicProperties(&music)) && music) {
            music->put_Title(HString(display.title));
            music->put_Artist(HString(display.artist));
            ComPtr<Media::IMusicDisplayProperties2> music2;
            if (SUCCEEDED(music.As(&music2)))
                music2->put_AlbumTitle(HString(display.album));
        }
        if (!before.enabled || display.coverPath != before.coverPath)
            m_updater->put_Thumbnail(reinterpret_cast<Streams::IRandomAccessStreamReference *>(
                thumbnail(display.coverPath).Get()));
        m_updater->Update();
    }

    // The cover from memory, so its file stays free (the metadata editor
    // may replace it while the audiobook plays).
    static ComPtr<IInspectable> thumbnail(const QString &coverPath)
    {
        ComPtr<IInspectable> reference;
        const QByteArray data = thumbnailData(coverPath);
        if (data.isEmpty())
            return reference;
        ComPtr<IStream> stream;
        stream.Attach(SHCreateMemStream(reinterpret_cast<const BYTE *>(data.constData()), UINT(data.size())));
        ComPtr<Streams::IRandomAccessStream> randomAccess;
        ComPtr<StreamReferenceStatics> statics;
        if (!stream
            || FAILED(CreateRandomAccessStreamOverStream(stream.Get(), BSOS_DEFAULT, IID_PPV_ARGS(&randomAccess)))
            || FAILED(RoGetActivationFactory(
                HString(QString::fromWCharArray(RuntimeClass_Windows_Storage_Streams_RandomAccessStreamReference)),
                kStreamReferenceStaticsIid, reinterpret_cast<void **>(statics.GetAddressOf())))
            || FAILED(statics->CreateFromStream(randomAccess.Get(), &reference))) {
            qWarning("Media controls: no cover for the overlay");
            reference.Reset();
        }
        return reference;
    }

private:
    MediaControls *m_owner;
    ComPtr<Media::ISystemMediaTransportControls> m_controls;
    ComPtr<Media::ISystemMediaTransportControlsDisplayUpdater> m_updater;
    EventRegistrationToken m_token = {};
};

#else

// No media controls on this system.
class MediaControls::Platform
{
public:
    explicit Platform(MediaControls *) {}
    void show(const Display &, const Display &) {}
};

#endif

MediaControls::MediaControls(PlayerController &player, LibraryController &library, QObject *parent)
    : QObject(parent)
    , m_player(player)
    , m_library(library)
{
    connect(&m_player, &PlayerController::workChanged, this, &MediaControls::update);
    connect(&m_player, &PlayerController::chapterChanged, this, &MediaControls::update);
    connect(&m_player, &PlayerController::playingChanged, this, [this] {
        if (m_player.playing())
            m_played = true;
        update();
    });
    // Title, authors or cover edited.
    connect(m_library.works(), &QAbstractItemModel::dataChanged, this, &MediaControls::update);
    connect(m_library.works(), &QAbstractItemModel::modelReset, this, &MediaControls::update);
    update();
}

MediaControls::~MediaControls() = default;

bool MediaControls::attach(QWindow *window)
{
#ifdef Q_OS_WIN
    if (!window || m_platform)
        return m_platform != nullptr;
    auto platform = std::make_unique<Platform>(this);
    if (!platform->attach(reinterpret_cast<HWND>(window->winId())))
        return false;
    m_platform = std::move(platform);
    m_platform->show(m_shown, Display());
    return true;
#else
    Q_UNUSED(window);
    return false;
#endif
}

bool MediaControls::createsThumbnail(const QString &coverPath)
{
#ifdef Q_OS_WIN
    return Platform::thumbnail(coverPath) != nullptr;
#else
    Q_UNUSED(coverPath);
    return false;
#endif
}

MediaControls::Display MediaControls::display() const
{
    Display display;
    if (!m_played || !m_player.hasWork())
        return display;
    const WorkModel *works = m_library.works();
    const Work *work = works ? works->workAt(works->rowOf(m_player.workId())) : nullptr;
    if (!work)
        return display;
    display.enabled = true;
    display.status = m_player.playing() ? Status::Playing : Status::Paused;
    display.title = work->title;
    display.artist = work->authors.join(QStringLiteral(", "));
    const int chapter = m_player.chapterIndex();
    const QVariantList chapters = m_player.chapters();
    if (chapter >= 0 && chapter < chapters.size()) {
        const QString title = chapters.at(chapter).toMap().value(QStringLiteral("title")).toString();
        display.album = title.isEmpty() ? tr("Kapitel %1").arg(chapter + 1) : title;
    }
    display.coverPath = work->coverPath(Format::Kind::Audiobook);
    return display;
}

void MediaControls::handleButton(Button button)
{
    if (!m_player.hasWork())
        return;
    switch (button) {
    case Button::Play:
        m_player.resume();
        break;
    case Button::Pause:
    case Button::Stop:
        m_player.pause();
        break;
    case Button::Next:
    case Button::FastForward:
        m_player.skip(kSkipSeconds);
        break;
    case Button::Previous:
    case Button::Rewind:
        m_player.skip(-kSkipSeconds);
        break;
    }
}

void MediaControls::update()
{
    const Display display = this->display();
    if (display == m_shown)
        return;
    const Display before = m_shown;
    m_shown = display;
    if (m_platform)
        m_platform->show(m_shown, before);
    emit displayChanged();
}
