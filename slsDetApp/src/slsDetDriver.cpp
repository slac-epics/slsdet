#include "slsDetDriver.h"

#include <epicsString.h>
#include <multiSlsDetector.h>

#define MAX_MQ_CAPACITY 4
#define DET_POS 0
#define MAX_DETS 1
#define DEFAULT_POLL_TIME 0.250
#define THREAD_TMO 2.0
#define TEMP_UNITS 1000.

static const char *driverName = "SlsDetDriver";

SlsDetDriver::SlsDetDriver(const std::string &hostName, const int id,
                           const char* portName, const int addr) :
  _pasynUser(pasynManager->createAsynUser(0,0)),
  _running(true),
  _pending(0),
  _cacheStr(NULL),
  _id(id),
  _addr(addr),
  _pos(DET_POS),
  _maxDets(MAX_DETS),
  _pollTime(DEFAULT_POLL_TIME),
  _portName(portName),
  _hostname(hostName),
  _thread(*this, hostName.c_str(), epicsThreadGetStackSize(epicsThreadStackMedium), epicsThreadPriorityMedium),
  _request(MAX_MQ_CAPACITY, sizeof(SlsDetMessage)),
  _reply(MAX_MQ_CAPACITY, sizeof(SlsDetMessage)),
  _det(NULL)
{
  pasynManager->connectDevice(_pasynUser, _portName, _addr);
  /* Create asynUser for debugging */
  _thread.start();
}

SlsDetDriver::~SlsDetDriver()
{
  /* Try to cleanup the reader thread... */
  _running = false;
  SlsDetMessage msg(SlsDetMessage::Exit);
  if ((stop() < 0) || (_pending > 0)) {
    _thread.exitWait(THREAD_TMO);
  } else {
    _thread.exitWait();
  }
  /* Shutdown the slsDetector interface */
  shutdown();
  /* Cleanup the asyn user */
  pasynManager->disconnect(_pasynUser);
  pasynManager->freeAsynUser(_pasynUser);
}

SlsDetMessage SlsDetDriver::checkOnline()
{
  std::string offline;
  SlsDetMessage rep(SlsDetMessage::Error);
  static const char *functionName = "checkOnline";

  if (_det) {
    asynPrint(_pasynUser, ASYN_TRACEIO_DRIVER,
              "%s:%s, port=%s, address=%d calling checkOnline\n",
              driverName, functionName, _portName, _addr);
    offline = _det->checkOnline();
    if (offline.empty()) {
      rep = SlsDetMessage(SlsDetMessage::Ok);
    }
    asynPrint(_pasynUser, ASYN_TRACEIO_DRIVER,
              "%s:%s, port=%s, address=%d checkOnline returned: %s\n",
              driverName, functionName, _portName, _addr, offline.c_str());
  }

  return rep;
}

SlsDetMessage SlsDetDriver::getHostname()
{
  int crit;
  std::string hostname;
  SlsDetMessage rep(SlsDetMessage::Error);
  static const char *functionName = "getHostname";

  if (_det) {
    asynPrint(_pasynUser, ASYN_TRACEIO_DRIVER,
              "%s:%s, port=%s, address=%d calling getHostname\n",
              driverName, functionName, _portName, _addr);
    hostname = _det->getHostname(_pos);
    if (!_det->getErrorMask()) {
    asynPrint(_pasynUser, ASYN_TRACEIO_DRIVER,
              "%s:%s, port=%s, address=%d getHostname returned: %s\n",
              driverName, functionName, _portName, _addr, hostname.c_str());
      rep = SlsDetMessage(SlsDetMessage::Ok, SlsDetMessage::String);
      rep.setString(cacheStr(hostname));
    } else {
      asynPrint(_pasynUser, ASYN_TRACE_ERROR,
                 "%s:%s: port=%s address=%d error calling getHostname: %s\n",
                 driverName, functionName, _portName, _addr, _det->getErrorMessage(crit).c_str());
    }
  }

  return rep;
}

SlsDetMessage SlsDetDriver::getDetectorsType()
{
  int crit;
  slsDetectorDefs::detectorType dettype;
  SlsDetMessage rep(SlsDetMessage::Error);
  static const char *functionName = "getDetectorsType";

  if (_det) {
    asynPrint(_pasynUser, ASYN_TRACEIO_DRIVER,
              "%s:%s, port=%s, address=%d calling getDetectorsType\n",
              driverName, functionName, _portName, _addr);
    dettype = _det->getDetectorsType(_pos);
    if (!_det->getErrorMask()) {
    asynPrint(_pasynUser, ASYN_TRACEIO_DRIVER,
              "%s:%s, port=%s, address=%d getDetectorsType returned: %s\n",
              driverName, functionName, _portName, _addr, slsDetectorDefs::getDetectorType(dettype).c_str());
      rep = SlsDetMessage(SlsDetMessage::Ok, SlsDetMessage::Int32);
      rep.setInteger(dettype);
    } else {
      asynPrint(_pasynUser, ASYN_TRACE_ERROR,
                 "%s:%s: port=%s address=%d error calling getDetectorsType: %s\n",
                 driverName, functionName, _portName, _addr, _det->getErrorMessage(crit).c_str());
    }
  }

  return rep;
}

SlsDetMessage SlsDetDriver::getRunStatus()
{
  int crit;
  slsDetectorDefs::runStatus status;
  SlsDetMessage rep(SlsDetMessage::Error);
  static const char *functionName = "getRunStatus";

  if (_det) {
    asynPrint(_pasynUser, ASYN_TRACEIO_DRIVER,
              "%s:%s, port=%s, address=%d calling getRunStatus\n",
              driverName, functionName, _portName, _addr);
    status = _det->getRunStatus();
    if (!_det->getErrorMask()) {
      asynPrint(_pasynUser, ASYN_TRACEIO_DRIVER,
                "%s:%s, port=%s, address=%d getRunStatus returned status: %d (%s)\n",
                driverName, functionName, _portName, _addr, status,
                slsDetectorDefs::runStatusType(status).c_str());
      rep = SlsDetMessage(SlsDetMessage::Ok, SlsDetMessage::Int32);
      rep.setInteger(status);
    } else {
      asynPrint(_pasynUser, ASYN_TRACE_ERROR,
                 "%s:%s: port=%s address=%d error calling getRunStatus: %s\n",
                 driverName, functionName, _portName, _addr, _det->getErrorMessage(crit).c_str());
    }
  }

  return rep;
}

SlsDetMessage SlsDetDriver::getNumberOfDetectors()
{
  int crit;
  int num_dets;
  SlsDetMessage rep(SlsDetMessage::Error);
  static const char *functionName = "getNumberOfDetectors";

  if (_det) {
    asynPrint(_pasynUser, ASYN_TRACEIO_DRIVER,
              "%s:%s, port=%s, address=%d calling getNumberOfDetectors\n",
              driverName, functionName, _portName, _addr);
    num_dets = _det->getNumberOfDetectors();
    if (!_det->getErrorMask()) {
      asynPrint(_pasynUser, ASYN_TRACEIO_DRIVER,
                "%s:%s, port=%s, address=%d getNumberOfDetectors returned num_dets: %d\n",
                driverName, functionName, _portName, _addr, num_dets);
      rep = SlsDetMessage(SlsDetMessage::Ok, SlsDetMessage::Int32);
      rep.setInteger(num_dets);
    } else {
      asynPrint(_pasynUser, ASYN_TRACE_ERROR,
                 "%s:%s: port=%s address=%d error calling getNumberOfDetectors: %s\n",
                 driverName, functionName, _portName, _addr, _det->getErrorMessage(crit).c_str());
    }
  }

  return rep;
}

SlsDetMessage SlsDetDriver::getId(slsDetectorDefs::idMode mode)
{
  int crit;
  char buffer[32];
  int64_t raw_value;
  SlsDetMessage rep(SlsDetMessage::Error);
  static const char *functionName = "getId";

  if (_det) {
    asynPrint(_pasynUser, ASYN_TRACEIO_DRIVER,
              "%s:%s, port=%s, address=%d calling getId(%d)\n",
              driverName, functionName, _portName, _addr, mode);
    raw_value = _det->getId(mode, _pos);
    if (!_det->getErrorMask()) {
      asynPrint(_pasynUser, ASYN_TRACEIO_DRIVER,
                "%s:%s, port=%s, address=%d getId returned raw value: %ld\n",
                driverName, functionName, _portName, _addr, raw_value);
      epicsSnprintf(buffer, sizeof(buffer), "0x%lx", raw_value);
      rep = SlsDetMessage(SlsDetMessage::Ok, SlsDetMessage::String);
      rep.setString(cacheStr(buffer));
    } else {
      asynPrint(_pasynUser, ASYN_TRACE_ERROR,
                 "%s:%s: port=%s address=%d error calling getADC: %s\n",
                 driverName, functionName, _portName, _addr, _det->getErrorMessage(crit).c_str());
    }
  }

  return rep;
}

SlsDetMessage SlsDetDriver::getTemperature(slsDetectorDefs::dacIndex dac)
{
  int crit;
  int raw_value;
  SlsDetMessage rep(SlsDetMessage::Error);
  static const char *functionName = "getTemperature";
  
  if (_det) {
    asynPrint(_pasynUser, ASYN_TRACEIO_DRIVER,
              "%s:%s, port=%s, address=%d calling getADC\n",
              driverName, functionName, _portName, _addr);
    raw_value = _det->getADC(dac, _pos);
    if (!_det->getErrorMask()) {
      asynPrint(_pasynUser, ASYN_TRACEIO_DRIVER,
                "%s:%s, port=%s, address=%d getADC returned raw value: %d\n",
                driverName, functionName, _portName, _addr, raw_value);
      rep = SlsDetMessage(SlsDetMessage::Ok, SlsDetMessage::Float64);
      rep.setDouble(raw_value / TEMP_UNITS);
    } else {
      asynPrint(_pasynUser, ASYN_TRACE_ERROR,
                 "%s:%s: port=%s address=%d error calling getADC: %s\n",
                 driverName, functionName, _portName, _addr, _det->getErrorMessage(crit).c_str());
    }
  }

  return rep;
}

SlsDetMessage SlsDetDriver::powerChip(int value)
{
  int crit;
  int ret;
  SlsDetMessage rep(SlsDetMessage::Error);
  static const char *functionName = "powerChip";

  if (_det) {
    asynPrint(_pasynUser, ASYN_TRACEIO_DRIVER,
              "%s:%s, port=%s, address=%d calling powerChip(%d)\n",
              driverName, functionName, _portName, _addr, value);
    ret = _det->powerChip(value);
    if (!_det->getErrorMask()) {
      if (value < 0) { // this is a read
        asynPrint(_pasynUser, ASYN_TRACEIO_DRIVER,
                 "%s:%s, port=%s, address=%d powerChip read returned: %d\n",
                 driverName, functionName, _portName, _addr, ret);
        rep = SlsDetMessage(SlsDetMessage::Ok, SlsDetMessage::Int32);
        rep.setInteger(ret);
      } else { // this is a write
        asynPrint(_pasynUser, ASYN_TRACEIO_DRIVER,
                 "%s:%s, port=%s, address=%d powerChip write returned: %d\n",
                 driverName, functionName, _portName, _addr, ret);
        if (ret == value) {
          rep = SlsDetMessage(SlsDetMessage::Ok);
        } else {
          rep = SlsDetMessage(SlsDetMessage::Failed);
        }
      }
    } else {
      asynPrint(_pasynUser, ASYN_TRACE_ERROR,
                 "%s:%s: port=%s address=%d error calling powerChip: %s\n",
                 driverName, functionName, _portName, _addr, _det->getErrorMessage(crit).c_str());
    }
  }

  return rep;
}

SlsDetMessage SlsDetDriver::highVoltage(int value)
{
  int crit;
  int ret;
  SlsDetMessage rep(SlsDetMessage::Error);
  static const char *functionName = "highVoltage";

  if (_det) {
    asynPrint(_pasynUser, ASYN_TRACEIO_DRIVER,
              "%s:%s, port=%s, address=%d calling setHighVoltage(%d)\n",
              driverName, functionName, _portName, _addr, value);
    ret = _det->setHighVoltage(value);
    if (!_det->getErrorMask()) {
      if (value < 0) { // this is a read
        asynPrint(_pasynUser, ASYN_TRACEIO_DRIVER,
                 "%s:%s, port=%s, address=%d setHighVoltage read returned: %d\n",
                 driverName, functionName, _portName, _addr, ret);
        rep = SlsDetMessage(SlsDetMessage::Ok, SlsDetMessage::Int32);
        rep.setInteger(ret);
      } else { // this is a write
        asynPrint(_pasynUser, ASYN_TRACEIO_DRIVER,
                 "%s:%s, port=%s, address=%d setHighVoltage write returned: %d\n",
                 driverName, functionName, _portName, _addr, ret);
        if (ret == value) {
          rep = SlsDetMessage(SlsDetMessage::Ok);
        } else {
          rep = SlsDetMessage(SlsDetMessage::Failed);
        }
      }
    } else {
      asynPrint(_pasynUser, ASYN_TRACE_ERROR,
                 "%s:%s: port=%s address=%d error calling setHighVoltage: %s\n",
                 driverName, functionName, _portName, _addr, _det->getErrorMessage(crit).c_str());
    }
  }

  return rep;
}

SlsDetMessage SlsDetDriver::clockDivider(int value)
{
  int crit;
  int ret;
  int64_t errors;
  SlsDetMessage rep(SlsDetMessage::Error);
  static const char *functionName = "clockDivider";

  if (_det) {
    asynPrint(_pasynUser, ASYN_TRACEIO_DRIVER,
              "%s:%s, port=%s, address=%d calling setClockDivider(%d)\n",
              driverName, functionName, _portName, _addr, value);
    ret = _det->setClockDivider(value);
    errors = _det->getErrorMask();
    if (!errors) {
      if (value < 0) { // this is a read
        asynPrint(_pasynUser, ASYN_TRACEIO_DRIVER,
                 "%s:%s, port=%s, address=%d setClockDivider read returned: %d\n",
                 driverName, functionName, _portName, _addr, ret);
        rep = SlsDetMessage(SlsDetMessage::Ok, SlsDetMessage::Int32);
        rep.setInteger(ret);
      } else { // this is a write
        asynPrint(_pasynUser, ASYN_TRACEIO_DRIVER,
                 "%s:%s, port=%s, address=%d setClockDivider write returned: %d\n",
                 driverName, functionName, _portName, _addr, ret);
        if (ret == value) {
          rep = SlsDetMessage(SlsDetMessage::Ok);
        } else {
          rep = SlsDetMessage(SlsDetMessage::Failed);
        }
      }
    } else if (errors == (1<<_pos)) {
      _det->clearAllErrorMask(); // clear the error mask
      rep = SlsDetMessage(SlsDetMessage::Failed);
    } else {
      asynPrint(_pasynUser, ASYN_TRACE_ERROR,
                 "%s:%s: port=%s address=%d error calling setClockDivider: %s\n",
                 driverName, functionName, _portName, _addr, _det->getErrorMessage(crit).c_str());
    }
  }

  return rep;
}

SlsDetMessage SlsDetDriver::gainSettings(int value)
{
  int crit;
  slsDetectorDefs::detectorSettings ret;
  int64_t errors;
  SlsDetMessage rep(SlsDetMessage::Error);
  static const char *functionName = "gainSettings";

  if (_det) {
    asynPrint(_pasynUser, ASYN_TRACEIO_DRIVER,
              "%s:%s, port=%s, address=%d calling setSettings(%d)\n",
              driverName, functionName, _portName, _addr, value);
    ret = _det->setSettings((slsDetectorDefs::detectorSettings)value, _pos);
    errors = _det->getErrorMask();
    if (!errors) {
      if (value < 0) { // this is a read
        asynPrint(_pasynUser, ASYN_TRACEIO_DRIVER,
                 "%s:%s, port=%s, address=%d setSettings read returned: %d\n",
                 driverName, functionName, _portName, _addr, ret);
        rep = SlsDetMessage(SlsDetMessage::Ok, SlsDetMessage::Int32);
        rep.setInteger(ret);
      } else { // this is a write
        asynPrint(_pasynUser, ASYN_TRACEIO_DRIVER,
                 "%s:%s, port=%s, address=%d setSettings write returned: %d\n",
                 driverName, functionName, _portName, _addr, ret);
        if (ret == value) {
          rep = SlsDetMessage(SlsDetMessage::Ok);
        } else {
          rep = SlsDetMessage(SlsDetMessage::Failed);
        }
      }
    } else if (errors == (1<<_pos)) {
      _det->clearAllErrorMask(); // clear the error mask
      rep = SlsDetMessage(SlsDetMessage::Failed);
    } else {
      asynPrint(_pasynUser, ASYN_TRACE_ERROR,
                 "%s:%s: port=%s address=%d error calling setSettings: %s\n",
                 driverName, functionName, _portName, _addr, _det->getErrorMessage(crit).c_str());
    }
  }

  return rep;
}

SlsDetMessage SlsDetDriver::thresholdTemperature(double value)
{
  int crit;
  int ret;
  int threshold = value < 0. ? -1 : (int)(value * TEMP_UNITS);
  int64_t errors;
  SlsDetMessage rep(SlsDetMessage::Error);
  static const char *functionName = "thresholdTemperature";

  if (_det) {
    asynPrint(_pasynUser, ASYN_TRACEIO_DRIVER,
              "%s:%s, port=%s, address=%d calling setThresholdTemperature(%d)\n",
              driverName, functionName, _portName, _addr, threshold);
    ret = _det->setThresholdTemperature(threshold, _pos);
    errors = _det->getErrorMask();
    if (!errors) {
      if (threshold < 0) { // this is a read
        asynPrint(_pasynUser, ASYN_TRACEIO_DRIVER,
                 "%s:%s, port=%s, address=%d setThresholdTemperature read returned: %d\n",
                 driverName, functionName, _portName, _addr, ret);
        rep = SlsDetMessage(SlsDetMessage::Ok, SlsDetMessage::Float64);
        rep.setDouble(ret / TEMP_UNITS);
      } else { // this is a write
        asynPrint(_pasynUser, ASYN_TRACEIO_DRIVER,
                 "%s:%s, port=%s, address=%d setThresholdTemperature write returned: %d\n",
                 driverName, functionName, _portName, _addr, ret);
        rep = SlsDetMessage(SlsDetMessage::Ok);
      }
    } else if (errors == (1<<_pos)) {
      _det->clearAllErrorMask(); // clear the error mask
      rep = SlsDetMessage(SlsDetMessage::Failed);
    } else {
      asynPrint(_pasynUser, ASYN_TRACE_ERROR,
                 "%s:%s: port=%s address=%d error calling powerChip: %s\n",
                 driverName, functionName, _portName, _addr, _det->getErrorMessage(crit).c_str());
    }
  }

  return rep;
}

SlsDetMessage SlsDetDriver::temperatureControl(int value)
{
  int crit;
  int ret;
  int64_t errors;
  SlsDetMessage rep(SlsDetMessage::Error);
  static const char *functionName = "temperatureControl";

  if (_det) {
    asynPrint(_pasynUser, ASYN_TRACEIO_DRIVER,
              "%s:%s, port=%s, address=%d calling setTemperatureControl(%d)\n",
              driverName, functionName, _portName, _addr, value);
    ret = _det->setTemperatureControl(value, _pos);
    errors = _det->getErrorMask();
    if (!errors) {
      if (value < 0) { // this is a read
        asynPrint(_pasynUser, ASYN_TRACEIO_DRIVER,
                 "%s:%s, port=%s, address=%d setTemperatureControl read returned: %d\n",
                 driverName, functionName, _portName, _addr, ret);
        rep = SlsDetMessage(SlsDetMessage::Ok, SlsDetMessage::Int32);
        rep.setInteger(ret);
      } else { // this is a write
        asynPrint(_pasynUser, ASYN_TRACEIO_DRIVER,
                 "%s:%s, port=%s, address=%d setTemperatureControl write returned: %d\n",
                 driverName, functionName, _portName, _addr, ret);
        rep = SlsDetMessage(SlsDetMessage::Ok);
      }
    } else if (errors == (1<<_pos)) {
      _det->clearAllErrorMask(); // clear the error mask
      rep = SlsDetMessage(SlsDetMessage::Failed);
    } else {
      asynPrint(_pasynUser, ASYN_TRACE_ERROR,
                 "%s:%s: port=%s address=%d error calling setTemperatureControl: %s\n",
                 driverName, functionName, _portName, _addr, _det->getErrorMessage(crit).c_str());
    }
  }

  return rep;
}

SlsDetMessage SlsDetDriver::temperatureEvent(int value)
{
  int crit;
  int ret;
  int64_t errors;
  SlsDetMessage rep(SlsDetMessage::Error);
  static const char *functionName = "temperatureEvent";

  if (_det) {
    asynPrint(_pasynUser, ASYN_TRACEIO_DRIVER,
              "%s:%s, port=%s, address=%d calling setTemperatureEvent(%d)\n",
              driverName, functionName, _portName, _addr, value);
    ret = _det->setTemperatureEvent(value, _pos);
    errors = _det->getErrorMask();
    if (!errors) {
      if (value < 0) { // this is a read
        asynPrint(_pasynUser, ASYN_TRACEIO_DRIVER,
                 "%s:%s, port=%s, address=%d setTemperatureEvent read returned: %d\n",
                 driverName, functionName, _portName, _addr, ret);
        rep = SlsDetMessage(SlsDetMessage::Ok, SlsDetMessage::Int32);
        rep.setInteger(ret);
      } else { // this is a write
        asynPrint(_pasynUser, ASYN_TRACEIO_DRIVER,
                 "%s:%s, port=%s, address=%d setTemperatureEvent write returned: %d\n",
                 driverName, functionName, _portName, _addr, ret);
        rep = SlsDetMessage(SlsDetMessage::Ok);
      }
    } else if (errors == (1<<_pos)) {
      _det->clearAllErrorMask(); // clear the error mask
      rep = SlsDetMessage(SlsDetMessage::Failed);
    } else {
      asynPrint(_pasynUser, ASYN_TRACE_ERROR,
                 "%s:%s: port=%s address=%d error calling setTemperatureEvent: %s\n",
                 driverName, functionName, _portName, _addr, _det->getErrorMessage(crit).c_str());
    }
  }

  return rep;
}

unsigned SlsDetDriver::pending() const
{
  return _pending;
}

SlsDetMessage SlsDetDriver::request(SlsDetMessage request, double timeout)
{
  int nbytes;
  if (flush(timeout) >= 0) {
    SlsDetMessage ret;
    if (_request.send(&request, sizeof(request), timeout) >= 0) {
      _pending++;
      if ((nbytes = _reply.receive(&ret, sizeof(ret), timeout)) >= 0) {
        _pending--;
        if (nbytes != sizeof(ret)) {
          return SlsDetMessage(SlsDetMessage::Error);
        } else {
          return ret;
        }
      }
    }
  }

  return SlsDetMessage(SlsDetMessage::Timeout);
}

SlsDetMessage SlsDetDriver::request(SlsDetMessage::MessageType mtype, double timeout)
{
  return request(SlsDetMessage(mtype), timeout);
}

int SlsDetDriver::stop()
{
  SlsDetMessage msg(SlsDetMessage::Exit);
  _reply.send(&msg, sizeof(msg));
  return _request.send(&msg, sizeof(msg));
}

int SlsDetDriver::flush(double timeout)
{
  int ret = 0;
  SlsDetMessage msg;

  while (_pending > 0) {
    ret = _reply.receive(&msg, sizeof(msg), timeout);
    if (ret < 0) {
      break;
    } else {
      _pending--;
    }
  }

  return ret;
}

int SlsDetDriver::reply(SlsDetMessage::MessageType mtype)
{
  SlsDetMessage msg(mtype);
  return _reply.send(&msg, sizeof(msg));
}

int SlsDetDriver::reply(SlsDetMessage msg)
{
  return _reply.send(&msg, sizeof(msg));
}

void SlsDetDriver::initialize()
{
  int numDetectors;
  static const char *functionName = "initialize";

  asynPrint(_pasynUser, ASYN_TRACE_FLOW,
            "%s:%s: port=%s address=%d start\n",
            driverName, functionName, _portName, _addr);

  if (!_det) {
    try {
      _det = new multiSlsDetector(_id);
      _det->setHostname(_hostname.c_str());
      if ((numDetectors =_det->getNumberOfDetectors()) != _maxDets) {
        /* something is very wrong either det didn't connect or hostname was a compound one*/
        shutdown();
        asynPrint(_pasynUser, ASYN_TRACE_ERROR,
                 "%s:%s: port=%s address=%d only configured %d out of %d sub-detectors\n",
                 driverName, functionName, _portName, _addr, numDetectors, _maxDets);
      }
     } catch (...) {
      asynPrint(_pasynUser, ASYN_TRACE_ERROR,
                "%s:%s: port=%s address=%d failed to initialize detector\n",
                driverName, functionName, _portName, _addr);
    }
  }

  asynPrint(_pasynUser, ASYN_TRACE_FLOW,
            "%s:%s: port=%s address=%d end\n",
            driverName, functionName, _portName, _addr);
}

const char* SlsDetDriver::cacheStr(const char* str)
{
  if (_cacheStr) {
    free(_cacheStr);
    _cacheStr = NULL;
  }
  _cacheStr = epicsStrDup(str);
  return _cacheStr;
}

const char* SlsDetDriver::cacheStr(const std::string& str)
{
  return cacheStr(str.c_str());
}

void SlsDetDriver::run()
{
  int nbytes;
  SlsDetMessage req;
  SlsDetMessage rep;
  static const char *functionName = "run";

  asynPrint(_pasynUser, ASYN_TRACE_FLOW,
            "%s:%s: port=%s address=%d start\n",
            driverName, functionName, _portName, _addr);

  /* Keep trying until we connect to the detector. */
  while (!_det && _running) {
    initialize();
    epicsThread::sleep(_pollTime);
  }

  /* Flush any pending messages from the request queue */
  asynPrint(_pasynUser, ASYN_TRACE_FLOW,
            "*%s:%s: port=%s address=%d flushing %d pending requests\n",
            driverName, functionName, _portName, _addr, _request.pending());

  asynPrint(_pasynUser, ASYN_TRACE_FLOW,
            "*%s:%s: port=%s address=%d initialized\n",
            driverName, functionName, _portName, _addr);

  while (_running) {
    nbytes = _request.receive(&req, sizeof(req));
    if (nbytes < 0) {
      asynPrint(_pasynUser, ASYN_TRACE_ERROR,
                "%s:%s: port=%s address=%d failed reading request from queue\n",
                 driverName, functionName, _portName, _addr);
      reply(SlsDetMessage::Error);
    } else if (nbytes != sizeof(req)) {
      asynPrint(_pasynUser, ASYN_TRACE_ERROR,
                "%s:%s: port=%s address=%d invalid request from queue with size %d versus expected %zu\n",
                 driverName, functionName, _portName, _addr, nbytes, sizeof(req));
      reply(SlsDetMessage::Error);
    } else {
      asynPrint(_pasynUser, ASYN_TRACE_FLOW,
            "%s:%s: port=%s address=%d request received: %s\n",
            driverName, functionName, _portName, _addr, req.dump().c_str());
      switch (req.dtype()) {
      case SlsDetMessage::None:
        switch (req.mtype()) {
        case SlsDetMessage::CheckOnline:
          rep = checkOnline();
          break;
        case SlsDetMessage::ReadHostname:
          rep = getHostname();
          break;
        case SlsDetMessage::ReadDetType:
          rep = getDetectorsType();
          break;
        case SlsDetMessage::ReadRunStatus:
          rep = getRunStatus();
          break;
        case SlsDetMessage::ReadNumDetectors:
          rep = getNumberOfDetectors();
          break;
        case SlsDetMessage::ReadSerialnum:
          rep = getId(slsDetectorDefs::DETECTOR_SERIAL_NUMBER);
          break;
        case SlsDetMessage::ReadFirmwareVer:
          rep = getId(slsDetectorDefs::DETECTOR_FIRMWARE_VERSION);
          break;
        case SlsDetMessage::ReadSoftwareVer:
          rep = getId(slsDetectorDefs::DETECTOR_SOFTWARE_VERSION);
          break;
        case SlsDetMessage::ReadFpgaTemp:
          rep = getTemperature(slsDetectorDefs::TEMPERATURE_FPGA);
          break;
        case SlsDetMessage::ReadAdcTemp:
          rep = getTemperature(slsDetectorDefs::TEMPERATURE_ADC);
          break;
        case SlsDetMessage::ReadTempThreshold:
          rep = thresholdTemperature();
          break;
        case SlsDetMessage::ReadTempControl:
          rep = temperatureControl();
          break;
        case SlsDetMessage::ReadTempEvent:
          rep = temperatureEvent();
          break;
        case SlsDetMessage::ReadPowerChip:
          rep = powerChip();
          break;
        case SlsDetMessage::ReadHighVoltage:
          rep = highVoltage();
          break;
        case SlsDetMessage::ReadClockDivider:
          rep = clockDivider();
          break;
        case SlsDetMessage::ReadGainMode:
          rep = gainSettings();
          break;
        default:
          asynPrint(_pasynUser, ASYN_TRACE_WARNING,
                  "%s:%s: port=%s address=%d unsupported read request type: %s\n",
                  driverName, functionName, _portName, _addr, req.dump().c_str());
          rep = SlsDetMessage(SlsDetMessage::Invalid);
          break;
        }
        break;
      case SlsDetMessage::Int32:
        switch (req.mtype()) {
        case SlsDetMessage::WriteTempControl:
          rep = temperatureControl(req.asInteger());
          break;
        case SlsDetMessage::WriteTempEvent:
          rep = temperatureEvent(req.asInteger());
          break;
        case SlsDetMessage::WritePowerChip:
          rep = powerChip(req.asInteger());
          break;
        case SlsDetMessage::WriteHighVoltage:
          rep = highVoltage(req.asInteger());
          break;
        case SlsDetMessage::WriteClockDivider:
          rep = clockDivider(req.asInteger());
          break;
        case SlsDetMessage::WriteGainMode:
          rep = gainSettings(req.asInteger());
          break;
        default:
          asynPrint(_pasynUser, ASYN_TRACE_WARNING,
                  "%s:%s: port=%s address=%d unsupported write request type: %s\n",
                  driverName, functionName, _portName, _addr, req.dump().c_str());
          rep = SlsDetMessage(SlsDetMessage::Invalid);
          break;
        }
        break;
      case SlsDetMessage::Float64:
        switch (req.mtype()) {
        case SlsDetMessage::WriteTempThreshold:
          rep = thresholdTemperature(req.asDouble());
          break;
        default:
          asynPrint(_pasynUser, ASYN_TRACE_WARNING,
                  "%s:%s: port=%s address=%d unsupported write request type: %s\n",
                  driverName, functionName, _portName, _addr, req.dump().c_str());
          rep = SlsDetMessage(SlsDetMessage::Invalid);
          break;
        }
        break;
      default:
        asynPrint(_pasynUser, ASYN_TRACE_WARNING,
                  "%s:%s: port=%s address=%d unsupported request type: %s\n",
                  driverName, functionName, _portName, _addr, req.dump().c_str());
        rep = SlsDetMessage(SlsDetMessage::Invalid);
        break;
      }
      asynPrint(_pasynUser, ASYN_TRACE_FLOW,
            "%s:%s: port=%s address=%d reply sent: %s\n",
            driverName, functionName, _portName, _addr, rep.dump().c_str());
      reply(rep);
    }
  }

  asynPrint(_pasynUser, ASYN_TRACE_FLOW,
            "%s:%s: port=%s address=%d end\n",
            driverName, functionName, _portName, _addr);
}

void SlsDetDriver::shutdown()
{
  if (_det) {
    delete _det;
    _det = NULL;
    // free the shared memory associated with the detector
    multiSlsDetector::freeSharedMemory(_id);
  }
}
