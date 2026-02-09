#include "opcbrowsewidget.h"
#include "ui_opcbrowsewidget.h"

using namespace Qt::Literals::StringLiterals;

OpcBrowseWidget::OpcBrowseWidget(OPC_HELPER::OPCDataManager* dm_ptr, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::OpcBrowseWidget)
    , opc_data_manager_(dm_ptr)
{
    ui->setupUi(this);

    ui->tblvOPCTags->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    ui->tblvOPCTags->setItemDelegateForColumn(1, new SelectReadModeCheckBox(25, this));
    ui->tblvOPCTags->setSelectionMode(QAbstractItemView::NoSelection);

    QObject::connect(ui->twOPCServers, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT(sl_refresh_opc_tags_to_table(QTreeWidgetItem*,QTreeWidgetItem*)));
    QObject::connect(ui->tbClearTagsList, SIGNAL(clicked(bool)), this, SLOT(sl_tb_cleartagslist_clicked()));
    QObject::connect(ui->tbRefreshOPCList, SIGNAL(clicked(bool)), this, SLOT(sl_tb_refreshopclist_clicked()));
    QObject::connect(ui->tbRefreshOPCTags, SIGNAL(clicked(bool)), this, SLOT(sl_tb_refreshopctags_clicked()));

    console_ = new PlainTextConsole(this);
    console_->setMaximumBlockCount(100);
    console_->AddDTToMessage(true);
    ui->frOPCConsole->setLayout(new QVBoxLayout());
    ui->frOPCConsole->layout()->setContentsMargins(0, 0, 0, 0);
    ui->frOPCConsole->layout()->addWidget(console_);

    ui->twOPCServers->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(ui->twOPCServers, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(sl_opc_servers_tree_widget_context_menu_requested(QPoint)));
    QObject::connect(ui->twOPCServers, &QTreeView::expanded, this, [this](){ui->twOPCServers->resizeColumnToContents(0);});
    ui->twOPCServers->setSortingEnabled(false);

    OPC_HELPER::COPCClient opc_client;
    QObject::connect(&opc_client, SIGNAL(sg_send_message_to_console(QString)), console_, SLOT(sl_add_text_to_console(QString)));

    for(const auto& it: opc_data_manager_->GetHostNames()) {
        auto host_it = host_names_.insert(it).first;
        host_to_opc_servers_[&(*host_it)] = {};
    }

    if(!host_names_.contains(u"localhost"_s)) {
        auto host_it = host_names_.insert(u"localhost"_s).first;
        host_to_opc_servers_[&(*host_it)] = {};
    }

    for(const auto& host: host_names_) {
        for(const auto& it: opc_client.GetOPCServerNames(host)) {
            auto [serv_it, b] = host_to_opc_servers_.at(&host).insert(it);
            if(b) {
                opc_server_to_mutex_[&(*serv_it)];
                opc_server_to_table_model_[&(*serv_it)] = nullptr;
            }
        }
    }
    fill_opc_list_();    
}

OpcBrowseWidget::~OpcBrowseWidget()
{
    emit sg_stop_browsing_tags();
    while(opc_threads_count_ > 0) {
        QThread::currentThread()->eventDispatcher()->processEvents(QEventLoop::AllEvents);
        QThread::currentThread()->sleep(std::chrono::nanoseconds(100000));
    }
    delete ui;
}

void OpcBrowseWidget::opctable_set_column_widths_() {
    QTableView* opc_table_tags = ui->tblvOPCTags;

    int w_header = opc_table_tags->horizontalHeader()->geometry().width();
    if(w_header <= 0) return;

    int n_col = 2;
    int w_btns_cols = w_header > 300 ? (60 * n_col) : w_header / 2;

    for(int i = 0; i < n_col; ++i) {
        if(i == 0) {
            opc_table_tags->setColumnWidth(0, w_header - w_btns_cols);
        } else {
            opc_table_tags->setColumnWidth(i, w_btns_cols/(n_col-1));
        }
    }
    ui->tblvOPCTags->repaint();
}

void OpcBrowseWidget::resizeEvent(QResizeEvent* event) {
    opctable_set_column_widths_();
}

void OpcBrowseWidget::showEvent(QShowEvent* event) {
    opctable_set_column_widths_();
}

void OpcBrowseWidget::fill_opc_list_() {
    QTreeWidget* opc_tree = ui->twOPCServers;
    opc_tree->clear();
    opc_server_to_tree_item_.clear();

    for(const auto& it: host_names_) {
        QTreeWidgetItem* top_item;
        if(it == u"localhost"_s) {
            top_item = new QTreeWidgetItem({u"Этот компьютер"_s, u""_s}, QTreeWidgetItem::UserType);
        } else {
            top_item = new QTreeWidgetItem({it, u""_s});
        }

        for(const auto& server: host_to_opc_servers_.at(&it)) {
            QTreeWidgetItem* child_item = new QTreeWidgetItem(top_item);
            child_item->setText(0, server);
            bool opc_list = opc_server_to_tags_list_buffer_.count(&server) > 0;
            size_t tags_count = opc_list ? opc_server_to_tags_list_buffer_.at(&server).size() : 0;
            child_item->setText(1, QString("[%1]").arg(tags_count));
            QString icon_path = !opc_list ? u":/img/icons/question_mark_icon.png"_s : u":/img/icons/circle_green_checkmark.svg"_s;
            QIcon item_icon(icon_path);
            child_item->setIcon(0, item_icon);
            opc_server_to_tree_item_[&server] = child_item;
        }
        opc_tree->addTopLevelItem(top_item);
    }
    ui->twOPCServers->resizeColumnToContents(0);
}

void OpcBrowseWidget::fill_tags_list_(const QString& hostname, const QString& server_name) {
    auto host_it = host_names_.find(hostname);
    if(host_it == host_names_.end()) return;

    auto server_it = host_to_opc_servers_.at(&(*host_it)).find(server_name);
    if(server_it == host_to_opc_servers_.at(&(*host_it)).end()) return;

    if(opc_server_to_tags_list_buffer_.count(&(*server_it)) == 0) {
        opc_server_to_tags_list_buffer_[&(*server_it)] = {};

        OPC_HELPER::OPCClientTagBrowser* browser = new OPC_HELPER::OPCClientTagBrowser(hostname, server_name, opc_server_to_tags_list_buffer_.at(&(*server_it)), opc_server_to_mutex_.at(&(*server_it)));
        QThread* opc_thread = new QThread(this);
        QObject::connect(browser, SIGNAL(sg_send_message_to_console(QString)), console_, SLOT(sl_add_text_to_console(QString)));
        QObject::connect(browser, SIGNAL(sg_opcclient_got_exception(QString)), console_, SLOT(sl_add_text_to_console(QString)));
        QObject::connect(this, SIGNAL(sg_stop_browsing_tags()), browser, SLOT(sl_stop_browsing()));
        QObject::connect(opc_thread, SIGNAL(started()), browser, SLOT(sl_process()));
        QObject::connect(browser, SIGNAL(sg_finished()), opc_thread, SLOT(quit()));
        QObject::connect(opc_thread, SIGNAL(finished()), opc_thread, SLOT(deleteLater()));
        QObject::connect(opc_thread, &QThread::finished, this, [this](){--opc_threads_count_;});
        QObject::connect(browser, SIGNAL(sg_get_part_tag_names_from_server(const QString&,const QString&,size_t)), this, SLOT(sl_browser_get_part_tags(const QString&,const QString&,size_t)));
        QObject::connect(browser, SIGNAL(sg_get_all_tag_names_from_server(const QString&,const QString&,size_t)), this, SLOT(sl_browser_get_all_tags(const QString&,const QString&,size_t)));
        QObject::connect(opc_thread, &QThread::finished, [browser] {delete browser;});

        browser->moveToThread(opc_thread);
        opc_thread->start();
        ++opc_threads_count_;
        console_->sl_add_text_to_console(QString("Запрос списка тэгов сервера %1 на хосте %2.").arg(server_name, hostname));
        return;
    }

    if(opc_server_to_mutex_.at(&(*server_it)).tryLock(1)) {
        opc_server_to_mutex_.at(&(*server_it)).unlock();
    } else {
        return;
    }

    QTableView* opc_table_v = ui->tblvOPCTags;
    QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    QMutexLocker locker(&opc_server_to_mutex_.at(&(*server_it)));
    const std::vector<QString>& tag_list = opc_server_to_tags_list_buffer_.at(&(*server_it));

    if(!opc_server_to_table_model_.at(&(*server_it))) {
        opc_server_to_table_model_.at(&(*server_it)) = new OPCTagsViewerModel(tag_list, QString("%1@%2").arg(*host_it, *server_it), *opc_data_manager_, this);
    }
    opc_table_v->setModel(opc_server_to_table_model_.at(&(*server_it)));

    QGuiApplication::restoreOverrideCursor();
    opctable_set_column_widths_();
}

void OpcBrowseWidget::sl_refresh_opc_tags_to_table(QTreeWidgetItem* cur, QTreeWidgetItem* last) {
    ui->twOPCServers->resizeColumnToContents(0);
    if(cur && cur->parent()) {
        QString host = cur->parent()->text(0) == u"Этот компьютер"_s ? u"localhost"_s : cur->parent()->text(0);
        fill_tags_list_(host, cur->text(0));
        opctable_set_column_widths_();
    }
}

void OpcBrowseWidget::sl_tb_cleartagslist_clicked()
{
    opc_data_manager_->ClearMonitoringTags();
    ui->tblvOPCTags->repaint();
}

void OpcBrowseWidget::sl_tb_refreshopclist_clicked()
{
    emit sg_stop_browsing_tags();
    QThread::currentThread()->eventDispatcher()->processEvents(QEventLoop::AllEvents);

    ui->twOPCServers->clear();
    ui->tblvOPCTags->setModel(nullptr);

    host_to_opc_servers_.clear();
    opc_server_to_tree_item_.clear();

    for(const auto& it: host_names_) {
        OPC_HELPER::COPCClient opc_client;
        QObject::connect(&opc_client, SIGNAL(sg_send_message_to_console(QString)), console_, SLOT(sl_add_text_to_console(QString)));
        host_to_opc_servers_[&it] = {};
        for(const auto& server: opc_client.GetOPCServerNames(it)) {
            auto [serv_it, b] = host_to_opc_servers_.at(&it).insert(server);
            if(b) {
                opc_server_to_mutex_[&(*serv_it)];
                opc_server_to_table_model_[&(*serv_it)] = nullptr;
            }
        }
    }
    fill_opc_list_();
}

void OpcBrowseWidget::sl_tb_refreshopctags_clicked()
{
    emit sg_stop_browsing_tags();
    QThread::currentThread()->eventDispatcher()->processEvents(QEventLoop::AllEvents);

    if(ui->twOPCServers->currentItem() && ui->twOPCServers->currentItem()->parent()) {
        auto host_it = host_names_.find(ui->twOPCServers->currentItem()->parent()->text(0));
        if(host_it == host_names_.end()) return;

        auto server_it = host_to_opc_servers_.at(&(*host_it)).find(ui->twOPCServers->currentItem()->text(0));
        if(server_it == host_to_opc_servers_.at(&(*host_it)).end()) return;

        opc_server_to_tags_list_buffer_.erase(&(*server_it));
        ui->tblvOPCTags->setModel(nullptr);
        if(opc_server_to_table_model_.at(&(*server_it))) {
            opc_server_to_table_model_.at(&(*server_it))->deleteLater();
            opc_server_to_table_model_.at(&(*server_it)) = nullptr;
        }
        fill_tags_list_(*host_it, *server_it);
    }
}

void OpcBrowseWidget::sl_opc_servers_tree_widget_context_menu_requested(const QPoint &pos)
{
    selected_item_opc_tree_ = ui->twOPCServers->itemAt(pos);
    QAction *add_server = new QAction(u"Добавить сетевое расположение"_s, ui->twOPCServers);
    QAction *delete_server = new QAction(u"Удалить"_s, ui->twOPCServers);
    bool delete_allow = selected_item_opc_tree_ &&
                        (
                        (!selected_item_opc_tree_->parent() && (selected_item_opc_tree_->type() == QTreeWidgetItem::Type))
                         ||(selected_item_opc_tree_->parent() && (selected_item_opc_tree_->parent()->type() == QTreeWidgetItem::Type))
                        )
                        ;
    delete_server->setEnabled(delete_allow);
    QObject::connect(add_server, SIGNAL(triggered()), this, SLOT(sl_add_opc_server_to_tree()));
    QObject::connect(delete_server, SIGNAL(triggered()), this, SLOT(sl_delete_opc_server_from_tree()));

    QMenu context_menu(ui->twOPCServers);
    context_menu.addAction(add_server);
    context_menu.addSeparator();
    context_menu.addAction(delete_server);
    context_menu.exec(ui->twOPCServers->mapToGlobal(pos));
}

void OpcBrowseWidget::sl_add_opc_server_to_tree()
{
    OPCAddHostDialog* new_host_dialog = new OPCAddHostDialog(this);
    QObject::connect(new_host_dialog, SIGNAL(sg_add_new_host(const QString&)),this, SLOT(sl_add_new_host_to_tree(const QString&)));
    new_host_dialog->exec();
    new_host_dialog->deleteLater();
}

void OpcBrowseWidget::sl_delete_opc_server_from_tree()
{
    if(selected_item_opc_tree_ && !selected_item_opc_tree_->parent() && selected_item_opc_tree_->text(0) != u"Этот компьютер"_s) {
        emit sg_stop_browsing_tags();
        auto host_it = host_names_.find(selected_item_opc_tree_->text(0));
        ui->tblvOPCTags->setModel(nullptr);

        for(auto& it: host_to_opc_servers_.at(&(*host_it))) {
            while(!opc_server_to_mutex_.at(&it).tryLock(50)){};
            opc_server_to_mutex_.at(&it).unlock();
            opc_server_to_tags_list_buffer_.erase(&it);
            opc_server_to_mutex_.erase(&it);
            if(opc_server_to_table_model_.at(&it)) {
                opc_server_to_table_model_.at(&it)->deleteLater();
                opc_server_to_table_model_.at(&it) = nullptr;
            }
        }
        host_to_opc_servers_.erase(&(*host_it));
        host_names_.erase(host_it);
        fill_opc_list_();
    }
}

void OpcBrowseWidget::sl_add_new_host_to_tree(const QString& hostname)
{
    auto [host_it, b] = host_names_.insert(hostname);
    if(b) {
        OPC_HELPER::COPCClient opc_client;
        QObject::connect(&opc_client, SIGNAL(sg_send_message_to_console(QString)), console_, SLOT(sl_add_text_to_console(QString)));
        host_to_opc_servers_[&(*host_it)] = {};
        for(const auto& it: opc_client.GetOPCServerNames(hostname)) {
            auto [serv_it, b] = host_to_opc_servers_.at(&(*host_it)).insert(it);
            if(b) {
                opc_server_to_mutex_[&(*serv_it)];
                opc_server_to_table_model_[&(*serv_it)] = nullptr;
            }
        }
        fill_opc_list_();
    }
}

void OpcBrowseWidget::sl_browser_get_part_tags(const QString& hostname, const QString& server_name, size_t n_tags)
{
    auto host_it = host_names_.find(hostname);
    if(host_it == host_names_.end()) return;

    auto server_it = host_to_opc_servers_.at(&(*host_it)).find(server_name);
    if(server_it == host_to_opc_servers_.at(&(*host_it)).end()) return;

    if(opc_server_to_tree_item_.count(&(*server_it)) > 0 && (opc_server_to_tree_item_.at(&(*server_it)))) {
        opc_server_to_tree_item_.at(&(*server_it))->setText(1, QString("[%1]").arg(n_tags));
        console_->sl_add_text_to_console(QString("Сервер %1, в сетевом расположении %2, прочитано %3 тэгов.")
                                             .arg(server_name, hostname).arg(n_tags));
    }
}

void OpcBrowseWidget::sl_browser_get_all_tags(const QString& hostname, const QString& server_name, size_t n_tags)
{
    auto host_it = host_names_.find(hostname);
    if(host_it == host_names_.end()) return;

    auto server_it = host_to_opc_servers_.at(&(*host_it)).find(server_name);
    if(server_it == host_to_opc_servers_.at(&(*host_it)).end()) return;

    if(opc_server_to_tree_item_.count(&(*server_it)) > 0 && (opc_server_to_tree_item_.at(&(*server_it)))) {
        opc_server_to_tree_item_.at(&(*server_it))->setText(1, QString("[%1]").arg(n_tags));
        opc_server_to_tree_item_.at(&(*server_it))->setIcon(0,  QIcon(u":/img/icons/circle_green_checkmark.svg"_s));
        console_->sl_add_text_to_console(QString("Сервер %1, в сетевом расположении %2, прочитано все тэги.")
                                             .arg(server_name, hostname));
        ui->twOPCServers->setCurrentItem(opc_server_to_tree_item_.at(&(*server_it)));
        fill_tags_list_(hostname, server_name);
        opctable_set_column_widths_();
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

//===============================================================
//================ OPCAddServerDialog ===========================
//===============================================================

OPCAddHostDialog::OPCAddHostDialog(QWidget *parent)
    : QDialog(parent, Qt::Dialog)
{
    setWindowTitle("Добавить сетевое расположение");
    QVBoxLayout* vbla = new QVBoxLayout();
    le_value_ = new QLineEdit();
    le_value_->setAlignment(Qt::AlignCenter);

    QPushButton* ok_btn = new QPushButton("OK");
    QObject::connect(ok_btn, SIGNAL(pressed()), this, SLOT(sl_ok_pressed()));

    QPushButton* cancel_btn = new QPushButton("Отмена");
    QObject::connect(cancel_btn, SIGNAL(pressed()), this, SLOT(close()));
    QHBoxLayout* hbla = new QHBoxLayout();
    hbla->addWidget(ok_btn);
    hbla->addWidget(cancel_btn);
    vbla->addWidget(le_value_);
    vbla->addItem(hbla);

    setLayout(vbla);
}

void OPCAddHostDialog::sl_ok_pressed()
{
    if(le_value_->text().length() > 0) {
        emit sg_add_new_host(le_value_->text());
    }
    close();
}

//===============================================================
//================ OPCTagsViewerModel ===========================
//===============================================================

OPCTagsViewerModel::OPCTagsViewerModel(const std::vector<QString>& tags, const QString& tag_prefix, OPC_HELPER::OPCDataManager& opc_manager, QObject *parent)
    : QAbstractTableModel(parent)
    , opc_manager_(opc_manager)
    , tags_(tags)
    , tag_prefix_(tag_prefix)
{

}

int OPCTagsViewerModel::rowCount(const QModelIndex &parent) const
{
    return tags_.size();
}

int OPCTagsViewerModel::columnCount(const QModelIndex &parent) const
{
    return 2;
}

QVariant OPCTagsViewerModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid()) return {};

    if(role == Qt::DisplayRole && index.column() == 0) {
        if(index.row() < tags_.size()) {
            return tags_.at(index.row());
        } else {
            return {};
        }
    }

    if(role == Qt::DisplayRole && index.column() == 1) {
        auto tag_status = opc_manager_.CheckTagReadState(QString("%1#%2").arg(tag_prefix_, tags_.at(index.row())));
        return tag_status == OPC_HELPER::TAG_STATUS::PERIODIC_READ || tag_status == OPC_HELPER::TAG_STATUS::READ_BOTH;
    }

    if(role == Qt::BackgroundRole) {
        return {};
    }

    if(role == Qt::TextAlignmentRole) {
        if(index.column() == 0) {
            return Qt::AlignLeft;
        }
        return Qt::AlignCenter;
    }
    return {};
}

bool OPCTagsViewerModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid()) return false;

    if(index.column() == 1 && index.row() < tags_.size())
    {
        QString full_tag_name = QString("%1#%2").arg(tag_prefix_, tags_.at(index.row()));
        if(value.toBool()) {
            opc_manager_.AddTagToPeriodicReadList(full_tag_name);
        } else {
            opc_manager_.DeleteTagFromPeriodicRead(full_tag_name);
        }
        return true;
    }
    return false;
}

QModelIndex OPCTagsViewerModel::index(int row, int column, const QModelIndex &parent) const
{
    if(row >=0 && row < static_cast<int>(tags_.size()) && column >=0 && column < 2) {
        return createIndex(row, column);
    }
    return QModelIndex();
}

QVariant OPCTagsViewerModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch(section) {
        case 0: return QString("Имя тэга");
        case 1: return QString("Контроль\nизменения");
        }
    }
    if(role == Qt::DisplayRole && orientation == Qt::Vertical) {
        return section + 1;
    }

    if(role == Qt::TextAlignmentRole) {
        return Qt::AlignCenter;
    }

    return {};
}

void OPCTagsViewerModel::reset()
{
    QAbstractTableModel::beginResetModel();
    QAbstractTableModel::endResetModel();
}


//===================================================================
//================ SelectReadModeCheckBox ===========================
//===================================================================

void SelectReadModeCheckBox::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (index.column() == 1) {
        painter->save();
        QCheckBox cb_prototype;
        cb_prototype.setChecked(index.data().toBool());
        const qreal x = option.rect.center().x() - cb_indicator_size_ / 2;
        const qreal y = option.rect.center().y() - cb_indicator_size_ / 2;
        painter->translate(QPointF(x, y));
        cb_prototype.render(painter);
        painter->restore();
    } else {
        QStyledItemDelegate::paint(painter, option, index);
    }
}

bool SelectReadModeCheckBox::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index)
{

    if(event->type() == QEvent::MouseButtonRelease)
    {
        model->setData(index, !model->data(index).toBool());
        event->accept();
        return true;
    }
    return false;
}
