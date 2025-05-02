#include "drvAsynSlsDetPort.h"
#include "sls/Detector.h"

#include <iocsh.h>
#include <epicsExit.h>
#include <epicsThread.h>
#include <epicsString.h>

#include <epicsExport.h>

#include <cstdlib>
#include <cstring>

static const char *driverName = "SlsDet";

static void exitHandler(void *drvPvt) {
  SlsDet *pPvt = (SlsDet *)drvPvt;
  delete pPvt;
}

static std::string hexString(int64_t number)
{
  std::stringstream ss;
  ss << std::hex << std::showbase << number;
  return ss.str();
}

static std::chrono::nanoseconds secs_to_ns(double secs)
{
  std::chrono::duration<double> dursecs{secs};
  return std::chrono::duration_cast<std::chrono::nanoseconds>(dursecs);
}

static double ns_to_secs(std::chrono::nanoseconds ns)
{
  return std::chrono::duration<double>(ns).count();
}

/* Max size for enum strings */
#define MAX_ENUM_STRING_SIZE 32
#define MOD_ID_BITS 8

/* Port driver basic parameters */
#define SlsShmIdString      "SLS_SHM_ID"
#define SlsHeartbeatString  "SLS_HEARTBEAT"
#define SlsEnabledString    "SLS_DET_ENABLED"
#define SlsRunStatusString  "SLS_RUN_STATUS"
#define SlsConnStatusString "SLS_CONN_STATUS"
#define SlsNextFrameString  "SLS_NEXT_FRAME"
#define SlsHostNameString   "SLS_HOSTNAME"
#define SlsDetTypeString    "SLS_DET_TYPE"
/* Port driver version parameters */
#define SlsDetModuleIdString    "SLS_MODULE_ID"
#define SlsDetSerialNumString   "SLS_SERIAL_NUMBER"
#define SlsDetFirmwareVerString "SLS_FIRMWARE_VERSION"
#define SlsDetSoftwareVerString "SLS_SOFTWARE_VERSION"
#define SlsDetHardwareVerString "SLS_HARDWARE_VERSION"
#define SlsDetKernelVerString   "SLS_KERNEL_VERSION"
#define SlsDetChipVerString     "SLS_CHIP_VERSION"
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
#define SlsGetClockSpeedString    "SLS_GET_SPEED"
#define SlsSetClockSpeedString    "SLS_SET_SPEED"
#define SlsGetGainModeString      "SLS_GET_GAIN"
#define SlsSetGainModeString      "SLS_SET_GAIN"
#define SlsGetHighGain0String     "SLS_GET_HIGH_GAIN0"
#define SlsSetHighGain0String     "SLS_SET_HIGH_GAIN0"
#define SlsGetAdcPhaseString      "SLS_GET_ADC_PHASE"
#define SlsSetAdcPhaseString      "SLS_SET_ADC_PHASE"
/* Port driver trigger and exposure parameters */
#define SlsGetExpTimeString       "SLS_GET_EXPTIME"
#define SlsSetExpTimeString       "SLS_SET_EXPTIME"
#define SlsGetPeriodString        "SLS_GET_PERIOD"
#define SlsSetPeriodString        "SLS_SET_PERIOD"
#define SlsGetDelayString         "SLS_GET_DELAY"
#define SlsSetDelayString         "SLS_SET_DELAY"
#define SlsGetTriggerModeString   "SLS_GET_TRIGGER"
#define SlsSetTriggerModeString   "SLS_SET_TRIGGER"
#define SlsGetNumFramesString     "SLS_GET_NUM_FRAMES"
#define SlsSetNumFramesString     "SLS_SET_NUM_FRAMES"
#define SlsGetNumTriggersString   "SLS_GET_NUM_TRIGGERS"
#define SlsSetNumTriggersString   "SLS_SET_NUM_TRIGGERS"
/* Data interface configuration parameters */
#define SlsGetNumSfpString        "SLS_GET_NUM_SFP"
#define SlsSetNumSfpString        "SLS_SET_NUM_SFP"
#define SlsGetActiveSfpString     "SLS_GET_ACT_SFP"
#define SlsSetActiveSfpString     "SLS_SET_ACT_SFP"
#define SlsGetSrcUdpIpString      "SLS_GET_SRC_IP"
#define SlsSetSrcUdpIpString      "SLS_SET_SRC_IP"
#define SlsGetSrcUdpIp2String     "SLS_GET_SRC_IP2"
#define SlsSetSrcUdpIp2String     "SLS_SET_SRC_IP2"
#define SlsGetSrcUdpMacString     "SLS_GET_SRC_MAC"
#define SlsSetSrcUdpMacString     "SLS_SET_SRC_MAC"
#define SlsGetSrcUdpMac2String    "SLS_GET_SRC_MAC2"
#define SlsSetSrcUdpMac2String    "SLS_SET_SRC_MAC2"
#define SlsGetDestUdpIpString     "SLS_GET_DST_IP"
#define SlsSetDestUdpIpString     "SLS_SET_DST_IP"
#define SlsGetDestUdpIp2String    "SLS_GET_DST_IP2"
#define SlsSetDestUdpIp2String    "SLS_SET_DST_IP2"
#define SlsGetDestUdpMacString    "SLS_GET_DST_MAC"
#define SlsSetDestUdpMacString    "SLS_SET_DST_MAC"
#define SlsGetDestUdpMac2String   "SLS_GET_DST_MAC2"
#define SlsSetDestUdpMac2String   "SLS_SET_DST_MAC2"
#define SlsGetDestUdpPortString   "SLS_GET_DST_PORT"
#define SlsSetDestUdpPortString   "SLS_SET_DST_PORT"
#define SlsGetDestUdpPort2String  "SLS_GET_DST_PORT2"
#define SlsSetDestUdpPort2String  "SLS_SET_DST_PORT2"
#define SlsGetTxDelayFrameString  "SLS_GET_TX_DELAY"
#define SlsSetTxDelayFrameString  "SLS_SET_TX_DELAY"
#define SlsGetFlowControlString   "SLS_GET_FLOW_CTRL"
#define SlsSetFlowControlString   "SLS_SET_FLOW_CTRL"

#define sizeofArray(arr) sizeof(arr) / sizeof(arr[0])

const SlsDet::SlsDetEnumInfo SlsDet::SlsOnOffEnums[] = {
  {"Off", OFF,  epicsSevNone},
  {"On",  ON,   epicsSevNone}
};

const SlsDet::SlsDetEnumInfo SlsDet::SlsSfpSlotEnums[] = {
  {"Outer", OUTER,  epicsSevNone},
  {"Inner", INNER,  epicsSevNone},
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
  {sls::ToString(sls::defs::IDLE),          sls::defs::IDLE,          epicsSevNone},
  {sls::ToString(sls::defs::ERROR),         sls::defs::ERROR,         epicsSevMajor},
  {sls::ToString(sls::defs::WAITING),       sls::defs::WAITING,       epicsSevNone},
  {sls::ToString(sls::defs::RUN_FINISHED),  sls::defs::RUN_FINISHED,  epicsSevNone},
  {sls::ToString(sls::defs::TRANSMITTING),  sls::defs::TRANSMITTING,  epicsSevNone},
  {sls::ToString(sls::defs::RUNNING),       sls::defs::RUNNING,       epicsSevNone},
  {sls::ToString(sls::defs::STOPPED),       sls::defs::STOPPED,       epicsSevNone}
};

const SlsDet::SlsDetEnumInfo SlsDet::SlsDetTypesEnums[] = {
  {sls::ToString(sls::defs::GENERIC),         sls::defs::GENERIC,       epicsSevNone},
  {sls::ToString(sls::defs::EIGER),           sls::defs::EIGER,         epicsSevNone},
  {sls::ToString(sls::defs::GOTTHARD),        sls::defs::GOTTHARD,      epicsSevNone},
  {sls::ToString(sls::defs::JUNGFRAU),        sls::defs::JUNGFRAU,      epicsSevNone},
  {sls::ToString(sls::defs::CHIPTESTBOARD),   sls::defs::CHIPTESTBOARD, epicsSevNone},
  {sls::ToString(sls::defs::MOENCH),          sls::defs::MOENCH,        epicsSevNone},
  {sls::ToString(sls::defs::MYTHEN3),         sls::defs::MYTHEN3,       epicsSevNone},
  {sls::ToString(sls::defs::GOTTHARD2),       sls::defs::GOTTHARD2,     epicsSevNone},
};

const SlsDet::SlsDetEnumInfo SlsDet::SlsClkSpeedEnums[] = {
  {sls::ToString(sls::defs::FULL_SPEED),    sls::defs::FULL_SPEED,    epicsSevNone},
  {sls::ToString(sls::defs::HALF_SPEED),    sls::defs::HALF_SPEED,    epicsSevNone},
  {sls::ToString(sls::defs::QUARTER_SPEED), sls::defs::QUARTER_SPEED, epicsSevNone},
};

const SlsDet::SlsDetEnumInfo SlsDet::SlsGainEnums[] = {
  {sls::ToString(sls::defs::DYNAMIC),         sls::defs::DYNAMIC,         epicsSevNone},
  {sls::ToString(sls::defs::FORCE_SWITCH_G1), sls::defs::FORCE_SWITCH_G1, epicsSevNone},
  {sls::ToString(sls::defs::FORCE_SWITCH_G2), sls::defs::FORCE_SWITCH_G2, epicsSevNone},
  {sls::ToString(sls::defs::FIX_G1),          sls::defs::FIX_G1,          epicsSevNone},
  {sls::ToString(sls::defs::FIX_G2),          sls::defs::FIX_G2,          epicsSevNone},
  {sls::ToString(sls::defs::FIX_G0),          sls::defs::FIX_G0,          epicsSevMinor},
};

const SlsDet::SlsDetEnumInfo SlsDet::SlsTriggerEnums[] = {
  {sls::ToString(sls::defs::AUTO_TIMING),       sls::defs::AUTO_TIMING,       epicsSevNone},
  {sls::ToString(sls::defs::TRIGGER_EXPOSURE),  sls::defs::TRIGGER_EXPOSURE,  epicsSevNone},
  {sls::ToString(sls::defs::GATED),             sls::defs::GATED,             epicsSevNone},
  {sls::ToString(sls::defs::BURST_TRIGGER),     sls::defs::BURST_TRIGGER,     epicsSevNone},
  {sls::ToString(sls::defs::TRIGGER_GATED),     sls::defs::TRIGGER_GATED,     epicsSevNone},
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
  {SlsOnOffEnums,
   sizeofArray(SlsOnOffEnums),
   SlsEnabledString},
  {SlsOnOffEnums,
   sizeofArray(SlsOnOffEnums),
   SlsGetHighGain0String},
  {SlsOnOffEnums,
   sizeofArray(SlsOnOffEnums),
   SlsSetHighGain0String},
  {SlsOnOffEnums,
   sizeofArray(SlsOnOffEnums),
   SlsGetFlowControlString},
  {SlsOnOffEnums,
   sizeofArray(SlsOnOffEnums),
   SlsSetFlowControlString},
  {SlsOkTrippedEnums,
   sizeofArray(SlsOkTrippedEnums),
   SlsGetTempEventString},
  {SlsOkTrippedEnums,
   sizeofArray(SlsOkTrippedEnums),
   SlsSetTempEventString},
  {SlsSfpSlotEnums,
   sizeofArray(SlsSfpSlotEnums),
   SlsGetActiveSfpString},
  {SlsSfpSlotEnums,
   sizeofArray(SlsSfpSlotEnums),
   SlsSetActiveSfpString},
  {SlsConnStatusEnums,
   sizeofArray(SlsConnStatusEnums),
   SlsConnStatusString},
  {SlsRunStatusEnums,
   sizeofArray(SlsRunStatusEnums),
   SlsRunStatusString},
  {SlsDetTypesEnums,
   sizeofArray(SlsDetTypesEnums),
   SlsDetTypeString},
  {SlsClkSpeedEnums,
   sizeofArray(SlsClkSpeedEnums),
   SlsGetClockSpeedString},
  {SlsClkSpeedEnums,
   sizeofArray(SlsClkSpeedEnums),
   SlsSetClockSpeedString},
  {SlsGainEnums,
   sizeofArray(SlsGainEnums),
   SlsGetGainModeString},
  {SlsGainEnums,
   sizeofArray(SlsGainEnums),
   SlsSetGainModeString},
  {SlsTriggerEnums,
   sizeofArray(SlsTriggerEnums),
   SlsGetTriggerModeString},
  {SlsTriggerEnums,
   sizeofArray(SlsTriggerEnums),
   SlsSetTriggerModeString}
};

const size_t SlsDet::SlsDetEnumsSize = sizeofArray(SlsDet::SlsDetEnums);

static void slsDetStatusTaskC(void *drvPvt)
{
  SlsDet *pPvt = (SlsDet*) drvPvt;

  pPvt->statusTask();
}

/** Constructor for the SlsDet class
  */
SlsDet::SlsDet(const char *portName, const std::string& hostname, int id, double boot)
  : asynPortDriver(portName, 1,
      asynEnumMask | asynInt32Mask | asynInt64Mask | asynFloat64Mask | asynOctetMask | asynDrvUserMask, // Interfaces that we implement
      asynEnumMask | asynInt32Mask | asynInt64Mask | asynFloat64Mask | asynOctetMask,                   // Interfaces that do callbacks
      ASYN_MULTIDEVICE | ASYN_CANBLOCK, 1, /* ASYN_CANBLOCK=1, ASYN_MULTIDEVICE=1, autoConnect=1 */
      0, 0),  /* Default priority and stack size */
    _id(id),
    _exiting(false),
    _exited(0),
    _pollingPeriod(1.0), // seconds
    _fastPollingPeriod(0.1), // seconds
    _connPollingPeriod(boot), // seconds
    _hostname(hostname),
    _det(new sls::Detector(id))
{
  static const char *functionName = "SlsDet";

  /* Create an EPICS exit handler */
  epicsAtExit(exitHandler, this);

  createParam(SlsShmIdString,             asynParamInt32,   &_shmIdParam);
  createParam(SlsHeartbeatString,         asynParamInt32,   &_heartbeatParam);
  createParam(SlsEnabledString,           asynParamInt32,   &_enabledParam);
  createParam(SlsConnStatusString,        asynParamInt32,   &_connStatusParam);
  /* Add the features that read/write to the module */
  addFeature(SlsRunStatusString,          asynParamInt32,   SlsDetUtils::DETSTATUS,   false, false);
  addFeature(SlsNextFrameString,          asynParamInt64,   SlsDetUtils::NEXTFRAME,   false, false);
  /* Read-only constant features -> only read once on connect */
  addFeature(SlsHostNameString,           asynParamOctet,   SlsDetUtils::HOSTNAME,    true,  false);
  addFeature(SlsDetTypeString,            asynParamInt32,   SlsDetUtils::DETTYPE,     true,  false);
  addFeature(SlsDetModuleIdString,        asynParamInt32,   SlsDetUtils::MODULEID,    true,  false);
  addFeature(SlsDetSerialNumString,       asynParamOctet,   SlsDetUtils::SERIALNUM,   true,  false);
  addFeature(SlsDetFirmwareVerString,     asynParamOctet,   SlsDetUtils::FIRMWARE,    true,  false);
  addFeature(SlsDetSoftwareVerString,     asynParamOctet,   SlsDetUtils::SOFTWARE,    true,  false);
  addFeature(SlsDetHardwareVerString,     asynParamOctet,   SlsDetUtils::HARDWARE,    true,  false);
  addFeature(SlsDetKernelVerString,       asynParamOctet,   SlsDetUtils::KERNELVER,   true,  false);
  addFeature(SlsDetChipVerString,         asynParamFloat64, SlsDetUtils::CHIPVER,     true,  false);
  /* Temperature related features */
  addFeature(SlsFpgaTempString,           asynParamFloat64, SlsDetUtils::FPGATEMP,    false, false);
  addFeature(SlsAdcTempString,            asynParamFloat64, SlsDetUtils::ADCTEMP,     false, false);
  addFeature(SlsSetTempThresholdString,   asynParamFloat64, SlsDetUtils::THRESHOLD,   false, true );
  addFeature(SlsGetTempThresholdString,   asynParamFloat64, SlsDetUtils::THRESHOLD,   false, false);
  addFeature(SlsSetTempControlString,     asynParamInt32,   SlsDetUtils::TEMPCONTROL, false, true );
  addFeature(SlsGetTempControlString,     asynParamInt32,   SlsDetUtils::TEMPCONTROL, false, false);
  addFeature(SlsSetTempEventString,       asynParamInt32,   SlsDetUtils::TEMPEVENT,   false, true );
  addFeature(SlsGetTempEventString,       asynParamInt32,   SlsDetUtils::TEMPEVENT,   false, false);
  /* Misc module configuration features */
  addFeature(SlsSetChipPowerString,       asynParamInt32,   SlsDetUtils::CHIPPOWER,   false, true );
  addFeature(SlsGetChipPowerString,       asynParamInt32,   SlsDetUtils::CHIPPOWER,   false, false);
  addFeature(SlsSetHighVoltageString,     asynParamInt32,   SlsDetUtils::HIGHVOLTAGE, false, true );
  addFeature(SlsGetHighVoltageString,     asynParamInt32,   SlsDetUtils::HIGHVOLTAGE, false, false);
  addFeature(SlsSetClockSpeedString,      asynParamInt32,   SlsDetUtils::CLOCKSPEED,  false, true );
  addFeature(SlsGetClockSpeedString,      asynParamInt32,   SlsDetUtils::CLOCKSPEED,  false, false);
  addFeature(SlsSetGainModeString,        asynParamInt32,   SlsDetUtils::GAINMODE,    false, true );
  addFeature(SlsGetGainModeString,        asynParamInt32,   SlsDetUtils::GAINMODE,    false, false);
  addFeature(SlsSetHighGain0String,       asynParamInt32,   SlsDetUtils::HIGHGAIN0,   false, true );
  addFeature(SlsGetHighGain0String,       asynParamInt32,   SlsDetUtils::HIGHGAIN0,   false, false);
  addFeature(SlsSetAdcPhaseString,        asynParamInt32,   SlsDetUtils::ADCPHASE,    false, true );
  addFeature(SlsGetAdcPhaseString,        asynParamInt32,   SlsDetUtils::ADCPHASE,    false, false);
  /* Trigger and exposure time related features */
  addFeature(SlsSetExpTimeString,         asynParamFloat64, SlsDetUtils::EXPTIME,     false, true );
  addFeature(SlsGetExpTimeString,         asynParamFloat64, SlsDetUtils::EXPTIME,     false, false);
  addFeature(SlsSetPeriodString,          asynParamFloat64, SlsDetUtils::PERIOD,      false, true );
  addFeature(SlsGetPeriodString,          asynParamFloat64, SlsDetUtils::PERIOD,      false, false);
  addFeature(SlsSetDelayString,           asynParamFloat64, SlsDetUtils::DELAY,       false, true );
  addFeature(SlsGetDelayString,           asynParamFloat64, SlsDetUtils::DELAY,       false, false);
  addFeature(SlsSetTriggerModeString,     asynParamInt32,   SlsDetUtils::TRIGMODE,    false, true );
  addFeature(SlsGetTriggerModeString,     asynParamInt32,   SlsDetUtils::TRIGMODE,    false, false);
  addFeature(SlsSetNumFramesString,       asynParamInt64,   SlsDetUtils::NUMFRAMES,   false, true );
  addFeature(SlsGetNumFramesString,       asynParamInt64,   SlsDetUtils::NUMFRAMES,   false, false);
  addFeature(SlsSetNumTriggersString,     asynParamInt64,   SlsDetUtils::NUMTRIG,     false, true );
  addFeature(SlsGetNumTriggersString,     asynParamInt64,   SlsDetUtils::NUMTRIG,     false, false);
  /* Data interface configuration parameters */
  addFeature(SlsSetNumSfpString,          asynParamInt32,   SlsDetUtils::NUMINTFACE,  false, true );
  addFeature(SlsGetNumSfpString,          asynParamInt32,   SlsDetUtils::NUMINTFACE,  false, false);
  addFeature(SlsSetActiveSfpString,       asynParamInt32,   SlsDetUtils::ACTINTFACE,  false, true );
  addFeature(SlsGetActiveSfpString,       asynParamInt32,   SlsDetUtils::ACTINTFACE,  false, false);
  addFeature(SlsSetSrcUdpIpString,        asynParamOctet,   SlsDetUtils::SRCUDPIP,    false, true );
  addFeature(SlsGetSrcUdpIpString,        asynParamOctet,   SlsDetUtils::SRCUDPIP,    false, false);
  addFeature(SlsSetSrcUdpMacString,       asynParamOctet,   SlsDetUtils::SRCUDPMAC,   false, true );
  addFeature(SlsGetSrcUdpMacString,       asynParamOctet,   SlsDetUtils::SRCUDPMAC,   false, false);
  addFeature(SlsSetSrcUdpIp2String,       asynParamOctet,   SlsDetUtils::SRCUDPIP2,   false, true );
  addFeature(SlsGetSrcUdpIp2String,       asynParamOctet,   SlsDetUtils::SRCUDPIP2,   false, false);
  addFeature(SlsSetSrcUdpMac2String,      asynParamOctet,   SlsDetUtils::SRCUDPMAC2,  false, true );
  addFeature(SlsGetSrcUdpMac2String,      asynParamOctet,   SlsDetUtils::SRCUDPMAC2,  false, false);
  addFeature(SlsSetDestUdpIpString,       asynParamOctet,   SlsDetUtils::DESTUDPIP,   false, true );
  addFeature(SlsGetDestUdpIpString,       asynParamOctet,   SlsDetUtils::DESTUDPIP,   false, false);
  addFeature(SlsSetDestUdpMacString,      asynParamOctet,   SlsDetUtils::DESTUDPMAC,  false, true );
  addFeature(SlsGetDestUdpMacString,      asynParamOctet,   SlsDetUtils::DESTUDPMAC,  false, false);
  addFeature(SlsSetDestUdpIp2String,      asynParamOctet,   SlsDetUtils::DESTUDPIP2,  false, true );
  addFeature(SlsGetDestUdpIp2String,      asynParamOctet,   SlsDetUtils::DESTUDPIP2,  false, false);
  addFeature(SlsSetDestUdpMac2String,     asynParamOctet,   SlsDetUtils::DESTUDPMAC2, false, true );
  addFeature(SlsGetDestUdpMac2String,     asynParamOctet,   SlsDetUtils::DESTUDPMAC2, false, false);
  addFeature(SlsSetDestUdpPortString,     asynParamInt32,   SlsDetUtils::DESTUDPPORT, false, true );
  addFeature(SlsGetDestUdpPortString,     asynParamInt32,   SlsDetUtils::DESTUDPPORT, false, false);
  addFeature(SlsSetDestUdpPort2String,    asynParamInt32,   SlsDetUtils::DESTUDPPORT2,false, true );
  addFeature(SlsGetDestUdpPort2String,    asynParamInt32,   SlsDetUtils::DESTUDPPORT2,false, false);
  addFeature(SlsSetTxDelayFrameString,    asynParamInt32,   SlsDetUtils::TXDELAYFRAME,false, true );
  addFeature(SlsGetTxDelayFrameString,    asynParamInt32,   SlsDetUtils::TXDELAYFRAME,false, false);
  addFeature(SlsSetFlowControlString,     asynParamInt32,   SlsDetUtils::FLOWCONTROL, false, true );
  addFeature(SlsGetFlowControlString,     asynParamInt32,   SlsDetUtils::FLOWCONTROL, false, false);

  /* Initialize the global parameters */
  setIntegerParam(_shmIdParam, id);
  callParamCallbacks();

  /* allocate memory to use for enum callbacks */
  for (unsigned i=0; i<SLS_MAX_ENUMS; i++) {
    _enumStrings[i] = new char[MAX_ENUM_STRING_SIZE+1];
  }

  /* Create the epicsEvent for signaling to the status task when parameters should have changed.
   * This will cause it to do a poll immediately, rather than wait for the poll time period.
   */
  this->statusEvent = epicsEventMustCreate(epicsEventEmpty);
  if (!this->statusEvent) {
    printf("%s:%s epicsEventCreate failure for start event\n", driverName, functionName);
    return;
  }

  /* send a signal to the status poller task */
  epicsEventSignal(statusEvent);

  int stackSize = epicsThreadGetStackSize(epicsThreadStackMedium);

  /* Create the thread that updates the detector status */
  int status = (epicsThreadCreate("SlsDetStatusTask",
                                  epicsThreadPriorityMedium,
                                  stackSize,
                                  (EPICSTHREADFUNC)slsDetStatusTaskC,
                                  this) == NULL);
  if (status) {
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
              "%s:%s: epicsThreadCreate failure for status task\n",
              driverName, functionName);
    return;
  }
}

SlsDet::~SlsDet()
{
  _exiting = true;
  epicsEventSignal(statusEvent);
  while(_exited < 1) {
    epicsThreadSleep(0.2);
  }
  this->lock();
  /* cleans up shared memory */
  if (_det) {
    _det.reset();
    sls::freeSharedMemory(_id);
  }
  for (unsigned i=0; i<SLS_MAX_ENUMS; i++) {
    if (_enumStrings[i]) delete[] _enumStrings[i];
  }
  this->unlock();
}

void SlsDet::addFeature(const char *name,
                        asynParamType type,
                        SlsDetUtils::Feature feature,
                        bool init,
                        bool writable)
{
  int index;
  createParam(name, type, &index);
  if (type == asynParamInt32) {
    _intFeatures.emplace(index, SlsDetUtils::IntFeature(feature, init, writable));
  } else if (type == asynParamInt64) {
    _int64Features.emplace(index, SlsDetUtils::Int64Feature(feature, init, writable));
  } else if (type == asynParamFloat64) {
    _doubleFeatures.emplace(index, SlsDetUtils::DoubleFeature(feature, init, writable));
  } else if (type == asynParamOctet) {
    _stringFeatures.emplace(index, SlsDetUtils::StringFeature(feature, init, writable));
  }
}

void SlsDet::writeStringToDetector(SlsDetUtils::Feature feature, const std::string& value)
{
  static const char *functionName = "writeStringToDetector";
  try {
    if ((feature == SlsDetUtils::SRCUDPIP)  || (feature == SlsDetUtils::SRCUDPIP2) ||
        (feature == SlsDetUtils::DESTUDPIP) || (feature == SlsDetUtils::DESTUDPIP2)) {
      sls::IpAddr ip(value);
      bool isSrc = (feature == SlsDetUtils::SRCUDPIP)  || (feature == SlsDetUtils::SRCUDPIP2);
      bool isSecond = (feature == SlsDetUtils::SRCUDPIP2) || (feature == SlsDetUtils::DESTUDPIP2);
      std::string prefix = isSrc ? "Source" : "Dest";
      std::string suffix = isSecond ? "2" : "";
      asynPrint(pasynUserSelf, ASYN_TRACEIO_DEVICE,
                "%s:%s: port=%s hostname=%s -> set%sUDPIP%s(%s)\n",
                driverName, functionName, this->portName,
                _hostname.c_str(), prefix.c_str(),
                suffix.c_str(), value.c_str());
      if (feature == SlsDetUtils::SRCUDPIP) {
        _det->setSourceUDPIP(ip);
      } else if (feature == SlsDetUtils::SRCUDPIP2) {
        _det->setSourceUDPIP2(ip);
      } else if (feature == SlsDetUtils::DESTUDPIP) {
        _det->setDestinationUDPIP(ip);
      } else if (feature == SlsDetUtils::DESTUDPIP2) {
        _det->setDestinationUDPIP2(ip);
      }
    } else if ((feature == SlsDetUtils::SRCUDPMAC)  || (feature == SlsDetUtils::SRCUDPMAC2) ||
               (feature == SlsDetUtils::DESTUDPMAC) || (feature == SlsDetUtils::DESTUDPMAC2)) {
      sls::MacAddr mac(value);
      bool isSrc = (feature == SlsDetUtils::SRCUDPMAC)  || (feature == SlsDetUtils::SRCUDPMAC2);
      bool isSecond = (feature == SlsDetUtils::SRCUDPMAC2) || (feature == SlsDetUtils::DESTUDPMAC2);
      std::string prefix = isSrc ? "Source" : "Dest";
      std::string suffix = isSecond ? "2" : "";
      asynPrint(pasynUserSelf, ASYN_TRACEIO_DEVICE,
                "%s:%s: port=%s hostname=%s -> set%sUDPMAC%s(%s)\n",
                driverName, functionName, this->portName,
                _hostname.c_str(), prefix.c_str(),
                suffix.c_str(), value.c_str());
      if (feature == SlsDetUtils::SRCUDPMAC) {
        _det->setSourceUDPMAC(mac);
      } else if (feature == SlsDetUtils::SRCUDPMAC2) {
        _det->setSourceUDPMAC2(mac);
      } else if (feature == SlsDetUtils::DESTUDPMAC) {
        _det->setDestinationUDPMAC(mac);
      } else if (feature == SlsDetUtils::DESTUDPMAC2) {
        _det->setDestinationUDPMAC2(mac);
      }
    } else {
      std::string feature_name = SlsDetUtils::FeatureName(feature);
      asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s, port=%s hostname=%s - unexpected feature %s\n",
                driverName, functionName, this->portName,
                _hostname.c_str(), feature_name.c_str());
    }
  } catch (const sls::SocketError &err) {
    std::string feature_name = SlsDetUtils::FeatureName(feature);
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
              "%s:%s, port=%s hostname=%s feature=%s - %s\n",
              driverName, functionName, this->portName,
              _hostname.c_str(), feature_name.c_str(),
              err.what());
    // rethrow connection errors to outer-handler
    throw;
  } catch (const sls::RuntimeError &err) {
    std::string feature_name = SlsDetUtils::FeatureName(feature);
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
              "%s:%s, port=%s hostname=%s feature=%s - %s\n",
              driverName, functionName, this->portName,
              _hostname.c_str(), feature_name.c_str(),
              err.what());
  }
}

void SlsDet::writeIntToDetector(SlsDetUtils::Feature feature, epicsInt32 value)
{
  static const char *functionName = "writeIntToDetector";

  try {
    if (feature == SlsDetUtils::TEMPCONTROL) {
      // write temp control enable/disable to the module
      asynPrint(pasynUserSelf, ASYN_TRACEIO_DEVICE,
                "%s:%s: port=%s hostname=%s -> setTemperatureControl(%s)\n",
                driverName, functionName, this->portName,
                _hostname.c_str(), value ? "true" : "false");
      _det->setTemperatureControl(value);
    } else if (feature == SlsDetUtils::TEMPEVENT) {
      // clear the temp event on the module
      if (value == 0) {
        asynPrint(pasynUserSelf, ASYN_TRACEIO_DEVICE,
                  "%s:%s: port=%s hostname=%s -> resetTemperatureEvent()\n",
                  driverName, functionName, this->portName,
                  _hostname.c_str());
        _det->resetTemperatureEvent();
      }
    } else if (feature == SlsDetUtils::CHIPPOWER) {
      // write chip power to module
      asynPrint(pasynUserSelf, ASYN_TRACEIO_DEVICE,
                "%s:%s: port=%s hostname=%s -> setPowerChip(%s)\n",
                driverName, functionName, this->portName,
                _hostname.c_str(), value ? "true" : "false");
      _det->setPowerChip(value);
    } else if (feature == SlsDetUtils::HIGHVOLTAGE) {
      // write high volage to module
      asynPrint(pasynUserSelf, ASYN_TRACEIO_DEVICE,
                "%s:%s: port=%s hostname=%s -> setHighVoltage(%d)\n",
                driverName, functionName, this->portName,
                _hostname.c_str(), value);
      _det->setHighVoltage(value);
    } else if (feature == SlsDetUtils::CLOCKSPEED) {
      // write clock speed to the module
      auto clock_speed = static_cast<sls::defs::speedLevel>(value);
      asynPrint(pasynUserSelf, ASYN_TRACEIO_DEVICE,
                "%s:%s: port=%s hostname=%s -> setReadoutSpeed(%s)\n",
                driverName, functionName, this->portName,
                _hostname.c_str(), sls::ToString(clock_speed).c_str());
      _det->setReadoutSpeed(clock_speed);
    } else if (feature == SlsDetUtils::GAINMODE) {
      // write gain mode to module
      auto gain_mode = static_cast<sls::defs::gainMode>(value);
      asynPrint(pasynUserSelf, ASYN_TRACEIO_DEVICE,
                "%s:%s: port=%s hostname=%s -> setGainMode(%s)\n",
                driverName, functionName, this->portName,
                _hostname.c_str(), sls::ToString(gain_mode).c_str());
      _det->setGainMode(gain_mode);
    } else if (feature == SlsDetUtils::HIGHGAIN0) {
      // write high gain 0 to module
      sls::defs::detectorSettings det_setting = value ? sls::defs::HIGHGAIN0 : sls::defs::GAIN0;
      asynPrint(pasynUserSelf, ASYN_TRACEIO_DEVICE,
                "%s:%s: port=%s hostname=%s -> setSettings(%s)\n",
                driverName, functionName, this->portName,
                _hostname.c_str(), sls::ToString(det_setting).c_str());
      _det->setSettings(det_setting);
    } else if (feature == SlsDetUtils::ADCPHASE) {
      // write adc phase to module
      asynPrint(pasynUserSelf, ASYN_TRACEIO_DEVICE,
                "%s:%s: port=%s hostname=%s -> setADCPhase(%d)\n",
                driverName, functionName, this->portName,
                _hostname.c_str(), value);
      _det->setADCPhase(value);
    } else if (feature == SlsDetUtils::TRIGMODE) {
      // write trigger mode to module
      auto trigger_mode = static_cast<sls::defs::timingMode>(value);
      asynPrint(pasynUserSelf, ASYN_TRACEIO_DEVICE,
                "%s:%s: port=%s hostname=%s -> setTimingMode(%s)\n",
                driverName, functionName, this->portName,
                _hostname.c_str(), sls::ToString(trigger_mode).c_str());
      _det->setTimingMode(trigger_mode);
    } else if (feature == SlsDetUtils::NUMINTFACE) {
      // write the number of active sfp+ slots to module
      asynPrint(pasynUserSelf, ASYN_TRACEIO_DEVICE,
                "%s:%s: port=%s hostname=%s -> setNumberofUDPInterfaces(%d)\n",
                driverName, functionName, this->portName,
                _hostname.c_str(), value);
      _det->setNumberofUDPInterfaces(value);
    } else if (feature == SlsDetUtils::ACTINTFACE) {
      // select the sfp+ slot to use (only meaningful when num sfp+ is one)
      asynPrint(pasynUserSelf, ASYN_TRACEIO_DEVICE,
                "%s:%s: port=%s hostname=%s -> selectUDPInterface(%d)\n",
                driverName, functionName, this->portName,
                _hostname.c_str(), value);
      _det->selectUDPInterface(value);
    } else if (feature == SlsDetUtils::DESTUDPPORT) {
      // write the dest udp port for first sfp
      auto port = static_cast<uint16_t>(value);
      asynPrint(pasynUserSelf, ASYN_TRACEIO_DEVICE,
                "%s:%s: port=%s hostname=%s -> setDestinationUDPPort(%u)\n",
                driverName, functionName, this->portName,
                _hostname.c_str(), port);
      _det->setDestinationUDPPort(port);
    } else if (feature == SlsDetUtils::DESTUDPPORT2) {
      // write the dest udp port for second sfp
      auto port = static_cast<uint16_t>(value);
      asynPrint(pasynUserSelf, ASYN_TRACEIO_DEVICE,
                "%s:%s: port=%s hostname=%s -> setDestinationUDPPort2(%u)\n",
                driverName, functionName, this->portName,
                _hostname.c_str(), port);
      _det->setDestinationUDPPort2(port);
    } else if (feature == SlsDetUtils::TXDELAYFRAME) {
      // write tx delay to module
      asynPrint(pasynUserSelf, ASYN_TRACEIO_DEVICE,
                "%s:%s: port=%s hostname=%s -> setTransmissionDelayFrame(%d)\n",
                driverName, functionName, this->portName,
                _hostname.c_str(), value);
      _det->setTransmissionDelayFrame(value);
    } else if (feature == SlsDetUtils::FLOWCONTROL) {
      // enable/disable flow control (a.k.a. pause frame) support
      asynPrint(pasynUserSelf, ASYN_TRACEIO_DEVICE,
                "%s:%s: port=%s hostname=%s -> setTenGigaFlowControl(%d)\n",
                driverName, functionName, this->portName,
                _hostname.c_str(), value);
      _det->setTenGigaFlowControl(value);
    } else {
      std::string feature_name = SlsDetUtils::FeatureName(feature);
      asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s, port=%s hostname=%s - unexpected feature %s\n",
                driverName, functionName, this->portName,
                _hostname.c_str(), feature_name.c_str());
    }
  } catch (const sls::SocketError &err) {
    std::string feature_name = SlsDetUtils::FeatureName(feature);
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
              "%s:%s, port=%s hostname=%s feature=%s - %s\n",
              driverName, functionName, this->portName,
              _hostname.c_str(), feature_name.c_str(),
              err.what());
    // rethrow connection errors to outer-handler
    throw;
  } catch (const sls::RuntimeError &err) {
    std::string feature_name = SlsDetUtils::FeatureName(feature);
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
              "%s:%s, port=%s hostname=%s feature=%s - %s\n",
              driverName, functionName, this->portName,
              _hostname.c_str(), feature_name.c_str(),
              err.what());
  }
}

void SlsDet::writeInt64ToDetector(SlsDetUtils::Feature feature, epicsInt64 value)
{
  static const char *functionName = "writeInt64ToDetector";

  try {
    if (feature == SlsDetUtils::NUMFRAMES) {
      // write the number of frames per trigger
      asynPrint(pasynUserSelf, ASYN_TRACEIO_DEVICE,
                "%s:%s: port=%s hostname=%s -> setNumberOfFrames(%lld)\n",
                driverName, functionName, this->portName,
                _hostname.c_str(), value);
      _det->setNumberOfFrames(value);
    } else if (feature == SlsDetUtils::NUMTRIG) {
      // write the number of triggers per acquisition
      asynPrint(pasynUserSelf, ASYN_TRACEIO_DEVICE,
                "%s:%s: port=%s hostname=%s -> setNumberOfTriggers(%lld)\n",
                driverName, functionName, this->portName,
                _hostname.c_str(), value);
      _det->setNumberOfTriggers(value);
    } else {
      std::string feature_name = SlsDetUtils::FeatureName(feature);
      asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s, port=%s hostname=%s - unexpected feature %s\n",
                driverName, functionName, this->portName,
                _hostname.c_str(), feature_name.c_str());
    }
  } catch (const sls::SocketError &err) {
    std::string feature_name = SlsDetUtils::FeatureName(feature);
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
              "%s:%s, port=%s hostname=%s feature=%s - %s\n",
              driverName, functionName, this->portName,
              _hostname.c_str(), feature_name.c_str(),
              err.what());
    // rethrow connection errors to outer-handler
    throw;
  } catch (const sls::RuntimeError &err) {
    std::string feature_name = SlsDetUtils::FeatureName(feature);
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
              "%s:%s, port=%s hostname=%s feature=%s - %s\n",
              driverName, functionName, this->portName,
              _hostname.c_str(), feature_name.c_str(),
              err.what());
  }
}

void SlsDet::writeDoubleToDetector(SlsDetUtils::Feature feature, double value)
{
  static const char *functionName = "writeDoubleToDetector";

  try {
    if (feature == SlsDetUtils::THRESHOLD) {
      // wrrite temp thershold to the module
      auto temp_threshold = static_cast<int>(value);
      asynPrint(pasynUserSelf, ASYN_TRACEIO_DEVICE,
                "%s:%s: port=%s hostname=%s -> setThresholdTemperature(%d)\n",
                driverName, functionName, this->portName,
                _hostname.c_str(), temp_threshold);
      _det->setThresholdTemperature(temp_threshold);
    } else if (feature == SlsDetUtils::EXPTIME) {
      // write exptime to the module
      std::chrono::nanoseconds exptime_ns = secs_to_ns(value);
      asynPrint(pasynUserSelf, ASYN_TRACEIO_DEVICE,
                "%s:%s: port=%s hostname=%s -> setExptime(%lu)\n",
                driverName, functionName, this->portName,
                _hostname.c_str(), exptime_ns.count());
      _det->setExptime(exptime_ns);
    } else if (feature == SlsDetUtils::PERIOD) {
      // write exp period to the module
      std::chrono::nanoseconds period_ns = secs_to_ns(value);
      asynPrint(pasynUserSelf, ASYN_TRACEIO_DEVICE,
                "%s:%s: port=%s hostname=%s -> setPeriod(%lu)\n",
                driverName, functionName, this->portName,
                    _hostname.c_str(), period_ns.count());
      _det->setPeriod(period_ns);
    } else if (feature == SlsDetUtils::DELAY) {
      // write trigger delay to the module
      std::chrono::nanoseconds delay_ns = secs_to_ns(value);
      asynPrint(pasynUserSelf, ASYN_TRACEIO_DEVICE,
                "%s:%s: port=%s hostname=%s -> setDelayAfterTrigger(%lu)\n",
                driverName, functionName, this->portName,
                _hostname.c_str(), delay_ns.count());
      _det->setDelayAfterTrigger(delay_ns);
    } else {
      std::string feature_name = SlsDetUtils::FeatureName(feature);
      asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s, port=%s hostname=%s - unexpected feature %s\n",
                driverName, functionName, this->portName,
                _hostname.c_str(), feature_name.c_str());
    }
  } catch (const sls::SocketError &err) {
    std::string feature_name = SlsDetUtils::FeatureName(feature);
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
              "%s:%s, port=%s hostname=%s feature=%s - %s\n",
              driverName, functionName, this->portName,
              _hostname.c_str(), feature_name.c_str(),
              err.what());
    // rethrow connection errors to outer-handler
    throw;
  } catch (const sls::RuntimeError &err) {
    std::string feature_name = SlsDetUtils::FeatureName(feature);
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
              "%s:%s, port=%s hostname=%s feature=%s - %s\n",
              driverName, functionName, this->portName,
              _hostname.c_str(), feature_name.c_str(),
              err.what());
  }
}

bool SlsDet::readStringFromDetector(SlsDetUtils::Feature feature, std::string& value)
{
  bool status = true;
  static const char *functionName = "readStringFromDetector";

  try {
    if (feature == SlsDetUtils::HOSTNAME) {
      value = _hostname;
    } else if (feature == SlsDetUtils::SERIALNUM) {
      value = hexString(_det->getSerialNumber().squash());
    } else if (feature == SlsDetUtils::FIRMWARE) {
      value = hexString(_det->getFirmwareVersion().squash());
    } else if (feature == SlsDetUtils::SOFTWARE) {
      value = _det->getDetectorServerVersion().squash();
    } else if (feature == SlsDetUtils::HARDWARE) {
      value = _det->getHardwareVersion().squash();
    } else if (feature == SlsDetUtils::KERNELVER) {
      value = _det->getKernelVersion().squash();
    } else if ((feature == SlsDetUtils::SRCUDPIP)  || (feature == SlsDetUtils::SRCUDPIP2) ||
               (feature == SlsDetUtils::DESTUDPIP) || (feature == SlsDetUtils::DESTUDPIP2)) {
      sls::IpAddr ip;
      if (feature == SlsDetUtils::SRCUDPIP) {
        ip = _det->getSourceUDPIP().squash();
      } else if (feature == SlsDetUtils::SRCUDPIP2) {
        ip = _det->getSourceUDPIP2().squash();
      } else if (feature == SlsDetUtils::DESTUDPIP) {
        ip = _det->getDestinationUDPIP().squash();
      } else if (feature == SlsDetUtils::DESTUDPIP2) {
        ip = _det->getDestinationUDPIP2().squash();
      }
      value = ip.str();
    } else if ((feature == SlsDetUtils::SRCUDPMAC)  || (feature == SlsDetUtils::SRCUDPMAC2) ||
               (feature == SlsDetUtils::DESTUDPMAC) || (feature == SlsDetUtils::DESTUDPMAC2)) {
      sls::MacAddr mac;
      if (feature == SlsDetUtils::SRCUDPMAC) {
        mac = _det->getSourceUDPMAC().squash();
      } else if (feature == SlsDetUtils::SRCUDPMAC2) {
        mac = _det->getSourceUDPMAC2().squash();
      } else if (feature == SlsDetUtils::DESTUDPMAC) {
        mac = _det->getDestinationUDPMAC().squash();
      } else if (feature == SlsDetUtils::DESTUDPMAC2) {
        mac = _det->getDestinationUDPMAC2().squash();
      }
      value = mac.str();
    } else {
      std::string feature_name = SlsDetUtils::FeatureName(feature);
      asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s, port=%s hostname=%s - unexpected feature %s\n",
                driverName, functionName, this->portName,
                _hostname.c_str(), feature_name.c_str());
      status = false;
    }
  } catch (const sls::SocketError &err) {
    std::string feature_name = SlsDetUtils::FeatureName(feature);
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
              "%s:%s, port=%s hostname=%s feature=%s - %s\n",
              driverName, functionName, this->portName,
              _hostname.c_str(), feature_name.c_str(),
              err.what());
    // rethrow connection errors to outer-handler
    throw;
  } catch (const sls::RuntimeError &err) {
    std::string feature_name = SlsDetUtils::FeatureName(feature);
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
              "%s:%s, port=%s hostname=%s feature=%s - %s\n",
              driverName, functionName, this->portName,
              _hostname.c_str(), feature_name.c_str(),
              err.what());
    status = false;
  }

  return status;
}

bool SlsDet::readIntFromDetector(SlsDetUtils::Feature feature, epicsInt32& value)
{
  bool status = true;
  static const char *functionName = "readIntFromDetector";

  try {
    if (feature == SlsDetUtils::DETTYPE) {
      value = _det->getDetectorType().squash();
    } else if (feature == SlsDetUtils::MODULEID) {
      value = _det->getModuleId().squash();
    } else if (feature == SlsDetUtils::DETSTATUS) {
      value = _det->getDetectorStatus().squash();
    } else if (feature == SlsDetUtils::TEMPCONTROL) {
      value = _det->getTemperatureControl().squash();
    } else if (feature == SlsDetUtils::TEMPEVENT) {
      value = _det->getTemperatureEvent().squash();
    } else if (feature == SlsDetUtils::CHIPPOWER) {
      value = _det->getPowerChip().squash();
    } else if (feature == SlsDetUtils::HIGHVOLTAGE) {
      value = _det->getHighVoltage().squash();
    } else if (feature == SlsDetUtils::CLOCKSPEED) {
      value = _det->getReadoutSpeed().squash();
    } else if (feature == SlsDetUtils::GAINMODE) {
      value = _det->getGainMode().squash();
    } else if (feature == SlsDetUtils::HIGHGAIN0) {
      value = _det->getSettings().squash() == sls::defs::HIGHGAIN0;
    } else if (feature == SlsDetUtils::ADCPHASE) {
      value = _det->getADCPhase().squash();
    } else if (feature == SlsDetUtils::TRIGMODE) {
      value = _det->getTimingMode().squash();
    } else if (feature == SlsDetUtils::NUMFRAMES) {
      value = _det->getNumberOfFrames().squash();
    } else if (feature == SlsDetUtils::NUMTRIG) {
      value = _det->getNumberOfTriggers().squash();
    } else if (feature == SlsDetUtils::NUMINTFACE) {
      value = _det->getNumberofUDPInterfaces().squash();
    } else if (feature == SlsDetUtils::ACTINTFACE) {
      value = _det->getSelectedUDPInterface().squash();
    } else if (feature == SlsDetUtils::DESTUDPPORT) {
      value = _det->getDestinationUDPPort().squash();
    } else if (feature == SlsDetUtils::DESTUDPPORT2) {
      value = _det->getDestinationUDPPort2().squash();
    } else if (feature == SlsDetUtils::TXDELAYFRAME) {
      value = _det->getTransmissionDelayFrame().squash();
    } else if (feature == SlsDetUtils::FLOWCONTROL) {
      value = _det->getTenGigaFlowControl().squash();
    } else {
      asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s, port=%s hostname=%s - unexpected feature %s\n",
                driverName, functionName, this->portName,
                _hostname.c_str(), SlsDetUtils::FeatureName(feature).c_str());
      status = false;
    }
  } catch (const sls::SocketError &err) {
    std::string feature_name = SlsDetUtils::FeatureName(feature);
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
              "%s:%s, port=%s hostname=%s feature=%s - %s\n",
              driverName, functionName, this->portName,
              _hostname.c_str(), feature_name.c_str(),
              err.what());
    // rethrow connection errors to outer-handler
    throw;
  } catch (const sls::RuntimeError &err) {
    std::string feature_name = SlsDetUtils::FeatureName(feature);
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
              "%s:%s, port=%s hostname=%s feature=%s - %s\n",
              driverName, functionName, this->portName,
              _hostname.c_str(), feature_name.c_str(),
              err.what());
    status = false;
  }

  return status;
}

bool SlsDet::readInt64FromDetector(SlsDetUtils::Feature feature, epicsInt64& value)
{
  bool status = true;
  static const char *functionName = "readInt64FromDetector";

  try {
    if (feature == SlsDetUtils::NEXTFRAME) {
      value = _det->getNextFrameNumber().squash();
    } else if (feature == SlsDetUtils::NUMFRAMES) {
      value = _det->getNumberOfFrames().squash();
    } else if (feature == SlsDetUtils::NUMTRIG) {
      value = _det->getNumberOfTriggers().squash();
    } else {
      asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s, port=%s hostname=%s - unexpected feature %s\n",
                driverName, functionName, this->portName,
                _hostname.c_str(), SlsDetUtils::FeatureName(feature).c_str());
      status = false;
    }
  } catch (const sls::SocketError &err) {
    std::string feature_name = SlsDetUtils::FeatureName(feature);
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
              "%s:%s, port=%s hostname=%s feature=%s - %s\n",
              driverName, functionName, this->portName,
              _hostname.c_str(), feature_name.c_str(),
              err.what());
    // rethrow connection errors to outer-handler
    throw;
  } catch (const sls::RuntimeError &err) {
    std::string feature_name = SlsDetUtils::FeatureName(feature);
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
              "%s:%s, port=%s hostname=%s feature=%s - %s\n",
              driverName, functionName, this->portName,
              _hostname.c_str(), feature_name.c_str(),
              err.what());
    status = false;
  }

  return status;
}

bool SlsDet::readDoubleFromDetector(SlsDetUtils::Feature feature, double& value)
{
  bool status = true;
  static const char *functionName = "readDoubleFromDetector";

  try {
    if (feature == SlsDetUtils::CHIPVER) {
      value = _det->getChipVersion().squash();
    } else if (feature == SlsDetUtils::THRESHOLD) {
      value = _det->getThresholdTemperature().squash();
    } else if (feature == SlsDetUtils::EXPTIME) {
      value = ns_to_secs(_det->getExptime().squash());
    } else if (feature == SlsDetUtils::PERIOD) {
      value = ns_to_secs(_det->getPeriod().squash());
    } else if (feature == SlsDetUtils::DELAY) {
      value = ns_to_secs(_det->getDelayAfterTrigger().squash());
    } else if (feature == SlsDetUtils::FPGATEMP) {
      value = _det->getTemperature(sls::defs::TEMPERATURE_FPGA).squash();
    } else if (feature == SlsDetUtils::ADCTEMP) {
      value = _det->getTemperature(sls::defs::TEMPERATURE_ADC).squash();
    } else {
      asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s, port=%s hostname=%s - unexpected feature %s\n",
                driverName, functionName, this->portName,
                _hostname.c_str(), SlsDetUtils::FeatureName(feature).c_str());
      status = false;
    }
  } catch (const sls::SocketError &err) {
    std::string feature_name = SlsDetUtils::FeatureName(feature);
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
              "%s:%s, port=%s hostname=%s feature=%s - %s\n",
              driverName, functionName, this->portName,
              _hostname.c_str(), feature_name.c_str(),
              err.what());
    // rethrow connection errors to outer-handler
    throw;
  } catch (const sls::RuntimeError &err) {
    std::string feature_name = SlsDetUtils::FeatureName(feature);
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
              "%s:%s, port=%s hostname=%s feature=%s - %s\n",
              driverName, functionName, this->portName,
              _hostname.c_str(), feature_name.c_str(),
              err.what());
    status = false;
  }

  return status;
}


bool SlsDet::isEnabled() {
  int enabled;
  if (getIntegerParam(_enabledParam, &enabled) == asynSuccess) {
    return enabled == ON;
  } else {
    return false;
  }
}

bool SlsDet::isConnected() {
  int conn;
  if (getIntegerParam(_connStatusParam, &conn) == asynSuccess) {
    return conn == CONNECTED;
  } else {
    return false;
  }
}

void SlsDet::updateEnums()
{
  /* Update all the enums in the SlsDetEnums list */
  for (unsigned nEnum=0; nEnum<SlsDetEnumsSize; nEnum++) {
    int reason;
    if (findParam(SlsDetEnums[nEnum].name, &reason) != asynSuccess) continue;
    const SlsDetEnumInfo* elem = SlsDetEnums[nEnum].enums;
    int nElem;
    for (nElem=0; (nElem<(int)SlsDetEnums[nEnum].size) && (nElem<SLS_MAX_ENUMS); ++nElem) {
        std::strncpy(_enumStrings[nElem], elem[nElem].name.c_str(), MAX_ENUM_STRING_SIZE)[MAX_ENUM_STRING_SIZE] = '\0';
        _enumValues[nElem] = elem[nElem].value;
        _enumSeverities[nElem] = elem[nElem].severity;
    }
    doCallbacksEnum(_enumStrings, _enumValues, _enumSeverities, nElem, reason, 0);
  }
}

bool SlsDet::connectDetector()
{
  bool status = true;
  static const char *functionName = "connectDetector";

  asynPrint(pasynUserSelf, ASYN_TRACE_FLOW,
            "%s:%s, port=%s attempting to connect detector: %s\n",
            driverName, functionName, this->portName, _hostname.c_str());

  try {
    std::vector<std::string> hostnames {_hostname};
    try {
      _det->setHostname(hostnames);
    } catch (const sls::RuntimeError &err) {
      auto detstat = _det->getDetectorStatus();
      if (detstat.any(sls::defs::RUNNING) || detstat.any(sls::defs::WAITING)) {
        _det->stopDetector();
        _det->setHostname(hostnames);
      } else {
        // if detector wasn't running or stop fails re-raise to outer handler
        throw;
      }
    }
  } catch (const sls::RuntimeError &err) {
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
              "%s:%s, port=%s failed to connect to detector: %s - %s\n",
              driverName, functionName, this->portName, _hostname.c_str(), err.what());
    status = false;
  }

  return status;
}

asynStatus SlsDet::writeFloat64(asynUser *pasynUser, epicsFloat64 value)
{
  const char* name = NULL;
  int function = pasynUser->reason;
  asynStatus status = asynSuccess;
  static const char *functionName = "writeFloat64";

  getParamName(function, &name);
  if (name) {
    asynPrint(pasynUser, ASYN_TRACEIO_DEVICE,
              "%s:%s: port=%s received write request (%g) for parameter: %s\n",
              driverName, functionName, this->portName, value, name);
  } else {
    asynPrint(pasynUser, ASYN_TRACE_ERROR,
              "%s:%s: port=%s received write request for parameter with no name\n",
              driverName, functionName, this->portName);
    return asynError;
  }

  /* Does not allow writes if module is not connected */
  if (!isConnected()) {
    asynPrint(pasynUser, ASYN_TRACE_ERROR,
              "%s:%s: error, status=%d param=%s, value=%f: can't write to detector is not connected!\n",
              driverName, functionName, status, name, value);
    return asynError;
  }

  /* Save backup of old value and set the parameter */
  epicsFloat64 oldValue;
  getDoubleParam(function, &oldValue);
  status = setDoubleParam(function, value);

  /* Search the parameter map for matching parameters */
  auto it = _doubleFeatures.find(function);
  if (it != _doubleFeatures.end()) {
    if (it->second.isWritable()) {
      it->second.setRequested();
    } else {
      status = asynError;
    }
  } else { // Other functions we call the base class method
    status = asynPortDriver::writeFloat64(pasynUser, value);
  }

  if (status != asynSuccess) {
    asynPrint(pasynUser, ASYN_TRACE_ERROR,
              "%s:%s: error, status=%d param=%s, value=%f: param not writable!\n",
              driverName, functionName, status, name, value);
    setDoubleParam(function, oldValue);
  }

  /* Do callbacks so higher layers see any changes */
  callParamCallbacks();

  /* Send a signal to the poller task which will make it do a poll, and switch to the fast poll rate */
  epicsEventSignal(statusEvent);

  return status;
}

asynStatus SlsDet::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
  const char* name = NULL;
  int function = pasynUser->reason;
  asynStatus status = asynSuccess;
  static const char *functionName = "writeInt32";

  getParamName(function, &name);
  if (name) {
    asynPrint(pasynUser, ASYN_TRACEIO_DEVICE,
              "%s:%s: port=%s received write request (%d) for parameter: %s\n",
              driverName, functionName, this->portName, value, name);
  } else {
    asynPrint(pasynUser, ASYN_TRACE_ERROR,
              "%s:%s: port=%s received write request for parameter with no name\n",
              driverName, functionName, this->portName);
    return asynError;
  }

  /* Only allow writes to the enable parameter if module is not connected */
  if (!isConnected() && (function != _enabledParam)) {
    asynPrint(pasynUser, ASYN_TRACE_ERROR,
              "%s:%s: error, status=%d param=%s, value=%d: can't write to detector is not connected!\n",
              driverName, functionName, status, name, value);
    return asynError;
  }

  /* Save backup of old value and set the parameter */
  epicsInt32 oldValue;
  getIntegerParam(function, &oldValue);
  status = setIntegerParam(function, value);

  /* Search the parameter map for matching parameters */
  auto it = _intFeatures.find(function);
  if (it != _intFeatures.end()) {
    if (it->second.isWritable()) {
      it->second.setRequested();
    } else {
      status = asynError;
    }
  } else { // Other functions we call the base class method
    status = asynPortDriver::writeInt32(pasynUser, value);
  }

  if (status != asynSuccess) {
    asynPrint(pasynUser, ASYN_TRACE_ERROR,
              "%s:%s: error, status=%d param=%s, value=%d: param not writable!\n",
              driverName, functionName, status, name, value);
    setIntegerParam(function, oldValue);
  }

  /* Do callbacks so higher layers see any changes */
  callParamCallbacks();

  /* Send a signal to the poller task which will make it do a poll, and switch to the fast poll rate */
  epicsEventSignal(statusEvent);

  return status;
}

asynStatus SlsDet::writeInt64(asynUser *pasynUser, epicsInt64 value)
{
  const char* name = NULL;
  int function = pasynUser->reason;
  asynStatus status = asynSuccess;
  static const char *functionName = "writeInt64";

  getParamName(function, &name);
  if (name) {
    asynPrint(pasynUser, ASYN_TRACEIO_DEVICE,
              "%s:%s: port=%s received write request (%lld) for parameter: %s\n",
              driverName, functionName, this->portName, value, name);
  } else {
    asynPrint(pasynUser, ASYN_TRACE_ERROR,
              "%s:%s: port=%s received write request for parameter with no name\n",
              driverName, functionName, this->portName);
    return asynError;
  }

  /* Only allow writes to the enable parameter if module is not connected */
  if (!isConnected() && (function != _enabledParam)) {
    asynPrint(pasynUser, ASYN_TRACE_ERROR,
              "%s:%s: error, status=%d param=%s, value=%lld: can't write to detector is not connected!\n",
              driverName, functionName, status, name, value);
    return asynError;
  }

  /* Save backup of old value and set the parameter */
  epicsInt64 oldValue;
  getInteger64Param(function, &oldValue);
  status = setInteger64Param(function, value);

  /* Search the parameter map for matching parameters */
  auto it = _int64Features.find(function);
  if (it != _int64Features.end()) {
    if (it->second.isWritable()) {
      it->second.setRequested();
    } else {
      status = asynError;
    }
  } else { // Other functions we call the base class method
    status = asynPortDriver::writeInt64(pasynUser, value);
  }

  if (status != asynSuccess) {
    asynPrint(pasynUser, ASYN_TRACE_ERROR,
              "%s:%s: error, status=%d param=%s, value=%lld: param not writable!\n",
              driverName, functionName, status, name, value);
    setInteger64Param(function, oldValue);
  }

  /* Do callbacks so higher layers see any changes */
  callParamCallbacks();

  /* Send a signal to the poller task which will make it do a poll, and switch to the fast poll rate */
  epicsEventSignal(statusEvent);

  return status;
}

asynStatus SlsDet::writeOctet(asynUser *pasynUser, const char *value, size_t nChars, size_t *nActual)
{
  const char* name = NULL;
  int function = pasynUser->reason;
  asynStatus status = asynSuccess;
  static const char *functionName = "writeOctet";

  getParamName(function, &name);
  if (name) {
    asynPrint(pasynUser, ASYN_TRACEIO_DEVICE,
              "%s:%s: port=%s received write request (%s) for parameter: %s\n",
              driverName, functionName, this->portName, value, name);
  } else {
    asynPrint(pasynUser, ASYN_TRACE_ERROR,
              "%s:%s: port=%s received write request for parameter with no name\n",
              driverName, functionName, this->portName);
    return asynError;
  }

  /* Save backup of old value and set the parameter */
  std::string oldValue;
  getStringParam(function, oldValue);
  status = setStringParam(function, value);
  *nActual = nChars;

  /* Search the parameter map for matching parameters */
  auto it = _stringFeatures.find(function);
  if (it != _stringFeatures.end()) {
    if (it->second.isWritable()) {
      it->second.setRequested();
    } else {
      status = asynError;
    }
  } else { // Other functions we call the base class method
    status = asynPortDriver::writeOctet(pasynUser, value, nChars, nActual);
  }

  if (status != asynSuccess) {
    asynPrint(pasynUser, ASYN_TRACE_ERROR,
              "%s:%s: error, status=%d param=%s, value=%s: param not writable!\n",
              driverName, functionName, status, name, value);
    setStringParam(function, oldValue);
  }

  /* Do callbacks so higher layers see any changes */
  callParamCallbacks();

  /* Send a signal to the poller task which will make it do a poll, and switch to the fast poll rate */
  epicsEventSignal(statusEvent);

  return status;
}

asynStatus SlsDet::readEnum(asynUser *pasynUser, char *strings[], int values[],
                            int severities[], size_t nElements, size_t *nIn)
{
  const char* name = NULL;
  int function = pasynUser->reason;
  size_t matched_size = 0;
  asynStatus status = asynSuccess;
  const SlsDetEnumInfo *matched_enums = NULL;
  static const char *functionName = "readEnum";

  getParamName(function, &name);
  if (name) {
    asynPrint(pasynUser, ASYN_TRACEIO_DEVICE,
              "%s:%s: port=%s received enum read request for parameter: %s\n",
               driverName, functionName, this->portName, name);
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
              "%s:%s: port=%s enum found for parameter: %s\n",
               driverName, functionName, this->portName, name);
      size_t i;
      for (i = 0; ((i < matched_size) && (i < nElements)); ++i) {
        if (strings[i]) std::free(strings[i]);
        strings[i] = epicsStrDup(matched_enums[i].name.c_str());
        values[i] = matched_enums[i].value;
        severities[i] = matched_enums[i].severity;
      }
      *nIn = i;
    } else {
      asynPrint(pasynUser, ASYN_TRACEIO_DEVICE,
              "%s:%s: port=%s no enum found for parameter: %s\n",
               driverName, functionName, this->portName, name);
      *nIn = 0;
      status = asynError;
    }
  } else {
    asynPrint(pasynUser, ASYN_TRACE_ERROR,
              "%s:%s: port=%s received enum read request for parameter with no name\n",
               driverName, functionName, this->portName);
    *nIn = 0;
    status = asynError;
  }

  return status;
}

void SlsDet::statusTask(void)
{
  unsigned int status = 0;
  bool last = false;
  bool enabled = false;
  bool connected = false;
  bool init = false;
  double timeout = 0.0;
  unsigned int forcedFastPolls = 0;
  int heartbeat = 0;
  static const char *functionName = "statusTask";

  asynPrint(pasynUserSelf, ASYN_TRACE_FLOW,
            "%s:%s: Status thread starting ...\n",
            driverName, functionName);

  while(!_exiting) {
    this->lock();
    if (forcedFastPolls > 0) {
      timeout = _fastPollingPeriod;
      forcedFastPolls--;
    } else {
      timeout = _pollingPeriod;
    }
    this->unlock();

    if (timeout != 0.0) {
      status = epicsEventWaitWithTimeout(statusEvent, timeout);
    } else {
      status = epicsEventWait(statusEvent);
    }
    if (status == epicsEventWaitOK) {
      asynPrint(pasynUserSelf, ASYN_TRACE_FLOW,
        "%s:%s: Got status event\n",
        driverName, functionName);
      // We got an event, rather than a timeout.  This is because other software
      // knows that data has arrived, or device should have changed state (parameters changed, etc.).
      // Force a minimum number of fast polls, because the device status
      // might not have changed in the first few polls
      forcedFastPolls = 5;
    }

    if (_exiting) break;

    // check if we are enabled and connected
    last = enabled;
    this->lock();
    enabled = isEnabled();
    connected = isConnected();
    init = !connected;
    // Look for requested writes in the string features
    for (auto& kv : _stringFeatures) {
      // clear old pending flags
      kv.second.clearPending();
      // if the feature is not writable then ignore it
      if (!kv.second.isWritable()) {
        continue;
      }

      if (kv.second.isRequested()) {
        std::string temp_val;
        // clear the requested write flag
        kv.second.clearRequested();
        // read the value from parameter db
        getStringParam(kv.first, temp_val);
        // cache value and set pending module write flag
        kv.second.write(temp_val);
        kv.second.setPending();
      }
    }
    // Look for requested writes in the int features
    for (auto& kv : _intFeatures) {
      // clear old pending flags
      kv.second.clearPending();
      // if the feature is not writable then ignore it
      if (!kv.second.isWritable()) {
        continue;
      }

      if (kv.second.isRequested()) {
        epicsInt32 temp_val;
        // clear the requested write flag
        kv.second.clearRequested();
        // read the value from parameter db
        getIntegerParam(kv.first, &temp_val);
        // cache value and set pending module write flag
        kv.second.write(temp_val);
        kv.second.setPending();
      }
    }
    // Look for requested writes in the int64 features
    for (auto& kv : _int64Features) {
      // clear old pending flags
      kv.second.clearPending();
      // if the feature is not writable then ignore it
      if (!kv.second.isWritable()) {
        continue;
      }

      if (kv.second.isRequested()) {
        epicsInt64 temp_val;
        // clear the requested write flag
        kv.second.clearRequested();
        // read the value from parameter db
        getInteger64Param(kv.first, &temp_val);
        // cache value and set pending module write flag
        kv.second.write(temp_val);
        kv.second.setPending();
      }
    }
    // Look for requested writes in the double features
    for (auto& kv : _doubleFeatures) {
      // clear old pending flags
      kv.second.clearPending();
      // if the feature is not writable then ignore it
      if (!kv.second.isWritable()) {
        continue;
      }

      if (kv.second.isRequested()) {
        double temp_val;
        // clear the requested write flag
        kv.second.clearRequested();
        // read the value from parameter db
        getDoubleParam(kv.first, &temp_val);
        // cache value and set pending module write flag
        kv.second.write(temp_val);
        kv.second.setPending();
      }
    }
    this->unlock();

    if (!enabled) continue;

    // if enabled and not connected try to connect
    if (init) {
      asynPrint(pasynUserSelf, ASYN_TRACE_FLOW,
                "%s:%s, port=%s hostname=%s - attempting to connect\n",
                driverName, functionName, this->portName, _hostname.c_str());
      do {
        epicsThreadSleep(_connPollingPeriod);
        connected = connectDetector();
      } while(!connected);
      this->lock();
      // do extra initialization on connect
      updateEnums();
      setIntegerParam(_connStatusParam, CONNECTED);
      /* Call the callbacks to update any changes */
      callParamCallbacks();
      this->unlock();
      asynPrint(pasynUserSelf, ASYN_TRACE_FLOW,
                "%s:%s, port=%s hostname=%s - connected\n",
                driverName, functionName, this->portName, _hostname.c_str());
    } else if (!last) {
      asynPrint(pasynUserSelf, ASYN_TRACE_FLOW,
                "%s:%s, port=%s hostname=%s - re-enabling\n",
                driverName, functionName, this->portName, _hostname.c_str());
      // wait on re-enable before talking to modules
      epicsThreadSleep(_connPollingPeriod);
    }

    try {
      // Do the i/o to the module for the string features
      for (auto& kv : _stringFeatures) {
        if (kv.second.isWritable()) {
          if (kv.second.isPending()) {
            writeStringToDetector(kv.second.feature(), kv.second.read());
          }
        } else if (init || !kv.second.isInitOnly()) {
          std::string temp_val;
          if (readStringFromDetector(kv.second.feature(), temp_val)) {
            kv.second.write(temp_val);
            kv.second.setPending();
          }
        }
      }
      // Do the i/o to the module for the int features
      for (auto& kv : _intFeatures) {
        if (kv.second.isWritable()) {
          if (kv.second.isPending()) {
            writeIntToDetector(kv.second.feature(), kv.second.read());
          }
        } else if (init || !kv.second.isInitOnly()) {
          epicsInt32 temp_val;
          if (readIntFromDetector(kv.second.feature(), temp_val)) {
            kv.second.write(temp_val);
            kv.second.setPending();
          }
        }
      }
      // Do the i/o to the module for the int64 features
      for (auto& kv : _int64Features) {
        if (kv.second.isWritable()) {
          if (kv.second.isPending()) {
            writeInt64ToDetector(kv.second.feature(), kv.second.read());
          }
        } else if (init || !kv.second.isInitOnly()) {
          epicsInt64 temp_val;
          if (readInt64FromDetector(kv.second.feature(), temp_val)) {
            kv.second.write(temp_val);
            kv.second.setPending();
          }
        }
      }
      // Do the i/o to the module for the double features
      for (auto& kv : _doubleFeatures) {
        if (kv.second.isWritable()) {
          if (kv.second.isPending()) {
            writeDoubleToDetector(kv.second.feature(), kv.second.read());
          }
        } else if (init || !kv.second.isInitOnly()) {
          double temp_val;
          if (readDoubleFromDetector(kv.second.feature(), temp_val)) {
            kv.second.write(temp_val);
            kv.second.setPending();
          }
        }
      }
    } catch (const sls::RuntimeError &err) {
      asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s, port=%s hostname=%s - %s\n",
                driverName, functionName, this->portName, _hostname.c_str(), err.what());

      // if the connection failed don't spam retry too fast
      enabled = false;
      continue;
    }

    this->lock();
    // Loop over the string features
    for (const auto& kv : _stringFeatures) {
      /* if the feature is writable then ignore it.
       * Also only do init only ones on connect */
      if (kv.second.isWritable() || (kv.second.isInitOnly() && !init)) {
        continue;
      }

      // pending flag is set on successful read from module
      if (kv.second.isPending()) {
        setStringParam(kv.first, kv.second.read());
      }
    }
    // Loop over the integer features
    for (const auto& kv : _intFeatures) {
      /* if the feature is writable then ignore it.
       * Also only do init only ones on connect */
      if (kv.second.isWritable() || (kv.second.isInitOnly() && !init)) {
        continue;
      }

      // pending flag is set on successful read from module
      if (kv.second.isPending()) {
        setIntegerParam(kv.first, kv.second.read());
      }
    }
    // Loop over the integer features
    for (const auto& kv : _int64Features) {
      /* if the feature is writable then ignore it.
       * Also only do init only ones on connect */
      if (kv.second.isWritable() || (kv.second.isInitOnly() && !init)) {
        continue;
      }

      // pending flag is set on successful read from module
      if (kv.second.isPending()) {
        setInteger64Param(kv.first, kv.second.read());
      }
    }
    //Loop over the double features
    for (const auto& kv : _doubleFeatures) {
      /* if the feature is writable then ignore it.
       * Also only do init only ones on connect */
      if (kv.second.isWritable() || (kv.second.isInitOnly() && !init)) {
        continue;
      }

      // pending flag is set on successful read from module
      if (kv.second.isPending()) {
        setDoubleParam(kv.first, kv.second.read());
      }
    }

    /* Finally update the heartbeat counter */
    setIntegerParam(_heartbeatParam, ++heartbeat);

    /* Call the callbacks to update any changes */
    callParamCallbacks();
    this->unlock();
  } // End of loop

  asynPrint(pasynUserSelf, ASYN_TRACE_FLOW,
            "%s:%s: Status thread exiting ...\n",
            driverName, functionName);

  _exited++;
}

/** Configuration command, called directly or from iocsh */
extern "C" int SlsDetConfigure(const char *portName, const char *hostName, int detid, int modid, double boot)
{
  sls::freeSharedMemory((detid<<MOD_ID_BITS) | modid);
  new SlsDet(portName, hostName, (detid<<MOD_ID_BITS) | modid, boot);
  return(asynSuccess);
}


static const iocshArg configArg0 = { "Port name",         iocshArgString};
static const iocshArg configArg1 = { "Detector Hostname", iocshArgString};
static const iocshArg configArg2 = { "Detector Id",       iocshArgInt};
static const iocshArg configArg3 = { "Module Id",         iocshArgInt};
static const iocshArg configArg4 = { "Boot Time",         iocshArgDouble};
static const iocshArg * const configArgs[] = {&configArg0,
                                              &configArg1,
                                              &configArg2,
                                              &configArg3,
                                              &configArg4};
static const iocshFuncDef configFuncDef = {"SlsDetConfigure", 5, configArgs};
static void configCallFunc(const iocshArgBuf *args)
{
  SlsDetConfigure(args[0].sval, args[1].sval, args[2].ival, args[3].ival, args[4].dval);
}

void drvSlsDetRegister(void)
{
  iocshRegister(&configFuncDef,configCallFunc);
}

extern "C" {
epicsExportRegistrar(drvSlsDetRegister);
}

#undef MAX_ENUM_STRING_SIZE
#undef MOD_ID_BITS
#undef sizeofArray
