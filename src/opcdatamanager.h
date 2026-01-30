#ifndef OPCDATAMANAGER_H
#define OPCDATAMANAGER_H

#include <QObject>
#include <QRegularExpression>
#include <set>

#include "copcclient.h"
#include "opctag.h"
#include "opcclientworker.h"

namespace OPC_HELPER {

enum class TAG_STATUS {
    NOT_FOUND,
    PERIODIC_READ,
    READ_ON_REQUEST,
    READ_BOTH
};

struct OPC_PERIODIC_THREAD_STATUS {
    QString opc_hostname;
    int errors_periodic_opc_server_count = 0;
    int errors_server_status_periodic_count = 0;
    int opc_period_reading = 2;
    bool period_reading_on = false;
    bool request_stop_periodic_reading = false;
};

class OPCDataManager: public QObject
{
    Q_OBJECT
public:
    OPCDataManager();
    ~OPCDataManager();
    TAG_STATUS CheckTagReadState(const QString& tag_name) const;
    size_t AddTagToPeriodicReadList(const QString& hostname, const QString& server_name, const QString& tag_name);
    size_t AddTagToPeriodicReadList(const QString& full_tag_name);
    size_t DeleteTagFromPeriodicRead(const QString& tag_name);
    size_t GetTagId(const QString& tag_name) const;
    size_t GetTagId(const QString& hostname, const QString& server_name, const QString& tag_name) const;
    void ClearMonitoringTags();
    std::vector<std::shared_ptr<OPCTag>> GetPeriodicTags() const;
    std::vector<std::shared_ptr<OPCTag>> GetPeriodicTags(const QString& hostname) const;
    std::map<size_t, std::shared_ptr<OPCTag>> GetIdToTagPeriodicTags() const;
    std::shared_ptr<OPCTag> GetOPCTag(size_t id) const;
    const std::set<QString>& GetHostNames() const;
    bool SaveDataToFile();
    bool SaveDataToFile(const QString& folderpath);
    bool RestoreDataFromFile();
    bool PeriodicReadingOn() const;
    int GetPeriodReading() const;
    void SetPeriodReading(int period);

    void ReadTagsOnce(std::vector<std::shared_ptr<OPCTag>>& tags);
    void StartPeriodReading();
    void StartPeriodReading(int period);
    void StopPeriodReading();

signals:
    void sg_send_message_to_console(QString mes);
    void sg_set_text_state(QString text);
    void sg_reading_periodic_complete();
    void sg_reading_request_complete();
    void sg_reading_error();
    void sg_periodic_started();
    void sg_periodic_finished();
    void sg_stop_periodic_reading();
    void sg_set_period_reading(int period);
    void sg_periodic_list_changed();

private slots:
    void sl_thread_read_error(size_t n_tags);
    void sl_thread_periodic_started();
    void sl_thread_periodic_finished();
    void sl_thread_on_request_started();
    void sl_thread_on_request_finished();
    void sl_thread_send_exception(QString text);
    void sl_thread_send_opc_status(QString host, QString server, OPCSERVERSTATE state);
    void sl_thread_period_reading_complete(size_t n_tags);
    void sl_thread_onrequest_reading_complete(size_t n_tags);

private:
    const int TIME_WAITING_THREAD_ = 600;
    const int MAX_PERIODIC_ERRORS_COUNT = 10;
    QRegularExpression tag_name_check_re_;
    size_t last_id_ = 0;
    bool period_reading_on_ = false;
    bool request_stop_periodic_reading_ = false;
    int opc_threads_on_request_count_ = 0;
    std::set<QString> tag_names_;
    std::set<QString> hostnames_;
    std::unordered_map<const QString*, std::set<QString>> host_to_opc_servers_;
    std::unordered_map<const QString*, const QString*> opc_server_to_host_;
    std::unordered_map<const QString*, size_t> tag_name_to_id_;
    std::unordered_map<size_t, const QString*> id_tag_to_name_tag_;
    std::unordered_map<const QString*, std::set<size_t>> opc_server_name_to_id_tags_set_;
    std::unordered_map<size_t, const QString*> opc_tag_to_server_name_;
    std::unordered_map<size_t, std::shared_ptr<OPCTag>> id_tag_to_OPCTag_pointer_;
    std::set<size_t> opc_tags_id_value_controlled_;

    size_t check_or_add_tag_(const QString& hostname, const QString& server_name, const QString& tag_name);
    size_t check_or_add_tag_(const QString& full_tag_name);

    int errors_periodic_opc_server_count_ = 0;
    int errors_server_status_periodic_count_ = 0;
    int opc_period_reading_ = 2;

    bool restore_data_from_json_();
    void start_period_reading_();
};

} // namespace

#endif // OPCDATAMANAGER_H
