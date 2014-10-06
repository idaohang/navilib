
#include <cstdlib>
#include <map>
#include <string>
#include <iostream>

#include "CGps.h"

// --------------------------------------------
// Main program
int main(int argc, char* argv[])
{
    const int READ_INTERVAL = 1; // sec
    CUart ser_port;
    boost::system::error_code errcode;

    std::cout << "Options:" << std::endl;
    std::cout << "-f [file name], default file: gps.log" << std::endl;
    std::cout << "or" << std::endl;
    std::cout << "<tty name> <speed>, default: /dev/ttySC0 38400" << std::endl;

    bool file_mode = false;
    if(argc > 1 && argv[1] && strcmp(argv[1], "-f") == 0)
    {
        file_mode = true;
    }

    if(file_mode)
    {
        const char *file_name = argv[2] ? argv[2] : "gps.log";
        FILE *fp = fopen(file_name, "r");
        if(!fp)
        {
            perror("fopen");
            return 1;
        }

        CGps GPS;

        std::string pos_val;
        bool data_present;
        
        for(int k = 0; ; k++)
        {
            char buf[500];
            if(!fgets(buf, sizeof(buf), fp))
            {
                break;
            }
            // Parse NMEA sentence
            std::string file_line(buf);
            if(GPS.parseSentence(file_line))
            {
        
                pos_val = GPS.getBearing(data_present);
                std::cout << "Position = " << pos_val << (data_present ? (" - " + file_line) : " - error: old data") << std::endl;
            }
#if 0
            else
            {
                std::cout << "Parse failed -- line = " << file_line << std::endl;
            }
#endif
        }
        if(fp)
        {
            fclose(fp);
        }
        return 0;
    }
    // ==================
    // open serial port
    // ==================
    const int UART_TIMEOUT = 1; // sec
    const int UART_SPEED = (argc > 2) ? atoi(argv[2]) : 38400;
    const char *UART_PORT = (argc > 1) ? argv[1] : "/dev/ttySC0"; 

	std::cout << "Opening UART port " << UART_PORT << " at " << UART_SPEED << " baud .." << std::endl;

    ser_port.open(UART_PORT, UART_SPEED, errcode);

    // NOTE: error_code - platform dependent
    // errcode.default_error_condition(); - way to convert to platform independent error code
    // std::cout << "COM : " << errcode.category().name() <<" errcode = " << errcode.value() << " what = " << errcode.message() << std::endl;
    if(errcode.value())
    {
        std::cout << "UART open error -- " << UART_PORT << " : " << errcode.message() << std::endl;
        return 2;
    }

    // set timeout for read(), sec
    ser_port.setTimeout(boost::posix_time::seconds(UART_TIMEOUT));

    // ==================
    // open GPS
    // ==================
    CGps GPS(ser_port);
    int err;
    if( (err = GPS.Init()) )
    {
        std::cout << "GPS init failed -- error = " << err << std::endl;
        return 3;
    }

    // basic loop
    // 1) get data from GPS object
    // 2) delay 1 sec
    std::string pos_val;
    bool data_present;
    
    for(int k = 0; ; k++)
    {
        // Poll PIDs
        if( (err = GPS.Poll()) )
        {

            pos_val = GPS.getBearing(data_present);
            std::cout << "Position=" << pos_val << (data_present ? "" : " - error: old data") << std::endl;
        }
        else
        {
            std::cout << "GPS poll failed -- error = " << err << std::endl;
        }

        // Delay
		sleep(READ_INTERVAL);
    }
    return 0;
}
