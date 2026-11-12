#include "playercontroller.h"

#include <QAudioOutput>
#include <QCoreApplication>
#include <QDateTime>
#include <QFileInfo>
#include <QMediaPlayer>
#include <QSettings>
#include <QUrl>
#include <QVariantMap>

#include <algorithm>

#include "librarycontroller.h"

namespace {

constexpr int kSaveIntervalMs = 15000;     // save the position this often while playing
constexpr qint64 kRestartChapterMs = 3000; // "previous" within this time goes to the previous chapter
constexpr int kSleepFadeSeconds = 10;      // the sleep timer fades out over this time
constexpr qint64 kFinishedMarginMs = 1000; // this close to the end counts as finished
constexpr qint64 kPositionToleranceMs = 300; // smaller differences are not worth a seek

// The last audiobook, remembered together with its library folder; sample
// data (--mock, no library folder) is not remembered.
const QString kWorkKey = QStringLiteral("player/workId");
const QString kLibraryKey = QStringLiteral("player/library");
const QString kSpeedKey = QStringLiteral("player/speed");

// Index of the first audiobook format of the work, -1 if there is none.
int audiobookIndex(const Work &work)
{
    for (int i = 0; i < work.formats.size(); ++i) {
        if (work.formats.at(i).kind == Format::Kind::Audiobook)
            return i;
    }
    return -1;
}

} // namespace

PlayerController::PlayerController(LibraryController &library, QQmlEngine *engine, QObject *parent)
    : QObject(parent)
    , m_library(library)
    , m_engine(engine)
{
    Q_ASSERT(!s_instance);
    s_instance = this;

    m_player = new QMediaPlayer(this);
    m_output = new QAudioOutput(this);
    m_player->setAudioOutput(m_output);
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
    m_player->setPitchCompensation(true); // faster playback without higher voices
#endif

    connect(m_player, &QMediaPlayer::positionChanged, this, &PlayerController::onPlayerPosition);
    connect(m_player, &QMediaPlayer::mediaStatusChanged, this, &PlayerController::onMediaStatusChanged);
    connect(m_player, &QMediaPlayer::errorOccurred, this, [this](QMediaPlayer::Error error, const QString &text) {
        if (error == QMediaPlayer::NoError)
            return;
        qWarning("Player: %s", qPrintable(text));
        setError(PlaybackFailed);
        setPlaying(false);
    });

    m_saveTimer.setInterval(kSaveIntervalMs);
    connect(&m_saveTimer, &QTimer::timeout, this, &PlayerController::savePosition);
    m_sleepTimer.setInterval(1000);
    connect(&m_sleepTimer, &QTimer::timeout, this, &PlayerController::onSleepTick);
    connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, &PlayerController::savePosition);

    QSettings settings;
    setSpeed(settings.value(kSpeedKey, 1.0).toDouble());
    // The last audiobook, paused where it was.
    const QString root = m_library.libraryRoot();
    // Without a stored folder (saved before it was stored) the work belongs to this library.
    const QString lastWork = !root.isEmpty() && settings.value(kLibraryKey, root).toString() == root
                                 ? settings.value(kWorkKey).toString() : QString();
    if (!lastWork.isEmpty() && !load(lastWork))
        setError(NoError); // e.g. deleted meanwhile: just no player
}

PlayerController::~PlayerController()
{
    savePosition();
    s_instance = nullptr;
}

PlayerController *PlayerController::create(QQmlEngine *qmlEngine, QJSEngine *)
{
    Q_ASSERT(s_instance && s_instance->m_engine == qmlEngine);
    QJSEngine::setObjectOwnership(s_instance, QJSEngine::CppOwnership);
    return s_instance;
}

QVariantList PlayerController::chapters() const
{
    QVariantList list;
    for (const AudioTimeline::Entry &chapter : m_timeline.chapters()) {
        list.append(QVariantMap{ { QStringLiteral("title"), chapter.title },
                                 { QStringLiteral("start"), double(chapter.startMs) },
                                 { QStringLiteral("duration"), double(chapter.durationMs) } });
    }
    return list;
}

double PlayerController::chapterStart() const
{
    return m_chapterIndex >= 0 ? double(m_timeline.chapters().at(m_chapterIndex).startMs) : 0.0;
}

double PlayerController::chapterDuration() const
{
    return m_chapterIndex >= 0 ? double(m_timeline.chapters().at(m_chapterIndex).durationMs) : 0.0;
}

void PlayerController::setSpeed(double speed)
{
    speed = std::clamp(speed, 0.5, 3.0);
    if (qFuzzyCompare(speed, m_speed) && qFuzzyCompare(m_player->playbackRate(), speed))
        return;
    m_speed = speed;
    m_player->setPlaybackRate(speed);
    QSettings().setValue(kSpeedKey, speed);
    emit speedChanged();
}

bool PlayerController::load(const QString &workId)
{
    if (workId == m_workId && hasWork())
        return true;
    const WorkModel *works = m_library.works();
    const Work *work = works->workAt(works->rowOf(workId));
    const int index = work ? audiobookIndex(*work) : -1;
    if (index < 0)
        return false;

    const Format &format = work->formats.at(index);
    AudioTimeline timeline(format);
    const bool filesExist = !timeline.isEmpty()
                            && std::all_of(timeline.parts().cbegin(), timeline.parts().cend(),
                                           [](const AudioTimeline::Part &part) { return QFileInfo::exists(part.filePath); });
    m_library.setFileMissing(workId, index, !filesExist);
    if (!filesExist) {
        setError(FileMissing);
        return false;
    }

    unload();
    m_workId = workId;
    m_timeline = timeline;
    // A finished audiobook starts again from the beginning.
    const bool finished = format.progress >= 1.0 || format.positionMs >= m_timeline.durationMs() - kFinishedMarginMs;
    m_positionMs = finished ? 0 : std::clamp<qint64>(format.positionMs, 0, m_timeline.durationMs());
    m_savedPositionMs = m_positionMs;
    m_chapterIndex = m_timeline.chapterAt(m_positionMs);
    if (!m_library.libraryRoot().isEmpty()) {
        QSettings settings;
        settings.setValue(kWorkKey, workId);
        settings.setValue(kLibraryKey, m_library.libraryRoot());
    }
    setError(NoError);
    emit workChanged();
    emit chapterChanged();
    emit positionChanged();
    return true;
}

bool PlayerController::play(const QString &workId, int chapter)
{
    if (!load(workId))
        return false;
    if (chapter >= 0 && chapter < m_timeline.chapters().size())
        seek(double(m_timeline.chapters().at(chapter).startMs));

    // Remember for "recently opened" and "Continue".
    const WorkModel *works = m_library.works();
    if (const Work *work = works->workAt(works->rowOf(workId))) {
        Work opened = *work;
        opened.markOpened(audiobookIndex(opened), QDateTime::currentDateTime());
        m_library.saveWork(opened);
    }
    resume();
    return true;
}

void PlayerController::togglePlay()
{
    if (m_playing)
        pause();
    else
        resume();
}

void PlayerController::pause()
{
    if (!hasWork())
        return;
    if (m_player->playbackState() == QMediaPlayer::PlayingState)
        m_player->pause();
    setPlaying(false);
    savePosition();
}

void PlayerController::resume()
{
    if (!hasWork() || m_playing)
        return;
    if (m_positionMs >= m_timeline.durationMs() - kFinishedMarginMs)
        setPositionMs(0); // at the end: listen again from the start

    const int part = m_timeline.partAt(m_positionMs);
    const qint64 offset = m_positionMs - m_timeline.parts().at(part).startMs;
    setPlaying(true);
    if (part != m_part || m_player->mediaStatus() == QMediaPlayer::NoMedia
        || m_player->mediaStatus() == QMediaPlayer::EndOfMedia) {
        loadPart(part, offset); // plays once the file is loaded
        return;
    }
    if (m_pendingOffsetMs < 0 && qAbs(m_player->position() - offset) > kPositionToleranceMs)
        m_player->setPosition(offset);
    m_player->play();
}

void PlayerController::seek(double positionMs)
{
    if (!hasWork())
        return;
    const qint64 target = std::clamp<qint64>(qint64(positionMs), 0, m_timeline.durationMs());
    const int part = m_timeline.partAt(target);
    const qint64 offset = target - m_timeline.parts().at(part).startMs;
    if (part == m_part && m_pendingOffsetMs < 0 && m_player->mediaStatus() != QMediaPlayer::NoMedia)
        m_player->setPosition(offset);
    else if (part == m_part) {
        m_pendingOffsetMs = offset; // the file is still loading
        m_player->setPosition(offset);
    }
    else if (m_playing)
        loadPart(part, offset);
    else
        m_part = -1; // another file: loaded when playback resumes
    setPositionMs(target);
    if (!m_playing)
        savePosition();
}

void PlayerController::skip(int seconds)
{
    seek(double(m_positionMs + qint64(seconds) * 1000));
}

void PlayerController::playChapter(int index)
{
    if (index < 0 || index >= m_timeline.chapters().size())
        return;
    seek(double(m_timeline.chapters().at(index).startMs));
    resume();
}

void PlayerController::nextChapter()
{
    if (m_chapterIndex + 1 < m_timeline.chapters().size())
        seek(double(m_timeline.chapters().at(m_chapterIndex + 1).startMs));
}

void PlayerController::previousChapter()
{
    if (m_chapterIndex < 0)
        return;
    const qint64 start = m_timeline.chapters().at(m_chapterIndex).startMs;
    if (m_positionMs - start > kRestartChapterMs || m_chapterIndex == 0)
        seek(double(start));
    else
        seek(double(m_timeline.chapters().at(m_chapterIndex - 1).startMs));
}

void PlayerController::close()
{
    if (!hasWork())
        return;
    pause();
    unload();
    if (!m_library.libraryRoot().isEmpty())
        QSettings().remove(kWorkKey);
    emit workChanged();
    emit chapterChanged();
    emit positionChanged();
}

void PlayerController::setSleepTimer(int minutes)
{
    m_sleepAtChapterEnd = false;
    m_sleepSecondsLeft = std::max(0, minutes) * 60;
    m_output->setVolume(1.0);
    if (m_sleepSecondsLeft > 0)
        m_sleepTimer.start();
    else
        m_sleepTimer.stop();
    emit sleepTimerChanged();
}

void PlayerController::setSleepAtChapterEnd()
{
    setSleepTimer(0);
    m_sleepAtChapterEnd = true;
    emit sleepTimerChanged();
}

void PlayerController::savePosition()
{
    // Only what the player itself moved is stored: an audiobook that was
    // loaded but not played keeps its state in the library, e.g. "finished"
    // (it starts again at 0 here) or a state the user set meanwhile.
    if (!hasWork() || m_positionMs == m_savedPositionMs)
        return;
    const WorkModel *works = m_library.works();
    const Work *current = works->workAt(works->rowOf(m_workId));
    const int index = current ? audiobookIndex(*current) : -1;
    if (index < 0)
        return;

    Work work = *current;
    Format &format = work.formats[index];
    const qint64 duration = m_timeline.durationMs();
    const double progress = m_positionMs >= duration - kFinishedMarginMs ? 1.0
                          : duration > 0 ? double(m_positionMs) / double(duration) : 0.0;
    m_savedPositionMs = m_positionMs;
    if (format.positionMs == m_positionMs && qFuzzyCompare(format.progress + 1, progress + 1))
        return;
    format.positionMs = m_positionMs;
    format.progress = progress;
    m_library.saveWork(work);
}

void PlayerController::setPlaying(bool playing)
{
    if (playing == m_playing)
        return;
    m_playing = playing;
    if (playing)
        m_saveTimer.start();
    else
        m_saveTimer.stop();
    emit playingChanged();
}

void PlayerController::setError(Error error)
{
    if (error == m_error)
        return;
    m_error = error;
    emit errorChanged();
}

void PlayerController::setPositionMs(qint64 positionMs)
{
    positionMs = std::clamp<qint64>(positionMs, 0, m_timeline.durationMs());
    if (positionMs != m_positionMs) {
        m_positionMs = positionMs;
        emit positionChanged();
    }
    const int chapter = m_timeline.chapterAt(m_positionMs);
    if (chapter == m_chapterIndex)
        return;
    const bool reachedNext = m_playing && chapter == m_chapterIndex + 1;
    m_chapterIndex = chapter;
    emit chapterChanged();
    if (reachedNext && m_sleepAtChapterEnd) {
        m_sleepAtChapterEnd = false;
        emit sleepTimerChanged();
        pause();
    } else if (reachedNext) {
        savePosition();
    }
}

void PlayerController::loadPart(int part, qint64 offsetMs)
{
    // After a change of the source, the FFmpeg backend first reports
    // LoadedMedia too early (without LoadingMedia before it) and only then
    // really loads the file, which drops a position set in between. So the
    // offset is set once the file is really loaded (LoadedMedia after
    // LoadingMedia, at the latest BufferedMedia); until then the player's
    // positions are ignored. play() issued right away survives the reload.
    m_part = part;
    m_pendingOffsetMs = offsetMs;
    m_loadingSeen = false;
    m_player->setSource(QUrl::fromLocalFile(m_timeline.parts().at(part).filePath));
    if (m_playing)
        m_player->play();
}

void PlayerController::onMediaStatusChanged()
{
    const QMediaPlayer::MediaStatus status = m_player->mediaStatus();
    switch (status) {
    case QMediaPlayer::LoadingMedia:
        m_loadingSeen = true;
        break;
    case QMediaPlayer::LoadedMedia:
    case QMediaPlayer::BufferedMedia:
        if (m_pendingOffsetMs >= 0 && (m_loadingSeen || status == QMediaPlayer::BufferedMedia)) {
            const qint64 offset = m_pendingOffsetMs;
            m_pendingOffsetMs = -1;
            if (offset > 0)
                m_player->setPosition(offset);
            if (m_playing && m_player->playbackState() != QMediaPlayer::PlayingState)
                m_player->play();
        }
        break;
    case QMediaPlayer::EndOfMedia:
        m_pendingOffsetMs = -1;
        if (m_part + 1 < m_timeline.parts().size()) {
            // The next file continues. Changing the source from within this
            // signal confuses the FFmpeg backend, so it happens right after.
            const int next = m_part + 1;
            QMetaObject::invokeMethod(this, [this, next] {
                if (m_playing && m_part == next - 1)
                    loadPart(next, 0);
            }, Qt::QueuedConnection);
        } else {
            // Finished. Positions the player reports from now on (e.g. 0
            // after pause()) are not the listening position.
            m_part = -1;
            setPositionMs(m_timeline.durationMs());
            setPlaying(false);
            savePosition();
        }
        break;
    case QMediaPlayer::InvalidMedia:
        m_pendingOffsetMs = -1;
        setError(PlaybackFailed);
        setPlaying(false);
        break;
    default:
        break;
    }
}

void PlayerController::onPlayerPosition(qint64 partPositionMs)
{
    if (m_part < 0 || m_part >= m_timeline.parts().size())
        return;
    if (m_pendingOffsetMs >= 0)
        return; // still loading: e.g. 0 is not the listening position
    setPositionMs(m_timeline.parts().at(m_part).startMs + partPositionMs);
}

void PlayerController::onSleepTick()
{
    if (!m_playing)
        return; // only listening time counts
    --m_sleepSecondsLeft;
    if (m_sleepSecondsLeft <= kSleepFadeSeconds)
        m_output->setVolume(std::max(0, m_sleepSecondsLeft) / double(kSleepFadeSeconds));
    if (m_sleepSecondsLeft <= 0) {
        pause();
        setSleepTimer(0); // also restores the volume
        return;
    }
    emit sleepTimerChanged();
}

void PlayerController::unload()
{
    if (hasWork())
        savePosition();
    m_player->stop();
    m_player->setSource(QUrl());
    m_workId.clear();
    m_timeline = AudioTimeline();
    m_part = -1;
    m_pendingOffsetMs = -1;
    m_positionMs = 0;
    m_chapterIndex = -1;
    setPlaying(false);
}
