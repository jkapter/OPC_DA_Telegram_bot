#include "opctag.h"

using namespace OPC_HELPER;

QString OPC_HELPER::GetVTypeString(const unsigned short usType)
{
    switch (usType)    {
    case VARENUM::VT_R4 : return QString("VT_R4");
    case VARENUM::VT_R8 : return QString("VT_R8");
    case VARENUM::VT_ARRAY : return QString("VT_ARRAY");
    case VARENUM::VT_BOOL : return QString("VT_BOOL");
    case VARENUM::VT_BSTR : return QString("VT_BSTR");
    case VARENUM::VT_DECIMAL : return QString("VT_DECIMAL");
    case VARENUM::VT_I1 : return QString("VT_I1");
    case VARENUM::VT_I2 : return QString("VT_I2");
    case VARENUM::VT_I4 : return QString("VT_I4");
    case VARENUM::VT_I8 : return QString("VT_I8");
    case VARENUM::VT_UI2: return QString("VT_UI2");
    case VARENUM::VT_UI4: return QString("VT_UI4");
    case VARENUM::VT_UI1: return QString("VT_UI1");
    case VARENUM::VT_FILETIME : return QString("FILETIME");
    default : return QString("Unknown Type");
    }
}
QString OPC_HELPER::GetQualityString(const unsigned short usQuality)
{
    switch (usQuality) {
    case 0x00: return "Bad";
    case 0x04: return "Config Error";
    case 0x08: return "Not Connected";
    case 0x0C: return "Device Failure";
    case 0x10: return "Sensor Failure";
    case 0x14: return "Last Known";
    case 0x18: return "Comm Failure";
    case 0x1C: return "Out of Service";
    case 0x20: return "Initializing";
    case 0x40: return "Uncertain";
    case 0x44: return "Last Usable";
    case 0x50: return "Sensor Calibration";
    case 0x54: return "EGU Exceeded";
    case 0x58: return "Sub Normal";
    case 0xC0: return "Good";
    case 0xD8: return "Local Override";
    default: return "Unknown";
    }
}

QString OPC_HELPER::GetFTString(const FILETIME& ft)
{
    SYSTEMTIME dt;
    FileTimeToSystemTime(&ft, &dt);
    return QString("%1-%2-%3 %4:%5:%6").arg(dt.wYear).arg(dt.wMonth).arg(dt.wDay).arg(dt.wHour).arg(dt.wMinute).arg(dt.wSecond);
}

OPC_HELPER::OpcValueType operator-(OPC_HELPER::OpcValueType lhs, OPC_HELPER::OpcValueType rhs) {
    switch(lhs.index()) {
    case 0: return std::get<int64_t>(lhs) - toLongLong(rhs);
    case 1: return std::get<double>(lhs) - toDouble(rhs);
    case 2: return std::get<QString>(lhs);
    }
    return 0;
}

OPC_HELPER::OpcValueType operator+(OPC_HELPER::OpcValueType lhs, OPC_HELPER::OpcValueType rhs) {
    switch(lhs.index()) {
    case 0: return std::get<int64_t>(lhs) + toLongLong(rhs);
    case 1: return std::get<double>(lhs) + toDouble(rhs);
    case 2: return QString("%1%2").arg(toString(lhs), toString(rhs));
    }
    return 0;
}

bool operator<(OpcValueType lhs, OpcValueType rhs)
{
    switch(lhs.index()) {
    case 0: return std::get<int64_t>(lhs) < toLongLong(rhs);
    case 1: return std::get<double>(lhs) < toDouble(rhs);
    case 2: return std::get<QString>(lhs) < toString(rhs);
    }
    return false;
}

bool operator==(OpcValueType lhs, OpcValueType rhs)
{
    switch(lhs.index()) {
    case 0: return std::get<int64_t>(lhs) == toLongLong(rhs);
    case 1: return std::get<double>(lhs) == toDouble(rhs);
    case 2: return std::get<QString>(lhs) == toString(rhs);
    }
    return false;
}

bool operator>(OpcValueType lhs, OpcValueType rhs)
{
    switch(lhs.index()) {
    case 0: return std::get<int64_t>(lhs) > toLongLong(rhs);
    case 1: return std::get<double>(lhs) > toDouble(rhs);
    case 2: return std::get<QString>(lhs) > toString(rhs);
    }
    return false;
}

QString OPC_HELPER::toString(OpcValueType val)
{
    if(val.valueless_by_exception()) return "";
    switch(val.index()) {
    case 0: return QString("%1").arg(std::get<0>(val));
    case 1: return QString("%1").arg(std::get<1>(val));
    case 2: return QString("%1").arg(std::get<2>(val));
    }
    return "";
}

double OPC_HELPER::toDouble(OpcValueType val) {
    if(val.index() == 0) return static_cast<double>(std::get<int64_t>(val));
    if(val.index() == 1) return std::get<double>(val);

    QString str = std::get<QString>(val);
    bool b = false;
    double ret_val = str.toDouble(&b);
    if(b) {
        return ret_val;
    } else {
        return 0;
    }
}

int64_t OPC_HELPER::toLongLong(OpcValueType val) {
    if(val.index() == 0) return std::get<int64_t>(val);
    if(val.index() == 1) return std::numeric_limits<int64_t>::max() > std::get<double>(val) ? static_cast<int64_t>(std::get<double>(val)) : std::numeric_limits<int64_t>::max();

    QString str = std::get<QString>(val);
    bool b = false;
    double ret_val = str.toLongLong(&b);
    if(b) {
        return ret_val;
    } else {
        return 0;
    }
}


OPCTag::OPCTag(const QString& server_name, const QString& tag_name)
    : server_name_(server_name)
    , tag_name_(tag_name)
    , tag_name_wstring_(tag_name.toStdWString())
{
}

void OPCTag::SetType(VARENUM type) {
    type_ = type;
}

tagOPCITEMDEF OPCTag::GetItemDefStruct() {
    tagOPCITEMDEF ret_def;
    ret_def.szItemID = tag_name_wstring_.data();
    ret_def.szAccessPath = NULL;
    ret_def.bActive = TRUE;
    ret_def.hClient = 0;
    ret_def.vtRequestedDataType = type_;
    ret_def.dwBlobSize = 0;
    ret_def.pBlob = NULL;
    return ret_def;
}

void OPCTag::SetOPCItemState(tagOPCITEMSTATE* item_state) {
    if(!item_state) return;
    last_value_ = *item_state;
    type_ = VARENUM(last_value_.vDataValue.vt);
}

FILETIME OPCTag::GetLastTimeState() const
{
    return last_value_.ftTimeStamp;
}

QString OPCTag::GetStringValue(bool use_substitute_values) const {
    QString ret_str;
    if(ValueIsInteger()) ret_str = QString::number(GetIntValue());
    if(ValueIsUnsignedInteger()) ret_str = QString::number(GetUintValue());
    if(ValueIsReal()) ret_str = QString::number(GetDoubleValue());
    if(ValueIsBool()) ret_str = GetBoolValue() ? "ДА" : "НЕТ";
    if(ValueIsString()) ret_str = QString::fromStdWString(last_value_.vDataValue.bstrVal);
    if(use_substitute_values && substitute_values_.count(ret_str) > 0) ret_str = substitute_values_.at(ret_str);
    return ret_str;
}

QString OPCTag::GetStringType() const {
    return GetVTypeString(type_);
}

int64_t OPCTag::GetIntValue() const {
    int64_t ret_val = std::numeric_limits<int64_t>::max();

    switch(type_) {
    case VT_I2: ret_val = static_cast<int64_t>(last_value_.vDataValue.iVal); break;
    case VT_I4: ret_val = static_cast<int64_t>(last_value_.vDataValue.lVal); break;
    case VT_I1: ret_val = static_cast<int64_t>(last_value_.vDataValue.iVal); break;
    case VT_I8: ret_val = static_cast<int64_t>(last_value_.vDataValue.llVal); break;
    case VT_INT: ret_val = static_cast<int64_t>(last_value_.vDataValue.intVal); break;
    case VT_R4: ret_val = static_cast<int64_t>(last_value_.vDataValue.fltVal); break;
    case VT_R8: ret_val = static_cast<int64_t>(last_value_.vDataValue.dblVal); break;
    case VT_BOOL: ret_val = last_value_.vDataValue.boolVal == VARIANT_TRUE ? 1 : 0;
    default: ;
    }

    return ret_val;
}

uint64_t OPCTag::GetUintValue() const {
    uint64_t ret_val = std::numeric_limits<uint64_t>::max();

    switch(type_) {
    case VT_UI1: ret_val = static_cast<uint64_t>(last_value_.vDataValue.uiVal); break;
    case VT_UI2: ret_val = static_cast<uint64_t>(last_value_.vDataValue.uiVal); break;
    case VT_UI4: ret_val = static_cast<uint64_t>(last_value_.vDataValue.ulVal); break;
    case VT_UI8: ret_val = static_cast<uint64_t>(last_value_.vDataValue.ullVal); break;
    case VT_UINT: ret_val = static_cast<uint64_t>(last_value_.vDataValue.uintVal); break;
    case VT_BOOL: ret_val = last_value_.vDataValue.boolVal == VARIANT_TRUE ? 1 : 0;
    default: ;
    }

    return ret_val;
}

double OPCTag::GetDoubleValue() const {
    double ret_val = std::numeric_limits<double>::max();

    switch(type_) {
    case VT_R4: ret_val = static_cast<double>(last_value_.vDataValue.fltVal); break;
    case VT_R8: ret_val = static_cast<double>(last_value_.vDataValue.dblVal); break;
    case VT_I2: ret_val = static_cast<double>(last_value_.vDataValue.iVal); break;
    case VT_I4: ret_val = static_cast<double>(last_value_.vDataValue.lVal); break;
    case VT_I1: ret_val = static_cast<double>(last_value_.vDataValue.iVal); break;
    case VT_I8: ret_val = static_cast<double>(last_value_.vDataValue.llVal); break;
    case VT_INT: ret_val = static_cast<double>(last_value_.vDataValue.intVal); break;
    case VT_UI1: ret_val = static_cast<double>(last_value_.vDataValue.uiVal); break;
    case VT_UI2: ret_val = static_cast<double>(last_value_.vDataValue.uiVal); break;
    case VT_UI4: ret_val = static_cast<double>(last_value_.vDataValue.ulVal); break;
    case VT_UI8: ret_val = static_cast<double>(last_value_.vDataValue.ullVal); break;
    case VT_UINT: ret_val = static_cast<double>(last_value_.vDataValue.uintVal); break;
    default: ;
    }

    if(gain_value_.has_value()) {
        ret_val *= gain_value_.value();
    }

    return ret_val;
}

bool OPCTag::GetBoolValue() const {
    return last_value_.vDataValue.boolVal == VARIANT_TRUE;
}

WORD OPCTag::GetTagQuality() const {
    return last_value_.wQuality;
}

bool OPCTag::TagQualityIsGood() const {
    return last_value_.wQuality == OPC_QUALITY_GOOD;
}

QString OPCTag::GetTimeStamp() const {
    return GetFTString(last_value_.ftTimeStamp);
}

bool OPCTag::ValueIsInteger() const {
    return (!gain_value_.has_value())
            && ((type_ == VT_I2)
            || (type_ == VT_I4)
            || (type_ == VT_I1)
            || (type_ == VT_I8)
            || (type_ == VT_INT));
}

bool OPCTag::ValueIsReal() const {
    return (type_ == VT_R4)
            || (type_ == VT_R8)
            || (gain_value_.has_value());
}

bool OPCTag::ValueIsUnsignedInteger() const {
    return (!gain_value_.has_value())
            && ((type_ == VT_UI2)
            || (type_ == VT_UI4)
            || (type_ == VT_UI1)
            || (type_ == VT_UI8)
            || (type_ == VT_UINT));
}

bool OPCTag::ValueIsString() const {
    return type_ == VT_BSTR;
}

bool OPCTag::ValueIsBool() const {
    return type_ == VT_BOOL;
}

QString OPCTag::GetServerName() const {
    return server_name_;
}

QString OPCTag::GetTagName() const {
    return tag_name_;
}

QString OPCTag::GetCommentString() const {
    return comment_;
}

void OPCTag::SetCommentString(QString str) {
    comment_ = str;
}

OpcValueType OPCTag::GetValue(bool use_substitute_values) const
{
    if(ValueIsBool()) return static_cast<int64_t>(GetBoolValue());
    if(ValueIsInteger()) return GetIntValue();
    if(ValueIsUnsignedInteger()) return GetUintValue() < std::numeric_limits<int64_t>::max() ? static_cast<int64_t>(GetUintValue()) : std::numeric_limits<int64_t>::max();
    if(ValueIsReal()) return GetDoubleValue();
    return GetStringValue(use_substitute_values);
}

void OPCTag::SetGainOption(double gain)
{
    gain_value_ = gain;
}

void OPCTag::ResetGainOption()
{
    gain_value_ = std::nullopt;
}

std::optional<double> OPCTag::GetGainOption() const
{
    return gain_value_;
}

void OPCTag::AddEnumStringValues(QString raw_value, QString substitute_value)
{
    substitute_values_[raw_value] = substitute_value;
}

const std::unordered_map<QString, QString>& OPCTag::GetEnumStringValues() const
{
    return substitute_values_;
}

void OPCTag::ClearEnumStringValues()
{
    substitute_values_.clear();
}

bool OPCTag::SetValueToWrite(OpcValueType val)
{
    if(!ValueIsInteger() && !ValueIsReal() && !ValueIsUnsignedInteger() && !ValueIsString() && !ValueIsBool()) return false;
    int64_t iVal = 0;
    double dVal = 0.0;
    QString strVal{};
    if(ValueIsInteger() || ValueIsUnsignedInteger() || ValueIsBool()) iVal = toLongLong(val);
    if(ValueIsReal()) dVal = toDouble(val);
    if(ValueIsString()) strVal = toString(val);
    VARIANT tVar;
    tVar.vt = type_;
    switch(type_) {
    case VT_I2: tVar.iVal = static_cast<SHORT>(iVal); break;
    case VT_I4: tVar.lVal = static_cast<LONG>(iVal); break;
    case VT_I1: tVar.iVal = static_cast<SHORT>(iVal); break;
    case VT_I8: tVar.llVal = static_cast<LONGLONG>(iVal); break;
    case VT_INT: tVar.intVal = static_cast<INT>(iVal); break;
    case VT_R4: tVar.fltVal = static_cast<FLOAT>(dVal); break;
    case VT_R8: tVar.dblVal = static_cast<DOUBLE>(dVal); break;
    case VT_UI2: tVar.uiVal = static_cast<USHORT>(iVal); break;
    case VT_UI4: tVar.ulVal = static_cast<ULONG>(iVal); break;
    case VT_UI1: tVar.uiVal = static_cast<USHORT>(iVal); break;
    case VT_UI8: tVar.ullVal = static_cast<ULONGLONG>(iVal); break;
    case VT_UINT: tVar.uintVal = static_cast<UINT>(iVal); break;
    case VT_BSTR: {
        buffer_string_ = strVal.toStdWString();
        tVar.bstrVal = const_cast<wchar_t*>(buffer_string_.c_str());
        break;
        }
    case VT_BOOL: tVar.boolVal = iVal == 0 ? VARIANT_FALSE : VARIANT_TRUE; break;
    default : return false;
    }
    value_to_write_ = tVar;
    return true;
}

std::optional<VARIANT> OPCTag::GetOPCVariantToWrite() const {
    return value_to_write_;
}

void OPCTag::ResetValueToWrite() {
    value_to_write_.reset();
}
