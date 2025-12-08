#include "opcbrowsewidget.h"
#include "ui_opcbrowsewidget.h"

OpcBrowseWidget::OpcBrowseWidget(OPC_HELPER::OPCDataManager* dm_ptr, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::OpcBrowseWidget)
    , opc_data_manager_(dm_ptr)
{
    ui->setupUi(this);

    QTableWidget* opc_table_tags = ui->tblOPCTags;

    opc_table_tags->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    opc_table_tags->setColumnCount(2);
    opc_table_tags->setHorizontalHeaderItem(0, new QTableWidgetItem("Имя тэга"));
    opc_table_tags->setHorizontalHeaderItem(1, new QTableWidgetItem("Контроль\nизменения"));

    QObject::connect(ui->lwOPCServers, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)), this, SLOT(sl_refresh_opc_tags_to_table(QListWidgetItem*,QListWidgetItem*)));
    QObject::connect(ui->tbClearTagsList, SIGNAL(clicked(bool)), this, SLOT(sl_tb_cleartagslist_clicked()));
    QObject::connect(ui->tbRefreshOPCList, SIGNAL(clicked(bool)), this, SLOT(sl_tb_refreshopclist_clicked()));
    QObject::connect(ui->tbRefreshOPCTags, SIGNAL(clicked(bool)), this, SLOT(sl_tb_refreshopctags_clicked()));

    fill_opc_list_();
}

OpcBrowseWidget::~OpcBrowseWidget()
{
    delete ui;
}

void OpcBrowseWidget::opctable_set_column_widths_() {
    QTableWidget* opc_table_tags = ui->tblOPCTags;

    int w_header = opc_table_tags->horizontalHeader()->geometry().width();
    w_header -= 5;

    int n_col = opc_table_tags->columnCount();
    int w_btns_cols = w_header > 300 ? (60 * n_col) : w_header / 2;

    for(int i = 0; i < n_col; ++i) {
        if(i == 0) {
            opc_table_tags->setColumnWidth(0, w_header - w_btns_cols);
        } else {
            opc_table_tags->setColumnWidth(i, w_btns_cols/(n_col-1));
        }
    }
}

void OpcBrowseWidget::resizeEvent(QResizeEvent* event) {
    opctable_set_column_widths_();
}

void OpcBrowseWidget::showEvent(QShowEvent* event) {
    opctable_set_column_widths_();
}

void OpcBrowseWidget::fill_opc_list_() {
    QListWidget* opc_list_w = ui->lwOPCServers;

    QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    OPC_HELPER::COPCClient opc_client;
    opc_list_w->clear();
    for(const auto& name: opc_client.GetOPCServerNames()) {
        opc_list_w->addItem(name);
    }
    QGuiApplication::restoreOverrideCursor();

}

void OpcBrowseWidget::fill_tags_list_(QString&& server_name) {
    QString s = std::move(server_name);
    fill_tags_list_(s);
}

void OpcBrowseWidget::fill_tags_list_(QString& server_name) {
    QTableWidget* opc_table = ui->tblOPCTags;

    QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    if(opc_server_to_tags_list_buffer_.count(server_name) == 0) {
        OPC_HELPER::COPCClient client;
        opc_server_to_tags_list_buffer_[server_name] = client.GetOPCTagsNames(server_name);
    }

    const std::vector<QString>& tag_list = opc_server_to_tags_list_buffer_.at(server_name);

    opc_table->clearContents();
    int i = 0;
    opc_table->setRowCount(tag_list.size());

    for(const auto& tag: tag_list) {
        QTableWidgetItem* tag_item_widget = new QTableWidgetItem(tag);
        tag_item_widget->setFlags(Qt::NoItemFlags);
        opc_table->setItem(i++, 0, tag_item_widget);

        OPC_HELPER::TAG_STATUS tag_status = opc_data_manager_->CheckTagReadState(tag);

        int tag_per_state = 0;
        if(tag_status == OPC_HELPER::TAG_STATUS::PERIODIC_READ || tag_status == OPC_HELPER::TAG_STATUS::READ_BOTH) {
            tag_per_state = 1;
        }

        OPCCheckBoxTableItem* item_to_perodic_read = new OPCCheckBoxTableItem(tag, OPC_HELPER::TAG_STATUS::PERIODIC_READ, tag_per_state, this);
        QObject::connect(item_to_perodic_read, SIGNAL(sg_change_state(QString,OPC_HELPER::TAG_STATUS,int)), this, SLOT(sl_opc_table_tag_change_state_reading(QString,OPC_HELPER::TAG_STATUS,int)));
        opc_table->setCellWidget(i-1, 1, item_to_perodic_read);
    }
    QGuiApplication::restoreOverrideCursor();
    opc_table->repaint();
    opctable_set_column_widths_();
}

void OpcBrowseWidget::sl_refresh_opc_tags_to_table(QListWidgetItem* cur, QListWidgetItem* last) {

    if(cur) {
        fill_tags_list_(cur->text());
    }
}

void OpcBrowseWidget::sl_opc_table_tag_change_state_reading(QString tag, OPC_HELPER::TAG_STATUS st, int state) {
    if(st == OPC_HELPER::TAG_STATUS::PERIODIC_READ || st == OPC_HELPER::TAG_STATUS::READ_BOTH) {
        if(state == 2) {
            opc_data_manager_->AddTagToPeriodicReadList(ui->lwOPCServers->currentItem()->text(), tag);
        } else {
            opc_data_manager_->DeleteTagFromPeriodicRead(tag);
        }
    };
}

void OpcBrowseWidget::sl_tb_cleartagslist_clicked()
{
    opc_data_manager_->ClearMonitoringTags();
    if(ui->lwOPCServers->currentItem()) {
        fill_tags_list_(ui->lwOPCServers->currentItem()->text());
    }
}

void OpcBrowseWidget::sl_tb_refreshopclist_clicked()
{
    fill_opc_list_();
    ui->tblOPCTags->clear();
    ui->tblOPCTags->setHorizontalHeaderItem(0, new QTableWidgetItem("Имя тэга"));
    ui->tblOPCTags->setHorizontalHeaderItem(1, new QTableWidgetItem("Контроль\nизменения"));
}

void OpcBrowseWidget::sl_tb_refreshopctags_clicked()
{
    if(ui->lwOPCServers->currentItem()) {
        opc_server_to_tags_list_buffer_.erase(ui->lwOPCServers->currentItem()->text());
        fill_tags_list_(ui->lwOPCServers->currentItem()->text());
    }
}

//===============================================================
//================ OPCCheckBoxTableItem =========================
//===============================================================

OPCCheckBoxTableItem::OPCCheckBoxTableItem(QString tag, OPC_HELPER::TAG_STATUS type, bool checked, QWidget *parent)
    : QWidget(parent)
    , tag_name_(tag)
    , tag_place_(type)
{
    QHBoxLayout* hbox_la = new QHBoxLayout(this);
    QCheckBox* chb_ = new QCheckBox(this);
    hbox_la->addWidget(chb_);
    hbox_la->setAlignment(Qt::AlignCenter);
    hbox_la->setContentsMargins(0, 0, 0, 0);
    chb_->setChecked(checked);

    QObject::connect(chb_, SIGNAL(stateChanged(int)), this, SLOT(sl_checkbox_changed_state(int)));
}

void OPCCheckBoxTableItem::sl_checkbox_changed_state(int state) {
    emit sg_change_state(tag_name_, tag_place_, state);
}

void OPCCheckBoxTableItem::SetCheckBoxState(bool state) {
    chb_->setChecked(state);
}
