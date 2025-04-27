TOP=..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

# The files in this directory are installed first because they are
# used by the slsDetector driver
INC += $(patsubst ../%, %, $(wildcard ../sls/*))

# Note, the following files were manually copied from the slsDetector build directory
# to this directory after building
ifeq (linux-x86_64, $(findstring linux-x86_64, $(T_A)))
LIB_INSTALLS_Linux += ../os/linux-x86_64/libSlsDetector.so
LIB_INSTALLS_Linux += ../os/linux-x86_64/libSlsSupport.so
LIB_INSTALLS_Linux += ../os/linux-x86_64/libSlsDetector.a
LIB_INSTALLS_Linux += ../os/linux-x86_64/libSlsSupport.a
LIB_INSTALLS_Linux += ../os/linux-x86_64/libzmq.a
LIB_INSTALLS_Linux += ../os/linux-x86_64/libmd5sls.a

else ifeq (rhel7-x86_64, $(findstring rhel7-x86_64, $(T_A)))
LIB_INSTALLS_Linux += ../os/rhel7-x86_64/libSlsDetector.so
LIB_INSTALLS_Linux += ../os/rhel7-x86_64/libSlsSupport.so
LIB_INSTALLS_Linux += ../os/rhel7-x86_64/libSlsDetector.a
LIB_INSTALLS_Linux += ../os/rhel7-x86_64/libSlsSupport.a
LIB_INSTALLS_Linux += ../os/rhel7-x86_64/libzmq.a
LIB_INSTALLS_Linux += ../os/rhel7-x86_64/libmd5sls.a

else ifeq (rhel9-x86_64, $(findstring rhel9-x86_64, $(T_A)))
LIB_INSTALLS_Linux += ../os/rocky9-x86_64/libSlsDetector.so
LIB_INSTALLS_Linux += ../os/rocky9-x86_64/libSlsSupport.so
LIB_INSTALLS_Linux += ../os/rocky9-x86_64/libSlsDetector.a
LIB_INSTALLS_Linux += ../os/rocky9-x86_64/libSlsSupport.a
LIB_INSTALLS_Linux += ../os/rocky9-x86_64/libzmq.a
LIB_INSTALLS_Linux += ../os/rocky9-x86_64/libmd5sls.a

endif

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

