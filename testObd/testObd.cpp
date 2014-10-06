
#include <cstdlib>
#include <map>
#include <string>
#include <iostream>
//#include <boost/thread.hpp> // sleep()

#include "CSerialPort.h"
#include "PidScanner.h"


// --------------------------------------------
// Main program
int main(int argc, char* argv[])
{
   CUart ser_port;
   boost::system::error_code errcode;

   printf("Parameters: <port name> <speed>\n");
   // ==================
   // open serial port
   // ==================
   // TODO: get from config
   const int OBD_UART_SPEED = (argc > 2) ? atoi(argv[2]) : 9600;
   const char* OBD_UART_PORT = (argc > 1) ? argv[1] : "COM4";
   const int OBD_UART_TIMEOUT = 2; // sec
   const int OBD_READ_INTERVAL = 4; // sec
	
   std::cout << "Opening UART port " << OBD_UART_PORT << " at " << OBD_UART_SPEED << " baud .." << std::endl;
   ser_port.open(OBD_UART_PORT, OBD_UART_SPEED, errcode);

   // NOTE: error_code - platform dependent
   // errcode.default_error_condition(); - way to convert to platform independent error code
   // std::cout << "COM : " << errcode.category().name() <<" errcode = " << errcode.value() << " what = " << errcode.message() << std::endl;
   if(errcode.value())
   {
      std::cout << "UART open error -- " << OBD_UART_PORT << " : " << errcode.message() << std::endl;
      return 2;
   }

   // set timeout for read(), sec
   ser_port.setTimeout(boost::posix_time::seconds(OBD_UART_TIMEOUT));

  // ===================
  // read in PID scanner
  // ===================

    // Create PID-scanner object
    CPidScanner pid_scanner(ser_port, OBD_READ_INTERVAL);

    // Init ELM device
    int rc;
    if((rc = pid_scanner.Init()) != 0)
    {
        std::cout << "PidScanner Initialization error : " << rc << std::endl;
        return 3;
    }
    
    // basic loop
    // 1) get data from PID-scanner object
    // 2) delay 1 sec
    for(int k = 0; ; k++)
    {
        // Poll PIDs
        if(pid_scanner.Poll())
        {

             float pid_val = -1;
             bool pid_present = false;

            // Speed
             pid_val = pid_scanner.getSpeed(pid_present);
            std::cout << "Speed=" << pid_val << (pid_present ? "" : " - error: old data") << std::endl;

             // RPM
              pid_val = pid_scanner.getRpm(pid_present);
             std::cout << "RPM=" << pid_val << (pid_present ? "" : " - error: old data") << std::endl;

             // Throttle
              pid_val = pid_scanner.getThrottle(pid_present);
             std::cout << "Throttle=" << pid_val << (pid_present ? "" : " - error: old data") << std::endl;

             // Voltage
              pid_val = pid_scanner.getVoltage(pid_present);
             std::cout << "Voltage=" << pid_val << (pid_present ? "" : " - error: old data") << std::endl;

             // Fuel
              pid_val = pid_scanner.getFuel(pid_present);
             std::cout << "Fuel=" << pid_val << (pid_present ? "" : " - error: old data") << std::endl;

             // Odometer
              pid_val = pid_scanner.getOdometer(pid_present);
             std::cout << "Odometer=" << pid_val << (pid_present ? "" : " - error: old data") << std::endl;

             std::cout << std::endl;
        }

        // Delay
        // boost::this_thread::sleep(boost::posix_time::millisec(OBD_READ_INTERVAL/2));
		sleep(OBD_READ_INTERVAL/2);
    }
}
