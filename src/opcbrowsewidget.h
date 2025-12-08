#ifndef OPCBROWSEWIDGET_H
#define OPCBROWSEWIDGET_H

#include <QWidget>
#include <QListWidgetItem>
#include <QCheckBox>
#include <QPushButton>

#include "copcclient.h"
#include "opcdatamanager.h"

namespace Ui {
class OpcBrowseWidget;
}

class OpcBrowseWidget : public QWidget
{
    Q_OBJECT

public:
    explicit OpcBrowseWidget(OPC_HELPER::OPCDataManager* dm_ptr, QWidget *parent = nullptr);
    ~OpcBrowseWidget();

    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;

signals:
    void sg_set_main_window_status_bar_message(QString message);

private slots:
    void sl_refresh_opc_tags_to_table(QListWidgetItem* cur, QListWidgetItem* last);
    void sl_tb_cleartagslist_clicked();
    void sl_opc_table_tag_change_state_reading(QString,OPC_HELPER::TAG_STATUS,int);
    void sl_tb_refreshopclist_clicked();
    void sl_tb_refreshopctags_clicked();

private:
    Ui::OpcBrowseWidget *ui;
    OPC_HELPER::OPCDataManager* opc_data_manager_;
    std::unordered_map<QString, std::vector<QString>> opc_server_to_tags_list_buffer_;

    void opctable_set_column_widths_();
    void fill_opc_list_();
    void fill_tags_list_(QString& server_name);
    void fill_tags_list_(QString&& server_name);
};


class OPCCheckBoxTableItem: public QWidget
{
    Q_OBJECT
public:
    explicit OPCCheckBoxTableItem(QString tag, OPC_HELPER::TAG_STATUS type, bool checked, QWidget *parent = nullptr);
    void SetCheckBoxState(bool state);

signals:
    void sg_change_state(QString tag, OPC_HELPER::TAG_STATUS place, int state);

private slots:
    void sl_checkbox_changed_state(int state);

private:
    QString tag_name_;
    OPC_HELPER::TAG_STATUS tag_place_;
    QCheckBox* chb_;
};


#endif // OPCBROWSEWIDGET_H
