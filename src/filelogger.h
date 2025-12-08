#ifndef FILELOGGER_H
#define FILELOGGER_H

#include <QFile>
#include <QDateTime>
#include <QDebug>
#include <QtCore>

class FileLogger
{
public:
    FileLogger();
    FileLogger(int max_size);
    ~FileLogger();
    void SetMaxSize(int size);
    void LogText(const QString& text);
    void LogText(const QString&& text);
    bool SetLogName(QString fn);

    FileLogger& operator<<(QString& text);
    FileLogger& operator<<(QString&& text);

private:
    int max_size_ = 10000;
    QFile file_;
    QString file_name_ = "log.txt";

    void check_and_rename_();
};

#endif // FILELOGGER_H
