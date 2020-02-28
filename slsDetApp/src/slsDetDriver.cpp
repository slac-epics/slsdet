#include "slsDetDriver.h"

#include <multiSlsDetector.h>

#define MAX_MQ_CAPACITY 4
#define MAX_DETS 1
#define DEFAULT_POLL_TIME 0.250
#define THREAD_TMO 2.0

static const char *driverName = "SlsDetDriver";

SlsDetDriver::SlsDetDriver(const std::string &hostName, const int id,
                           const char* portName, const int addr) :
  _pasynUser(pasynManager->createAsynUser(0,0)),
  _running(true),
  _pending(0),
  _id(id),
  _addr(addr),
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

std::string SlsDetDriver::getHostname(asynStatus* status)
{
  if (status) *status = asynSuccess;
  return std::string("fake");
}

int SlsDetDriver::getNumberOfDetectors(asynStatus* status)
{
  if (status) *status = asynSuccess;
  return 1;
}

slsDetectorDefs::runStatus SlsDetDriver::getRunStatus(asynStatus* status)
{
  if (status) *status = asynSuccess;
  return slsDetectorDefs::IDLE;
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
    raw_value = _det->getADC(dac);
    if (!_det->getErrorMask()) {
      asynPrint(_pasynUser, ASYN_TRACEIO_DRIVER,
                "%s:%s, port=%s, address=%d getADC returned raw value: %d\n",
                driverName, functionName, _portName, _addr, raw_value);
      rep = SlsDetMessage(SlsDetMessage::Ok, SlsDetMessage::Float64);
      rep.setDouble(raw_value / 1000.);
    } else {
      asynPrint(_pasynUser, ASYN_TRACE_ERROR,
                 "%s:%s: port=%s address=%d error calling getADC: %s\n",
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
  while(_request.pending() > 0) {
    _request.receive(&req, sizeof(req));
    reply(SlsDetMessage::Error);
  }

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
      switch (req.mtype()) {
      case SlsDetMessage::CheckOnline:
        rep = SlsDetDriver::checkOnline();
        break;
      case SlsDetMessage::ReadFpgaTemp:
        rep = getTemperature(slsDetectorDefs::TEMPERATURE_FPGA);
        break;
      case SlsDetMessage::ReadAdcTemp:
        rep = getTemperature(slsDetectorDefs::TEMPERATURE_ADC);
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
