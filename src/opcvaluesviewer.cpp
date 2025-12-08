#include "opcvaluesviewer.h"
#include "ui_opcvaluesviewer.h"

using namespace OPC_HELPER;

OPCValuesViewer::OPCValuesViewer(OPC_HELPER::OPCDataManager* dm_ptr, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::OPCValuesViewer)
    , opc_data_manager_(dm_ptr)
{
    ui->setupUi(this);
    QObject::connect(opc_data_manager_, SIGNAL(sg_periodic_started()), this, SLOT(sl_periodic_thread_started()));
    QObject::connect(opc_data_manager_, SIGNAL(sg_periodic_finished()), this, SLOT(sl_periodic_thread_finished()));

    ui->spbPeriodReading->setValue(opc_data_manager_->GetPeriodReading());
    if(opc_data_manager_->PeriodicReadingOn()) {
        ui->pbStartOPC->setEnabled(false);
    } else {
        ui->pbStartOPC->setEnabled(true);
    }

    opc_values_viewer_model_ = new OPCValuesViewerModel(opc_data_manager_->GetIdToTagPeriodicTags(), this);
    ui->tvOPCValuesViewer->setModel(opc_values_viewer_model_);
    ui->tvOPCValuesViewer->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tvOPCValuesViewer->verticalHeader()->hide();
    ui->tvOPCValuesViewer->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    QObject::connect(ui->tvOPCValuesViewer, SIGNAL(clicked(QModelIndex)), opc_values_viewer_model_, SLOT(sl_table_view_cell_clicked(QModelIndex)));
    QObject::connect(ui->tvOPCValuesViewer, SIGNAL(doubleClicked(QModelIndex)), opc_values_viewer_model_, SLOT(sl_table_view_cell_double_clicked(QModelIndex)));
    QObject::connect(ui->tvOPCValuesViewer->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(sl_item_comment_processing(QItemSelection,QItemSelection)));
    QObject::connect(opc_data_manager_, SIGNAL(sg_reading_periodic_complete()), opc_values_viewer_model_, SLOT(sl_tags_values_updated()));
    QObject::connect(opc_data_manager_, SIGNAL(sg_reading_request_complete()), opc_values_viewer_model_, SLOT(sl_tags_values_updated()));
    QObject::connect(opc_data_manager_, SIGNAL(sg_periodic_list_changed()), this, SLOT(sl_periodic_tags_list_changed()));
    QObject::connect(ui->pbReadOnce, SIGNAL(clicked(bool)), this, SLOT(sl_pb_readonce_clicked()));
    QObject::connect(ui->spbPeriodReading, SIGNAL(valueChanged(int)), this, SLOT(sl_spb_periodreading_valuechanged(int)));
    QObject::connect(ui->pbStartOPC, SIGNAL(clicked(bool)), this, SLOT(sl_pb_startopc_clicked()));
    QObject::connect(ui->pbStopOPC, SIGNAL(clicked(bool)), this, SLOT(sl_pb_stopopc_clicked()));

    console_ = new PlainTextConsole(this);
    console_->setMaximumBlockCount(100);
    console_->AddDTToMessage(true);
    ui->frOPCConsole->setLayout(new QVBoxLayout());
    ui->frOPCConsole->layout()->setContentsMargins(0, 0, 0, 0);
    ui->frOPCConsole->layout()->addWidget(console_);
    QObject::connect(opc_data_manager_, SIGNAL(sg_send_message_to_console(QString)), console_, SLOT(sl_add_text_to_console(QString)));
}

OPCValuesViewer::~OPCValuesViewer()
{
    delete ui;
    ui = nullptr;
}

void OPCValuesViewer::set_column_widths_(QTableView* tbl) {
    if(!tbl) return;
    int sum_width = tbl->horizontalHeader()->geometry().width();//(tbl->width() - 17) > 0 ? tbl->width() - 17 : tbl->width();
    int w1, w2, w3;
    if(sum_width >= 300) {
        w1 = (10*sum_width) / 100;
        w2 = (46*sum_width) / 100;
        w3 = ((44/4)*sum_width) / 100;
    } else {
        w1 = 10;
        w2 = 100;
        w3 = (sum_width - 111) / 4;
    }
    tbl->setColumnWidth(0, w1);
    tbl->setColumnWidth(1, w2);
    tbl->setColumnWidth(2, w3);
    tbl->setColumnWidth(3, w3);
    tbl->setColumnWidth(4, w3);
    tbl->setColumnWidth(5, w3);
}


void OPCValuesViewer::resizeEvent(QResizeEvent* event) {
    set_column_widths_(ui->tvOPCValuesViewer);
}

void OPCValuesViewer::showEvent(QShowEvent* event) {
    set_column_widths_(ui->tvOPCValuesViewer);
    QTextEdit* txtedit = ui->teTagPeriodicComment;
    txtedit->setPlainText("");
}

void OPCValuesViewer::sl_pb_readonce_clicked()
{   auto vec_per = opc_data_manager_->GetPeriodicTags();
    opc_data_manager_->ReadTagsOnce(vec_per);
}

void OPCValuesViewer::sl_item_comment_processing(QItemSelection selected, QItemSelection deselected) {
    QTextEdit* txtedit = ui->teTagPeriodicComment;

    if(!deselected.isEmpty()) {
        auto deselected_ind = deselected.indexes().front();
        auto tag_ptr = opc_values_viewer_model_->GetTagPtr(deselected_ind);
        if(tag_ptr) {
             tag_ptr->SetCommentString(txtedit->toPlainText());
        }
        txtedit->document()->setPlainText("");
    }

    if(!selected.isEmpty()) {
        auto selected_ind =  selected.indexes().front();
        auto tag_ptr = opc_values_viewer_model_->GetTagPtr(selected_ind);
        if(tag_ptr) {
            txtedit->document()->setPlainText(tag_ptr->GetCommentString());
        }
    }
}

void OPCValuesViewer::sl_spb_periodreading_valuechanged(int arg1)
{
    opc_data_manager_->SetPeriodReading(arg1);
}

void OPCValuesViewer::sl_pb_startopc_clicked()
{
    opc_data_manager_->StartPeriodReading(ui->spbPeriodReading->value());
}

void OPCValuesViewer::sl_periodic_thread_started() {
    if(ui) {
        ui->pbStartOPC->setEnabled(false);
    }
}

void OPCValuesViewer::sl_periodic_thread_finished() {
    if(ui) {
        ui->pbStartOPC->setEnabled(true);
    }
}

void OPCValuesViewer::sl_pb_stopopc_clicked()
{
    opc_data_manager_->StopPeriodReading();
}

void OPCValuesViewer::sl_periodic_tags_list_changed()
{
    opc_values_viewer_model_->SetTagsToTable(opc_data_manager_->GetIdToTagPeriodicTags());
}

//====================================================================================
//================= O P C V a l u e W r i t e D i a l o g ============================
//====================================================================================

OPCValueWriteDialog::OPCValueWriteDialog(std::shared_ptr<OPC_HELPER::OPCTag> tag_ptr, QWidget *parent)
    : QDialog(parent, Qt::Dialog)
    , tag_ptr_(tag_ptr)
{
    setWindowTitle("Значение для записи");
    QVBoxLayout* vbla = new QVBoxLayout();
    le_value_ = new QLineEdit();
    le_value_->setAlignment(Qt::AlignCenter);
    le_value_->setEnabled(tag_ptr_.get());

    QValidator* le_validator = nullptr;
    if(tag_ptr_) {
        if(!le_validator && tag_ptr_->ValueIsReal()) le_validator = new QDoubleValidator(-10000.0, 10000.0, 1);
        if(!le_validator && tag_ptr_->ValueIsInteger()) le_validator = new QIntValidator(-10000, 10000);
        if(!le_validator && tag_ptr_->ValueIsUnsignedInteger()) le_validator = new QIntValidator(0, 10000);
        if(!le_validator && tag_ptr_->ValueIsBool()) le_validator = new QIntValidator(0,1);
        le_value_->setText(tag_ptr_->GetStringValue(false));
    }
    le_value_->setValidator(le_validator);

    QPushButton* ok_btn = new QPushButton("OK");
    QObject::connect(ok_btn, SIGNAL(pressed()), this, SLOT(sl_set_value_to_tag_and_close()));

    QPushButton* cancel_btn = new QPushButton("Отмена");
    QObject::connect(cancel_btn, SIGNAL(pressed()), this, SLOT(close()));
    QHBoxLayout* hbla = new QHBoxLayout();
    hbla->addWidget(ok_btn);
    hbla->addWidget(cancel_btn);
    vbla->addWidget(le_value_);
    vbla->addItem(hbla);

    setLayout(vbla);
}

void OPCValueWriteDialog::sl_set_value_to_tag_and_close()
{
    if(tag_ptr_) {
        OPC_HELPER::OpcValueType val;
        if(tag_ptr_->ValueIsReal()) {
            bool b = false;
            double dblVal = 0.0;
            dblVal = le_value_->text().toDouble(&b);
            if(b) val = dblVal;
        }
        if(tag_ptr_->ValueIsInteger() || tag_ptr_->ValueIsBool()) {
            bool b = false;
            double intVal = 0.0;
            intVal = le_value_->text().toInt(&b);
            if(b) val = intVal;
        }
        if(tag_ptr_->ValueIsUnsignedInteger()) {
            bool b = false;
            double uintVal = 0.0;
            uintVal = le_value_->text().toUInt(&b);
            if(b) val = uintVal;
        }
        if(!val.valueless_by_exception()) {
            tag_ptr_->SetValueToWrite(val);
        }
    }
    close();
}


//====================================================================================
//================= O P C V a l u e s V i e w e r M o d e l ==========================
//====================================================================================

OPCValuesViewerModel::OPCValuesViewerModel(QObject *parent): QAbstractTableModel(parent)
{

}

OPCValuesViewerModel::OPCValuesViewerModel(std::map<size_t, std::shared_ptr<OPC_HELPER::OPCTag> > &&tags_map, QObject *parent)
    : QAbstractTableModel(parent)
    , id_to_tag_(std::move(tags_map))
{
    auto ids_view = std::views::keys(id_to_tag_);
    id_tags_ordered_ = {ids_view.begin(), ids_view.end()};
}

void OPCValuesViewerModel::SetTagsToTable(std::map<size_t, std::shared_ptr<OPC_HELPER::OPCTag> > &&tags_map)
{

    id_to_tag_ = std::move(tags_map);
    auto ids_view = std::views::keys(id_to_tag_);
    id_tags_ordered_ = {ids_view.begin(), ids_view.end()};
    reset();
}

int OPCValuesViewerModel::rowCount(const QModelIndex &parent) const
{
    return id_tags_ordered_.size();
}

int OPCValuesViewerModel::columnCount(const QModelIndex &parent) const
{
    return 6;
    //{"ID", "Имя тэга", "Тип", "Значение", "Качество", "Обработка"}
}

QVariant OPCValuesViewerModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid()) return {};

    if(role == Qt::DisplayRole) {
        return get_column_data_from_tag_(index);
    }

    if(role == Qt::BackgroundRole) {
        if(index.column() == 5) { /*постобработка тэга*/
            if(index.row() >= static_cast<int>(id_tags_ordered_.size())) return {};
            size_t id = id_tags_ordered_.at(index.row());
            if(id_to_tag_.count(id) == 0 || !id_to_tag_.at(id)) return {};
            auto tag_ptr = id_to_tag_.at(id);
            bool tag_has_modificators = (tag_ptr->GetGainOption().has_value()) || (tag_ptr->GetEnumStringValues().size() > 0);
            return tag_has_modificators ? QBrush(QColor(114, 255, 138)) : QBrush(QColor(245, 245, 245));
            /* {background-color: #72FF8A;} : {background-color: #F5F5F5;} */
        }
    }

    if(role == Qt::TextAlignmentRole) {
        if(index.column() == 1) {
            return Qt::AlignLeft;
        }
        return Qt::AlignCenter;
    }
    return {};
}

QVariant OPCValuesViewerModel::get_column_data_from_tag_(const QModelIndex &index) const
{
    //{"ID", "Имя тэга", "Тип", "Значение", "Качество", "Обработка"}
    if(!index.isValid()) return {};
    if(index.row() >= static_cast<int>(id_tags_ordered_.size())) return {};
    size_t id = id_tags_ordered_.at(index.row());

    if(id_to_tag_.count(id) == 0 || !id_to_tag_.at(id)) return {};
    auto tag_ptr = id_to_tag_.at(id);

    switch(index.column()) {
    case 0: return id;
    case 1: return tag_ptr->GetTagName();
    case 2: return tag_ptr->GetStringType();
    case 3: return tag_ptr->GetStringValue();
    case 4: return OPC_HELPER::GetQualityString(tag_ptr->GetTagQuality());
    case 5: {
        bool tag_has_modificators = (tag_ptr->GetGainOption().has_value()) || (tag_ptr->GetEnumStringValues().size() > 0);
        return tag_has_modificators ? QString("ОБРАБОТКА") : QString("НЕТ");
       }
    }
       return {};
}

QModelIndex OPCValuesViewerModel::index(int row, int column, const QModelIndex &parent) const
{
    if(row >=0 && row < static_cast<int>(id_tags_ordered_.size()) && column >=0 && column < 6) {
        return createIndex(row, column);
    }
    return QModelIndex();
}

QVariant OPCValuesViewerModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch(section) {
        case 0: return QString("ID");
        case 1: return QString("Имя тэга");
        case 2: return QString("Тип");
        case 3: return QString("Значение");
        case 4: return QString("Качество");
        case 5: return QString("Обработка");
        }
    }
    return {};
}

void OPCValuesViewerModel::reset()
{
    QAbstractTableModel::beginResetModel();
    QAbstractTableModel::endResetModel();
}

OPCTag *OPCValuesViewerModel::GetTagPtr(const QModelIndex &index) const
{
    if(!index.isValid() || index.row() >= static_cast<int>(id_tags_ordered_.size())) return nullptr;
    if(id_to_tag_.count(id_tags_ordered_.at(index.row())) == 0) return nullptr;
    return id_to_tag_.at(id_tags_ordered_.at(index.row())).get();
}

void OPCValuesViewerModel::sl_table_view_cell_clicked(const QModelIndex &index)
{
    if(!index.isValid() || index.row() >= static_cast<int>(id_tags_ordered_.size())) return;
    if(index.column() == 5) { /*постобработка тэга*/
        if(id_to_tag_.count(id_tags_ordered_.at(index.row())) == 0 || !id_to_tag_.at(id_tags_ordered_.at(index.row()))) return;
        auto tag_ptr = id_to_tag_.at(id_tags_ordered_.at(index.row()));

        OPCTagPostProcessingWidget* process_wdg = new OPCTagPostProcessingWidget(tag_ptr);
        process_wdg->setWindowFlag(Qt::Window);
        process_wdg->setWindowModality(Qt::WindowModality::WindowModal);
        process_wdg->setWindowTitle(QString("Пост обработка тэга %1").arg(tag_ptr->GetTagName()));
        process_wdg->exec();
        process_wdg->deleteLater();
        emit dataChanged(index, index, {Qt::DisplayRole, Qt::BackgroundRole});
    }
}

void OPCValuesViewerModel::sl_table_view_cell_double_clicked(const QModelIndex &index)
{
    if(!index.isValid() || index.row() >= static_cast<int>(id_tags_ordered_.size())) return;
    if(index.column() == 3) { /*значение*/
        if(id_to_tag_.count(id_tags_ordered_.at(index.row())) == 0 || !id_to_tag_.at(id_tags_ordered_.at(index.row()))) return;
        auto tag_ptr = id_to_tag_.at(id_tags_ordered_.at(index.row()));

        OPCValueWriteDialog* set_value_dialog = new OPCValueWriteDialog(tag_ptr);
        set_value_dialog->exec();
        set_value_dialog->deleteLater();
    }
}

void OPCValuesViewerModel::sl_tags_values_updated()
{
    if(rowCount(QModelIndex()) == 0) return;
    auto index_start = createIndex(0, 3);
    auto index_end = createIndex(rowCount(QModelIndex()) - 1, 4);
    emit dataChanged(index_start, index_end, {Qt::DisplayRole});
}

