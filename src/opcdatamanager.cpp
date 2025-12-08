#include "opcdatamanager.h"

using namespace OPC_HELPER;

OPCDataManager::OPCDataManager()
{
    RestoreDataFromFile(true /*json*/);
    QTimer::singleShot(50, this, [this](){emit sg_set_text_state("OPC клиент остановлен");});
}

OPCDataManager::~OPCDataManager() {
    emit sg_stop_periodic_reading();
    request_stop_periodic_reading_ = true;
    QTimer::singleShot(TIME_WAITING_THREAD_*1000, this, [this](){period_reading_on_ = false; opc_threads_on_request_count_ = 0;});
    while(period_reading_on_ || opc_threads_on_request_count_ > 0) {
        QThread::currentThread()->eventDispatcher()->processEvents(QEventLoop::AllEvents);
    }

    QDir app_dir(qApp->applicationDirPath());
    bool autosave_flag = app_dir.exists("autosave");
    if(!autosave_flag) {
        autosave_flag = app_dir.mkdir("autosave");
        if(autosave_flag) {
            qInfo() << QString("Папка автосохранения конфигурационных файлов успешно создана.");
        } else {
            qWarning() << QString("Не удалось создать папку для автосохранения конфигурационных файлов.");
        }
    }

    if(autosave_flag) {
        QString path = QString("%1/autosave").arg(app_dir.absolutePath());
        qInfo() << QString("Автосохранение кофигурационных файлов OPCDataManager в папке %1").arg(path);
        SaveDataToFile(path);
    } else {
        qWarning() << "Автосохранение кофигурационных файлов OPCDataManager не удалось.";
    }

    qInfo() << "OPCDataManager деструктор.";
}

TAG_STATUS OPCDataManager::CheckTagReadState(QString tag_name) const {
    if(tag_names_.count(tag_name) == 0) {
        return TAG_STATUS::NOT_FOUND;
    }

    const QString* tag_ref = &(*tag_names_.find(tag_name));

    size_t id = tag_name_to_id_.at(tag_ref);

    bool rp;

    rp = opc_tags_id_value_controlled_.count(id) > 0;

    if(rp) {
        return TAG_STATUS::PERIODIC_READ;
    } else {
        return TAG_STATUS::NOT_FOUND;
    }
}

size_t OPCDataManager::DeleteTagFromPeriodicRead(QString tag_name) {
    if(tag_names_.count(tag_name) == 0) {
        return 0;
    }

    const QString* tag_ref = &(*tag_names_.find(tag_name));
    size_t id = tag_name_to_id_.at(tag_ref);

    if(opc_tags_id_value_controlled_.count(id) == 0) {
        return 0;
    }

    opc_tags_id_value_controlled_.erase(id);
    emit sg_periodic_list_changed();
    return id;
}

size_t OPCDataManager::GetTagId(QString tag_name) const
{
    if(tag_names_.count(tag_name)) {
        auto it = tag_names_.find(tag_name);
        return tag_name_to_id_.at(&(*it));
    }
    return -1;
}


size_t OPCDataManager::check_or_add_tag_(const QString& server_name, const QString& tag_name) {
    const QString* server_name_ptr;
    if(server_names_.count(server_name) == 0) {
        server_name_ptr = &(*server_names_.insert(server_name).first);
    } else {
        server_name_ptr = &(*server_names_.find(server_name));
    }
    if(tag_names_.count(tag_name) == 0) {
        auto [tag_ref, b] = tag_names_.insert(tag_name);
        tag_name_to_id_[&(*tag_ref)] = ++last_id_;
        opc_server_name_to_id_tags_set_[server_name_ptr].insert(last_id_);
        opc_tag_to_server_name_[last_id_] = server_name_ptr;
        id_tag_to_OPCTag_pointer_[last_id_] = std::make_shared<OPCTag>(server_name, tag_name);
        return last_id_;
    } else {
        const QString* tag_ref = &(*tag_names_.find(tag_name));
        size_t id = tag_name_to_id_.at(tag_ref);
        if(opc_tag_to_server_name_.at(id) != server_name_ptr) {
            const QString* last_sn = opc_tag_to_server_name_.at(id);
            opc_server_name_to_id_tags_set_[last_sn].erase(id);
            opc_tag_to_server_name_[id] = server_name_ptr;
            opc_server_name_to_id_tags_set_[server_name_ptr].insert(id);
        }
        return id;
    }
}


size_t OPCDataManager::AddTagToPeriodicReadList(QString server_name, QString tag_name) {
    size_t id = check_or_add_tag_(server_name, tag_name);
    opc_tags_id_value_controlled_.insert(id);
    emit sg_periodic_list_changed();
    return id;
}

void OPCDataManager::ClearMonitoringTags() {
    opc_tags_id_value_controlled_.clear();
    emit sg_periodic_list_changed();
}

std::vector<std::shared_ptr<OPCTag>> OPCDataManager::GetPeriodicTags() const {
    std::vector<std::shared_ptr<OPCTag>> ret_vec;
    for(const auto& it: opc_tags_id_value_controlled_) {
        ret_vec.push_back(id_tag_to_OPCTag_pointer_.at(it));
    }
    return ret_vec;
}

std::map<size_t, std::shared_ptr<OPCTag>> OPCDataManager::GetIdToTagPeriodicTags() const {
    std::map<size_t, std::shared_ptr<OPCTag>> ret_map;
    for(const auto& id: opc_tags_id_value_controlled_) {
        ret_map[id] = id_tag_to_OPCTag_pointer_.at(id);
    }
    return ret_map;
}

std::shared_ptr<OPCTag> OPCDataManager::GetOPCTag(size_t id) const {
    if(id_tag_to_OPCTag_pointer_.count(id) > 0) {
        return id_tag_to_OPCTag_pointer_.at(id);
    }
    return nullptr;
}

bool OPCDataManager::SaveDataToFile() {
    QString path = qApp->applicationDirPath();
    return SaveDataToFile(path);
}

bool OPCDataManager::SaveDataToFile(const QString& foldername) {
    QString filename = QString("%1/opctags.json").arg(foldername);
    QFile file(filename);
    if(file.open(QIODeviceBase::WriteOnly)) {
        QJsonArray json_ar;
        for(const auto& [name, id]: tag_name_to_id_) {
            QJsonObject t_obj;
            t_obj.insert("tag_name", *name);
            t_obj.insert("tag_comment", id_tag_to_OPCTag_pointer_.at(id)->GetCommentString());
            t_obj.insert("id", static_cast<int64_t>(id));
            t_obj.insert("server", *opc_tag_to_server_name_.at(id));
            t_obj.insert("periodic", opc_tags_id_value_controlled_.count(id) != 0);
            if(id_tag_to_OPCTag_pointer_.at(id)->GetGainOption().has_value()) {
                t_obj.insert("gain", id_tag_to_OPCTag_pointer_.at(id)->GetGainOption().value());
            }
            auto subst_map = id_tag_to_OPCTag_pointer_.at(id)->GetEnumStringValues();
            if(subst_map.size() > 0) {
                QJsonArray subst_array;
                for(const auto& [val, substitute_val]: subst_map) {
                    subst_array.append(QJsonObject{{val, substitute_val}});
                }
                t_obj.insert("substitute_values", std::move(subst_array));
            }
            json_ar.append(std::move(t_obj));
        }
        QJsonDocument json_doc(json_ar);
        bool res;
        res = file.write(json_doc.toJson()) != (-1);
        file.close();
        qInfo() << QString("OPCDataManager: сохранение данных в файл %1.").arg(filename);
        return res;
    }
    qWarning() << QString("OPCDataManager: не удалось сохраненить данные в файл %1.").arg(filename);
    return false;
}


bool OPCDataManager::RestoreDataFromFile(bool json) {
    if(json) {
        emit sg_send_message_to_console("OPCDataManager: чтение данных из файла opctags.json.");
        return restore_data_from_json_();
    } else {
        emit sg_send_message_to_console("OPCDataManager: чтение данных из файла opctags.dat.");
        return restore_data_from_dat_();
    }
}

bool OPCDataManager::PeriodicReadingOn() const
{
    return period_reading_on_;
}

int OPCDataManager::GetPeriodReading() const
{
    return opc_period_reading_;
}

void OPCDataManager::SetPeriodReading(int period)
{
    opc_period_reading_ = period;
    emit sg_set_period_reading(period);
    qInfo() << QString("OPCDataManager изменен период чтения, новое значение %1.").arg(period);
}

void OPCDataManager::ReadTagsOnce(std::vector<std::shared_ptr<OPCTag>>& tags)
{
    QThread* opc_thread_req = new QThread(this);
    QString thread_name = QString("ON_REQUEST_%1").arg(QDateTime::currentSecsSinceEpoch());
    opc_thread_req->setObjectName(thread_name);
    OPC_HELPER::OPCCLientOnRequest* opc_client_req = new OPC_HELPER::OPCCLientOnRequest(tags);
    QObject::connect(opc_client_req, SIGNAL(sg_send_message_to_console(QString)), this, SIGNAL(sg_send_message_to_console(QString)));
    QObject::connect(opc_client_req, SIGNAL(sg_opcclient_got_exception(QString)), this, SIGNAL(sg_send_message_to_console(QString)));
    QObject::connect(opc_thread_req, SIGNAL(started()), opc_client_req, SLOT(sl_process()));
    QObject::connect(opc_client_req, SIGNAL(sg_finished()), opc_thread_req, SLOT(quit()));
    QObject::connect(opc_thread_req, SIGNAL(finished()), opc_thread_req, SLOT(deleteLater()));
    QObject::connect(opc_thread_req, SIGNAL(finished()), this, SLOT(sl_thread_on_request_finished()));
    QObject::connect(opc_thread_req, SIGNAL(started()), this, SLOT(sl_thread_on_request_started()));
    QObject::connect(opc_client_req, SIGNAL(sg_reading_complete(size_t)), this, SLOT(sl_thread_onrequest_reading_complete(size_t)));

    opc_client_req->moveToThread(opc_thread_req);
    opc_thread_req->start();
    ++opc_threads_on_request_count_;

    QString log_message = QString("OPCDataManager: опрос тэгов по запросу. Новый поток %1").arg(thread_name);

    emit sg_send_message_to_console(log_message);
    qInfo() << log_message;
}

void OPCDataManager::StartPeriodReading()
{
    start_period_reading_();
    emit sg_set_text_state("OPC клиент: запуск");
}

void OPCDataManager::StartPeriodReading(int period)
{
    opc_period_reading_ = period;
    StartPeriodReading();
}

void OPCDataManager::StopPeriodReading()
{
    request_stop_periodic_reading_ = true;
    emit sg_set_text_state("OPC клиент: останов");
    emit sg_stop_periodic_reading();
}

bool OPCDataManager::restore_data_from_dat_() {
    QFile file("opctags.dat");
    if(file.open(QIODeviceBase::ReadOnly)) {
        QDataStream in(&file);
        QString header;
        size_t tags_number;
        int period;
        bool was_running;
        in >> header;
        in >> period;
        in >> was_running;
        in >> tags_number;
        for(size_t i = 0; i < tags_number; ++i) {
            QString tag_name;
            QString server_name;
            QString comment;
            bool read_on_request;
            bool read_periodically;
            size_t id;
            in >> id;
            in >> tag_name;
            in >> comment;
            in >> server_name;
            in >> read_on_request;
            in >> read_periodically;
            auto [tag_ref, b] = tag_names_.insert(tag_name);
            auto [server_ref, b2] = server_names_.insert(server_name);
            tag_name_to_id_[&(*tag_ref)] = id;
            id_tag_to_name_tag_[id] = &(*tag_ref);
            opc_server_name_to_id_tags_set_[&(*server_ref)].insert(id);
            opc_tag_to_server_name_[id] = &(*server_ref);
            id_tag_to_OPCTag_pointer_[id] = std::make_shared<OPCTag>(server_name, tag_name);
            id_tag_to_OPCTag_pointer_.at(id)->SetCommentString(comment);

            if(read_periodically) {
                opc_tags_id_value_controlled_.insert(id);
            }

            last_id_ = id > last_id_ ? id : last_id_;
        }

        std::vector<std::shared_ptr<OPCTag>> periodic_tags;
        for(const auto & it: opc_tags_id_value_controlled_) {
            periodic_tags.push_back(id_tag_to_OPCTag_pointer_.at(it));
        }
        return in.status() == 0;
    }
    return false;
}

bool OPCDataManager::OPCDataManager::restore_data_from_json_() {
    tag_names_.clear();
    server_names_.clear();
    tag_name_to_id_.clear();
    id_tag_to_name_tag_.clear();
    opc_server_name_to_id_tags_set_.clear();
    opc_tag_to_server_name_.clear();
    id_tag_to_OPCTag_pointer_.clear();
    opc_tags_id_value_controlled_.clear();

    QFile file("opctags.json");
    if(file.open(QIODeviceBase::ReadOnly)) {
        QJsonParseError json_error;
        QJsonDocument input_doc = QJsonDocument::fromJson(file.readAll(), &json_error);

        if(json_error.error != QJsonParseError::NoError) {
            QString log_message = QString("OPCDataManager: файл opctags.json поврежден.");
            emit sg_send_message_to_console(log_message);
            qInfo() << log_message;
            return false;
        }

        if(input_doc.isArray()) {
            for(int i = 0; i < input_doc.array().size(); ++i) {
                QJsonObject item_obj = input_doc.array().at(i).toObject();
                if((item_obj.contains("tag_name") && item_obj.value("tag_name").isString())
                    && (item_obj.contains("tag_comment") && item_obj.value("tag_comment").isString())
                    && (item_obj.contains("server") && item_obj.value("server").isString())
                    && (item_obj.contains("periodic") && item_obj.value("periodic").isBool())
                    && (item_obj.contains("id") && item_obj.value("id").isDouble())) {

                    auto [tag_ref, b] = tag_names_.insert(item_obj.value("tag_name").toString());
                    auto [server_ref, b2] = server_names_.insert(item_obj.value("server").toString());
                    size_t id = static_cast<size_t>(item_obj.value("id").toInteger());
                    tag_name_to_id_[&(*tag_ref)] = id;
                    id_tag_to_name_tag_[id] = &(*tag_ref);
                    opc_server_name_to_id_tags_set_[&(*server_ref)].insert(id);
                    opc_tag_to_server_name_[id] = &(*server_ref);
                    id_tag_to_OPCTag_pointer_[id] = std::make_shared<OPCTag>(item_obj.value("server").toString(), item_obj.value("tag_name").toString());
                    id_tag_to_OPCTag_pointer_.at(id)->SetCommentString(item_obj.value("tag_comment").toString());

                    if(item_obj.value("periodic").toBool()) {
                        opc_tags_id_value_controlled_.insert(id);
                    }

                    if(item_obj.contains("gain") && item_obj.value("gain").isDouble()) {
                        id_tag_to_OPCTag_pointer_.at(id)->SetGainOption(item_obj.value("gain").toDouble());
                    }
                    if(item_obj.contains("substitute_values") && item_obj.value("substitute_values").isArray()) {
                        QJsonArray subst_array = item_obj.value("substitute_values").toArray();
                        for(auto it = subst_array.begin(); it < subst_array.end(); ++it) {
                            for(const auto & key: (*it).toObject().keys()) {
                                id_tag_to_OPCTag_pointer_.at(id)->AddEnumStringValues(key, (*it).toObject().value(key).toString());
                            }
                        }
                    }

                    last_id_ = id > last_id_ ? id : last_id_;

                } else {
                    tag_names_.clear();
                    server_names_.clear();
                    tag_name_to_id_.clear();
                    id_tag_to_name_tag_.clear();
                    opc_server_name_to_id_tags_set_.clear();
                    opc_tag_to_server_name_.clear();
                    id_tag_to_OPCTag_pointer_.clear();
                    opc_tags_id_value_controlled_.clear();
                    return false;
                }
            }
        }
        return true;
    }
    return false;
}

void OPCDataManager::start_period_reading_()
{
    if(period_reading_on_) {
        emit sg_stop_periodic_reading();
        request_stop_periodic_reading_ = true;
        QTimer::singleShot(TIME_WAITING_THREAD_*1000, this, [this](){
            period_reading_on_ = false;
            opc_threads_on_request_count_ = 0;
            emit sg_periodic_finished();
        });
        while(period_reading_on_) {
            QThread::currentThread()->eventDispatcher()->processEvents(QEventLoop::AllEvents);
        }
    }

    QThread* opc_thread_per = new QThread(this);
    QString thread_name = QString("PERIODIC_%1").arg(QDateTime::currentSecsSinceEpoch());
    opc_thread_per->setObjectName(thread_name);
    std::vector<std::shared_ptr<OPCTag>> tags_vec = GetPeriodicTags();
    OPC_HELPER::OPCCLientPeriodic* opc_client_per = new OPC_HELPER::OPCCLientPeriodic(opc_period_reading_, tags_vec);
    QObject::connect(opc_client_per, SIGNAL(sg_send_message_to_console(QString)), this, SIGNAL(sg_send_message_to_console(QString)));
    QObject::connect(opc_client_per, SIGNAL(sg_opcclient_got_exception(QString)), this, SIGNAL(sg_send_message_to_console(QString)));
    QObject::connect(opc_client_per, SIGNAL(sg_opcclient_got_exception(QString)), this, SLOT(sl_thread_send_exception(QString)));
    QObject::connect(opc_client_per, SIGNAL(sg_server_error(QString,OPCSERVERSTATE)), this, SLOT(sl_thread_send_opc_status(QString,OPCSERVERSTATE)));
    QObject::connect(opc_thread_per, SIGNAL(started()), this, SLOT(sl_thread_periodic_started()));
    QObject::connect(opc_thread_per, SIGNAL(started()), opc_client_per, SLOT(sl_process()));
    QObject::connect(opc_client_per, SIGNAL(sg_finished()), opc_thread_per, SLOT(quit()));
    QObject::connect(opc_thread_per, SIGNAL(finished()), this, SLOT(sl_thread_periodic_finished()));
    QObject::connect(opc_thread_per, SIGNAL(finished()), opc_thread_per, SLOT(deleteLater()));
    QObject::connect(this, SIGNAL(sg_stop_periodic_reading()), opc_client_per, SLOT(sl_stop()));
    QObject::connect(opc_client_per, SIGNAL(sg_reading_error(size_t)), this, SLOT(sl_thread_read_error(size_t)));
    QObject::connect(opc_client_per, SIGNAL(sg_reading_complete(size_t)), this, SLOT(sl_thread_period_reading_complete(size_t)));

    opc_client_per->moveToThread(opc_thread_per);
    opc_thread_per->start();

    errors_periodic_opc_server_count_ = 0;
    errors_server_status_periodic_count_ = 0;

    QString log_message = QString("OPCDataManager: запуск клиента периодического опроса. Поток %1").arg(thread_name);
    emit sg_send_message_to_console(log_message);
    qInfo() << log_message;
    emit sg_set_text_state("OPC клиент: запуск");

}

void OPCDataManager::sl_thread_read_error(size_t n_tags)
{
    emit sg_reading_error();
    ++errors_periodic_opc_server_count_;
    QString message = QString("OPCDataManager: ошибка чтения тэгов прочитано %1 из %2. Ошибка подряд: %3")
                          .arg(n_tags)
                          .arg(GetPeriodicTags().size())
                          .arg(errors_periodic_opc_server_count_);
    emit sg_send_message_to_console(message);
    qWarning() << message;
    if(errors_periodic_opc_server_count_ > MAX_PERIODIC_ERRORS_COUNT && !request_stop_periodic_reading_) {
        emit sg_stop_periodic_reading();
        request_stop_periodic_reading_ = true;
        QTimer::singleShot(opc_period_reading_*2000, this, &OPCDataManager::start_period_reading_);
        emit sg_send_message_to_console(QString("OPCDataManager: таймер на рестарт клиента периодического опроса."));
        emit sg_set_text_state("OPC клиент: перезапуск по ошибке");
        qWarning() << QString("OPCDataManager: таймер на рестарт клиента периодического опроса.");
    }
}

void OPCDataManager::sl_thread_periodic_started()
{
    period_reading_on_ = true;
    request_stop_periodic_reading_ = false;
    emit sg_periodic_started();
    emit sg_set_text_state("OPC клиент в работе");
}

void OPCDataManager::sl_thread_on_request_started() {
}

void OPCDataManager::sl_thread_on_request_finished() {
    --opc_threads_on_request_count_;
}

void OPCDataManager::sl_thread_periodic_finished()
{
    period_reading_on_ = false;
    emit sg_periodic_finished();
    emit sg_set_text_state("OPC клиент остановлен");
    if(!request_stop_periodic_reading_) {
        QTimer::singleShot(opc_period_reading_*2000, this, &OPCDataManager::start_period_reading_);
        QString log_message = QString("OPCDataManager: неожиданное завершение потока клиента, перезапуск.");
        emit sg_send_message_to_console(log_message);
        qCritical() << log_message;
        emit sg_set_text_state("OPC клиент перезапуск");
    }
}

void OPCDataManager::sl_thread_send_exception(QString text)
{
    QString log_message = QString("OPCDataManager: получено исключение потока ОРС-клиента: %1").arg(text);
    emit sg_send_message_to_console(log_message);
    qCritical() << log_message;
    emit sg_stop_periodic_reading();
    emit sg_send_message_to_console(QString("OPCDataManager: таймер на рестарт клиента периодического опроса."));
    emit sg_set_text_state("OPC клиент перезапуск");
}

void OPCDataManager::sl_thread_send_opc_status(QString server, OPCSERVERSTATE state)
{
    QString ser_state;
    switch(state) {
    case OPC_STATUS_RUNNING: ser_state = "OPC_STATUS_RUNNING"; break;
    case OPC_STATUS_FAILED: ser_state = "OPC_STATUS_FAILED"; break;
    case OPC_STATUS_NOCONFIG: ser_state = "OPC_STATUS_NOCONFIG"; break;
    case OPC_STATUS_SUSPENDED: ser_state = "OPC_STATUS_SUSPENDED"; break;
    case OPC_STATUS_TEST: ser_state = "OPC_STATUS_TEST"; break;
    case OPC_STATUS_COMM_FAULT:	ser_state = "OPC_STATUS_COMM_FAULT"; break;
    default: ser_state = "UNKNOWN"; break;
    }

    QString log_message = QString("OPCDataManager: сервер %1 получено состояние %2").arg(server, ser_state);
    emit sg_send_message_to_console(log_message);
    qWarning() << log_message;

    if(state != OPC_STATUS_RUNNING) ++errors_server_status_periodic_count_;
    if(errors_server_status_periodic_count_ > MAX_PERIODIC_ERRORS_COUNT && !request_stop_periodic_reading_) {
        log_message = QString("OPCDataManager: перезапуск по максимальному количеству ошибок чтения.");
        emit sg_send_message_to_console(log_message);
        qWarning() << log_message;
        emit sg_stop_periodic_reading();
        request_stop_periodic_reading_ = true;
        QTimer::singleShot(opc_period_reading_*2000, this, &OPCDataManager::start_period_reading_);
    }
}

void OPCDataManager::sl_thread_period_reading_complete(size_t n_tags)
{
    errors_periodic_opc_server_count_ = 0;
    errors_server_status_periodic_count_ = 0;
    emit sg_reading_periodic_complete();
}

void OPCDataManager::sl_thread_onrequest_reading_complete(size_t n_tags)
{
    emit sg_reading_request_complete();
}
