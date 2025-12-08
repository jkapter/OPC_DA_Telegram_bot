#ifndef TGBOTSETTINGSWIDGET_H
#define TGBOTSETTINGSWIDGET_H

#include <QWidget>

#include "tgbotmanager.h"
#include "plaintextconsole.h"
#include "hintinputdialog.h"

namespace Ui {
class TgBotSettingsWidget;
}

class TgBotSettingsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TgBotSettingsWidget(TgBotManager* bot_manager, QWidget *parent = nullptr);
    ~TgBotSettingsWidget();

signals:
    void sg_change_auto_restart_app_checkbox(Qt::CheckState state);

public slots:
    void sl_set_autorestart_app_checkbox(bool b);

private slots:
    void sl_bot_status_changed();
    void sl_pb_startbot_clicked();
    void sl_pb_stopbot_clicked();
    void sl_console_output_message(QString mes);
    void sl_cb_autorestartbot_statechanged(int arg1);
    void sl_tb_refreshusers_clicked();
    void sl_tb_verifyuser_clicked();
    void sl_tb_setadmin_clicked();
    void sl_tb_resetadmin_clicked();
    void sl_tb_sendmessage_clicked();
    void sl_get_text_message_to_clients(QString message);
    void sl_tb_sendtoall_clicked();
    void sl_tb_banuser_clicked();
    void sl_tab_botsettings_current_changed(int index);
    void sl_cb_addbotnametochannelmessages_check_state_changed(const Qt::CheckState &arg1);

protected:
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    Ui::TgBotSettingsWidget *ui;
    PlainTextConsole* console_;

    TgBotManager* bot_manager_ = nullptr;

    void users_table_update_();
    void show_message_input_dialog_();

    std::vector<size_t> temp_clients_;
};

#endif // TGBOTSETTINGSWIDGET_H
