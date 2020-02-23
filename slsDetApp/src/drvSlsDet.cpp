#include <asynPortDriver.h>
#include <iocsh.h>
#include <alarm.h>
#include <epicsExit.h>
#include <epicsString.h>

#include <string.h>

#include "drvSlsDet.h"
#include "multiSlsDetector.h"

#include <epicsExport.h>

static const char *driverName = "SlsDet";

static void exitHandler(void *drvPvt) {
  SlsDet *pPvt = (SlsDet *)drvPvt;
  delete pPvt;
}

const SlsDet::SlsDetEnumInfo SlsDet::SlsConnStatusEnums[] = {
  {"Disconnected",  0, epicsSevMajor},
  {"Connected",     1, epicsSevNone}
};
const size_t SlsDet::SlsConnStatusNumEnums = sizeof(SlsDet::SlsConnStatusEnums) / sizeof(SlsDet::SlsConnStatusEnums[0]);

/* Port driver parameters */
#define SlsNumDetString       "SLS_NUM_DETS"
#define SlsRunStatusString    "SLS_RUN_STATUS"
#define SlsConnStatusString   "SLS_CONN_STATUS"
#define SlsHostNameString     "SLS_HOSTNAME"
#define SlsFpgaTempString     "SLS_FPGA_TEMP"
#define SlsAdcTempString      "SLS_ADC_TEMP"

/** Constructor for the SlsDet class
  */
SlsDet::SlsDet(const char *portName, const char *hostName, int maxdets, int id)
  : asynPortDriver(portName, maxdets, 
      asynEnumMask | asynInt32Mask | asynFloat64Mask | asynOctetMask | asynDrvUserMask, // Interfaces that we implement
      asynEnumMask | asynInt32Mask,                                                     // Interfaces that do callbacks
      ASYN_MULTIDEVICE | ASYN_CANBLOCK, 1, /* ASYN_CANBLOCK=1, ASYN_MULTIDEVICE=1, autoConnect=1 */
      0, 0),  /* Default priority and stack size */
    _maxdets(maxdets),
    _id(id),
    _hostname(hostName),
    _det(0)
{
  /* Create an EPICS exit handler */
  epicsAtExit(exitHandler, this);

  createParam(SlsNumDetString,    asynParamInt32,   &_numDetValue);
  createParam(SlsRunStatusString, asynParamInt32,   &_runStatusValue);
  createParam(SlsConnStatusString,asynParamInt32,   &_connStatusValue);
  createParam(SlsHostNameString,  asynParamOctet,   &_hostNameValue);
  createParam(SlsFpgaTempString,  asynParamFloat64, &_fpgaTempValue);
  createParam(SlsAdcTempString,   asynParamFloat64, &_adcTempValue);

  
}

SlsDet::~SlsDet()
{
  shutdown();
}

asynStatus SlsDet::initialize(asynUser *pasynUser)
{
  int conn;
  int addr;
  asynStatus status;
  static const char *functionName = "initialize";

  status = getAddress(pasynUser, &addr); if (status != asynSuccess) return status;

  if (!_det) {
    try {
      _det = new multiSlsDetector(_id);
      _det->setHostname(_hostname.c_str());
      int numDetectors = _det->getNumberOfDetectors();
      if (numDetectors != _maxdets) {
        /* partial initialized multidets can have wrong addresses so teardown the whole thing */
        shutdown();
        asynPrint(pasynUser, ASYN_TRACE_ERROR,
                 "%s:%s: only configured %d out of %d sub-detectors\n",
                 driverName, functionName, numDetectors, _maxdets);
        status = asynDisabled;
      } else {
        setIntegerParam(_numDetValue, numDetectors);
        callParamCallbacks();
      }
    } catch (...) {
      asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s: failed to initialize detector\n",
                driverName, functionName);
      status = asynDisabled;
    }
  }

  if (status != asynSuccess) {
    return status;
  } else if (_det) {
    for (int addr=0; addr<_maxdets; addr++) {
      getIntegerParam(addr, _connStatusValue, &conn);
      if (!conn) {
        printf("before\n");
        std::string reply = _det->getHostname(addr);
        setStringParam(addr, _hostNameValue, reply);
        slsDetectorDefs::runStatus s = _det->getRunStatus();
        printf("the reply was %s\n", _det->runStatusType(s).c_str());
        status = checkError(pasynUser, functionName, false);
        if (status == asynSuccess) {
          printf("reconn set %d\n", status);
          setIntegerParam(addr, _connStatusValue, 1);
        }
        callParamCallbacks(addr);
        printf("after\n");
      }
    }
    if (status == asynSuccess) pasynManager->exceptionConnect(pasynUser);
  } else {
    asynPrint(pasynUser, ASYN_TRACE_ERROR,
              "%s:%s: detector interface must exist to be able to configure\n",
              driverName, functionName);
    status = asynDisabled;
  }

  asynPrint(pasynUser, ASYN_TRACE_FLOW,
            "%s:%s:, pasynUser=%p\n",
            driverName, functionName, pasynUserSelf);

  return status;
}

asynStatus SlsDet::uninitialize(asynUser *pasynUser, bool shutdown)
{
  int addr;
  asynStatus status;
  static const char *functionName = "uninitialize";

  status = getAddress(pasynUser, &addr); if (status != asynSuccess) return status;

  if (shutdown) {
    for (addr=0; addr<_maxdets; addr++) {
      setIntegerParam(addr, _connStatusValue, 0);
    }
    callParamCallbacks();
    this->shutdown();
  } else {
    status = getAddress(pasynUser, &addr);
    printf("killing addr %d\n", addr);
    if (status == asynSuccess) {
      setIntegerParam(addr, _connStatusValue, 0);
      callParamCallbacks(addr);
    }
  }

  pasynManager->exceptionDisconnect(pasynUser);
  asynPrint(pasynUser, ASYN_TRACE_FLOW,
            "%s:%s:, pasynUser=%p\n",
            driverName, functionName, pasynUser);
  return status;
}

void SlsDet::shutdown()
{
  if (_det) {
    delete _det;
    _det = NULL;
    // free the shared memory associated with the detector
    multiSlsDetector::freeSharedMemory(_id);
  }
}

asynStatus SlsDet::connect(asynUser *pasynUser)
{
  return initialize(pasynUser);
}

asynStatus SlsDet::disconnect(asynUser *pasynUser)
{
  return uninitialize(pasynUser, true);
}

asynStatus SlsDet::checkError(asynUser *pasynUser, const char* caller, bool disconnect)
{
  int addr;
  int crit_err = 0;
  int64_t err_bit;
  int64_t err_mask;

  // Get the addr and get the bit to 
  this->getAddress(pasynUser, &addr);
  err_bit = 1L << addr;
  err_mask = _det->getErrorMask();

  printf("error (mask, bit) %ld %ld\n", err_mask, err_bit);

  if (err_mask & err_bit) {
    asynPrint(pasynUser, ASYN_TRACE_ERROR,
             "%s:%s, port %s, ERROR (0x%lx) reading from address %d - %s\n",
             driverName, caller, this->portName, err_mask, addr,
             _det->getErrorMessage(crit_err).c_str());
    if (disconnect && crit_err) {
      uninitialize(pasynUser);
    }
    // clear the error mask
    _det->clearAllErrorMask();
    return asynTimeout;
  } else {
    return asynSuccess;
  }
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

  if (conn) {
    raw_value = _det->getRunStatus();
    if (status == asynSuccess) {
      *value = raw_value;
      asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
                "%s:%s, port %s, read raw value %d (%s) from address %d\n",
                driverName, functionName, this->portName, raw_value,
                slsDetectorDefs::runStatusType(raw_value).c_str(), addr);
      setIntegerParam(addr, function, *value);
      callParamCallbacks(addr);
    }
  } else {
    status = asynDisconnected;
  }

  return status;
}

asynStatus SlsDet::readTemperature(asynUser *pasynUser, epicsFloat64 *value,
                                   slsDetectorDefs::dacIndex dac)
{
  int addr;
  int conn;
  int raw_value;
  int function = pasynUser->reason;
  asynStatus status = asynSuccess;
  static const char *functionName = "readTemperature";

  this->getAddress(pasynUser, &addr);
  getIntegerParam(addr, _connStatusValue, &conn);

  if (conn) {
    raw_value = _det->getADC(dac, addr);
    status = checkError(pasynUser, functionName);
    if (status == asynSuccess) {
      *value = raw_value / 1000.;
      asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
                "%s:%s, port %s, read raw (converted) value %d (%g) from address %d\n",
                driverName, functionName, this->portName, raw_value, *value, addr);
      setDoubleParam(addr, function, *value);
      callParamCallbacks(addr);
    }
  } else {
    status = asynDisconnected;
  }

  return status;
}

asynStatus SlsDet::readFloat64(asynUser *pasynUser, epicsFloat64 *value)
{
  int function = pasynUser->reason;
  asynStatus status = asynSuccess;

  if (function == _fpgaTempValue) {
    status = readTemperature(pasynUser, value, slsDetectorDefs::TEMPERATURE_FPGA);
  } else if (function == _adcTempValue) {
    status = readTemperature(pasynUser, value, slsDetectorDefs::TEMPERATURE_ADC);
  } else { // Other functions we call the base class method
     status = asynPortDriver::readFloat64(pasynUser, value);
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
      strings[i] = epicsStrDup(SlsConnStatusEnums[i].name);
      values[i] = SlsConnStatusEnums[i].value;
      severities[i] = SlsConnStatusEnums[i].severity;
    }
    *nIn = i;
    return asynSuccess;
  } else {
    *nIn = 0;
    return asynError;
  }
}

/** Configuration command, called directly or from iocsh */
extern "C" int SlsDetConfigure(const char *portName, const char *hostName, int maxdets, int id)
{
  new SlsDet(portName, hostName, maxdets, id);
  return(asynSuccess);
}


static const iocshArg configArg0 = { "Port name",         iocshArgString};
static const iocshArg configArg1 = { "Detector Hostname", iocshArgString};
static const iocshArg configArg2 = { "Detector Num",      iocshArgInt};
static const iocshArg configArg3 = { "Detector Id",       iocshArgInt};
static const iocshArg * const configArgs[] = {&configArg0,
                                              &configArg1,
                                              &configArg2,
                                              &configArg3};
static const iocshFuncDef configFuncDef = {"SlsDetConfigure", 4, configArgs};
static void configCallFunc(const iocshArgBuf *args)
{
  SlsDetConfigure(args[0].sval, args[1].sval, args[2].ival, args[3].ival);
}

void drvSlsDetRegister(void)
{
  iocshRegister(&configFuncDef,configCallFunc);
}

extern "C" {
epicsExportRegistrar(drvSlsDetRegister);
}


