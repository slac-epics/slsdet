#ifndef drvAsynSlsDetPort_H
#define drvAsynSlsDetPort_H

#include <asynPortDriver.h>
#include <sls_detector_defs.h>
#include <alarm.h>

#include <vector>

#include "slsDetMessage.h"

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
  SlsDet(const char *portName, const std::vector<std::string>& hostnames, int id);
  virtual ~SlsDet();

  /* These are the methods that we override from asynPortDriver */
  virtual asynStatus connect(asynUser *pasynUser);
  virtual asynStatus disconnect(asynUser *pasynUser);
  virtual asynStatus readInt32(asynUser *pasynUser, epicsInt32 *value);
  virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
  virtual asynStatus readFloat64(asynUser *pasynUser, epicsFloat64 *value);
  virtual asynStatus readEnum(asynUser *pasynUser, char *strings[], int values[],
                              int severities[], size_t nElements, size_t *nIn);
  /* cleans up the slsDetectorPackage resources */
  virtual void shutdown();

protected:
  /* These are the methods that communicate with the detector */
  virtual asynStatus readRunStatus(asynUser *pasynUser, epicsInt32 *value);
  virtual asynStatus readDetector(asynUser *pasynUser, SlsDetMessage::MessageType mtype);
  virtual asynStatus initialize(asynUser *pasynUser);
  virtual asynStatus uninitialize(asynUser *pasynUser);
  virtual int isConnected(int addr);
  virtual void updateEnums(int addr);
  // enum information
  enum ConnectionStatus { DISCONNECTED=0, CONNECTED=1 };
  typedef struct {
    const std::string name;
    int value;
    epicsAlarmSeverity severity;
  } SlsDetEnumInfo;
  static const SlsDetEnumInfo SlsConnStatusEnums[];
  static const size_t SlsConnStatusNumEnums;
  static const SlsDetEnumInfo SlsRunStatusEnums[];
  static const size_t SlsRunStatusNumEnums;
  char* _enumStrings[SLS_MAX_ENUMS];
  int   _enumValues[SLS_MAX_ENUMS];
  int   _enumSeverities[SLS_MAX_ENUMS];
  // parameters
  int _initValue;
  int _numDetValue;
  int _runStatusValue;
  int _connStatusValue;
  int _hostNameValue;
  int _fpgaTempValue;
  int _adcTempValue;

private:
  typedef std::vector<SlsDetDriver*> SlsDetList;
  typedef SlsDetList::iterator SlsDetListIter;

private:
  const int                 _id;
  std::vector<std::string>  _hostnames;
  SlsDetList                _dets;
};

#endif
