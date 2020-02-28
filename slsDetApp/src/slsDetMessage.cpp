#include "slsDetMessage.h"

#include <cstring>
#include <sstream>

#define MAX_MQ_CAPACITY 4
#define MAX_DETS 1
#define DEFAULT_POLL_TIME 0.250

std::string SlsDetMessage::messageType(MessageType mtype)
{
  switch(mtype) {
  case NoOp:
    return std::string("NoOp");
  case Ok:
    return std::string("Ok");
  case Error:
    return std::string("Error");
  case Exit:
    return std::string("Exit");
  case Invalid:
    return std::string("Invalid");
  case Timeout:
    return std::string("Timeout");
  case CheckOnline:
    return std::string("CheckOnline");
  case ReadHostname:
    return std::string("ReadHostname");
  case ReadRunStatus:
    return std::string("ReadRunStatus");
  case ReadFpgaTemp:
    return std::string("ReadFpgaTemp");
  case ReadAdcTemp:
    return std::string("ReadAdcTemp");
  default:
    return std::string("Unknown");
  }
}

std::string SlsDetMessage::dataType(DataType dtype)
{
  switch(dtype) {
  case None:
    return std::string("None");
  case Int32:
    return std::string("Int32");
  case Int64:
    return std::string("Int64");
  case Float64:
    return std::string("Float64");
  case String:
    return std::string("String");
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
  if (_dtype ==Int32) {
    return _data.ival;
  } else {
    return 0;
  }
}

epicsInt64 SlsDetMessage::asInteger64() const
{
  if (_dtype ==Int32) {
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

bool SlsDetMessage::setString(const std::string& value)
{
  if (_dtype == String) {
    _data.pgp = (void *) value.c_str();
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
