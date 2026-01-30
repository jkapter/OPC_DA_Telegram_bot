#ifndef OPCCLIENTWORKER_H
#define OPCCLIENTWORKER_H

#include <QObject>
#include <QThread>

#include <deque>

#include "copcclient.h"
#include "opctag.h"



namespace OPC_HELPER {

class OPCClientTagBrowser: public QObject
{
    Q_OBJECT
public:
    OPCClientTagBrowser(const QString& hostname, const QString& server_name, std::vector<QString>& tags_ret_vec, QMutex& vec_lock,  QObject *parent = nullptr)
        : QObject(parent)
        , hostname_(hostname)
        , server_name_(server_name)
        , tags_list_(tags_ret_vec)
        , vec_lock_(vec_lock)
    {}

signals:
    void sg_browse_complete(size_t n_tags);
    void sg_get_part_tag_names_from_server(const QString& host, const QString& server, size_t n_tags);
    void sg_get_all_tag_names_from_server(const QString& host, const QString& server, size_t n_tags);
    void sg_finished();
    void sg_send_message_to_console(QString mes);
    void sg_opcclient_got_exception(QString what);

public slots:
    void sl_process();
    void sl_stop_browsing();

private:
    QString hostname_;
    QString server_name_;
    std::vector<QString>& tags_list_;
    QMutex& vec_lock_;
    bool request_interrupt_ = false;
};

class OPCClientInterface : public QObject
{
    Q_OBJECT
public:
    OPCClientInterface(std::vector<std::shared_ptr<OPCTag>>& tags, QObject *parent = nullptr);
    OPCClientInterface(const QString& hostname, std::vector<std::shared_ptr<OPCTag>>& tags, QObject *parent = nullptr);
    void SetTagsList(const std::vector<std::shared_ptr<OPCTag>>& tags);
    void SetTagsList(const std::vector<std::shared_ptr<OPCTag>>&& tags);

signals:
    void sg_reading_complete(size_t n_tags);
    void sg_reading_error(size_t n_tags);
    void sg_finished();
    void sg_send_message_to_console(QString mes);
    void sg_server_error(QString host, QString server, OPCSERVERSTATE st);
    void sg_opcclient_got_exception(QString what);
    void sg_writing_tags(size_t n_tags);

public slots:
    void virtual sl_process() = 0;
    void sl_stop();

protected:
    std::vector<std::shared_ptr<OPCTag>> tags_;
    std::set<QString> hostnames_;
    std::unordered_map<const QString*, std::set<QString>> hostname_to_server_names_;
    bool request_interrupt_ = false;
};

class OPCCLientPeriodic: public OPCClientInterface
{
    Q_OBJECT
public:
    explicit OPCCLientPeriodic(int period, std::vector<std::shared_ptr<OPCTag>>& tags, QObject *parent = nullptr);
    explicit OPCCLientPeriodic(const QString& hostname, int period, std::vector<std::shared_ptr<OPCTag>>& tags, QObject *parent = nullptr);
    void SetPeriod(int per);

public slots:
    void sl_process() override;
    void sl_set_period_reading(int period);

private:
    int period_ = 5;

};

class OPCCLientOnRequest: public OPCClientInterface
{
    Q_OBJECT
public:
    OPCCLientOnRequest(std::vector<std::shared_ptr<OPCTag>>& tags, QObject *parent = nullptr): OPCClientInterface(tags, parent) {}

public slots:
    void sl_process() override;
};


} //namespace

#endif // OPCCLIENTWORKER_H
