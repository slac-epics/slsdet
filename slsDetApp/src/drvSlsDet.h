#ifndef drvSlsDet_H
#define drvSlsDet_H

#include <asynPortDriver.h>
#include <sls_detector_defs.h>

class multiSlsDetector;

/** Class definition for the SlsDet class
  */
class SlsDet : public asynPortDriver {
public:
  SlsDet(const char *portName, const char *hostName, int maxdets, int id);
  virtual ~SlsDet();

  /* These are the methods that we override from asynPortDriver */
  virtual asynStatus connect(asynUser *pasynUser);
  virtual asynStatus disconnect(asynUser *pasynUser);
  virtual asynStatus readInt32(asynUser *pasynUser, epicsInt32 *value);
  virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
  virtual asynStatus readFloat64(asynUser *pasynUser, epicsFloat64 *value);
  virtual asynStatus readEnum(asynUser *pasynUser, char *strings[], int values[],
                              int severities[], size_t nElements, size_t *nIn);

protected:
  /* These are the methods that communicate with the detector */
  virtual asynStatus checkError(asynUser *pasynUser, const char* caller, bool disconnect=true);
  virtual asynStatus readTemperature(asynUser *pasynUser, epicsFloat64 *value,
                                     slsDetectorDefs::dacIndex dac);
  virtual asynStatus readRunStatus(asynUser *pasynUser, epicsInt32 *value);
  virtual asynStatus initialize(asynUser *pasynUser);
  virtual asynStatus uninitialize(asynUser *pasynUser, bool shutdown=false);
  virtual void shutdown();
  // enum information
  typedef struct {
    const std::string name;
    int value;
    epicsAlarmSeverity severity;
  } SlsDetEnumInfo;
  static const SlsDetEnumInfo SlsConnStatusEnums[];
  static const size_t SlsConnStatusNumEnums;
  static const SlsDetEnumInfo SlsRunStatusEnums[];
  static const size_t SlsRunStatusNumEnums;
  // parameters
  int _numDetValue;
  int _runStatusValue;
  int _connStatusValue;
  int _hostNameValue;
  int _fpgaTempValue;
  int _adcTempValue;

private:
  const int                 _maxdets;
  const int                 _id;
  std::string               _hostname;
  multiSlsDetector*         _det;
};

#endif
