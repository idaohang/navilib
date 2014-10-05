#ifndef __CGPIO_H__
#define __CGPIO_H__

#include <sys/ioctl.h>
#include <fcntl.h>
#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <cassert>
#include <typeinfo>
#include "user-gpio-drv.h"
#include "CRecurrent.h"

// -----------------------------------------
/// GPIO control class.
/// Based on gpiolib for access to GPIO from user space.
/// http://svn.hylands.org/linux/gpio/lib
class CGpio : public CRecurrent
{
private:
    enum GpioLines
    {
        GPIO_LED_LINE   = 147, // 0 - light, 1 - no light
        GPIO_GSM_LINE   = 23,
        GPIO_LTRANSA_LINE = 144,
        GPIO_LTRANSB_LINE = 22,
        /*
                GIO2 - Relay (open/close)
                GIO4 - IN1 (input 0-12v/0-5v)
                GIO5 - IN2 (input 0-12v/0-5v)
                GIO6 - OUT1 (output 0-12v)
                GIO7 - OUT2 (output 0-12v)
            */
        GPIO_GIO0  = 384, // GIO0
        GPIO_GIO1  = GPIO_GIO0 + 1, // GIO1
        GPIO_GIO2  = GPIO_GIO0 + 2, // GIO2 - Relay
        GPIO_GIO3  = GPIO_GIO0 + 3, // GIO3
        GPIO_GIO4  = GPIO_GIO0 + 4, // GIO4 - IN1
        GPIO_GIO5  = GPIO_GIO0 + 5, // GIO5 - IN2
        GPIO_GIO6  = GPIO_GIO0 + 6, // GIO6 - OUT1
        GPIO_GIO7  = GPIO_GIO0 + 7, // GIO7 - OUT2

        GPIO_RELAY_LINE = GPIO_GIO2, // 0 - open, 1 - closed
        GIO_IN1 = GPIO_GIO4,
        GIO_IN2 = GPIO_GIO5,
        GIO_OUT1 = GPIO_GIO6,
        GIO_OUT2 = GPIO_GIO7,
        NUMBER_GIO = 8, // number of GIOs
    };

public:
    CGpio(int poll_interval) : CRecurrent(poll_interval), m_initialized(false) {};
    virtual ~CGpio() { gpio_term(); };
    
    /// Initialize GPIO lines
    int Init() 
    {
        int rc;
        
        m_initialized = true;

        // Init gpio-user interface
        if ( (rc = gpio_init()) < 0 )
        {
            //perror( "gpio_init failed" );
            return rc;
        }
        // Initialize GPIO lines
        // Switch on level translators A and B
        gpio_request(GPIO_LTRANSA_LINE, "GPIO_LTRANSA");
        gpio_request(GPIO_LTRANSB_LINE, "GPIO_LTRANSB");
        gpio_direction_output(GPIO_LTRANSA_LINE, 1);
        gpio_direction_output(GPIO_LTRANSB_LINE, 1);
        // Switch off LED
        gpio_request(GPIO_LED_LINE, "GPIO_LED");
        gpio_direction_output(GPIO_LED_LINE, 0);
        // Switch off Relay
        gpio_request(GPIO_RELAY_LINE, "GIO2_RELAY");
        gpio_direction_output(GPIO_RELAY_LINE, 0);
        // Switch on GSM 
        gpio_request(GPIO_GSM_LINE, "GPIO_GSM");
        gpio_direction_output(GPIO_GSM_LINE, 1);

        // Init ext lines 1-4 for output and switch off them. Should be done with I2C driver for PortExtender.
        gpio_request(GIO_IN1, "GIO4_IN1");
        gpio_direction_input(GIO_IN1);
        gpio_request(GIO_IN2, "GIO5_IN2");
        gpio_direction_input(GIO_IN2);
        gpio_request(GIO_OUT1, "GIO6_OUT1");
        gpio_direction_output(GIO_OUT1, 0);
        gpio_request(GIO_OUT2, "GIO7_OUT2");
        gpio_direction_output(GIO_OUT2, 0);

        for(int i =0; i < NUMBER_GIO; i++)
        {
           m_gio_out_state[i] = 0xFF;
           m_gio_in_state[i] = 0xFF;
           m_gio_in_state_change[i] = 0;
        }
        return rc;
    };

    /// Poll all external GPIO lines and return its state
    int Poll()
    {
        int rc = 0;
        if(isPollTime())
        {
           pollINline(1);
           pollINline(2);
           rc = 1;
        }
        return rc;
    };

    /// Set Realy on/off
    int setRelayClosed(bool state)
    {
        int err;
        int level = 0; // 0 - open, 1 - closed
        if(state)
        {
            level = 1;
        }
        err = gpio_direction_output( GPIO_RELAY_LINE, level);
        if(!err)
        {
           m_gio_out_state[GPIO_RELAY_LINE - GPIO_GIO0] = level;
        }
        return err;
    };

    /// Get Realy state
    int getRelay()
    {
        return m_gio_out_state[GPIO_RELAY_LINE - GPIO_GIO0];
    };

    /// Switch LED on/off
    int setLedLight(bool state)
    {
        int level = 1; // 0 - light, 1 - no light
        if(state)
        {
            level = 0;
        }
        return gpio_direction_output( GPIO_LED_LINE, level);
    };

    /// Switch GSM on/off
    int setGSM(bool state)
    {
        int level = 0; // 0 - off, 1 - on
        if(state)
        {
            level = 1;
        }
        return gpio_direction_output( GPIO_GSM_LINE, state);
    };

    /// Set OUT1(GIO6), OUT2(GIO7) on/off
    // using I2C driver for PortExtender.
    // Input:
    // line - OUT number (1, 2)
    // state - true(ON), false(OFF)
    /// Note: since OUT lines are write-only, it's state saved in array m_gio_out_state[]
    int setOUTline(unsigned line, unsigned char state)
    {
        int err;
        unsigned gpio_line = 0xFFFF;

        switch(line)
        {
            case 1:
                gpio_line = GIO_OUT1;
                break;
            case 2:
                gpio_line = GIO_OUT2;
                break;
            default:
                return -1;
                break;
        }
        err = gpio_direction_output(gpio_line, state);
        if(!err)
        {
           m_gio_out_state[gpio_line - GPIO_GIO0] = state;
        }
        return err;
    };

    /// Get OUT1(GIO6), OUT2(GIO7) state
    // using internal table
    // Input:
    // line - OUT number (1, 2)
    // Output:
    // state - true(ON), false(OFF)
    int getOUTline(unsigned line)
    {
        int state = -1;
        switch(line)
        {
            case 1:
                state = m_gio_out_state[GIO_OUT1 - GPIO_GIO0];
                break;
            case 2:
                state = m_gio_out_state[GIO_OUT2 - GPIO_GIO0];
                break;
            default:
                state = -1;
                break;
        }
        return state;
    };

    /// Get IN1(GIO4). IN2(GIO5) state
    // using internal table
    // Input:
    // line - IN number (1, 2)
    // Output:
    // state - true(ON), false(OFF)
    int getINline(unsigned line)
    {
        int state = -1;
        switch(line)
        {
            case 1:
                state = m_gio_in_state[GIO_IN1- GPIO_GIO0];
                break;
            case 2:
                state = m_gio_in_state[GIO_IN2 - GPIO_GIO0];
                break;
            default:
                state = -1;
                break;
        }
        return state;
    };

    /// Get state change of IN1(GIO4). IN2(GIO5)
    // using internal table
    // Input:
    // line - IN number (1, 2)
    // Output:
    // state - 0 - no change, 1 - from 0 to 1, -1 - from 1 to 0
    int getINlineChange(unsigned line)
    {
        int state = -1;
        switch(line)
        {
            case 1:
                state = m_gio_in_state_change[GIO_IN1- GPIO_GIO0];
                break;
            case 2:
                state = m_gio_in_state_change[GIO_IN2 - GPIO_GIO0];
                break;
            default:
                state = -1;
                break;
        }
        return state;
    };

    /// Poll state of IN1(GIO4). IN2(GIO5)
    // using I2C driver for PortExtender.
    // Input:
    // line - IN number (1, 2)
    // Output:
    // line state if OK
    // (-errno) if error
    ///  Note: previous state is saved (if changed) in m_gio_in_state[] to find state change
    int pollINline(unsigned line)
    {
        int rc = 0;
        unsigned gpio_line = 0xFFFF;
        switch(line)
        {
            case 1:
                gpio_line = GIO_IN1;
                break;
            case 2:
                gpio_line = GIO_IN2;
                break;
            default:
                return -1;
                break;
        }
        rc = gpio_direction_input(gpio_line);
        if(rc == 0)
        {
            int state;
            state = gpio_get_value( gpio_line);
            if(state >= 0) 
            {
               if(state > m_gio_in_state[gpio_line - GPIO_GIO0])
               {
                  m_gio_in_state[gpio_line - GPIO_GIO0] = state;
                  m_gio_in_state_change[gpio_line - GPIO_GIO0] = 1;
               }
               else if (state < m_gio_in_state[gpio_line - GPIO_GIO0])
               {
                  m_gio_in_state[gpio_line - GPIO_GIO0] = state;
                  m_gio_in_state_change[gpio_line - GPIO_GIO0] = -1;
               }
               else
               {
                  m_gio_in_state_change[gpio_line - GPIO_GIO0] = 0;
               }
            }                  
            return state;
        }
        return rc;
    };

private:
    
    bool m_initialized;
    unsigned m_gio_out_state[NUMBER_GIO]; // memory for state of OUT lines (they are write-only so we need to emulate read)
    unsigned m_gio_in_state[NUMBER_GIO];
    int16_t  m_gio_in_state_change[NUMBER_GIO];

    static int  gFd;

    ///   Opens the GPIO device
    int gpio_init( void )
    {
        if ( gFd < 0 )
        {
            if (( gFd = open( "/dev/user-gpio", O_RDWR )) < 0 )
            {
                return -1;
            }
        }
        return 0;
    }

    ///   Terminates the GPIO library.
    void gpio_term( void )
    {
        if ( gFd >= 0 )
        {
            close( gFd );
            gFd = -1;
        }
    }

    ///   Requests a GPIO - fails if the GPIO is already "owned"
    ///   Returns 0 on success, -1 on error.
    int  gpio_request( unsigned gpio, const char *label )
    {
        GPIO_Request_t  request;

        request.gpio = gpio;
        strncpy( request.label, label, sizeof( request.label ));
        request.label[ sizeof( request.label ) - 1 ] = '\0';

        if ( ioctl( gFd, GPIO_IOCTL_REQUEST, &request ) != 0 )
        {
            return -errno;
        }
        return 0;
    }

    ///   Releases a previously request GPIO
    void gpio_free( unsigned gpio )
    {
        ioctl( gFd, GPIO_IOCTL_FREE, gpio );
    }

    ///   Configures a GPIO for input
    int  gpio_direction_input( unsigned gpio )
    {
        if ( ioctl( gFd, GPIO_IOCTL_DIRECTION_INPUT, gpio ) != 0 )
        {
            return -errno;
        }
        return 0;
    }

    ///   Configures a GPIO for output and sets the initial value.
    ///   Returns 0 if the direction was set successfully, negative on error.
    int  gpio_direction_output( unsigned gpio, int initialValue )
    {
        GPIO_Value_t    setDirOutput;

        setDirOutput.gpio = gpio;
        setDirOutput.value = initialValue;

        if ( ioctl( gFd, GPIO_IOCTL_DIRECTION_OUTPUT, &setDirOutput ) != 0 )
        {
            return -errno;
        }
        return 0;
    }

    ///   Retrieves the value of a GPIO pin.
    ///   Returns 0 if the pin is low, non-zero (not necessarily 1) if the pin is high.
    ///   Returns negative if an error occurs.
    int  gpio_get_value( unsigned gpio )
    {
        GPIO_Value_t    getValue;

        getValue.gpio = gpio;

        if ( ioctl( gFd, GPIO_IOCTL_GET_VALUE, &getValue ) != 0 )
        {
            return -errno;
        }
        return getValue.value;
    }

    ///   Sets the value of the GPIO pin.
    void gpio_set_value( unsigned gpio, int value )
    {
        GPIO_Value_t    setValue;

        setValue.gpio = gpio;
        setValue.value = value;

        ioctl( gFd, GPIO_IOCTL_SET_VALUE, &setValue );
    }

};

int CGpio::gFd = -1;

#endif // __CGPIO_H__
