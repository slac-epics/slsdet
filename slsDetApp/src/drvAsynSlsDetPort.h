#ifndef drvAsynSlsDetPort_H
#define drvAsynSlsDetPort_H

#include <asynPortDriver.h>
#include <epicsEvent.h>
#include <alarm.h>

#include <memory>
#include <vector>

#include "SlsDetUtils.h"

/* Check if the asyn version supports 64bit ints */
#if ASYN_VERSION >= 4 && ASYN_REVISION >= 37
#define ASYN_HAS_INT64
#endif

#define SLS_MAX_ENUMS 16

namespace sls {
  class Detector;
}

/** Class definition for the SlsDet class
  */
class SlsDet : public asynPortDriver {
public:
  SlsDet(const char *portName, const std::string& hostname, int id, double boot);
  virtual ~SlsDet();

  /* These are the methods that we override from asynPortDriver */
  virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
  virtual asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value);
  virtual asynStatus writeOctet(asynUser *pasynUser, const char *value, size_t nChars, size_t *nActual);
  virtual asynStatus readEnum(asynUser *pasynUser, char *strings[], int values[],
                              int severities[], size_t nElements, size_t *nIn);

  // Should be private, but are called from C so must be public
  void statusTask(void);

protected:
  virtual bool isEnabled();
  virtual bool isConnected();
  virtual bool connectDetector();
  virtual void updateEnums();
  virtual void addFeature(const char *name,
                          asynParamType type,
                          SlsDetUtils::Feature feature,
                          bool init,
                          bool writable);
  virtual void writeStringToDetector(SlsDetUtils::Feature feature,
                                     const std::string& value);
  virtual void writeIntToDetector(SlsDetUtils::Feature feature,
                                  int value);
  virtual void writeDoubleToDetector(SlsDetUtils::Feature feature,
                                     double value);

  virtual bool readStringFromDetector(SlsDetUtils::Feature feature,
                                      std::string& value);
  virtual bool readIntFromDetector(SlsDetUtils::Feature feature,
                                   int& value);
  virtual bool readDoubleFromDetector(SlsDetUtils::Feature feature,
                                      double& value);

  // enum information
  enum ConnectionStatus { DISCONNECTED=0, CONNECTED=1 };
  enum OnOff { OFF=0, ON=1 };
  enum OkTripped { OK=0, TRIPPED=1 };
  enum SfpSlot { OUTER=0, INNER=1 };
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
  static const SlsDetEnumInfo SlsSfpSlotEnums[];
  static const SlsDetEnumInfo SlsConnStatusEnums[];
  static const SlsDetEnumInfo SlsRunStatusEnums[];
  static const SlsDetEnumInfo SlsDetTypesEnums[];
  static const SlsDetEnumInfo SlsClkSpeedEnums[];
  static const SlsDetEnumInfo SlsGainEnums[];
  static const SlsDetEnumInfo SlsTriggerEnums[];
  static const SlsDetEnumSet SlsDetEnums[];
  static const size_t SlsDetEnumsSize;
  char* _enumStrings[SLS_MAX_ENUMS];
  int   _enumValues[SLS_MAX_ENUMS];
  int   _enumSeverities[SLS_MAX_ENUMS];
  // parameters
  int _shmIdParam;
  int _heartbeatParam;
  int _enabledParam;
  int _connStatusParam;

private:
  const int                       _id;
  bool                            _exiting;
  int                             _exited;
  double                          _pollingPeriod;
  double                          _fastPollingPeriod;
  double                          _connPollingPeriod;
  std::string                     _hostname;
  std::unique_ptr<sls::Detector>  _det;
  SlsDetUtils::IntFeatureMap      _intFeatures;
  SlsDetUtils::DoubleFeatureMap   _doubleFeatures;
  SlsDetUtils::StringFeatureMap   _stringFeatures;

  /* signals status task */
  epicsEventId                    statusEvent;
};

#endif
