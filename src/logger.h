#ifndef LOGGER_H
#define LOGGER_H

#include <QFile>
#include <QDebug>
#include <QDateTime>
#include <QMutex>

class Logger
{
public:
    Logger() = default;
    explicit Logger(const QString& dir, const QString& name);
    static void Init(const QString& dir, const QString& name);
    static void Clear();
    static void SetMaxSize(unsigned int new_size);
    static void LogMessage(QtMsgType type, const QMessageLogContext & context, const QString & message);
    ~Logger();


private:
    static QFile* logfile_;
    static QtMessageHandler original_handler_;
    static const std::unordered_map<QtMsgType, QString> type_names_;

    static unsigned int max_size_;

    static QString file_name_;
    static QString app_directory_;

    static QMutex logger_mtx_;

    static void check_and_rename_();

};

#endif // LOGGER_H
