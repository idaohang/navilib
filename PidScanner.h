
#ifndef __CPIDSCANNER_H__
#define __CPIDSCANNER_H__

#include <map>
#include <string>
#include "CSerialPort.h"
#include "CRecurrent.h"

// --------------------------------------------
// this object keeps all raw pids as map of string pairs (pid name, value)
class CPidRecord : public std::map<std::string, std::string>
{
public:
   CPidRecord() {};
   virtual ~CPidRecord() {};
   friend std::ostream& operator<< (std::ostream& o, CPidRecord const& rec);
private:

};

// --------------------------------------------
// PID-scanner object
class CPidScanner : public CRecurrent
{
public:
   CPidScanner(CUart & port, int poll_interval = 1) : CRecurrent(poll_interval), m_port (port), m_initialized(false) {};
   virtual ~CPidScanner() { m_port.close(); };

   /// Standard OBD Modes
   enum OBD_Mode
   {
     OBDII_MODE_SHOW_CURRENT_DATA                     =  (0x01),
     OBDII_MODE_FREEZE_FRAME_DATA                     =  (0x02),
     OBDII_MODE_SHOW_STORED_DTC                       =  (0x03),
     OBDII_MODE_CLEAR_DTC_AND_STORED_VALUES           =  (0x04),
     OBDII_MODE_TEST_RESULTS_O2_SENSOR_MONITORING     =  (0x05),
     OBDII_MODE_TEST_RESULTS_OTHER_SENSOR_MONITORING  =  (0x06),
     OBDII_MODE_SHOW_PENDING_DTC                      =  (0x07),
     OBDII_MODE_CONTROL_OPERATION                     =  (0x08),
     OBDII_MODE_REQUEST_VEHICLE_INFORMATION           =  (0x09),
     OBDII_MODE_PERMANENT_DTC                         =  (0x0A)
    };

    /// Standard OBD PIDs
   enum OBD_Pid
   {
     OBDII_PID_CALCULATED_ENGINE_LOAD_VALUE           =  (0x04),
     OBDII_PID_ENGINE_COOLANT_TEMPERATURE             =  (0x05),
     OBDII_PID_SHORT_TERM_FUEL_BANK_1                 =  (0x06),
     OBDII_PID_LONG_TERM_FUEL_BANK_1                  =  (0x07),
     OBDII_PID_FUEL_PRESSUE                           =  (0x0A),
     OBDII_PID_ENGINE_RPM                             =  (0x0C),
     OBDII_PID_VEHICLE_SPEED                          =  (0x0D),
     OBDII_PID_THROTTLE_POSITION                      =  (0x11),
     OBDII_PID_FUEL_LEVEL_INPUT                       =  (0x2F),
     OBDII_PID_ECU_VOLTAGE                            =  (0x42),
     };

     /// processResponse return values
    enum ResponseStatus
    {
       OK                 ,
       HEX_DATA           ,
       BUS_BUSY           ,
       BUS_ERROR          ,
       BUS_INIT_ERROR     ,
       UNABLE_TO_CONNECT  ,
       CAN_ERROR          ,
       DATA_ERROR         ,
       DATA_ERROR2        ,
       ERR_NO_DATA        ,
       BUFFER_FULL        ,
       SERIAL_ERROR       ,
       UNKNOWN_CMD        ,
       RUBBISH            ,

       INTERFACE_ID       ,
       INTERFACE_ELM320   ,
       INTERFACE_ELM322   ,
       INTERFACE_ELM323   ,
       INTERFACE_ELM327   ,
       INTERFACE_OBDLINK  ,
       STN_MFR_STRING     ,
       ELM_MFR_STRING     ,
       // not response, but timeout of read_until()
       READ_TIMEOUT
    };
    
    /// readResponse returned data type
    enum ResponseType
    {
        DATA, // ok, some data 
        EMPTY, // empty response
        READ_ERROR, // read error
        TIMEOUT // read timeout
    };

   /**
   * Initialize ELM
   * - ATZ reset to default
   * - ATE0 set echo mode off
   * - ATL0 set linefeed mode
   * - set optional params from config string
   * - ATSP3 - set protocol ISO 9141-2 (???)
   * - TODO: Add ignition on/off handling (???)
   * - ATSI - slow init (takes 2-3 sec)
   * \param ???
   * \return 0 - OK, 1 - timeout, 2 - simulation mode on
   */
   int Init();

   /**
   * Polls all PIDs with interval "poll_interval"
   * \return 0 - OK, otherwise - error
   */
   int Poll();

   /**
   * Get last polled speed.
   * \param 
   * [out] present - true if PID present, otherwise false
   * \return speed - returned pid value 
   */
   float getSpeed(bool & present);

   /**
   * Get last polled RPM.
   * \param 
   * [out] present - true if PID present, otherwise false
   * \return rpm - returned pid value 
   */
   float getRpm(bool & present);

   /**
   * Get last polled fuel level value in % of full tank
   * \param 
   * [out] present - true if PID present, otherwise false
   * \return rpm - returned pid value 
   */
   float getFuel(bool & present);

   /**
   * Get last polled throttle position.
   * \param 
   * [out] present - true if PID present, otherwise false
   * \return rpm - returned pid value 
   */
   float getThrottle(bool & present);

   /**
   * Get last polled odometer data.
   * \param 
   * [out] present - true if PID present, otherwise false
   * \return rpm - returned pid value 
   */
   float getOdometer(bool & present);

   /**
   * Get last polled voltage on board.
   * \param 
   * [out] present - true if PID present, otherwise false
   * \return rpm - returned pid value 
   */
   float getVoltage(bool & present);

   /**
   * Polls car speed.
   * \return true if PID received, otherwise false
   */
   bool pollSpeed();

   /**
   * Polls RPM.
   * \return true if PID received, otherwise false
   */
   bool pollRpm();

   /**
   * Polls fuel level.
   * \return true if PID received, otherwise false
   */
   bool pollFuel();

   /**
   * Polls throttle position.
   * \return true if PID received, otherwise false
   */
   bool pollThrottle();

   /**
   * Polls odometer data.
   * \return true if PID received, otherwise false
   */
   bool pollOdometer();

   /**
   * Polls voltage on ECU.
   * \return true if PID received, otherwise false
   */
   bool pollVoltage();

   /**
   * Send string of "mode" + "pid".
   * Ignores and removes empty lines.
   * \param 
   * [in] mode - 
   * [in] pid - 
   * [out] pid_val - returned pid value as string
   * \return true if PID received, otherwise false
   */
   bool pollPid(int mode, int pid, std::string & pid_val);
   
   /**
    * Get available at the moment PIDs
    * \param 
    * [out] std::map <string PID name> <string PID value>
    * \return number of PIDs
    */
    int getPids(CPidRecord & record);


private:

    typedef struct {
        float value;
        bool present;
    } pid_elem;

    pid_elem m_speed;
    pid_elem m_rpm;
    pid_elem m_fuel;
    pid_elem m_throttle;
    pid_elem m_odometer;
    pid_elem m_voltage;

  /**
   * Send string and expect answer string with timeout.
   * Ignores and removes empty lines.
   * \param 
   * [in] send_str - string to send
   * [in] expt_str - expected string
   * [out] rcv_srr - returned string
   * [in] timeout - timeout, msec, default 1000 msec
   * \return ResponseStatus
   */
   ResponseStatus sendExpect(std::string send_str, std::string exp_str, std::string &rcv_str, int timeout = 1000);

   /**
   * Process responses from ELM device
   * \param 
   * [in] command sent
   * [in] response received
   * \return next state
   */
   ResponseStatus processResponse(std::string &resp_recv);

   /**
   * Send command to ELM device
   * Adds EOL character at the end
   * \param 
   * AT command to send
   */
   void sendCommand(std::string cmd);

   /**
   * Read response from ELM device
   * \param 
   * [in] timeout, msec
   * [in] delimiter string
   * [out] response string
   * \return 
   * PROMPT - ELM prompt ">" received
   * EMPTY - empty string received
   * DATA - ELM data received
   * TIMEOUT - timeout expired
   */
   ResponseType readResponse(std::string &resp_recv, int timeout, const std::string& delim = "\r");
    

    /// OBD timeouts
    enum OBD_Timeout
    {
        OBD_REQUEST_TIMEOUT  = 9900,
        ATZ_TIMEOUT          = 1500,
        AT_TIMEOUT           = 500,
        ECU_TIMEOUT          = 5000
    };
   
   /// UART port object
   CUart & m_port;

   /// Init flag
   bool m_initialized;

};

#endif // __CPIDSCANNER_H__


