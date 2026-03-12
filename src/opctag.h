#ifndef OPCTAG_H
#define OPCTAG_H

#include <QObject>
#include <QString>

#include "opcda.h"

#include <variant>
#include <cstdint>
#include <optional>
#include <unordered_map>
#include <string>

namespace OPC_HELPER {

QString GetVTypeString(const unsigned short usType);
QString GetQualityString(const unsigned short usQuality);
QString GetFTString(const FILETIME& ft);

using OpcValueType = std::variant<int64_t, double, QString>;
OpcValueType operator-(OpcValueType lhs, OpcValueType rhs);
OpcValueType operator+(OpcValueType lhs, OpcValueType rhs);
bool operator<(OpcValueType lhs, OpcValueType rhs);
bool operator>(OpcValueType lhs, OpcValueType rhs);
bool operator==(OpcValueType lhs, OpcValueType rhs);

QString toString(OpcValueType val);
double toDouble(OpcValueType val);
int64_t toLongLong(OpcValueType val);

class OPCTag
{
public:
    OPCTag() = delete;
    explicit OPCTag(const QString& fullname);
    explicit OPCTag(const QString& server_name, const QString& tag_name);
    explicit OPCTag(const QString& hostname, const QString& server_name, const QString& tag_name);

    //void SetType(VARENUM type);

    tagOPCITEMDEF GetItemDefStruct(); //client
    void SetOPCItemState(tagOPCITEMSTATE* item_state);//client

    FILETIME GetLastTimeState() const;
    QString GetTimeStamp() const;

    const QString& GetServerName() const; //all
    const  QString& GetTagName() const; // all
    const QString& GetHostname() const; // client, dm
    QString GetFullName() const;

    QString GetStringType() const; //viewer, widgets
    QString GetStringValue(bool use_substitute_values = true) const; //viewer, tgman
    int64_t GetIntValue() const;
    uint64_t GetUintValue() const;
    double GetDoubleValue() const;
    bool GetBoolValue() const;
    OpcValueType GetValue(bool use_substitute_values = true) const; //tgobj

    WORD GetTagQuality() const; //viewer, tgman
    bool TagQualityIsGood() const; //tgobject

    bool ValueIsInteger() const; //viewer, widgets
    bool ValueIsReal() const; //viewer, widgets
    bool ValueIsUnsignedInteger() const; //viewer, widgets
    bool ValueIsString() const; //viewer, widgets
    bool ValueIsBool() const; //viewer, widgets

    const QString& GetCommentString() const; //viewer, opcman
    void SetCommentString(const QString& str); //viewer, opcman
    void SetCommentString(QString&& str); //viewer, opcman

    std::optional<double> GetGainOption() const; //opcman, widget
    void SetGainOption(double gain); //opcman, widget
    void ResetGainOption(); //widget

    void AddEnumStringValues(const QString& raw_value, const QString& substitute_value); //opcman, widget
    const std::unordered_map<QString, QString>& GetEnumStringValues() const; //viewer, opcman, widget
    void ClearEnumStringValues();

    bool SetValueToWrite(OpcValueType val); //viewer, tgobj
    std::optional<VARIANT> GetOPCVariantToWrite() const; //client
    void ResetValueToWrite();

private:
    QString hostname_;
    QString server_name_;
    QString tag_name_;
    VARENUM type_ = VT_EMPTY;
    tagOPCITEMSTATE last_value_ = {};
    std::optional<VARIANT> value_to_write_ = std::nullopt;
    std::wstring buffer_string_;
    std::wstring tag_name_wstring_;
    QString comment_;
    std::optional<double> gain_value_ = std::nullopt;
    std::unordered_map<QString, QString> substitute_values_;
};

/*
const WORD OPC_QUALITY_MASK	=	0xc0;
const WORD OPC_STATUS_MASK	=	0xfc;
const WORD OPC_LIMIT_MASK	=	0x3;
const WORD OPC_QUALITY_BAD	=	0;
const WORD OPC_QUALITY_UNCERTAIN	=	0x40;
const WORD OPC_QUALITY_GOOD	=	0xc0;
const WORD OPC_QUALITY_CONFIG_ERROR	=	0x4;
const WORD OPC_QUALITY_NOT_CONNECTED	=	0x8;
const WORD OPC_QUALITY_DEVICE_FAILURE	=	0xc;
const WORD OPC_QUALITY_SENSOR_FAILURE	=	0x10;
const WORD OPC_QUALITY_LAST_KNOWN	=	0x14;
const WORD OPC_QUALITY_COMM_FAILURE	=	0x18;
const WORD OPC_QUALITY_OUT_OF_SERVICE	=	0x1c;
const WORD OPC_QUALITY_WAITING_FOR_INITIAL_DATA	=	0x20;
const WORD OPC_QUALITY_LAST_USABLE	=	0x44;
const WORD OPC_QUALITY_SENSOR_CAL	=	0x50;
const WORD OPC_QUALITY_EGU_EXCEEDED	=	0x54;
const WORD OPC_QUALITY_SUB_NORMAL	=	0x58;
const WORD OPC_QUALITY_LOCAL_OVERRIDE	=	0xd8;
const WORD OPC_LIMIT_OK	=	0;
const WORD OPC_LIMIT_LOW	=	0x1;
const WORD OPC_LIMIT_HIGH	=	0x2;
const WORD OPC_LIMIT_CONST	=	0x3;
*/

} //namespace
#endif // OPCTAG_H
