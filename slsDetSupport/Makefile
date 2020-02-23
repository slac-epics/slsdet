TOP=..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

# The files in this directory are installed first because they are
# used by the slsDetector driver
INC += ansi.h
INC += detectorData.h
INC += fileIO.h
INC += fileIOStatic.h
INC += angularConversion.h
INC += angularConversionStatic.h
INC += badChannelCorrections.h
INC += postProcessing.h
INC += error_defs.h
INC += sls_detector_defs.h
INC += sls_detector_funcs.h
INC += slsDetectorUsers.h
INC += sls_receiver_defs.h
INC += sls_receiver_funcs.h
INC += slsReceiverUsers.h
INC += slsDetectorBase.h
INC += slsDetectorUtils.h
INC += slsDetectorActions.h
INC += multiSlsDetector.h
INC += multiSlsDetectorCommand.h

# Note, the following files were manually copied from the slsDetector build directory
# to this directory after building
ifeq (linux-x86_64, $(findstring linux-x86_64, $(T_A)))
LIB_INSTALLS_Linux += ../os/linux-x86_64/libSlsDetector.so
LIB_INSTALLS_Linux += ../os/linux-x86_64/libSlsReceiver.so
LIB_INSTALLS_Linux += ../os/linux-x86_64/libSlsDetector.a
LIB_INSTALLS_Linux += ../os/linux-x86_64/libSlsReceiver.a
LIB_INSTALLS_Linux += ../os/linux-x86_64/libzmq.a

else ifeq (rhel6-x86_64, $(findstring rhel6-x86_64, $(T_A)))
LIB_INSTALLS_Linux += ../os/rhel6-x86_64/libSlsDetector.so
LIB_INSTALLS_Linux += ../os/rhel6-x86_64/libSlsReceiver.so
LIB_INSTALLS_Linux += ../os/rhel6-x86_64/libSlsDetector.a
LIB_INSTALLS_Linux += ../os/rhel6-x86_64/libSlsReceiver.a
LIB_INSTALLS_Linux += ../os/rhel6-x86_64/libzmq.a

else ifeq (rhel7-x86_64, $(findstring rhel7-x86_64, $(T_A)))
LIB_INSTALLS_Linux += ../os/rhel7-x86_64/libSlsDetector.so
LIB_INSTALLS_Linux += ../os/rhel7-x86_64/libSlsReceiver.so
LIB_INSTALLS_Linux += ../os/rhel7-x86_64/libSlsDetector.a
LIB_INSTALLS_Linux += ../os/rhel7-x86_64/libSlsReceiver.a
LIB_INSTALLS_Linux += ../os/rhel7-x86_64/libzmq.a

endif

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

