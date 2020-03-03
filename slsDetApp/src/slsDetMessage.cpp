#include "slsDetMessage.h"

#include <cstring>
#include <sstream>

#define MAX_MQ_CAPACITY 4
#define MAX_DETS 1
#define DEFAULT_POLL_TIME 0.250
#define ENUM_TO_STR(name)     \
  case name:                  \
    return std::string(#name)

std::string SlsDetMessage::messageType(MessageType mtype)
{
  switch(mtype) {
  ENUM_TO_STR(NoOp);
  ENUM_TO_STR(Ok);
  ENUM_TO_STR(Error);
  ENUM_TO_STR(Exit);
  ENUM_TO_STR(Invalid);
  ENUM_TO_STR(Timeout);
  ENUM_TO_STR(Failed);
  ENUM_TO_STR(CheckOnline);
  ENUM_TO_STR(ReadHostname);
  ENUM_TO_STR(ReadDetType);
  ENUM_TO_STR(ReadRunStatus);
  ENUM_TO_STR(ReadNumDetectors);
  ENUM_TO_STR(ReadSerialnum);
  ENUM_TO_STR(ReadFirmwareVer);
  ENUM_TO_STR(ReadSoftwareVer);
  ENUM_TO_STR(ReadFpgaTemp);
  ENUM_TO_STR(ReadAdcTemp);
  ENUM_TO_STR(ReadTempThreshold);
  ENUM_TO_STR(WriteTempThreshold);
  ENUM_TO_STR(ReadTempControl);
  ENUM_TO_STR(WriteTempControl);
  ENUM_TO_STR(ReadTempEvent);
  ENUM_TO_STR(WriteTempEvent);
  ENUM_TO_STR(ReadPowerChip);
  ENUM_TO_STR(WritePowerChip);
  ENUM_TO_STR(ReadHighVoltage);
  ENUM_TO_STR(WriteHighVoltage);
  ENUM_TO_STR(ReadClockDivider);
  ENUM_TO_STR(WriteClockDivider);
  ENUM_TO_STR(ReadGainMode);
  ENUM_TO_STR(WriteGainMode);
  default:
    return std::string("Unknown");
  }
}

std::string SlsDetMessage::dataType(DataType dtype)
{
  switch(dtype) {
  ENUM_TO_STR(None);
  ENUM_TO_STR(Int32);
  ENUM_TO_STR(Int64);
  ENUM_TO_STR(Float64);
  ENUM_TO_STR(String);
  default:
    return std::string("Unknown");
  }
}

SlsDetMessage::SlsDetMessage() :
  _mtype(NoOp),
  _dtype(None)
{
  std::memset(&_data, 0, sizeof(_data));
}

SlsDetMessage::SlsDetMessage(MessageType mtype) :
  _mtype(mtype),
  _dtype(None)
{
  std::memset(&_data, 0, sizeof(_data));
}

SlsDetMessage::SlsDetMessage(MessageType mtype, DataType dtype) :
  _mtype(mtype),
  _dtype(dtype)
{
  std::memset(&_data, 0, sizeof(_data));
}

SlsDetMessage::MessageType SlsDetMessage::mtype() const
{
  return _mtype;
}

SlsDetMessage::DataType SlsDetMessage::dtype() const
{
  return _dtype;
}

epicsInt32 SlsDetMessage::asInteger() const
{
  if (_dtype == Int32) {
    return _data.ival;
  } else {
    return 0;
  }
}

epicsInt64 SlsDetMessage::asInteger64() const
{
  if (_dtype == Int64) {
    return _data.i64val;
  } else {
    return 0;
  }
}

epicsFloat64 SlsDetMessage::asDouble() const
{
  if (_dtype == Float64) {
    return _data.dval;
  } else {
    return 0.0;
  }
}

std::string SlsDetMessage::asString() const
{
  if (_dtype == String) {
    return std::string((const char*) _data.pgp);
  } else {
    return std::string();
  }
}

bool SlsDetMessage::getInteger(epicsInt32* value) const
{
  if (value && _dtype ==Int32) {
    *value = _data.ival;
    return true;
  } else {
    return false;
  }
}

bool SlsDetMessage::getInteger64(epicsInt64* value) const
{  
  if (value && _dtype == Int64) {
    *value = _data.i64val;
    return true;
  } else {
    return false;
  }
}

bool SlsDetMessage::getDouble(epicsFloat64* value) const
{
  if (value && _dtype == Float64) {
    *value = _data.dval;
    return true;
  } else {
    return false;
  }
}

bool SlsDetMessage::getString(std::string& value) const
{
  if (_dtype == String) {
    value.assign((const char*) _data.pgp);
    return true;
  } else {
    return false;
  }
}

bool SlsDetMessage::setInteger(epicsInt32 value)
{
  if (_dtype == Int32) {
    _data.ival = value;
    return true;
  } else {
    return false;
  }
}

bool SlsDetMessage::setInteger64(epicsInt64 value)
{
  if (_dtype == Int64) {
    _data.i64val = value;
  return true;
  } else {
    return false;
  }
}

bool SlsDetMessage::setDouble(epicsFloat64 value)
{
  if (_dtype == Float64) {
    _data.dval = value;
    return true;
  } else {
    return false;
  }
}

bool SlsDetMessage::setString(const char* value)
{
  if (_dtype == String) {
    _data.pgp = (void *) value;
    return true;
  } else {
    return false;
  }
}

std::string SlsDetMessage::dump() const
{
  std::ostringstream stream;
  stream << "Message(" << messageType(_mtype) << ", ";
  stream << dataType(_dtype);
  switch (_dtype) {
  case Int32:
    stream << ", " << _data.ival;
    break;
  case Int64:
    stream << ", " << _data.i64val;
    break;
  case Float64:
    stream << ", " << _data.dval;
    break;
  case String:
    stream << ", " << (const char*) _data.pgp;
    break;
  default:
    break;
  }
  stream << ")";
  return stream.str();
}
