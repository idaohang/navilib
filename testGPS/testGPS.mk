TARGET := TestGPS

SRC_CXXFLAGS := -g -O0 -Wall -pipe -DDEBUGMODE
TGT_LDFLAGS := -L${TARGET_DIR}
TGT_LDLIBS  := -lboost_system
TGT_PREREQS := 

SOURCES := testGPS.cpp ../CSerialPort.cpp

SRC_INCDIRS := ..
