// PID scanner
// ELM device handler


#include "PidScanner.h"
#include <boost/thread.hpp> // sleep()
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>
#include <string>

using namespace std;
using namespace boost;
// --------------------------------------------------------------
int CPidScanner::Init()
{
   std::string rcv_str;
   ResponseStatus resp_status;
   enum InitState
   {
      RESET_SEND,
      CUSTOM_INIT,
      WAIT_ECU_TIMEOUT,
      DETECT_PROTOCOL,
      END_INIT
   };

   m_initialized = true;

   if( ! m_port.isOpen())
   {
      return 2;
   }

   InitState state = RESET_SEND;
   // ATZ, reset to NVRAM default
   std::string cmd;
   std::string custom_init = "ATI";

   // OPA japan JOBD init commands
    std::string opa_custom_init[] = 
    {
        "atib96",
        "atiia13",
        "atsh8113F1",
    };

   for(;;)
   {
      switch(state)
      {
      case RESET_SEND:
         cmd = "ATZ";
         resp_status = sendExpect(cmd, ">", rcv_str, ATZ_TIMEOUT);
         if(INTERFACE_ELM323 != resp_status
            && INTERFACE_ELM327 != resp_status
            && INTERFACE_ELM322 != resp_status
            && INTERFACE_ELM320 != resp_status)
         {
            return 1;
         }
        state = CUSTOM_INIT;
         break;
      case CUSTOM_INIT:
         resp_status = sendExpect(custom_init, ">", rcv_str, AT_TIMEOUT);
         if(OK == resp_status
            || INTERFACE_ELM323 == resp_status
            || INTERFACE_ELM327 == resp_status
            || INTERFACE_ELM322 == resp_status
            || INTERFACE_ELM320 == resp_status)
         {
            state = WAIT_ECU_TIMEOUT;
         }
         else if(resp_status == UNKNOWN_CMD)
         {
            // rewrite custom command and try again
            // TODO: probably just return error ?
            custom_init = "ATI";
            state = RESET_SEND;
         }
         else
         {
            return 1;
         }
         break;
      case WAIT_ECU_TIMEOUT:
         boost::this_thread::sleep(boost::posix_time::millisec(ECU_TIMEOUT));
         state = DETECT_PROTOCOL;
         break;
      case DETECT_PROTOCOL:
         cmd = "0100";
         resp_status = sendExpect(cmd, ">", rcv_str, AT_TIMEOUT);
         if(HEX_DATA != resp_status)
         {
             return 1;
         }
         state = END_INIT;
         break;
      case END_INIT:
         return 0;
         break;
      default:
         printf("ERROR: unknown state %d\n", state);
         return 2;
         break;
      } // switch(state)
   } // for(;;)
   return 0;
}

// --------------------------------------------------------------
float CPidScanner::getSpeed(bool & present)
{
    present = m_speed.present;
    return m_speed.value;
}
// --------------------------------------------------------------
float CPidScanner::getRpm(bool & present)
{
    present = m_rpm.present;
    return m_rpm.value;
}
// --------------------------------------------------------------
float CPidScanner::getFuel(bool & present)
{
    present = m_fuel.present;
    return m_fuel.value;
}
// --------------------------------------------------------------
float CPidScanner::getThrottle(bool & present)
{
    present = m_throttle.present;
    return m_throttle.value;
}
// --------------------------------------------------------------
float CPidScanner::getOdometer(bool & present)
{
    present = m_odometer.present;
    return m_odometer.value;
}
// --------------------------------------------------------------
float CPidScanner::getVoltage(bool & present)
{
    present = m_voltage.present;
    return m_voltage.value;
}
// --------------------------------------------------------------
bool CPidScanner::pollSpeed()
{
    std::string pid_str;
    uint16_t mode = OBDII_MODE_SHOW_CURRENT_DATA;
    uint16_t pid = OBDII_PID_VEHICLE_SPEED;
    bool ret = false;

    m_speed.present = false;
    if((ret = pollPid(mode, pid, pid_str)))
    {
       char * ptr;
       uint16_t val = (uint16_t)strtoul( pid_str.c_str(), & ptr, 16 );
       // Mode(hex)   PID(hex)   Data bytes returned     Description     Min value   Max value   Units   Formula
       // 01 	0D 	1 	Vehicle speed 	0 	255 	km/h 	A

       m_speed.value = val;
       m_speed.present = true;
    }
    return ret;
}
// --------------------------------------------------------------
bool CPidScanner::pollRpm()
{
    std::string pid_str;
    uint16_t mode = OBDII_MODE_SHOW_CURRENT_DATA;
    uint16_t pid = OBDII_PID_ENGINE_RPM;
    bool ret = false;

    m_rpm.present = false;
    if((ret = pollPid(mode, pid, pid_str)))
    {
       char * ptr;
       uint16_t val = (uint16_t)strtoul( pid_str.c_str(), & ptr, 16 );
       // Mode(hex)   PID(hex)   Data bytes returned     Description     Min value   Max value   Units   Formula
       // 01 	0C 	2 	Engine RPM 	0 	16,383.75 	rpm 	((A*256)+B)/4
       // Note: (A*256)+B already done in pollPid()
       m_rpm.value = val >> 2;
       m_rpm.present = true;
    }
    return ret;
}
// --------------------------------------------------------------
bool CPidScanner::pollFuel()
{
    std::string pid_str;
    uint16_t mode = OBDII_MODE_SHOW_CURRENT_DATA;
    uint16_t pid = OBDII_PID_FUEL_LEVEL_INPUT;
    bool ret = false;

    m_fuel.present = false;
    if((ret = pollPid(mode, pid, pid_str)))
    {
       char * ptr;
       uint16_t val = (uint16_t)strtoul( pid_str.c_str(), & ptr, 16 );
       // Mode(hex)   PID(hex)   Data bytes returned     Description     Min value   Max value   Units   Formula
       // 01 	2F 	1 	Fuel Level Input 	0 	100 	 % 	100*A/255
       m_fuel.value = (float)val / 2.55; // value in % of full tank
       m_fuel.present = true;
    }
    return ret;
}
// --------------------------------------------------------------
bool CPidScanner::pollThrottle()
{
    std::string pid_str;
    uint16_t mode = OBDII_MODE_SHOW_CURRENT_DATA;
    uint16_t pid = OBDII_PID_THROTTLE_POSITION;
    bool ret = false;

    m_throttle.present = false;
    if((ret = pollPid(mode, pid, pid_str)))
    {
       char * ptr;
       uint16_t val = (uint16_t)strtoul( pid_str.c_str(), & ptr, 16 );
       // Mode(hex)   PID(hex)   Data bytes returned     Description     Min value   Max value   Units   Formula
       // 01 	11 	1 	Throttle position 	0 	100 	 % 	A*100/255
       m_throttle.value = (float)val / 2.55;
       m_throttle.present = true;
    }
    return ret;
}
// --------------------------------------------------------------
bool CPidScanner::pollOdometer()
{
    bool ret = false;

    m_odometer.present = false;

    // TODO: no standard PID for odometer :-( so we need some replacement
    // TODO: remove next line from production code
#ifndef PRODUCTION_CODE
    return ret;

#else
    std::string pid_str;
    uint16_t mode = OBDII_MODE_SHOW_CURRENT_DATA;
    uint16_t pid = OBDII_PID_????;

    if(ret = pollPid(mode, pid, pid_str))
    {
       char * ptr;
       uint16_t val = (uint16_t)strtoul( pid_str.c_str(), & ptr, 16 );
       // Mode(hex)   PID(hex)   Data bytes returned     Description     Min value   Max value   Units   Formula
       // ???????
       m_odometer.value = (float)val / 2.55;
       m_odometer.present = true;
    }
    return ret;
#endif
}
// --------------------------------------------------------------
bool CPidScanner::pollVoltage()
{
    std::string pid_str;
    uint16_t mode = OBDII_MODE_SHOW_CURRENT_DATA;
    uint16_t pid = OBDII_PID_ECU_VOLTAGE;
    bool ret = false;

    m_voltage.present = false;
    if((ret = pollPid(mode, pid, pid_str)))
    {
       char * ptr;
       uint16_t val = (uint16_t)strtoul( pid_str.c_str(), & ptr, 16 );
       // Mode(hex)   PID(hex)   Data bytes returned     Description     Min value   Max value   Units   Formula
       // 01 	42 	2 	Control module voltage 	0 	65.535 	V 	((A*256)+B)/1000
       m_voltage.value = (float)val * 1e-3;
       m_voltage.present = true;
    }
    return ret;
}
// --------------------------------------------------------------
bool CPidScanner::pollPid(int mode, int pid, std::string & pid_str)
{
    ResponseStatus resp_status;
    std::string resp_str;
    bool ret = false;

    std::string cmd = str( boost::format("%02X %02X") % mode % pid ); 
    resp_status = sendExpect(cmd, ">", resp_str, AT_TIMEOUT);
    if(HEX_DATA == resp_status)
    {
        std::vector< std::string > split_vector;
        boost::split( split_vector, resp_str, is_any_of(" "), token_compress_on );
        if(split_vector.size() >= 3)
        {
            char * ptr;
            uint16_t mode_recv = (uint16_t)strtoul( split_vector[0].c_str(), & ptr, 16 ); 
            uint16_t pid_recv  = (uint16_t)strtoul( split_vector[1].c_str(), & ptr, 16 ); 
            if(mode == (mode_recv & ~0x40) && pid_recv == pid)
            {
                pid_str = split_vector[2];
                if(split_vector.size() > 3)
                {
                    pid_str += split_vector[3];
                }
                ret = true;
            }
        }
    }
    return ret;
}

// --------------------------------------------------------------
int CPidScanner::Poll()
{
    assert(m_initialized);
    
    int rc = 0;
    if(isPollTime())
    {
        pollFuel();
        pollOdometer();
        pollRpm();
        pollSpeed();
        pollThrottle();
        pollVoltage();
        rc = 1;
    }
    return rc;
}
// --------------------------------------------------------------
CPidScanner::ResponseStatus CPidScanner::sendExpect(std::string cmd, std::string exp_str, std::string &rcv_str, int timeout)
{
    ResponseType resp_type;
    ResponseStatus resp_status;

    sendCommand(cmd);
    resp_type = readResponse(rcv_str, timeout, exp_str);
    if(resp_type == TIMEOUT)
    {
        return READ_TIMEOUT;
    }
    // check if echo ON and switch it off (and linefeed off) if any
    size_t tpos;
    if( (tpos = rcv_str.find(cmd)) != std::string::npos)
    {
        std::string tmp_resp;
        sendCommand("ATE0");
        resp_type = readResponse(tmp_resp, AT_TIMEOUT, ">");
        if(resp_type == TIMEOUT)
        {
            return READ_TIMEOUT;
        }
        sendCommand("ATL0");
        resp_type = readResponse(tmp_resp, AT_TIMEOUT, ">");
        if(resp_type == TIMEOUT)
        {
            return READ_TIMEOUT;
        }
    }
    // Process response
    resp_status = processResponse(rcv_str);
    return resp_status;
}

// --------------------------------------------------------------
void CPidScanner::sendCommand(std::string cmd)
{
   m_port.writeString(cmd + "\r\n");
}

// --------------------------------------------------------------
CPidScanner::ResponseType CPidScanner::readResponse(std::string &resp_recv, int timeout, const std::string& delim)
{
   ResponseType res; 
   int rc;

   resp_recv.clear();

   m_port.setTimeout(boost::posix_time::milliseconds(timeout));

   rc = m_port.readStringUntil(resp_recv, delim);

   if(rc == CUart::resultTimeoutExpired)
   {
#if DEBUGMODE
      printf("%s -- timeout\n", __FUNCTION__);
#endif
      res = TIMEOUT; // timeout expired
   }
   else if(rc == CUart::resultError)
   {
#if DEBUGMODE
      printf("%s -- read error\n", __FUNCTION__);
#endif
      res = READ_ERROR; // read error
   }
   else if (resp_recv.empty())
   {
#if DEBUGMODE
      printf("%s -- empty\n", __FUNCTION__);
#endif
      res = EMPTY; // empty string received
   }
   else
   {
#if DEBUGMODE
      printf("%s -- data: %s\n", __FUNCTION__, resp_recv.c_str());
#endif
      res = DATA; // ELM data received
   }
   return res;
}

// --------------------------------------------------------------
CPidScanner::ResponseStatus CPidScanner::processResponse(std::string &msg_recv)
{
   uint16_t i = 0;
   bool is_hex_num = true;
   size_t tpos;


   size_t msg_pos = 0;
   // skip and erase unneeded responses
   if ((tpos = msg_recv.find("SEARCHING...", msg_pos)) != std::string::npos)
   {
      msg_pos += tpos;
   }
   else if ((tpos = msg_recv.find("BUS INIT: OK", msg_pos)) != std::string::npos)
   {
      msg_pos += tpos;
   }
   else if ((tpos = msg_recv.find("BUS INIT: ...OK", msg_pos)) != std::string::npos)
   {
      msg_pos += tpos;
   }
   // erase all the found
   msg_recv.erase(0, msg_pos);

   for(i = 0; i < msg_recv.size(); i++) //loop to check data
   {
      if (msg_recv[i] > ' ')  // if the character is not a special character or space
      {
         if (msg_recv[i] == '<') // Detect <DATA_ERROR
         {
            if (msg_recv.find("<DATA ERROR", i) != std::string::npos)
               return DATA_ERROR2;
            else
               return RUBBISH;
         }
         if (!isxdigit(msg_recv[i]) && msg_recv[i] != ':')
            is_hex_num = false;
         i++;
      }
   }

   if (is_hex_num)
   {
      return HEX_DATA;
   }

   if (msg_recv.find("NODATA") != std::string::npos)
      return ERR_NO_DATA;
   if (msg_recv.rfind("UNABLETOCONNECT") != std::string::npos)
      return UNABLE_TO_CONNECT;
   if (msg_recv.rfind("OK") != std::string::npos)
      return OK;
   if (msg_recv.rfind("BUSBUSY") != std::string::npos)
      return BUS_BUSY;
   if (msg_recv.rfind("DATAERROR") != std::string::npos)
      return DATA_ERROR;
   if (msg_recv.rfind("BUSERROR") != std::string::npos ||
       msg_recv.rfind("FBERROR") != std::string::npos)
      return BUS_ERROR;
   if (msg_recv.rfind("CANERROR") != std::string::npos)
      return CAN_ERROR;
   if (msg_recv.rfind("BUFFERFULL") != std::string::npos)
      return BUFFER_FULL;
   if (msg_recv.find("BUSINIT:ERROR") != std::string::npos ||
       msg_recv.find("BUSINIT:...ERROR") != std::string::npos)
      return BUS_INIT_ERROR;
   if (msg_recv.find("BUS INIT:")  != std::string::npos ||
       msg_recv.find("BUS INIT:...") != std::string::npos)
      return SERIAL_ERROR;
   if (msg_recv.find("?") != std::string::npos)
      return UNKNOWN_CMD;
   if (msg_recv.find("ELM320") != std::string::npos)
      return INTERFACE_ELM320;
   if (msg_recv.find("ELM322") != std::string::npos)
      return INTERFACE_ELM322;
   if (msg_recv.find("ELM323") != std::string::npos)
      return INTERFACE_ELM323;
   if (msg_recv.find("ELM327") != std::string::npos)
      return INTERFACE_ELM327;
   if (msg_recv.find("OBDLink") != std::string::npos)
      return INTERFACE_OBDLINK;
   if (msg_recv.find("SCANTOOL.NET") != std::string::npos)
      return STN_MFR_STRING;
   if (msg_recv.find("OBDIItoRS232Interpreter") != std::string::npos)
      return ELM_MFR_STRING;
   
   return RUBBISH;
}

// ------------------------------
int CPidScanner::getPids(CPidRecord & record)
{
   int n = 0, rc;
   std::string data_str(64, '\0');
   rc = m_port.readString(data_str);
   record.insert(std::make_pair("speed", data_str));
   n++;
   record.insert(std::make_pair("throttle", data_str));
   n++;
   record.insert(std::make_pair("pids", data_str));
   n++;
   return n;
}

// ------------------------------
// PID Record operator <<
std::ostream& operator<< (std::ostream& out, CPidRecord const& rec)
{
   for(CPidRecord::const_iterator ir = rec.begin(); ir != rec.end(); ++ir)
   {
      out << ir->first << " : " << ir->second << std::endl;
   }
   return out;
}

