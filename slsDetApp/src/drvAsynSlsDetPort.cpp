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

#define sizeofenum(enums) sizeof(enums) / sizeof(enums[0])

const SlsDet::SlsDetEnumInfo SlsDet::SlsConnStatusEnums[] = {
  {"Disconnected",  DISCONNECTED, epicsSevMajor},
  {"Connected",     CONNECTED, epicsSevNone}
};
const size_t SlsDet::SlsConnStatusNumEnums = sizeofenum(SlsDet::SlsConnStatusEnums);

const SlsDet::SlsDetEnumInfo SlsDet::SlsRunStatusEnums[] = {
  {slsDetectorDefs::runStatusType(slsDetectorDefs::IDLE), slsDetectorDefs::IDLE, epicsSevNone},
  {slsDetectorDefs::runStatusType(slsDetectorDefs::ERROR), slsDetectorDefs::ERROR, epicsSevMajor},
  {slsDetectorDefs::runStatusType(slsDetectorDefs::WAITING), slsDetectorDefs::WAITING, epicsSevNone},
  {slsDetectorDefs::runStatusType(slsDetectorDefs::RUN_FINISHED), slsDetectorDefs::RUN_FINISHED, epicsSevNone},
  {slsDetectorDefs::runStatusType(slsDetectorDefs::TRANSMITTING), slsDetectorDefs::TRANSMITTING, epicsSevNone},
  {slsDetectorDefs::runStatusType(slsDetectorDefs::RUNNING), slsDetectorDefs::RUNNING, epicsSevNone},
  {slsDetectorDefs::runStatusType(slsDetectorDefs::STOPPED), slsDetectorDefs::STOPPED, epicsSevNone}
};
const size_t SlsDet::SlsRunStatusNumEnums = sizeofenum(SlsRunStatusEnums);

#undef sizeofenum

/* Max size for enum strings */
#define MAX_ENUM_STRING_SIZE 25

/* Port driver parameters */
#define SlsInitString         "SLS_INIT"
#define SlsNumDetString       "SLS_NUM_DETS"
#define SlsRunStatusString    "SLS_RUN_STATUS"
#define SlsConnStatusString   "SLS_CONN_STATUS"
#define SlsHostNameString     "SLS_HOSTNAME"
#define SlsFpgaTempString     "SLS_FPGA_TEMP"
#define SlsAdcTempString      "SLS_ADC_TEMP"

/** Constructor for the SlsDet class
  */
SlsDet::SlsDet(const char *portName, const std::vector<std::string>& hostnames, int id)
  : asynPortDriver(portName, hostnames.size(),
      asynEnumMask | asynInt32Mask | asynFloat64Mask | asynOctetMask | asynDrvUserMask, // Interfaces that we implement
      asynEnumMask | asynInt32Mask,                                                     // Interfaces that do callbacks
      ASYN_MULTIDEVICE | ASYN_CANBLOCK, 1, /* ASYN_CANBLOCK=1, ASYN_MULTIDEVICE=1, autoConnect=1 */
      0, 0),  /* Default priority and stack size */
    _id(id),
    _hostnames(hostnames),
    _dets(hostnames.size(), NULL)
{
  /* Create an EPICS exit handler */
  epicsAtExit(exitHandler, this);

  createParam(SlsInitString,      asynParamInt32,   &_initValue);
  createParam(SlsNumDetString,    asynParamInt32,   &_numDetValue);
  createParam(SlsRunStatusString, asynParamInt32,   &_runStatusValue);
  createParam(SlsConnStatusString,asynParamInt32,   &_connStatusValue);
  createParam(SlsHostNameString,  asynParamOctet,   &_hostNameValue);
  createParam(SlsFpgaTempString,  asynParamFloat64, &_fpgaTempValue);
  createParam(SlsAdcTempString,   asynParamFloat64, &_adcTempValue);

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
            "%s:%s, port=%s, address=%d start\n",
            driverName, functionName, this->portName, addr);

  //pasynManager->exceptionConnect(pasynUser);

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
                "%s:%s, port=%s, address=%d failed to initialize detector\n",
                driverName, functionName, this->portName, addr);
      status = asynDisabled;
    }
  }

  if (_dets[addr]) {
    getIntegerParam(addr, _connStatusValue, &conn);
    getIntegerParam(_numDetValue, &numDet);
    if (!conn) {
      //std::string reply = _dets[addr]->getHostname();
      //setStringParam(addr, _hostNameValue, reply);
      reply = _dets[addr]->request(SlsDetMessage::CheckOnline, 0.5);
      if (reply.mtype() == SlsDetMessage::Ok) {
        pasynManager->exceptionConnect(pasynUser);
        updateEnums(addr);
        setIntegerParam(addr, _connStatusValue, CONNECTED);
        callParamCallbacks(addr);
        setIntegerParam(_numDetValue, ++numDet);
        callParamCallbacks();
        asynPrint(pasynUser, ASYN_TRACE_FLOW,
            "%s:%s, port=%s, address=%d connected to detector\n",
            driverName, functionName, this->portName, addr);
      } else {
        asynPrint(pasynUser, ASYN_TRACE_FLOW,
            "%s:%s, port=%s, address=%d failed to connect to detector\n",
            driverName, functionName, this->portName, addr);
      }
    }
  } else if (status == asynSuccess) {
    asynPrint(pasynUser, ASYN_TRACE_ERROR,
              "%s:%s, port=%s, address=%d failed to initialize the detector interface\n",
              driverName, functionName, this->portName, addr);
    status = asynDisabled;
  }

  asynPrint(pasynUser, ASYN_TRACE_FLOW,
            "%s:%s, port=%s, address=%d end\n",
            driverName, functionName, this->portName, addr);

  return status;
}

asynStatus SlsDet::uninitialize(asynUser *pasynUser)
{
  int addr;
  int numDet;
  asynStatus status;
  static const char *functionName = "uninitialize";

  status = getAddress(pasynUser, &addr); if (status != asynSuccess) return status;

  if (status == asynSuccess) {
    setIntegerParam(addr, _connStatusValue, DISCONNECTED);
    callParamCallbacks(addr);
    getIntegerParam(_numDetValue, &numDet);
    setIntegerParam(_numDetValue, numDet - 1);
    callParamCallbacks();
    pasynManager->exceptionDisconnect(pasynUser);
  }

  asynPrint(pasynUser, ASYN_TRACE_FLOW,
            "%s:%s:, pasynUser=%p\n",
            driverName, functionName, pasynUser);
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
  unsigned nElem;

  /* Update the enums for the connection status */
  for (nElem=0; (nElem<SlsConnStatusNumEnums) && (nElem<SLS_MAX_ENUMS); ++nElem) {
    std::strncpy(_enumStrings[nElem], SlsConnStatusEnums[nElem].name.c_str(), MAX_ENUM_STRING_SIZE)[MAX_ENUM_STRING_SIZE] = '\0';
    _enumValues[nElem] = SlsConnStatusEnums[nElem].value;
    _enumSeverities[nElem] = SlsConnStatusEnums[nElem].severity;
  }
  doCallbacksEnum(_enumStrings, _enumValues, _enumSeverities, nElem, _connStatusValue, addr);

  /* Update the enums for the run status */
  for (nElem=0; (nElem<SlsRunStatusNumEnums) && (nElem<SLS_MAX_ENUMS); ++nElem) {
    std::strncpy(_enumStrings[nElem], SlsRunStatusEnums[nElem].name.c_str(), MAX_ENUM_STRING_SIZE)[MAX_ENUM_STRING_SIZE] = '\0';
    _enumValues[nElem] = SlsRunStatusEnums[nElem].value;
    _enumSeverities[nElem] = SlsRunStatusEnums[nElem].severity;
  }
  doCallbacksEnum(_enumStrings, _enumValues, _enumSeverities, nElem, _runStatusValue, addr);
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
  int conn;
  int function = pasynUser->reason;
  double timeout = pasynUser->timeout;
  asynStatus status = this->getAddress(pasynUser, &addr);
  static const char *functionName = "readDetector";

  if (status == asynSuccess) {
    getIntegerParam(addr, _connStatusValue, &conn);
    if (isConnected(addr)) {
      asynPrint(pasynUser, ASYN_TRACE_FLOW,
                "%s:%s: port=%s address=%d sending request of type %s with timeout %g\n",
                driverName, functionName, this->portName, addr,
                SlsDetMessage::messageType(mtype).c_str(), timeout);
      SlsDetMessage reply = _dets[addr]->request(mtype, timeout);
      asynPrint(pasynUser, ASYN_TRACE_FLOW,
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
          status = asynError;
          break;
        }
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

asynStatus SlsDet::readRunStatus(asynUser *pasynUser, epicsInt32 *value)
{
  int addr;
  int conn;
  slsDetectorDefs::runStatus raw_value;
  int function = pasynUser->reason;
  asynStatus status = asynSuccess;
  static const char *functionName = "readRunStatus";

  this->getAddress(pasynUser, &addr);
  getIntegerParam(addr, _connStatusValue, &conn);

  if (conn == CONNECTED) {
    raw_value = _dets[addr]->getRunStatus(&status);
    if (status == asynSuccess) {
      *value = raw_value;
      asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
                "%s:%s, port %s, read raw value %d (%s) from address %d\n",
                driverName, functionName, this->portName, raw_value,
                slsDetectorDefs::runStatusType(raw_value).c_str(), addr);
      setIntegerParam(addr, function, *value);
      callParamCallbacks(addr);
    } else {
      uninitialize(pasynUser);
      asynPrint(pasynUser, ASYN_TRACE_FLOW,
                "%s:%s: port=%s address=%d disconnect on error\n",
                driverName, functionName, this->portName, addr);
    }
  } else {
    status = asynDisconnected;
  }

  return status;
}

asynStatus SlsDet::readFloat64(asynUser *pasynUser, epicsFloat64 *value)
{
  int addr;
  int function = pasynUser->reason;
  asynStatus status = asynSuccess;

  this->getAddress(pasynUser, &addr);

  if (function == _fpgaTempValue) {
    status = readDetector(pasynUser, SlsDetMessage::ReadFpgaTemp);
  } else if (function == _adcTempValue) {
    status = readDetector(pasynUser, SlsDetMessage::ReadAdcTemp);
  } else { // Other functions we call the base class method
    return asynPortDriver::readFloat64(pasynUser, value);
  }

  // if the read was successful then set value from parameters
  if (status == asynSuccess) {
    status = getDoubleParam(addr, function, value);
  }
  
  return status;
}

asynStatus SlsDet::readInt32(asynUser *pasynUser, epicsInt32 *value)
{
  int function = pasynUser->reason;
  asynStatus status = asynSuccess;

  if (function == _runStatusValue) {
    status = readRunStatus(pasynUser, value);
  } else { // Other functions we call the base class method
    status = asynPortDriver::readInt32(pasynUser, value);
  }

  return status;
}

asynStatus SlsDet::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
  int addr;
  int function = pasynUser->reason;
  asynStatus status = asynSuccess;

  this->getAddress(pasynUser, &addr);

  status = asynPortDriver::writeInt32(pasynUser, value);

  return status;
}

asynStatus SlsDet::readEnum(asynUser *pasynUser, char *strings[], int values[],
                            int severities[], size_t nElements, size_t *nIn)
{
  int i;
  int function = pasynUser->reason;

  if (function == _connStatusValue) {
    for (i = 0; ((i < (int)SlsConnStatusNumEnums) && (i < (int)nElements)); ++i) {
      if (strings[i]) free(strings[i]);
      strings[i] = epicsStrDup(SlsConnStatusEnums[i].name.c_str());
      values[i] = SlsConnStatusEnums[i].value;
      severities[i] = SlsConnStatusEnums[i].severity;
    }
    *nIn = i;
    return asynSuccess;
  } else if (function == _runStatusValue) {
    for (i = 0; ((i < (int)SlsRunStatusNumEnums) && (i < (int)nElements)); ++i) {
      if (strings[i]) free(strings[i]);
      strings[i] = epicsStrDup(SlsRunStatusEnums[i].name.c_str());
      values[i] = SlsRunStatusEnums[i].value;
      severities[i] = SlsRunStatusEnums[i].severity;
    }
    *nIn = i;
    return asynSuccess;
  } else {
    *nIn = 0;
    return asynError;
  }
}

/** Configuration command, called directly or from iocsh */
extern "C" int SlsDetConfigure(const char *portName, const char *hostName, int id)
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
  new SlsDet(portName, hostnames, id);
  return(asynSuccess);
}


static const iocshArg configArg0 = { "Port name",         iocshArgString};
static const iocshArg configArg1 = { "Detector Hostname", iocshArgString};
static const iocshArg configArg2 = { "Detector Id",       iocshArgInt};
static const iocshArg * const configArgs[] = {&configArg0,
                                              &configArg1,
                                              &configArg2};
static const iocshFuncDef configFuncDef = {"SlsDetConfigure", 3, configArgs};
static void configCallFunc(const iocshArgBuf *args)
{
  SlsDetConfigure(args[0].sval, args[1].sval, args[2].ival);
}

void drvSlsDetRegister(void)
{
  iocshRegister(&configFuncDef,configCallFunc);
}

extern "C" {
epicsExportRegistrar(drvSlsDetRegister);
}

#undef MAX_ENUM_STRING_SIZE
