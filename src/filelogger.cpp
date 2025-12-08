#include "filelogger.h"

FileLogger::FileLogger(): FileLogger(100000)
{

}

FileLogger::FileLogger(int max_size)
    : max_size_(max_size)
{
    QString path = qApp->applicationDirPath() + "/" + file_name_;
    file_.setFileName(path);
    file_.open(QIODevice::Append);
    file_.write(QString("========================================================================\r\n").toLocal8Bit());
    file_.write(QString("%1\t-\tСтарт программы\r\n").arg(QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss")).toLocal8Bit());
}

FileLogger::~FileLogger()
{
    if(file_.isOpen()) {
        file_.write(QString("%1\t-\tОстановка программы\r\n").arg(QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss")).toLocal8Bit());
        file_.close();
    }
}

bool FileLogger::SetLogName(QString fn)
{
    file_name_ = fn;
    QString path = qApp->applicationDirPath() + "/" + fn;
    if(file_.isOpen()) {
        return file_.rename(path);
    } else {
        file_.setFileName(path);
        return file_.open(QIODevice::Append);
    }
    return false;
}

void FileLogger::LogText(const QString &text)
{
    if(file_.isOpen()) {
        check_and_rename_();
        file_.write(QString("%1\t-\t%2\r\n").arg(QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss"), text).toLocal8Bit());
    }
}

void FileLogger::LogText(const QString &&text)
{
    if(file_.isOpen()) {
        check_and_rename_();
        file_.write(QString("%1\t-\t%2\r\n").arg(QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss"), std::move(text)).toLocal8Bit());
    }
}

FileLogger &FileLogger::operator<<(QString &text)
{
    LogText(text);
    return *this;
}

FileLogger &FileLogger::operator<<(QString &&text)
{
    LogText(text);
    return *this;
}

void FileLogger::check_and_rename_()
{
    if(file_.size() > max_size_) {
        QString path = qApp->applicationDirPath() + "/" + file_name_;
        file_.rename(QString("%1/%2_%3").arg(qApp->applicationDirPath(), QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")), file_name_);
        file_.close();
        file_.setFileName(path);
        file_.open(QIODevice::Append);
    }

}
