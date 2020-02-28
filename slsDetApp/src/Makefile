TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#

LIBRARY_IOC += slsDet

INC += slsDetMessage.h
INC += slsDetDriver.h
INC += drvAsynSlsDetPort.h

LIB_SRCS += slsDetMessage.cpp
LIB_SRCS += slsDetDriver.cpp
LIB_SRCS += drvAsynSlsDetPort.cpp

LIB_LIBS += SlsDetector
LIB_LIBS += SlsReceiver
LIB_LIBS += asyn
LIB_LIBS += $(EPICS_BASE_IOC_LIBS)

DBD += slsDetSupport.dbd

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE
