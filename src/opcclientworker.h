#ifndef OPCCLIENTWORKER_H
#define OPCCLIENTWORKER_H

#include <QObject>
#include <QThread>

#include <deque>

#include "copcclient.h"
#include "opctag.h"

namespace OPC_HELPER {

class OPCClientInterface : public QObject
{
    Q_OBJECT
public:
    OPCClientInterface(std::vector<std::shared_ptr<OPCTag>>& tags, QObject *parent = nullptr);
    void SetTagsList(std::vector<std::shared_ptr<OPCTag>>& tags);
    void SetTagsList(std::vector<std::shared_ptr<OPCTag>>&& tags);

signals:
    void sg_reading_complete(size_t n_tags);
    void sg_reading_error(size_t n_tags);
    void sg_finished();
    void sg_send_message_to_console(QString mes);
    void sg_server_error(QString server, OPCSERVERSTATE st);
    void sg_opcclient_got_exception(QString what);
    void sg_writing_tags(size_t n_tags);

public slots:
    void virtual sl_process() = 0;
    void sl_stop();

protected:
    std::vector<std::shared_ptr<OPCTag>> tags_;
    std::set<QString> server_names_;
    bool request_interrupt_ = false;
};

class OPCCLientPeriodic: public OPCClientInterface
{
    Q_OBJECT
public:
    explicit OPCCLientPeriodic(int period, std::vector<std::shared_ptr<OPCTag>>& tags, QObject *parent = nullptr);
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
