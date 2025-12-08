#ifndef TGBOTTHREADWRAPPER_H
#define TGBOTTHREADWRAPPER_H

#include <QThread>
#include <QDebug>
#include <QAbstractEventDispatcher>

#include "tgbot/tgbot.h"

class TgBotWorker: public QObject
{
    Q_OBJECT

public:
    explicit TgBotWorker(TgBot::Bot* bot, QObject* parent = 0);
    ~TgBotWorker();

signals:
    void sg_emit_text_message(QString mes);
    void sg_emit_exception(QString what);
    void sg_finished();

public slots:
    void sl_process();
    void sl_stop_bot();

private:
    TgBot::Bot* bot_;
    bool request_stop_bot_ = false;
};

#endif // TGBOTTHREADWRAPPER_H
