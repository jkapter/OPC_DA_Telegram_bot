#ifndef SELECTITEMSTABLEWIGGET_H
#define SELECTITEMSTABLEWIGGET_H

#include <QWidget>
#include <QTableWidget>
#include <QComboBox>
#include <QScrollBar>
#include <QHeaderView>
#include <QObject>

#include "tgbotmanager.h"

enum class SITW_TYPE {
    Messages,
    InlineButtons
};

class SelectItemsTableWidget : public QTableWidget
{
    Q_OBJECT
public:
    explicit SelectItemsTableWidget(TgBotManager& bot_manager, SITW_TYPE type, int rows_num, QWidget *parent = nullptr);
    void ReadCommandContent(const TGTrigger* command);
    void SetMessagesToCommand(TGTrigger* command);
    void ReadMessageContent(const TGMessage* message);
    void ResetContent();
    void SetButtonsToMessage(TGMessage* message);

signals:

private slots:
    void sl_table_item_changed(int cb_index);

protected:
    virtual void showEvent(QShowEvent* ev) override;
    virtual void resizeEvent(QResizeEvent* ev) override;
private:
    TgBotManager& bot_manager_;
    SITW_TYPE content_type_;

    void set_column_width_();
    void update_content_messages_();
    void update_content_buttons_();
};

#endif // SELECTITEMSTABLEWIGGET_H
