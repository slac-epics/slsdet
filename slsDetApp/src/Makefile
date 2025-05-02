TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#

LIBRARY_IOC += slsDet

USR_CXXFLAGS_Linux += -std=c++11

INC += drvAsynSlsDetPort.h

LIB_SRCS += drvAsynSlsDetPort.cpp

LIB_LIBS += SlsDetector
LIB_LIBS += SlsSupport
LIB_LIBS += asyn
LIB_LIBS += $(EPICS_BASE_IOC_LIBS)

DBD += slsDetSupport.dbd

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE
