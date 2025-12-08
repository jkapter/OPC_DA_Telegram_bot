#include "opcclientworker.h"

using namespace OPC_HELPER;

OPCClientInterface::OPCClientInterface(std::vector<std::shared_ptr<OPCTag>>& tags, QObject *parent)
    : QObject(parent)
    , tags_(tags)
{
    for(const auto& tag: tags_) {
        server_names_.insert(tag->GetServerName());
    }

    qInfo() << QString("ОРС клиент поток [%1]: новый экземпляр.").arg(QThread::currentThread()->objectName());
}

void OPCClientInterface::sl_stop()
{
    request_interrupt_ = true;
}

void OPCClientInterface::SetTagsList(std::vector<std::shared_ptr<OPCTag>>& tags) {
    tags_.clear();
    tags_.reserve(tags.size());
    tags_ = tags;
    server_names_.clear();

    for(const auto& tag: tags_) {
        server_names_.insert(tag->GetServerName());
    }

    qInfo() << QString("ОРС клиент поток [%1]: добавлено %2 тэгов для чтения.").arg(QThread::currentThread()->objectName()).arg(tags.size());
}

void OPCClientInterface::SetTagsList(std::vector<std::shared_ptr<OPCTag> > &&tags)
{
    auto vec = std::move(tags);
    SetTagsList(vec);
}

//=========================================================
//======= P E R I O D I C =================================
//=========================================================

OPCCLientPeriodic::OPCCLientPeriodic(int period, std::vector<std::shared_ptr<OPCTag>>& tags, QObject *parent)
    : OPCClientInterface(tags, parent)
{
    period_ = period > 1 ? period : 1;
}

void OPCCLientPeriodic::SetPeriod(int per) {
    period_ = per > 1 ? per : 1;
}

void OPCCLientPeriodic::sl_process()
{
    std::unique_ptr<OPC_HELPER::COPCClient> opc_client_ = std::make_unique<OPC_HELPER::COPCClient>();
    QObject::connect(opc_client_.get(), SIGNAL(sg_send_message_to_console(QString)), this, SIGNAL(sg_send_message_to_console(QString)));

    std::chrono::time_point<std::chrono::steady_clock> start_tick;

    try {
        opc_client_->AddTags(tags_);
        while(!request_interrupt_) {
            start_tick = std::chrono::steady_clock::now();
            size_t res = opc_client_->WriteTags();
            if(res > 0) {
                QString log_message = QString("ОРС клиент поток [%1]: записано %2 тэгов.").arg(QThread::currentThread()->objectName()).arg(res);

                emit sg_send_message_to_console(log_message);
                qInfo() << log_message;
                emit sg_writing_tags(res);
            }

            res = opc_client_->ReadTags();
            if(res == tags_.size()) {
                emit sg_reading_complete(res);
            } else {
                emit sg_reading_error(res);
                qWarning() << QString("ОРС клиент поток [%1]: ошибка чтения, прочитано %2 из %3 тэгов.")
                                  .arg(QThread::currentThread()->objectName())
                                  .arg(res)
                                  .arg(static_cast<quint64>(tags_.size()));
            }

            for(const auto& name: server_names_) {
                auto s_status = opc_client_->GetServerStatus(name);
                if(!s_status.has_value() || s_status.value().dwServerState != OPC_STATUS_RUNNING) {
                    emit sg_server_error(name, s_status.has_value() ? s_status.value().dwServerState : OPC_STATUS_COMM_FAULT);
                    QString log_message = QString("Поток ОРС-клиента [%1]: ошибка сервера %2 : %3")
                                                    .arg(QThread::currentThread()->objectName(), name)
                                                    .arg(s_status.has_value() ? static_cast<int>(s_status.value().dwServerState) : OPC_STATUS_COMM_FAULT);
                    emit sg_send_message_to_console(log_message);
                    qWarning() << log_message;
                }
            }

            auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_tick);
            while(!request_interrupt_ && duration_ms < std::chrono::milliseconds{period_*1000}) {
                QThread::currentThread()->eventDispatcher()->processEvents(QEventLoop::AllEvents);
                QThread::currentThread()->sleep(std::chrono::nanoseconds{100000});
                duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_tick);
            }
        }
    } catch (std::exception& e) {
        emit sg_opcclient_got_exception(QString::fromStdString(e.what()));
        emit sg_finished();
        if(request_interrupt_) request_interrupt_ = false;
    }
    emit sg_finished();
}

void OPCCLientPeriodic::sl_set_period_reading(int period)
{
    period_ = period;
}

//============================================================
//========= O N    R E Q U E S T =============================
//============================================================

void OPCCLientOnRequest::sl_process() {
    std::unique_ptr<OPC_HELPER::COPCClient> opc_client_ = std::make_unique<OPC_HELPER::COPCClient>();
    QObject::connect(opc_client_.get(), SIGNAL(sg_send_message_to_console(QString)), this, SIGNAL(sg_send_message_to_console(QString)));
    bool reading_complete = false;
    size_t res = 0;
    try {
        opc_client_->AddTags(tags_);
        opc_client_->WriteTags();
        res = opc_client_->ReadTags();
        reading_complete = res == tags_.size();

        for(const auto& name: server_names_) {
            auto s_status = opc_client_->GetServerStatus(name);
            if(!s_status.has_value() || s_status.value().dwServerState != OPC_STATUS_RUNNING) {
                emit sg_server_error(name, s_status.has_value() ? s_status.value().dwServerState : OPC_STATUS_COMM_FAULT);
                QString log_message = QString("Поток ОРС-клиента [%1]: ошибка сервера %2 : %3")
                                          .arg(QThread::currentThread()->objectName(), name)
                                          .arg(static_cast<int>(s_status.has_value() ? s_status.value().dwServerState : OPC_STATUS_COMM_FAULT));
                emit sg_send_message_to_console(log_message);
                qWarning() << log_message;
            }
        }

        tags_.clear();
        opc_client_->ClearTags();

    } catch (std::exception& e) {
        emit sg_opcclient_got_exception(QString::fromStdString(e.what()));
        emit sg_finished();
    }

    if(reading_complete) {
        emit sg_reading_complete(res);
    } else {
        emit sg_reading_error(res);
    }
    emit sg_finished();
}
