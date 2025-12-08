#ifndef TGMESSAGECONFIGURATIONWIDGET_H
#define TGMESSAGECONFIGURATIONWIDGET_H

#include <QWidget>

#include "selectitemstablewigget.h"

namespace Ui {
class TGMessageConfigurationWidget;
}

class TGMessageConfigurationWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TGMessageConfigurationWidget(TgBotManager& tg_bot_manager, QWidget *parent = nullptr);
    ~TGMessageConfigurationWidget();

public slots:
    void sl_change_object_data(const std::string &current, const std::string &previous);

private slots:
    void sl_opc_table_messages_double_click(int row, int column);

signals:
    void sg_tgobject_changed(const std::string& cur, const std::string& prev);

protected:
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    Ui::TGMessageConfigurationWidget *ui;

    TgBotManager& tg_bot_manager_;
    SelectItemsTableWidget* inline_buttons_to_message_ = nullptr;
    TGMessage* current_message_ = nullptr;

    void fill_opc_tags_table_(QTableWidget* tbl);
    void opc_table_set_column_width_(QTableWidget* tbl);
    void clear_message_data_();
    void save_to_current_message_();
    void load_data_from_message_(const std::string& id);
};

#endif // TGMESSAGECONFIFURATIONWIDGET_H
