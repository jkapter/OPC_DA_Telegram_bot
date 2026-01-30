#ifndef OPCBROWSEWIDGET_H
#define OPCBROWSEWIDGET_H

#include <QWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QCheckBox>
#include <QPushButton>
#include <QMenu>
#include <QDialog>
#include <QVBoxLayout>
#include <QLineEdit>

#include "copcclient.h"
#include "opcdatamanager.h"
#include "plaintextconsole.h"

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
    void sg_stop_browsing_tags();

private slots:
    void sl_refresh_opc_tags_to_table(QTreeWidgetItem* cur, QTreeWidgetItem* last);
    void sl_tb_cleartagslist_clicked();
    void sl_opc_table_tag_change_state_reading(QString,OPC_HELPER::TAG_STATUS,int);
    void sl_tb_refreshopclist_clicked();
    void sl_tb_refreshopctags_clicked();
    void sl_opc_servers_tree_widget_context_menu_requested(const QPoint& pos);
    void sl_add_opc_server_to_tree();
    void sl_delete_opc_server_from_tree();
    void sl_add_new_host_to_tree(QString hostname);
    void sl_browser_get_part_tags(QString hostname, QString server_name, size_t n_tags);
    void sl_browser_get_all_tags(QString hostname, QString server_name, size_t n_tags);

private:
    Ui::OpcBrowseWidget *ui;
    PlainTextConsole* console_;
    OPC_HELPER::OPCDataManager* opc_data_manager_;
    std::unordered_set<QString> host_names_;
    std::unordered_map<const QString*, std::set<QString>> host_to_opc_servers_;
    std::unordered_map<const QString*, std::vector<QString>> opc_server_to_tags_list_buffer_;
    std::unordered_map<const QString*, QMutex> opc_server_to_mutex_;
    std::unordered_map<const QString*, QTreeWidgetItem*> opc_server_to_tree_item_;

    QTreeWidgetItem* selected_item_opc_tree_ = nullptr;

    int opc_threads_count_ = 0;

    void opctable_set_column_widths_();
    void opc_tree_set_column_width();
    void fill_opc_list_();
    void fill_tags_list_(const QString& hostname, const QString& server_name);
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

class OPCAddHostDialog: public QDialog {
    Q_OBJECT
public:
    explicit OPCAddHostDialog(QWidget* parent = nullptr);

signals:
    void sg_add_new_host(QString hostname);

private slots:
    void sl_ok_pressed();

private:
    QLineEdit* le_value_;

};

#endif // OPCBROWSEWIDGET_H
