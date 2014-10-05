
#ifndef __CGPS_H__
#define __CGPS_H__

#include <map>
#include <string>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include "CSerialPort.h"
#include "CRecurrent.h"

using namespace std;
using namespace boost;

// --------------------------------------------
// GPS NMEA parser class
class CGps : public CRecurrent
{
public:
    CGps(CUart & port = *(CUart*)NULL, int poll_interval = 1) : 
	  CRecurrent(poll_interval), 
	  m_reads(0),
	  m_bearing("NO_BEARINGS_YET"),
	  m_port(port), 
	  m_initialized(false)  {};
   
    virtual ~CGps() { if (&m_port) m_port.close(); };


   /**
   * Initialize GPS chip
   * \return 0 - OK, 1 - timeout
   */
   int Init()
   {
        m_initialized = true;
        return 0;
   };

   /**
   * Polls with interval "poll_interval"
   * \return 0 - OK, otherwise - error
   */
   int Poll()
   {
      if(isPollTime())
      {
        if(pollBearing())
        {
            return 1;
        }
        return -1;
      }
      return 0;
   };

   /**
   * Get last polled bearing.
   * \param 
   * [out] present - true if data present, otherwise false
   * \return bearing - returned bearing string 
   */
   std::string getBearing(bool & present)
   {
        present = true;
        return m_bearing;
   };

  /**
    * Polls bearing.
    * \return true if OK, otherwise false
    */
    bool parseSentence(std::string & resp_str)
    {
      size_t tpos;
      std::vector< std::string > split_vector;
      std::string sentnc_str;
      bool ret = false;
      
      if( (tpos = resp_str.find("$GPRMC")) != std::string::npos)
      {
          /*
                  GPRMC data
                  1    = UTC of position fix
                  2    = Data status (V=navigation receiver warning)
                  3    = Latitude of fix
                  4    = N or S
                  5    = Longitude of fix
                  6    = E or W
                  7    = Speed over ground in knots
                  8    = Track made good in degrees True
                  9    = UT date
                  10   = Magnetic variation degrees (Easterly var. subtracts from true course)
                  11   = E or W
                  12   = Checksum
                  */

          sentnc_str = resp_str.substr(tpos);
          if(checksumOK(sentnc_str))
          {
              boost::split( split_vector, sentnc_str, is_any_of(","), token_compress_off );
              if(split_vector.size() >= 11)
              {
                  // concatenate words together
                  m_bearing = convertMMSS(split_vector[3]) + " " + split_vector[4] + ", "; 
                  m_bearing += convertMMSS(split_vector[5]) + " " + split_vector[6]; 
		  
		  m_reads = 0;
                  ret = true;
              }
          }
      }
      else if( (tpos = resp_str.find("$GPGLL")) != std::string::npos)
      {
        /*
              eg3. $GPGLL,5133.81,N,00042.25,W*75
                             1    2         3    4              5
      
                    1    5133.81   Current latitude
                    2    N         North/South
                    3    00042.25  Current longitude
                    4    W         East/West
                    5    *75       checksum        
               */

        sentnc_str = resp_str.substr(tpos);
        if(checksumOK(sentnc_str))
        {
              boost::split( split_vector, sentnc_str, is_any_of(","), token_compress_off );
              if(split_vector.size() >= 5)
              {
                  m_bearing = convertMMSS(split_vector[1]) + " " + split_vector[2] + ", "; 
                  m_bearing += convertMMSS(split_vector[3]) + " " + split_vector[4]; 
                  
		  m_reads = 0;
		  ret = true;
              }
        }
      }
      else if( (tpos = resp_str.find("$GPGGA")) != std::string::npos)
      {
        /*
              eg. $GPGGA,211733.00,5618.27292,N,04404.72176,E,1,07,1.21,250.1,M,6.2,M,,*69
                             1           2     3      4     5 6  7   8   9    10 11 12   14 
      
		    1    211733.00    UTC of position fix
                    2    5618.27292   Current latitude
                    3    N            North/South
                    4    04404.72176  Current longitude
                    5    E            East/West
                    14   *75          checksum        
               */

        sentnc_str = resp_str.substr(tpos);
        if(checksumOK(sentnc_str))
        {
              boost::split( split_vector, sentnc_str, is_any_of(","), token_compress_off );
              if(split_vector.size() >= 13)
              {
                  m_bearing = convertMMSS(split_vector[2]) + " " + split_vector[3] + ", "; 
                  m_bearing += convertMMSS(split_vector[4]) + " " + split_vector[5]; 
                  
		  m_reads = 0;
		  ret = true;
              }
        }
      }
      return ret;
    }

   /**
   * Polls bearing.
   * \return true if OK, otherwise false
   */
   bool pollBearing()
   {
        ReadStatus resp_status;
        std::string resp_str;
	
        // Read NMEA sentence from GPS UART
        resp_status = readSentence(resp_str);
	
	if(resp_status != DATA)
        {
	    checkIfBearingsExpired();
	    return false;
        }
        // Parse the sentence
        if(!parseSentence(resp_str))
	{
	    checkIfBearingsExpired();
	    return false;
	}
	return true;
   };

private:

    enum ReadStatus
    {
        DATA, // ok, some data 
        EMPTY, // empty response
        READ_ERROR, // read error
        TIMEOUT // read timeout
    };
    
    // --------------------------------------------------------------
    ReadStatus readSentence(std::string &resp_recv, int timeout = GPS_REQUEST_TIMEOUT, const std::string& delim = "\r\n")
    {
       ReadStatus res; 
       int rc;
    
       resp_recv.clear();
    
       m_port.setTimeout(boost::posix_time::milliseconds(timeout));
    
       rc = m_port.readStringUntil(resp_recv, delim);
    
       if(rc == CUart::resultTimeoutExpired)
       {
#if DEBUG
          printf("%s -- timeout\n", __FUNCTION__);
#endif
          res = TIMEOUT; // timeout expired
       }
       else if(rc == CUart::resultError)
       {
#if DEBUG
          printf("%s -- read error\n", __FUNCTION__);
#endif
          res = READ_ERROR; // read error
       }
       else if (resp_recv.empty())
       {
#if DEBUG
          printf("%s -- empty\n", __FUNCTION__);
#endif
          res = EMPTY; // empty string received
       }
       else
       {
#if DEBUG
          printf("%s -- data: %s\n", __FUNCTION__, resp_recv.c_str());
#endif
          res = DATA; // ELM data received
       }
       return res;
    }

    // parse "[m]mmss.xxxx" to "mm ss xx"
    std::string convertMMSS(const std::string & inp)
    {
        std::string mm, ss;
        char xxbuf[32];
        size_t ppos = inp.find(".");
        if(ppos != std::string::npos)
        {
            if(ppos <= 4)
            {
              mm = inp.substr(0, 2);
              ss = inp.substr(2, 2);
            }
            else
            {
              mm = inp.substr(0, 3);
              ss = inp.substr(3, 2);
            }
            int xxv = 60*atof(inp.substr(ppos).c_str());
            snprintf(xxbuf, sizeof(xxbuf), "%02d", xxv);
            
        }
        return mm + " " + ss + " " + xxbuf;
    };

    bool checksumOK(const std::string & str)
    {
        bool ret = false;
        size_t ppos = str.find("$");
        size_t cpos = str.find("*");
        uint16_t crc = 0;
        if(ppos != std::string::npos &&
           cpos != std::string::npos )
        {
            for(size_t pos = ppos+1; pos < cpos; pos++)
            {
                crc ^= static_cast<uint16_t>(str[pos]);
            }
            char* ptr_dummy;
            uint16_t crc_expected = strtoul(str.substr(cpos + 1).c_str(), &ptr_dummy, 16);
            if(crc == crc_expected)
            {
                //printf("CRC ok. Expect=%u calc=%u\n", crc_expected, crc);
                ret = true;
            }
            else
            {
#if DEBUG
                printf("CRC error. Expect=%u calc=%u\n", crc_expected, crc);
#endif
            }
        }
        return ret;
    };

    /// see comments for MAX_MSGS_WIHTOUT_BEARINGS for explanation
    void checkIfBearingsExpired()
    {
	if(++m_reads > MAX_MSGS_WIHTOUT_BEARINGS)
	    m_bearing = "NO_BEARINGS";
    }
    
    /// GPS timeouts
    enum GPS_Timeout
    {
        GPS_REQUEST_TIMEOUT  = 1000 // msec
    };

    /* Not every GPS message contains bearings. We extract them when they are present and skip messages
     * that do not contain them. But when GSP unit is not working readSentence() will return something
     * besides DATA. In that case we increment m_reads and if it becomes greater than MAX_MSGS_WIHTOUT_BEARINGS
     * we know that something went wrong with GPS unit.
     */
    static const int MAX_MSGS_WIHTOUT_BEARINGS = 10;
    
    /// See comment for MAX_MSGS_WIHTOUT_BEARINGS
    int m_reads;
    
   /// Position string
   std::string m_bearing;

   /// UART port object
   CUart & m_port;

   /// Init flag
   bool m_initialized;

   
   /// readResponse returned data type
};

#endif // __CGPS_H__


