#ifndef OPCTAG_H
#define OPCTAG_H

#include <QString>
#include <QDebug>

#include "opcda.h"

namespace OPC_HELPER {

QString GetVTypeString(const unsigned short usType);
QString GetQualityString(const unsigned short usQuality);
QString GetFTString(const FILETIME& ft);

using OpcValueType = std::variant<int64_t, double, QString>;
OpcValueType operator-(OpcValueType lhs, OpcValueType rhs);
OpcValueType operator+(OpcValueType lhs, OpcValueType rhs);
bool operator<(OpcValueType lhs, OpcValueType rhs);
bool operator>(OpcValueType lhs, OpcValueType rhs);
OpcValueType operator==(OpcValueType lhs, OpcValueType rhs);

QString toString(OpcValueType val);
double toDouble(OpcValueType val);
int64_t toLongLong(OpcValueType val);

class OPCTag
{
public:
    OPCTag() = delete;
    explicit OPCTag(const QString& server_name, const QString& tag_name);

    void SetType(VARENUM type);
    tagOPCITEMDEF GetItemDefStruct();
    void SetOPCItemState(tagOPCITEMSTATE* item_state);
    FILETIME GetLastTimeState() const;
    QString GetServerName() const;
    QString GetTagName() const;
    QString GetStringValue(bool use_substitute_values = true) const;
    QString GetStringType() const;
    int64_t GetIntValue() const;
    uint64_t GetUintValue() const;
    double GetDoubleValue() const;
    bool GetBoolValue() const;
    WORD GetTagQuality() const;
    bool TagQualityIsGood() const;
    QString GetTimeStamp() const;
    bool ValueIsInteger() const;
    bool ValueIsReal() const;
    bool ValueIsUnsignedInteger() const;
    bool ValueIsString() const;
    bool ValueIsBool() const;
    QString GetCommentString() const;
    void SetCommentString(QString str);
    OpcValueType GetValue(bool use_substitute_values = true) const;
    void SetGainOption(double gain);
    void ResetGainOption();
    std::optional<double> GetGainOption() const;
    void AddEnumStringValues(QString raw_value, QString substitute_value);
    const std::unordered_map<QString, QString>& GetEnumStringValues() const;
    void ClearEnumStringValues();

    bool SetValueToWrite(OpcValueType val);
    std::optional<VARIANT> GetOPCVariantToWrite() const;
    void ResetValueToWrite();

private:
    QString server_name_;
    QString tag_name_;
    VARENUM type_ = VT_EMPTY;
    tagOPCITEMSTATE last_value_;
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
