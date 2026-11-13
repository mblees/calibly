// This file is part of calibly.
// Copyright (C) 2026 mblees
// SPDX-License-Identifier: GPL-3.0-only

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

#include "logfile.h"

class TestLogFile : public QObject
{
    Q_OBJECT

private slots:
    // Messages land in the file of the day, with time and level.
    void writesMessages()
    {
        QTemporaryDir dir;
        LogFile::install(dir.path());
        QCOMPARE(LogFile::directory(), dir.path());
        qWarning().noquote() << QStringLiteral("Bibliothek nicht gefunden: E:/Bücher");
        qInfo("gestartet");

        QFile file(QDir(dir.path()).filePath(LogFile::fileName(QDate::currentDate())));
        QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString text = QString::fromUtf8(file.readAll());
        QVERIFY2(text.contains(QStringLiteral(" W Bibliothek nicht gefunden: E:/Bücher\n")), qPrintable(text));
        QVERIFY(text.contains(QStringLiteral(" I gestartet\n")));
        QVERIFY(text.startsWith(QDate::currentDate().toString(Qt::ISODate)));
    }

    // Files older than 14 days go; newer ones and other files stay.
    void removesOldFiles()
    {
        QTemporaryDir dir;
        const QDate today(2026, 11, 12);
        const QStringList names = { LogFile::fileName(today), LogFile::fileName(today.addDays(-14)),
                                    LogFile::fileName(today.addDays(-15)), LogFile::fileName(today.addDays(-200)),
                                    QStringLiteral("notizen.txt") };
        for (const QString &name : names) {
            QFile file(QDir(dir.path()).filePath(name));
            QVERIFY(file.open(QIODevice::WriteOnly));
        }
        LogFile::removeOld(dir.path(), today);
        QStringList left = QDir(dir.path()).entryList(QDir::Files);
        left.sort();
        QStringList expected = { names.at(0), names.at(1), names.at(4) };
        expected.sort();
        QCOMPARE(left, expected);
    }
};

QTEST_GUILESS_MAIN(TestLogFile)
#include "tst_logfile.moc"
