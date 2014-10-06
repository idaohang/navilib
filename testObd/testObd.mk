TARGET := TestObd

SRC_CXXFLAGS := -g -O0 -Wall -pipe -DDEBUGMODE
TGT_LDFLAGS := -L${TARGET_DIR}
TGT_LDLIBS  := -lboost_system -lboost_thread
TGT_PREREQS := 

SOURCES := testObd.cpp ../CSerialPort.cpp  ../PidScanner.cpp

SRC_INCDIRS := ..
