#ifndef slsDetDriver_H
#define slsDetDriver_H

#include <epicsThread.h>
#include <epicsMessageQueue.h>
#include <asynDriver.h>
#include <sls_detector_defs.h>

#include "slsDetMessage.h"

class multiSlsDetector;

/** Class definition for the SlsDetDriver class
  */
class SlsDetDriver : public epicsThreadRunable {
public:
  SlsDetDriver(const std::string &hostName, const int id,
               const char* portName, const int addr);
  virtual ~SlsDetDriver();
  virtual void run();
  virtual void shutdown();
  /* functions that call the slsdetector interface */
  virtual unsigned pending() const;
  virtual SlsDetMessage request(SlsDetMessage request, double timeout);
  virtual SlsDetMessage request(SlsDetMessage::MessageType mtype, double timeout);
  /* remove this */
  virtual std::string getHostname(asynStatus* status=NULL);
  virtual int getNumberOfDetectors(asynStatus* status=NULL);
  virtual slsDetectorDefs::runStatus getRunStatus(asynStatus* status=NULL);

protected:
  virtual int stop();
  virtual int flush(double timeout);
  virtual int reply(SlsDetMessage::MessageType mtype);
  virtual int reply(SlsDetMessage msg);
  virtual void initialize();
  virtual SlsDetMessage getTemperature(slsDetectorDefs::dacIndex dac);
  virtual SlsDetMessage checkOnline();

private:
  asynUser*         _pasynUser;
  bool              _running;
  unsigned          _pending;
  const int         _id;
  const int         _addr;
  const int         _maxDets;
  const double      _pollTime;
  const char*       _portName;
  std::string       _hostname;
  epicsThread       _thread;
  epicsMessageQueue _request;
  epicsMessageQueue _reply;
  multiSlsDetector* _det;
};

#endif
