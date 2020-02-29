#ifndef slsDetMessage_H
#define slsDetMessage_H

#include <epicsTypes.h>

#include <string>

/** Class definition for the SlsDetMessage class
 *   */
class SlsDetMessage {
public:
  /** Message types used by SlsDetDriver**/
  typedef enum {
    NoOp,
    Ok,
    Error,
    Exit,
    Invalid,
    Timeout,
    Failed,
    CheckOnline,
    ReadHostname,
    ReadRunStatus,
    ReadNumDetectors,
    ReadFpgaTemp,
    ReadAdcTemp,
    ReadPowerChip,
    WritePowerChip,
  } MessageType;

  /** Data types used by SlsDetDriver**/
  typedef enum {
    None,
    Int32,
    Int64,
    Float64,
    String,
  } DataType;

  typedef union {
    epicsInt32   ival;
    epicsInt64   i64val;
    epicsFloat64 dval;
    void         *pgp;
  } Storage;

public:
  SlsDetMessage();
  SlsDetMessage(MessageType mtype);
  SlsDetMessage(MessageType mtype, DataType dtype);

  MessageType mtype() const;
  DataType dtype() const;

  epicsInt32 asInteger() const;
  epicsInt64 asInteger64() const;
  epicsFloat64 asDouble() const;
  std::string asString() const;

  bool getInteger(epicsInt32* value) const;
  bool getInteger64(epicsInt64* value) const;
  bool getDouble(epicsFloat64* value) const;
  bool getString(std::string& value) const;

  bool setInteger(epicsInt32 value);
  bool setInteger64(epicsInt64 value);
  bool setDouble(epicsFloat64 value);
  bool setString(const std::string& value);

  std::string dump() const;

  static std::string messageType(MessageType mtype);
  static std::string dataType(DataType dtype);

private:
  MessageType _mtype;
  DataType    _dtype;
  Storage     _data;
};

#endif
