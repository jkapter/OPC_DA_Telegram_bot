#ifndef TGOBJECT_H
#define TGOBJECT_H

#include <QDebug>
#include <QTimer>

#include <string>
#include <charconv>
#include <functional>
#include <unordered_set>
#include <ranges>

#include "tgbot/Bot.h"
#include "opcdatamanager.h"

enum class USER_TYPE {
    UNDEFINED,
    NEW_USER,
    UNREGISTERED,
    REGISTRD,
    CHANNEL,
    ADMIN,
    BANNED
};

enum class TGOBJECT_TYPE {
    MESSAGES,
    EVENTS,
    COMMANDS,
    BUTTONS
};

QString tg_user_type_to_qstring(USER_TYPE type);
USER_TYPE tg_user_type_from_qstring(QString type);

class TGParent {
public:
    explicit TGParent(OPC_HELPER::OPCDataManager* opc_ptr);
    OPC_HELPER::OPCDataManager* OPCManager() const;
    TgBot::Bot* Bot();
    void AddOrUpdateChatID(int64_t user, USER_TYPE type);
    void DeleteChatID(int64_t user);
    void ClearChatIdData();
    void InitializeBot(const std::string& token);
    void ResetTgBotPtr();
    std::vector<int64_t> GetChatIDs(USER_TYPE min_auth_level = USER_TYPE::UNDEFINED) const;
    bool CheckUserChatId(int64_t chat_id, USER_TYPE min_auth_level) const;
    void BotSendMessage(int64_t chat_id, const std::string& text, TgBot::InlineKeyboardMarkup::Ptr inline_buttons = nullptr);
    const std::unordered_set<int64_t>& GetInactiveUsers() const;
    void SetBotNameForChannel(QString name);

private:
    OPC_HELPER::OPCDataManager* opc_ptr_ = nullptr;
    std::unique_ptr<TgBot::Bot> bot_ptr_;
    std::unordered_map<USER_TYPE, std::unordered_set<int64_t>> user_permission_to_chat_id_;
    std::unordered_set<int64_t> inactive_users_;
    std::optional<QString> bot_name_for_channel_;
};

class TGObject
{
public:
    explicit TGObject(TGParent* parent);
    TGParent* GetTGParent() const;
    const std::string& GetId() const;
    void SetId(const std::string& id);
    const std::string& GenerateRandomId();
    virtual QJsonObject SaveToJson() const = 0;
    void SetObjectInWork(bool b);
    bool IsInWork() const;
protected:
    TGParent* parent_;
    std::string id_;
    bool in_work_ = false;
};

class TGButtonWCallback;

class TGMessage: public TGObject
{
public:
    explicit TGMessage(TGParent* parent): TGObject(parent) {}
    explicit TGMessage(const std::string& message, TGParent* parent);
    explicit TGMessage(std::string&& message, TGParent* parent);

    void SetText(const std::string& mes);
    void SetText(const std::string&& mes);
    const std::string& GetText() const;
    const std::string GetTextToSend() const;
    std::vector<size_t> GetOPCTagIDs() const;

    bool HasButtons() const;
    bool HasTags() const;

    void Send(int64_t chat_id) const;

    void AddTGInlineButton(TGButtonWCallback* btn);
    void DeleteInlineButton(std::string& id);
    void ClearInlineButtons();
    std::vector<TGButtonWCallback*> GetButtons() const;

    virtual QJsonObject SaveToJson() const override;
protected:
    std::string message_;
    std::vector<TGButtonWCallback*> inline_buttons_;
    void screen_symbols_(std::string& text, const std::string& symbols) const;
    const std::string screened_symbols_= ".=-()";
    void parse_message_();
    void get_tags_ptr_();
    std::list<std::string> message_parts_;
    std::unordered_map<const std::string*, size_t> segments_to_tag_id_;
    std::unordered_map<size_t, std::shared_ptr<OPC_HELPER::OPCTag>> id_to_opc_tags_;
};

class TGTrigger: public TGObject {
public:
    TGTrigger(TGParent* parent) : TGObject(parent) {}
    void AddTGMessage(const TGMessage* message);
    void DeleteMessage(const std::string& id);
    void ClearMessages();
    const std::vector<const TGMessage*> GetMessages() const;
    bool HasMessages() const;

    void SetAuthorizationLevel(USER_TYPE type);
    USER_TYPE GetAuthorizationLevel() const;

    void AddOPCTagWValue(std::shared_ptr<OPC_HELPER::OPCTag>& tag, OPC_HELPER::OpcValueType value);
    const std::unordered_map<std::shared_ptr<OPC_HELPER::OPCTag>, OPC_HELPER::OpcValueType>& GetOpcTagsWSetValues() const;
    void ClearTagsToWrite();

protected:
    USER_TYPE user_type_ = USER_TYPE::UNDEFINED;
    std::unordered_map<std::shared_ptr<OPC_HELPER::OPCTag>, OPC_HELPER::OpcValueType> opc_tags_to_set_values_;

private:
    std::vector<const TGMessage*> messages_;
};

class TGTriggerUserCommand: public TGTrigger
{
public:
    TGTriggerUserCommand(TGParent* parent) : TGTrigger(parent) {}
    TGTriggerUserCommand(std::string& command, TGParent* parent);
    TGTriggerUserCommand(std::string&& command, TGParent* parent);
    void RegisterTrigger();
    void SetCommand(const std::string& command, const std::string& description);
    void IncludeToMainMenu(bool b);
    bool IncludedToMainMenu() const;
    std::pair<const std::string&, const std::string&> GetCommandAndDescription() const;
    TgBot::BotCommand::Ptr GetTGCommand() const;
    QJsonObject SaveToJson() const override;

private:
    std::string command_;
    std::string description_;
    bool included_to_main_menu_ = false;
};

enum class COMPARE_TYPE {
    EQUAL,
    NOT_EQUAL,
    LESS,
    GREATER,
    CHANGED
};

class TGTriggerTagValue: public TGTrigger
{
public:
    TGTriggerTagValue(TGParent* parent): TGTrigger(parent) {}
    void SetTagTrigger(std::shared_ptr<OPC_HELPER::OPCTag> tag_ptr, COMPARE_TYPE type, OPC_HELPER::OpcValueType value, OPC_HELPER::OpcValueType hysterezis);
    std::tuple<const OPC_HELPER::OPCTag*, COMPARE_TYPE, OPC_HELPER::OpcValueType, OPC_HELPER::OpcValueType> GetTagTrigger() const;
    bool CheckTrigger();
    void Execute() const;
    QJsonObject SaveToJson() const override;

private:
    OPC_HELPER::OpcValueType iVal_;
    OPC_HELPER::OpcValueType lastiVal_;
    OPC_HELPER::OpcValueType hysterezis_;
    COMPARE_TYPE type_;
    bool previous_state_ = false;
    bool first_scan_ = true;
    std::shared_ptr<OPC_HELPER::OPCTag> tag_ptr_ = nullptr;
    std::function<bool()> CheckF_ = [](){static bool b = false; return b;};
    std::function<bool()> CheckFHyst_ = [](){static bool b = false; return b;};
};

class TGButtonWCallback: public TGTrigger
{
public:
    TGButtonWCallback(TGParent* parent);
    void SetButtonName(const std::string& name);
    std::string GetButtonName() const;
    TgBot::InlineKeyboardButton::Ptr GetTgInlineButton();
    std::tuple<const std::string&, std::function<void(const TgBot::CallbackQuery::Ptr)>> GetCallBack() const;
    QJsonObject SaveToJson() const override;
private:
    TgBot::InlineKeyboardButton::Ptr tgbot_inline_button_;
};

class PeriodicalTask {
public:
    enum class Period {HOURLY, DAILY, WEEKLY, MONTHLY, YEARLY, NONE};

    PeriodicalTask(): period_(Period::HOURLY) {}
    PeriodicalTask(Period period): period_(period) {}
    void SetTime(QTime time);
    void SetPeriod(Period period);
    void SetDate(QDate date);
    int SetDayOfWeek(int day_of_week);
    std::tuple<PeriodicalTask::Period, QDate, QTime, int> GetTask() const;
    bool Check() const;
    QJsonObject ToJson() const;
    static std::optional<PeriodicalTask> FromJson(const QJsonObject& json_obj);
private:
    Period period_;
    std::optional<int> minute_;
    std::optional<int> hour_;
    std::optional<int> day_of_week_;
    std::optional<int> day_;
    std::optional<int> month_;
};

class TGScheduledEvent: public TGTrigger
{
public:
    TGScheduledEvent(TGParent* parent, PeriodicalTask::Period period);
    bool CheckTrigger();
    void Execute() const;
    void SetTask(const PeriodicalTask& task);
    const PeriodicalTask& GetTask() const;
    QJsonObject SaveToJson() const override;
private:
    PeriodicalTask task_;
    bool task_last_check_;
    bool first_scan_ = true;
};

#endif // TGOBJECT_H
