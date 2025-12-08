#include "tgbotthreadwrapper.h"

TgBotWorker::TgBotWorker(TgBot::Bot *bot, QObject *parent)
    : QObject(parent)
    , bot_(bot)
{
    qInfo() << QString("TgBotWorker конструктор.");
}

TgBotWorker::~TgBotWorker()
{
    qInfo() << QString("TgBotWorker деструктор.");
}

void TgBotWorker::sl_process()
{
    if(!bot_) {
        emit sg_finished();
        return;
    }

    emit sg_emit_text_message("Старт потока Бота.");
    qInfo() << QString("Старт потока бота.");

    try{
        bot_->getApi().getMyCommands();
    } catch    (std::exception &e) {
        emit sg_emit_exception(QString::fromStdString(e.what()));
        qCritical() << QString("Получено исключение процесса бота: %1").arg(QString::fromStdString(e.what()));
        emit sg_finished();
    }


    try {
        bot_->getApi().deleteWebhook();
        TgBot::TgLongPoll longPoll(*bot_, 1, 2, nullptr);
        emit sg_emit_text_message(QString("Бот: старт процесса"));
        qInfo() << QString("Телеграм бот: старт работы бота.");
        while (bot_ && !request_stop_bot_) {
            longPoll.start();
            QThread::currentThread()->eventDispatcher()->processEvents(QEventLoop::AllEvents);
        }
    } catch (std::exception& e) {
        emit sg_emit_text_message(QString("Получено исключение : %1").arg(e.what()));
        qCritical() << QString("Получено исключение процесса бота: %1").arg(QString::fromStdString(e.what()));
        emit sg_finished();
    }
    emit sg_finished();
}

void TgBotWorker::sl_stop_bot()
{
    request_stop_bot_ = true;
    qInfo() << QString("Запрос на остановку бота.");
}
