#ifndef TGCOMMANDCONFIGURATIONWIDGET_H
#define TGCOMMANDCONFIGURATIONWIDGET_H

#include <QWidget>

#include "tgbotmanager.h"
#include "selectitemstablewigget.h"

namespace Ui {
class TGCommandConfigurationWidget;
}

class TGCommandConfigurationWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TGCommandConfigurationWidget(TgBotManager& tg_bot_manager, QWidget *parent = nullptr);
    ~TGCommandConfigurationWidget();

public slots:
    void sl_change_object_data(const std::string &current, const std::string &previous);

signals:
    void sg_tgobject_changed(const std::string& cur, const std::string& prev);

private:
    Ui::TGCommandConfigurationWidget *ui;

    TgBotManager& tg_bot_manager_;
    SelectItemsTableWidget* message_to_command_ = nullptr;
    TGTriggerUserCommand* current_command_ = nullptr;

    void clear_command_data_();
    void save_to_current_command_();
    void load_data_from_command_(const std::string& id);
};

#endif // TGCOMMANDCONFIGURATIONWIDGET_H
