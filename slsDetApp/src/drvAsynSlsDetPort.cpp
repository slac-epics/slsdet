#include "drvAsynSlsDetPort.h"
#include "slsDetDriver.h"

#include <iocsh.h>
#include <epicsExit.h>
#include <epicsString.h>

#include <epicsExport.h>

#include <cstring>

static const char *driverName = "SlsDet";

static void exitHandler(void *drvPvt) {
  SlsDet *pPvt = (SlsDet *)drvPvt;
  pPvt->shutdown();
}

/* Max size for enum strings */
#define MAX_ENUM_STRING_SIZE 25

/* Port driver basic parameters */
#define SlsInitString       "SLS_INIT"
#define SlsNumDetString     "SLS_NUM_DETS"
#define SlsRunStatusString  "SLS_RUN_STATUS"
#define SlsConnStatusString "SLS_CONN_STATUS"
#define SlsHostNameString   "SLS_HOSTNAME"
#define SlsDetTypeString    "SLS_DET_TYPE"
/* Port driver version parameters */
#define SlsDetSerialNumString   "SLS_SERIAL_NUMBER"
#define SlsDetFirmwareVerString "SLS_FIRMWARE_VERSION"
#define SlsDetSoftwareVerString "SLS_SOFTWARE_VERSION"
/* Port driver temperature parameters */
#define SlsFpgaTempString         "SLS_FPGA_TEMP"
#define SlsAdcTempString          "SLS_ADC_TEMP"
#define SlsGetTempThresholdString "SLS_GET_TEMP_THRESHOLD"
#define SlsSetTempThresholdString "SLS_SET_TEMP_THRESHOLD"
#define SlsGetTempControlString   "SLS_GET_TEMP_CONTROL"
#define SlsSetTempControlString   "SLS_SET_TEMP_CONTROL"
#define SlsGetTempEventString     "SLS_GET_TEMP_EVENT"
#define SlsSetTempEventString     "SLS_SET_TEMP_EVENT"
/* Port driver advanced control parameters */
#define SlsGetChipPowerString     "SLS_GET_CHIP_POWER"
#define SlsSetChipPowerString     "SLS_SET_CHIP_POWER"
#define SlsGetHighVoltageString   "SLS_GET_HV"
#define SlsSetHighVoltageString   "SLS_SET_HV"
#define SlsGetClockDividerString  "SLS_GET_SPEED"
#define SlsSetClockDividerString  "SLS_SET_SPEED"
#define SlsGetGainModeString      "SLS_GET_GAIN"
#define SlsSetGainModeString      "SLS_SET_GAIN"

#define sizeofArray(arr) sizeof(arr) / sizeof(arr[0])

const SlsDet::SlsDetEnumInfo SlsDet::SlsOnOffEnums[] = {
  {"Off", OFF,  epicsSevNone},
  {"On",  ON,   epicsSevNone}
};

const SlsDet::SlsDetEnumInfo SlsDet::SlsOkTrippedEnums[] = {
  {"Ok",      OK,       epicsSevNone},
  {"Tripped", TRIPPED,  epicsSevMajor}
};

const SlsDet::SlsDetEnumInfo SlsDet::SlsConnStatusEnums[] = {
  {"Disconnected",  DISCONNECTED, epicsSevMajor},
  {"Connected",     CONNECTED,    epicsSevNone}
};

const SlsDet::SlsDetEnumInfo SlsDet::SlsRunStatusEnums[] = {
  {slsDetectorDefs::runStatusType(slsDetectorDefs::IDLE), slsDetectorDefs::IDLE, epicsSevNone},
  {slsDetectorDefs::runStatusType(slsDetectorDefs::ERROR), slsDetectorDefs::ERROR, epicsSevMajor},
  {slsDetectorDefs::runStatusType(slsDetectorDefs::WAITING), slsDetectorDefs::WAITING, epicsSevNone},
  {slsDetectorDefs::runStatusType(slsDetectorDefs::RUN_FINISHED), slsDetectorDefs::RUN_FINISHED, epicsSevNone},
  {slsDetectorDefs::runStatusType(slsDetectorDefs::TRANSMITTING), slsDetectorDefs::TRANSMITTING, epicsSevNone},
  {slsDetectorDefs::runStatusType(slsDetectorDefs::RUNNING), slsDetectorDefs::RUNNING, epicsSevNone},
  {slsDetectorDefs::runStatusType(slsDetectorDefs::STOPPED), slsDetectorDefs::STOPPED, epicsSevNone}
};

const SlsDet::SlsDetEnumInfo SlsDet::SlsDetTypesEnums[] = {
  {slsDetectorDefs::getDetectorType(slsDetectorDefs::GENERIC), slsDetectorDefs::GENERIC, epicsSevNone},
  {slsDetectorDefs::getDetectorType(slsDetectorDefs::MYTHEN), slsDetectorDefs::MYTHEN, epicsSevNone},
  {slsDetectorDefs::getDetectorType(slsDetectorDefs::PILATUS), slsDetectorDefs::PILATUS, epicsSevNone},
  {slsDetectorDefs::getDetectorType(slsDetectorDefs::EIGER), slsDetectorDefs::EIGER, epicsSevNone},
  {slsDetectorDefs::getDetectorType(slsDetectorDefs::GOTTHARD), slsDetectorDefs::GOTTHARD, epicsSevNone},
  {slsDetectorDefs::getDetectorType(slsDetectorDefs::PICASSO), slsDetectorDefs::PICASSO, epicsSevNone},
  {slsDetectorDefs::getDetectorType(slsDetectorDefs::AGIPD), slsDetectorDefs::AGIPD, epicsSevNone},
  {slsDetectorDefs::getDetectorType(slsDetectorDefs::MOENCH), slsDetectorDefs::MOENCH, epicsSevNone},
  {slsDetectorDefs::getDetectorType(slsDetectorDefs::JUNGFRAU), slsDetectorDefs::JUNGFRAU, epicsSevNone},
  {slsDetectorDefs::getDetectorType(slsDetectorDefs::JUNGFRAUCTB), slsDetectorDefs::JUNGFRAUCTB, epicsSevNone},
  {slsDetectorDefs::getDetectorType(slsDetectorDefs::PROPIX), slsDetectorDefs::PROPIX, epicsSevNone},
  {slsDetectorDefs::getDetectorType(slsDetectorDefs::MYTHEN3), slsDetectorDefs::MYTHEN3, epicsSevNone}
};

const SlsDet::SlsDetEnumInfo SlsDet::SlsClockDivEnums[] = {
  {"Full",    FULL,     epicsSevNone},
  {"Half",    HALF,     epicsSevNone},
  {"Quarter", QUARTER,  epicsSevNone}
};

const SlsDet::SlsDetEnumInfo SlsDet::SlsGainEnums[] = {
  {"Standard",      slsDetectorDefs::STANDARD,      epicsSevNone},
  {"Fast",          slsDetectorDefs::FAST,          epicsSevNone},
  {"HighGain",      slsDetectorDefs::HIGHGAIN,      epicsSevNone},
  {"DynamicGain",   slsDetectorDefs::DYNAMICGAIN,   epicsSevNone},
  {"LowGain",       slsDetectorDefs::LOWGAIN,       epicsSevNone},
  {"MediumGain",    slsDetectorDefs::MEDIUMGAIN,    epicsSevNone},
  {"VeryHighGain",  slsDetectorDefs::VERYHIGHGAIN,  epicsSevNone},
  {"LowNoise",      slsDetectorDefs::LOWNOISE,      epicsSevNone},
  {"DynamicHG0",    slsDetectorDefs::DYNAMICHG0,    epicsSevNone},
  {"FixGain1",      slsDetectorDefs::FIXGAIN1,      epicsSevNone},
  {"FixGain2",      slsDetectorDefs::FIXGAIN2,      epicsSevNone},
  {"ForceSwitchG1", slsDetectorDefs::FORCESWITCHG1, epicsSevNone},
  {"ForceSwitchG2", slsDetectorDefs::FORCESWITCHG2, epicsSevNone},
  {"VeryLowGain",   slsDetectorDefs::VERYLOWGAIN,   epicsSevNone},
};

const SlsDet::SlsDetEnumSet SlsDet::SlsDetEnums[] = {
  {SlsOnOffEnums,
   sizeofArray(SlsOnOffEnums),
   SlsGetChipPowerString},
  {SlsOnOffEnums,
   sizeofArray(SlsOnOffEnums),
   SlsSetChipPowerString},
  {SlsOnOffEnums,
   sizeofArray(SlsOnOffEnums),
   SlsGetTempControlString},
  {SlsOnOffEnums,
   sizeofArray(SlsOnOffEnums),
   SlsSetTempControlString},
  {SlsOkTrippedEnums,
   sizeofArray(SlsOkTrippedEnums),
   SlsGetTempEventString},
  {SlsOkTrippedEnums,
   sizeofArray(SlsOkTrippedEnums),
   SlsSetTempEventString},
  {SlsConnStatusEnums,
   sizeofArray(SlsConnStatusEnums),
   SlsConnStatusString},
  {SlsRunStatusEnums,
   sizeofArray(SlsRunStatusEnums),
   SlsRunStatusString},
  {SlsDetTypesEnums,
   sizeofArray(SlsDetTypesEnums),
   SlsDetTypeString},
  {SlsClockDivEnums,
   sizeofArray(SlsClockDivEnums),
   SlsGetClockDividerString},
  {SlsClockDivEnums,
   sizeofArray(SlsClockDivEnums),
   SlsSetClockDividerString},
  {SlsGainEnums,
   sizeofArray(SlsGainEnums),
   SlsGetGainModeString},
  {SlsGainEnums,
   sizeofArray(SlsGainEnums),
   SlsSetGainModeString}
};

const size_t SlsDet::SlsDetEnumsSize = sizeofArray(SlsDet::SlsDetEnums);

/** Constructor for the SlsDet class
  */
SlsDet::SlsDet(const char *portName, const std::vector<std::string>& hostnames, int id, double timeout)
  : asynPortDriver(portName, hostnames.size(),
      asynEnumMask | asynInt32Mask | asynFloat64Mask | asynOctetMask | asynDrvUserMask, // Interfaces that we implement
      asynEnumMask | asynInt32Mask,                                                     // Interfaces that do callbacks
      ASYN_MULTIDEVICE | ASYN_CANBLOCK, 1, /* ASYN_CANBLOCK=1, ASYN_MULTIDEVICE=1, autoConnect=1 */
      0, 0),  /* Default priority and stack size */
    _id(id),
    _timeout(timeout),
    _hostnames(hostnames),
    _dets(hostnames.size(), NULL)
{
  /* Create an EPICS exit handler */
  epicsAtExit(exitHandler, this);

  createParam(SlsInitString,              asynParamInt32,   &_initValue);
  createParam(SlsNumDetString,            asynParamInt32,   &_numDetValue);
  createParam(SlsRunStatusString,         asynParamInt32,   &_runStatusValue);
  createParam(SlsConnStatusString,        asynParamInt32,   &_connStatusValue);
  createParam(SlsHostNameString,          asynParamOctet,   &_hostNameValue);
  createParam(SlsDetTypeString,           asynParamInt32,   &_detTypeValue);
  createParam(SlsDetSerialNumString,      asynParamOctet,   &_detSerialNumberValue);
  createParam(SlsDetFirmwareVerString,    asynParamOctet,   &_detFirmwareVersionValue);
  createParam(SlsDetSoftwareVerString,    asynParamOctet,   &_detSoftwareVersionValue);
  createParam(SlsFpgaTempString,          asynParamFloat64, &_fpgaTempValue);
  createParam(SlsAdcTempString,           asynParamFloat64, &_adcTempValue);
  createParam(SlsGetTempThresholdString,  asynParamFloat64, &_getTempThresholdValue);
  createParam(SlsSetTempThresholdString,  asynParamFloat64, &_setTempThresholdValue);
  createParam(SlsGetTempControlString,    asynParamInt32,   &_getTempControlValue);
  createParam(SlsSetTempControlString,    asynParamInt32,   &_setTempControlValue);
  createParam(SlsGetTempEventString,      asynParamInt32,   &_getTempEventValue);
  createParam(SlsSetTempEventString,      asynParamInt32,   &_setTempEventValue);
  createParam(SlsGetChipPowerString,      asynParamInt32,   &_getChipPowerValue);
  createParam(SlsSetChipPowerString,      asynParamInt32,   &_setChipPowerValue);
  createParam(SlsGetHighVoltageString,    asynParamInt32,   &_getHighVoltageValue);
  createParam(SlsSetHighVoltageString,    asynParamInt32,   &_setHighVoltageValue);
  createParam(SlsGetClockDividerString,   asynParamInt32,   &_getClockDividerValue);
  createParam(SlsSetClockDividerString,   asynParamInt32,   &_setClockDividerValue);
  createParam(SlsGetGainModeString,       asynParamInt32,   &_getGainModeValue);
  createParam(SlsSetGainModeString,       asynParamInt32,   &_setGainModeValue);

  /* Initialize the SlsInit parameter */
  for (int addr=0; addr<(int)_dets.size(); addr++) {
    setIntegerParam(addr, _initValue, 0);
    callParamCallbacks(addr);
  }
  
  /* allocate memory to use for enum callbacks */
  for (unsigned i=0; i<SLS_MAX_ENUMS; i++) {
    _enumStrings[i] = new char[MAX_ENUM_STRING_SIZE+1];
  }
}

SlsDet::~SlsDet()
{
  /* send shutdown signal to slsDetDrivers */
  shutdown();
  /* delete the slsDetDriver instances */
  for(unsigned n=0; n<_dets.size(); n++) {
    if (_dets[n]) {
      delete _dets[n];
      _dets[n] = NULL;
    }
  }
  for (unsigned i=0; i<SLS_MAX_ENUMS; i++) {
    if (_enumStrings[i]) delete[] _enumStrings[i];
  }
}

asynStatus SlsDet::initialize(asynUser *pasynUser)
{
  int conn;
  int addr;
  int numDet;
  asynStatus status;
  SlsDetMessage reply;
  static const char *functionName = "initialize";

  status = getAddress(pasynUser, &addr); if (status != asynSuccess) return status;

  asynPrint(pasynUser, ASYN_TRACE_FLOW,
            "%s:%s, port=%s, address=%d attempting to connect detector: %s\n",
            driverName, functionName, this->portName, addr, _hostnames[addr].c_str());

  if (!_dets[addr]) {
    try {
      _dets[addr] = new SlsDetDriver(_hostnames[addr], _id + addr, this->portName, addr);
      /* Initialize the detector parameters */
      setIntegerParam(addr, _connStatusValue, DISCONNECTED);
      callParamCallbacks(addr);
      setIntegerParam(_numDetValue, 0);
      callParamCallbacks();
    } catch (...) {
      asynPrint(pasynUser, ASYN_TRACE_ERROR,
                "%s:%s, port=%s, address=%d failed to initialize detector: %s\n",
                driverName, functionName, this->portName, addr, _hostnames[addr].c_str());
      status = asynDisabled;
    }
  }

  if (_dets[addr]) {
    getIntegerParam(addr, _connStatusValue, &conn);
    getIntegerParam(_numDetValue, &numDet);
    if (!conn) {
      reply = _dets[addr]->request(SlsDetMessage::CheckOnline, _timeout);
      if (reply.mtype() == SlsDetMessage::Ok) {
        pasynManager->exceptionConnect(pasynUser);
        updateEnums(addr);
        setIntegerParam(addr, _connStatusValue, CONNECTED);
        callParamCallbacks(addr);
        setIntegerParam(_numDetValue, ++numDet);
        callParamCallbacks();
        asynPrint(pasynUser, ASYN_TRACE_FLOW,
            "%s:%s, port=%s, address=%d connected to detector: %s\n",
            driverName, functionName, this->portName, addr, _hostnames[addr].c_str());
      } else {
        asynPrint(pasynUser, ASYN_TRACE_ERROR,
            "%s:%s, port=%s, address=%d failed to connect to detector: %s\n",
            driverName, functionName, this->portName, addr, _hostnames[addr].c_str());
      }
    }
  } else if (status == asynSuccess) {
    asynPrint(pasynUser, ASYN_TRACE_ERROR,
              "%s:%s, port=%s, address=%d failed to initialize the detector interface: %s\n",
              driverName, functionName, this->portName, addr, _hostnames[addr].c_str());
    status = asynDisabled;
  }

  return status;
}

asynStatus SlsDet::uninitialize(asynUser *pasynUser)
{
  int addr;
  int numDet;
  asynStatus status;
  static const char *functionName = "uninitialize";

  status = getAddress(pasynUser, &addr); if (status != asynSuccess) return status;

  asynPrint(pasynUser, ASYN_TRACE_FLOW,
            "%s:%s, port=%s, address=%d disconnecting detector: %s\n",
            driverName, functionName, this->portName, addr, _hostnames[addr].c_str());

  setIntegerParam(addr, _connStatusValue, DISCONNECTED);
  callParamCallbacks(addr);
  getIntegerParam(_numDetValue, &numDet);
  setIntegerParam(_numDetValue, numDet - 1);
  callParamCallbacks();
  pasynManager->exceptionDisconnect(pasynUser);

  asynPrint(pasynUser, ASYN_TRACE_FLOW,
            "%s:%s, port=%s, address=%d disconnected detector: %s\n",
            driverName, functionName, this->portName, addr, _hostnames[addr].c_str());

  return status;
}

int SlsDet::isConnected(int addr) {
  int conn;
  if (getIntegerParam(addr, _connStatusValue, &conn) == asynSuccess) {
    return conn == CONNECTED;
  } else {
    return false;
  }
}

void SlsDet::shutdown()
{
  for(unsigned n=0; n<_dets.size(); n++) {
    if (_dets[n]) {
      _dets[n]->shutdown();
    }
  }
}

void SlsDet::updateEnums(int addr)
{
  /* Update all the enums in the SlsDetEnums list */
  for (unsigned nEnum=0; nEnum<SlsDetEnumsSize; nEnum++) {
    int reason;
    if (findParam(addr, SlsDetEnums[nEnum].name, &reason) != asynSuccess) continue;

    const SlsDetEnumInfo* elem = SlsDetEnums[nEnum].enums;
    int nElem;
    for (nElem=0; (nElem<(int)SlsDetEnums[nEnum].size) && (nElem<SLS_MAX_ENUMS); ++nElem) {
        std::strncpy(_enumStrings[nElem], elem[nElem].name.c_str(), MAX_ENUM_STRING_SIZE)[MAX_ENUM_STRING_SIZE] = '\0';
        _enumValues[nElem] = elem[nElem].value;
        _enumSeverities[nElem] = elem[nElem].severity;
    }
    doCallbacksEnum(_enumStrings, _enumValues, _enumSeverities, nElem, reason, addr);
  }
}

asynStatus SlsDet::connect(asynUser *pasynUser)
{
  return initialize(pasynUser);
}

asynStatus SlsDet::disconnect(asynUser *pasynUser)
{
  return uninitialize(pasynUser);
}

asynStatus SlsDet::readDetector(asynUser *pasynUser, SlsDetMessage::MessageType mtype)
{
  int addr;
  int function = pasynUser->reason;
  double timeout = pasynUser->timeout;
  asynStatus status = this->getAddress(pasynUser, &addr);
  static const char *functionName = "readDetector";

  if (status == asynSuccess) {
    if (isConnected(addr)) {
      asynPrint(pasynUser, ASYN_TRACEIO_DEVICE,
                "%s:%s: port=%s address=%d sending request of type %s with timeout %g seconds\n",
                driverName, functionName, this->portName, addr,
                SlsDetMessage::messageType(mtype).c_str(), timeout);
      SlsDetMessage reply = _dets[addr]->request(mtype, timeout);
      asynPrint(pasynUser, ASYN_TRACEIO_DEVICE,
                "%s:%s: port=%s address=%d received reply: %s\n",
                driverName, functionName, this->portName, addr, reply.dump().c_str());
      if (reply.mtype() == SlsDetMessage::Ok) {
        switch (reply.dtype()) {
        case SlsDetMessage::Int32:
          status = setIntegerParam(addr, function, reply.asInteger());
          callParamCallbacks(addr);
          break;
#ifdef ASYN_HAS_INT64
        case SlsDetMessage::Int64:
          status = setInteger64Param(addr, function, reply.asInteger64());
          callParamCallbacks(addr);
          break;
#endif
        case SlsDetMessage::Float64:
          status = setDoubleParam(addr, function, reply.asDouble());
          callParamCallbacks(addr);
          break;
        case SlsDetMessage::String:
          status = setStringParam(addr, function, reply.asString());
          callParamCallbacks(addr);
          break;
        default:
          asynPrint(pasynUser, ASYN_TRACE_ERROR,
                "%s:%s: port=%s address=%d received reply with unsupported datatype: %s\n",
                driverName, functionName, this->portName, addr,
                SlsDetMessage::dataType(reply.dtype()).c_str());
          status = asynError;
          break;
        }
      } else if (reply.mtype() == SlsDetMessage::Invalid) {
        asynPrint(pasynUser, ASYN_TRACE_ERROR,
                  "%s:%s: port=%s address=%d\n driver responded that request of type %s was invalid\n",
                  driverName, functionName, this->portName, addr,
                  SlsDetMessage::messageType(mtype).c_str());
        status = asynError;
      } else if (reply.mtype() == SlsDetMessage::Timeout) {
        asynPrint(pasynUser, ASYN_TRACE_ERROR,
                 "%s:%s: port=%s address=%d request of type %s timed out after %g seconds - disconnecting\n",
                 driverName, functionName, this->portName, addr,
                 SlsDetMessage::messageType(mtype).c_str(), timeout);
        status = asynTimeout;
        uninitialize(pasynUser);
      } else {
        asynPrint(pasynUser, ASYN_TRACE_ERROR,
                  "%s:%s: port=%s address=%d request of type %s failed - disconnecting\n",
                  driverName, functionName, this->portName, addr,
                  SlsDetMessage::messageType(mtype).c_str());
        status = asynDisconnected;
        uninitialize(pasynUser);
      }
    } else {
      status = asynDisconnected;
    }
  }

  return status;
}

asynStatus SlsDet::writeDetector(asynUser *pasynUser, SlsDetMessage msg)
{
  int addr;
  double timeout = pasynUser->timeout;
  asynStatus status = this->getAddress(pasynUser, &addr);
  static const char *functionName = "writeDetector";

  if (status == asynSuccess) {
    if (isConnected(addr)) {
      asynPrint(pasynUser, ASYN_TRACEIO_DEVICE,
                "%s:%s: port=%s address=%d sending request %s with timeout %g\n",
                driverName, functionName, this->portName, addr,
                msg.dump().c_str(), timeout);
      SlsDetMessage reply = _dets[addr]->request(msg, timeout);
      asynPrint(pasynUser, ASYN_TRACEIO_DEVICE,
                "%s:%s: port=%s address=%d received reply: %s\n",
                driverName, functionName, this->portName, addr, reply.dump().c_str());
      if (reply.mtype() == SlsDetMessage::Ok) {
        if (reply.dtype() != SlsDetMessage::None) {
          asynPrint(pasynUser, ASYN_TRACE_ERROR,
                "%s:%s: port=%s address=%d received reply with unsupported datatype: %s\n",
                driverName, functionName, this->portName, addr,
                SlsDetMessage::dataType(reply.dtype()).c_str());
          status = asynError;
        }
      } else if (reply.mtype() == SlsDetMessage::Failed) {
        status = asynError;
      } else if (reply.mtype() == SlsDetMessage::Invalid) {
        status = asynError;
      } else if (reply.mtype() == SlsDetMessage::Timeout) {
        status = asynTimeout;
        uninitialize(pasynUser);
      } else {
        status = asynDisconnected;
        uninitialize(pasynUser);
      }
    } else {
      status = asynDisconnected;
    }
  }

  return status;
}

asynStatus SlsDet::writeDetector(asynUser *pasynUser, SlsDetMessage::MessageType mtype,
                                 epicsFloat64 value)
{
  int addr;
  int function = pasynUser->reason;
  asynStatus status = this->getAddress(pasynUser, &addr);
  SlsDetMessage msg(mtype, SlsDetMessage::Float64);
  msg.setDouble(value);

  if (status == asynSuccess) {
    status = setDoubleParam(addr, function, value);
    callParamCallbacks(addr);
    if (status == asynSuccess) {
      status = writeDetector(pasynUser, msg);
    }
  }

  return status;
}

asynStatus SlsDet::writeDetector(asynUser *pasynUser, SlsDetMessage::MessageType mtype,
                                 epicsInt32 value)
{
  int addr;
  int function = pasynUser->reason;
  asynStatus status = this->getAddress(pasynUser, &addr);
  SlsDetMessage msg(mtype, SlsDetMessage::Int32);
  msg.setInteger(value);

  if (status == asynSuccess) {
    status = setIntegerParam(addr, function, value);
    callParamCallbacks(addr);
    if (status == asynSuccess) {
      status = writeDetector(pasynUser, msg);
    }
  }

  return status;
}

asynStatus SlsDet::readFloat64(asynUser *pasynUser, epicsFloat64 *value)
{
  const char* name = NULL;
  int addr;
  int function = pasynUser->reason;
  asynStatus status = asynSuccess;
  static const char *functionName = "readFloat64";

  status = getAddress(pasynUser, &addr); if (status != asynSuccess) return status;

  getParamName(addr, function, &name);
  if (name) {
    asynPrint(pasynUser, ASYN_TRACEIO_DEVICE,
              "%s:%s: port=%s address=%d received read request for parameter: %s\n",
               driverName, functionName, this->portName, addr, name);
  }

  if (function == _fpgaTempValue) {
    status = readDetector(pasynUser, SlsDetMessage::ReadFpgaTemp);
  } else if (function == _adcTempValue) {
    status = readDetector(pasynUser, SlsDetMessage::ReadAdcTemp);
  } else if (function == _getTempThresholdValue) {
    status = readDetector(pasynUser, SlsDetMessage::ReadTempThreshold);
  } else { // Other functions we call the base class method
    return asynPortDriver::readFloat64(pasynUser, value);
  }

  // if the read was successful then set value from parameters
  if (status == asynSuccess) {
    status = getDoubleParam(addr, function, value);
  }
  
  return status;
}

asynStatus SlsDet::writeFloat64(asynUser *pasynUser, epicsFloat64 value)
{
  const char* name = NULL;
  int addr;
  int function = pasynUser->reason;
  asynStatus status = asynSuccess;
  static const char *functionName = "writeFloat64";

  status = getAddress(pasynUser, &addr); if (status != asynSuccess) return status;

  getParamName(addr, function, &name);
  if (name) {
    asynPrint(pasynUser, ASYN_TRACEIO_DEVICE,
              "%s:%s: port=%s address=%d received write request (%g) for parameter: %s\n",
               driverName, functionName, this->portName, addr, value, name);
  }

  if (function == _setTempThresholdValue) {
    status = writeDetector(pasynUser, SlsDetMessage::WriteTempThreshold, value);
  } else {
    status = asynPortDriver::writeFloat64(pasynUser, value);
  }

  return status;
}

asynStatus SlsDet::readInt32(asynUser *pasynUser, epicsInt32 *value)
{
  const char* name = NULL;
  int addr;
  int function = pasynUser->reason;
  asynStatus status = asynSuccess;
  static const char *functionName = "readInt32";

  status = getAddress(pasynUser, &addr); if (status != asynSuccess) return status;

  getParamName(addr, function, &name);
  if (name) {
    asynPrint(pasynUser, ASYN_TRACEIO_DEVICE,
              "%s:%s: port=%s address=%d received read request for parameter: %s\n",
               driverName, functionName, this->portName, addr, name);
  }

  if (function == _runStatusValue) {
    status = readDetector(pasynUser, SlsDetMessage::ReadRunStatus);
  } else if (function == _detTypeValue) {
    status = readDetector(pasynUser, SlsDetMessage::ReadDetType);
  } else if (function == _getTempControlValue) {
    status = readDetector(pasynUser, SlsDetMessage::ReadTempControl);
  } else if (function == _getTempEventValue) {
    status = readDetector(pasynUser, SlsDetMessage::ReadTempEvent);
  } else if (function == _getChipPowerValue) {
    status = readDetector(pasynUser, SlsDetMessage::ReadPowerChip);
  } else if (function == _getHighVoltageValue) {
    status = readDetector(pasynUser, SlsDetMessage::ReadHighVoltage);
  } else if (function == _getClockDividerValue) {
    status = readDetector(pasynUser, SlsDetMessage::ReadClockDivider);
  } else if (function == _getGainModeValue) {
    status = readDetector(pasynUser, SlsDetMessage::ReadGainMode);
  } else { // Other functions we call the base class method
    return asynPortDriver::readInt32(pasynUser, value);
  }

  // if the read was successful then set value from parameters
  if (status == asynSuccess) {
    status = getIntegerParam(addr, function, value);
  }

  return status;
}

asynStatus SlsDet::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
  const char* name = NULL;
  int addr;
  int function = pasynUser->reason;
  asynStatus status = asynSuccess;
  static const char *functionName = "writeInt32";

  status = getAddress(pasynUser, &addr); if (status != asynSuccess) return status;

  getParamName(addr, function, &name);
  if (name) {
    asynPrint(pasynUser, ASYN_TRACEIO_DEVICE,
              "%s:%s: port=%s address=%d received write request (%d) for parameter: %s\n",
               driverName, functionName, this->portName, addr, value, name);
  }

  if (function == _setTempControlValue) {
    status = writeDetector(pasynUser, SlsDetMessage::WriteTempControl, value);
  } else if (function == _setTempEventValue) {
    status = writeDetector(pasynUser, SlsDetMessage::WriteTempEvent, value);
  } else if (function == _setChipPowerValue) {
    status = writeDetector(pasynUser, SlsDetMessage::WritePowerChip, value);
  } else if (function == _setHighVoltageValue) {
    status = writeDetector(pasynUser, SlsDetMessage::WriteHighVoltage, value);
  } else if (function == _setClockDividerValue) {
    status = writeDetector(pasynUser, SlsDetMessage::WriteClockDivider, value);
  } else if (function == _setGainModeValue) {
    status = writeDetector(pasynUser, SlsDetMessage::WriteGainMode, value);
  } else { // Other functions we call the base class method
    status = asynPortDriver::writeInt32(pasynUser, value);
  }

  return status;
}

asynStatus SlsDet::readOctet(asynUser *pasynUser,
                             char *value, size_t maxChars, size_t *nActual,
                             int *eomReason)
{
  const char* name = NULL;
  int addr;
  int function = pasynUser->reason;
  asynStatus status = asynSuccess;
  static const char *functionName = "readOctet";

  status = getAddress(pasynUser, &addr); if (status != asynSuccess) return status;

  getParamName(addr, function, &name);
  if (name) {
    asynPrint(pasynUser, ASYN_TRACEIO_DEVICE,
              "%s:%s: port=%s address=%d received read request for parameter: %s\n",
               driverName, functionName, this->portName, addr, name);
  }

  if (function == _hostNameValue) {
    status = readDetector(pasynUser, SlsDetMessage::ReadHostname);
  } else if (function == _detSerialNumberValue) {
    status = readDetector(pasynUser, SlsDetMessage::ReadSerialnum);
  } else if (function == _detFirmwareVersionValue) {
    status = readDetector(pasynUser, SlsDetMessage::ReadFirmwareVer);
  } else if (function == _detSoftwareVersionValue) {
    status = readDetector(pasynUser, SlsDetMessage::ReadSoftwareVer);
  } else {
    return asynPortDriver::readOctet(pasynUser, value, maxChars, nActual, eomReason);
  }

  // if the read was successful then set value from parameters
  if (status == asynSuccess) {
    status = getStringParam(addr, function, maxChars, value);
    if (eomReason) *eomReason = ASYN_EOM_END;
    *nActual = strlen(value)+1;
  }

  return status;
}

asynStatus SlsDet::readEnum(asynUser *pasynUser, char *strings[], int values[],
                            int severities[], size_t nElements, size_t *nIn)
{
  const char* name = NULL;
  int addr;
  int function = pasynUser->reason;
  size_t matched_size = 0;
  asynStatus status = asynSuccess;
  const SlsDetEnumInfo *matched_enums = NULL;
  static const char *functionName = "readEnum";

  status = getAddress(pasynUser, &addr); if (status != asynSuccess) return status;

  getParamName(addr, function, &name);
  if (name) {
    asynPrint(pasynUser, ASYN_TRACEIO_DEVICE,
              "%s:%s: port=%s address=%d received enum read request for parameter: %s\n",
               driverName, functionName, this->portName, addr, name);
    /* Search the enum (if any) that goes with the requested parameter. */
    for (unsigned nEnum=0; nEnum<SlsDetEnumsSize; nEnum++) {
      if (!std::strcmp(SlsDetEnums[nEnum].name, name)) {
        matched_enums = SlsDetEnums[nEnum].enums;
        matched_size = SlsDetEnums[nEnum].size;
        break;
      }
    }

    if (matched_enums) {
      asynPrint(pasynUser, ASYN_TRACEIO_DEVICE,
              "%s:%s: port=%s address=%d enum found for parameter: %s\n",
               driverName, functionName, this->portName, addr, name);
      size_t i;
      for (i = 0; ((i < matched_size) && (i < nElements)); ++i) {
        if (strings[i]) free(strings[i]);
        strings[i] = epicsStrDup(matched_enums[i].name.c_str());
        values[i] = matched_enums[i].value;
        severities[i] = matched_enums[i].severity;
      }
      *nIn = i;
    } else {
      asynPrint(pasynUser, ASYN_TRACEIO_DEVICE,
              "%s:%s: port=%s address=%d no enum found for parameter: %s\n",
               driverName, functionName, this->portName, addr, name);
      *nIn = 0;
      status = asynError;
    }
  } else {
    asynPrint(pasynUser, ASYN_TRACE_ERROR,
              "%s:%s: port=%s address=%d received enum read request for parameter with no name\n",
               driverName, functionName, this->portName, addr);
    *nIn = 0;
    status = asynError;
  }

  return status;
}

/** Configuration command, called directly or from iocsh */
extern "C" int SlsDetConfigure(const char *portName, const char *hostName, int id, double timeout)
{
  size_t last = 0;
  size_t next = 0;
  std::string token;
  std::string orig = hostName;
  std::vector<std::string> hostnames;
  while ((next = orig.find('+', last)) != std::string::npos) {
    token = orig.substr(last, next-last);
    if (!token.empty())
      hostnames.push_back(token);
    last = next + 1;
  }
  token = orig.substr(last);
  if (!token.empty())
     hostnames.push_back(token);
  new SlsDet(portName, hostnames, id, timeout);
  return(asynSuccess);
}


static const iocshArg configArg0 = { "Port name",         iocshArgString};
static const iocshArg configArg1 = { "Detector Hostname", iocshArgString};
static const iocshArg configArg2 = { "Detector Id",       iocshArgInt};
static const iocshArg configArg3 = { "Detector Timeout",  iocshArgDouble};
static const iocshArg * const configArgs[] = {&configArg0,
                                              &configArg1,
                                              &configArg2,
                                              &configArg3};
static const iocshFuncDef configFuncDef = {"SlsDetConfigure", 4, configArgs};
static void configCallFunc(const iocshArgBuf *args)
{
  SlsDetConfigure(args[0].sval, args[1].sval, args[2].ival, args[3].dval);
}

void drvSlsDetRegister(void)
{
  iocshRegister(&configFuncDef,configCallFunc);
}

extern "C" {
epicsExportRegistrar(drvSlsDetRegister);
}

#undef MAX_ENUM_STRING_SIZE
#undef sizeofArray
