#ifndef TGINLINEBUTTONSCONFIGURATIONWIDGET_H
#define TGINLINEBUTTONSCONFIGURATIONWIDGET_H

#include <QWidget>

#include "tgbotmanager.h"
#include "selectitemstablewigget.h"

namespace Ui {
class TGInlineButtonsConfigurationWidget;
}

class TGInlineButtonsConfigurationWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TGInlineButtonsConfigurationWidget(TgBotManager& tg_bot_manager, QWidget *parent = nullptr);
    ~TGInlineButtonsConfigurationWidget();

public slots:
    void sl_change_object_data(const std::string &current, const std::string &previous);

signals:
    void sg_tgobject_changed(const std::string& cur, const std::string& prev);

private:
    Ui::TGInlineButtonsConfigurationWidget *ui;

    TgBotManager& tg_bot_manager_;
    SelectItemsTableWidget* message_to_button_ = nullptr;
    TGButtonWCallback* current_button_ = nullptr;

    void clear_button_data_();
    void save_to_current_button_();
    void load_data_from_button_(const std::string& id);
};

#endif // TGINLINEBUTTONSCONFIGURATIONWIDGET_H
