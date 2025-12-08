#ifndef OPCVALUESVIEWER_H
#define OPCVALUESVIEWER_H

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLineEdit>

#include <unordered_map>
#include <ranges>

#include "opcclientworker.h"
#include "opcdatamanager.h"
#include "plaintextconsole.h"
#include "opctagpostprocessingwidget.h"

namespace Ui {
class OPCValuesViewer;
}

class OPCValuesViewerModel;

class OPCValuesViewer : public QWidget
{
    Q_OBJECT

public:
    explicit OPCValuesViewer(OPC_HELPER::OPCDataManager* dm_ptr, QWidget *parent = nullptr);
    ~OPCValuesViewer();

    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;

signals:
    void sg_set_main_window_status_bar_message(QString message);
    void sg_check_all_buttons_in_table();

private slots:
    void sl_pb_readonce_clicked();
    void sl_item_comment_processing(QItemSelection selected, QItemSelection deselected);
    void sl_spb_periodreading_valuechanged(int arg1);
    void sl_pb_startopc_clicked();
    void sl_periodic_thread_started();
    void sl_periodic_thread_finished();
    void sl_pb_stopopc_clicked();
    void sl_periodic_tags_list_changed();

private:
    Ui::OPCValuesViewer *ui;
    OPC_HELPER::OPCDataManager* opc_data_manager_;
    PlainTextConsole* console_;
    OPCValuesViewerModel* opc_values_viewer_model_;

    void set_column_widths_(QTableView* tbl);
};

class OPCValueWriteDialog: public QDialog {
    Q_OBJECT
public:
    OPCValueWriteDialog() = delete;
    explicit OPCValueWriteDialog(std::shared_ptr<OPC_HELPER::OPCTag> tag_ptr, QWidget* parent = nullptr);

private slots:
    void sl_set_value_to_tag_and_close();

private:
    std::shared_ptr<OPC_HELPER::OPCTag> tag_ptr_ = nullptr;
    QLineEdit* le_value_;

};

class OPCValuesViewerModel: public QAbstractTableModel
{
    Q_OBJECT
public:
    OPCValuesViewerModel(QObject* parent = nullptr);
    explicit OPCValuesViewerModel(std::map<size_t, std::shared_ptr<OPC_HELPER::OPCTag>>&& tags_map, QObject* parent = nullptr);
    void SetTagsToTable(std::map<size_t, std::shared_ptr<OPC_HELPER::OPCTag>>&& tags_map);
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    void reset();
    OPC_HELPER::OPCTag* GetTagPtr(const QModelIndex &index) const;

public slots:
    void sl_table_view_cell_clicked(const QModelIndex& index);
    void sl_table_view_cell_double_clicked(const QModelIndex& index);
    void sl_tags_values_updated();

private:
    std::map<size_t, std::shared_ptr<OPC_HELPER::OPCTag>> id_to_tag_;
    std::vector<size_t> id_tags_ordered_;

    QVariant get_column_data_from_tag_(const QModelIndex &index) const;
};

#endif // OPCVALUESVIEWER_H
