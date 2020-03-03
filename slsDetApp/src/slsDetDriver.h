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

protected:
  virtual int stop();
  virtual int flush(double timeout);
  virtual int reply(SlsDetMessage::MessageType mtype);
  virtual int reply(SlsDetMessage msg);
  virtual void initialize();
  virtual const char* cacheStr(const char* str);
  virtual const char* cacheStr(const std::string& str);
  virtual SlsDetMessage checkOnline();
  virtual SlsDetMessage getHostname();
  virtual SlsDetMessage getDetectorsType();
  virtual SlsDetMessage getRunStatus();
  virtual SlsDetMessage getNumberOfDetectors();
  virtual SlsDetMessage getId(slsDetectorDefs::idMode mode);
  virtual SlsDetMessage getTemperature(slsDetectorDefs::dacIndex dac);
  virtual SlsDetMessage thresholdTemperature(double value=-1.0);
  virtual SlsDetMessage temperatureControl(int value=-1);
  virtual SlsDetMessage temperatureEvent(int value=-1);
  virtual SlsDetMessage powerChip(int value=-1);
  virtual SlsDetMessage highVoltage(int value=-1);
  virtual SlsDetMessage clockDivider(int value=-1);
  virtual SlsDetMessage gainSettings(int value=-1);

private:
  asynUser*         _pasynUser;
  bool              _running;
  unsigned          _pending;
  char*             _cacheStr;
  const int         _id;
  const int         _addr;
  const int         _pos;
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
