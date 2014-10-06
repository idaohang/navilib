
# BOOST := "c:/Program Files/boost/boost_1_44"
BOOST_LIB := /usr/lib

LOG4CPP := log4cpp
LOG4CPP_INC := ${LOG4CPP}/include
LOG4CPP_LIB := ${LOG4CPP}/src/.libs

TINYXML := tinyxml

OBJS := testObd.o ../CSerialPort.o  ../PidScanner.o

CPPFLAGS += -I..
#-I${LOG4CPP_INC} -I${TINYXML}

#LDFLAGS += -L${LOG4CPP_LIB} -L${TINYXML} -L${BOOST_LIB}

all: testObd

testObd: ${OBJS}  
	$(LINK.cc) $+ -o $@ -llibboost_system-mt -llibboost_thread-mt

clean:
	$(RM) testObd ${OBJS}

#testObd.o: testObd.h