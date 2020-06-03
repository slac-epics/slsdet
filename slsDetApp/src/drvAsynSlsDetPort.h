#ifndef drvAsynSlsDetPort_H
#define drvAsynSlsDetPort_H

#include "slsDetMessage.h"

#include <sls_detector_defs.h>
#include <asynPortDriver.h>
#include <alarm.h>

#include <vector>

/* Check if the asyn version supports 64bit ints */
#if ASYN_VERSION >= 4 && ASYN_REVISION >= 37
#define ASYN_HAS_INT64
#endif

#define SLS_MAX_ENUMS 16

class SlsDetDriver;

/** Class definition for the SlsDet class
  */
class SlsDet : public asynPortDriver {
public:
  SlsDet(const char *portName, const std::vector<std::string>& hostnames, int id, double timeout);
  virtual ~SlsDet();

  /* These are the methods that we override from asynPortDriver */
  virtual asynStatus connect(asynUser *pasynUser);
  virtual asynStatus disconnect(asynUser *pasynUser);
  virtual asynStatus readInt32(asynUser *pasynUser, epicsInt32 *value);
  virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
  virtual asynStatus readFloat64(asynUser *pasynUser, epicsFloat64 *value);
  virtual asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value);
  virtual asynStatus readOctet(asynUser *pasynUser,
                               char *value, size_t maxChars, size_t *nActual,
                               int *eomReason);
  virtual asynStatus readEnum(asynUser *pasynUser, char *strings[], int values[],
                              int severities[], size_t nElements, size_t *nIn);
  /* cleans up the slsDetectorPackage resources */
  virtual void shutdown();

protected:
  /* These are the methods that communicate with the detector */
  virtual asynStatus readDetector(asynUser *pasynUser, SlsDetMessage::MessageType mtype);
  virtual asynStatus writeDetector(asynUser *pasynUser, SlsDetMessage msg);
  virtual asynStatus writeDetector(asynUser *pasynUser, SlsDetMessage::MessageType mtype,
                                   epicsFloat64 value);
  virtual asynStatus writeDetector(asynUser *pasynUser, SlsDetMessage::MessageType mtype,
                                   epicsInt32 value);
  virtual asynStatus initialize(asynUser *pasynUser);
  virtual asynStatus uninitialize(asynUser *pasynUser);
  virtual int isConnected(int addr);
  virtual void updateEnums(int addr);
  // enum information
  enum ConnectionStatus { DISCONNECTED=0, CONNECTED=1 };
  enum OnOff { OFF=0, ON=1 };
  enum OkTripped { OK=0, TRIPPED=1 };
  enum ClockSpeed { FULL=0, HALF=1, QUARTER=2 };
  typedef struct {
    const std::string name;
    int value;
    epicsAlarmSeverity severity;
  } SlsDetEnumInfo;
  typedef struct {
    const SlsDetEnumInfo *enums;
    size_t size;
    const char *name;
  } SlsDetEnumSet;
  static const SlsDetEnumInfo SlsOnOffEnums[];
  static const SlsDetEnumInfo SlsOkTrippedEnums[];
  static const SlsDetEnumInfo SlsConnStatusEnums[];
  static const SlsDetEnumInfo SlsRunStatusEnums[];
  static const SlsDetEnumInfo SlsDetTypesEnums[];
  static const SlsDetEnumInfo SlsClockDivEnums[];
  static const SlsDetEnumInfo SlsGainEnums[];
  static const SlsDetEnumSet SlsDetEnums[];
  static const size_t SlsDetEnumsSize;
  char* _enumStrings[SLS_MAX_ENUMS];
  int   _enumValues[SLS_MAX_ENUMS];
  int   _enumSeverities[SLS_MAX_ENUMS];
  // parameters
  int _initValue;
  int _numDetValue;
  int _runStatusValue;
  int _connStatusValue;
  int _hostNameValue;
  int _detTypeValue;
  int _detEnabledValue;
  int _detSerialNumberValue;
  int _detFirmwareVersionValue;
  int _detSoftwareVersionValue;
  int _fpgaTempValue;
  int _adcTempValue;
  int _getTempThresholdValue;
  int _setTempThresholdValue;
  int _getTempControlValue;
  int _setTempControlValue;
  int _getTempEventValue;
  int _setTempEventValue;
  int _getChipPowerValue;
  int _setChipPowerValue;
  int _getHighVoltageValue;
  int _setHighVoltageValue;
  int _getClockDividerValue;
  int _setClockDividerValue;
  int _getGainModeValue;
  int _setGainModeValue;

private:
  typedef std::vector<SlsDetDriver*> SlsDetList;
  typedef SlsDetList::iterator SlsDetListIter;

private:
  const int                 _id;
  const double              _timeout;
  std::vector<std::string>  _hostnames;
  SlsDetList                _dets;
};

#endif
