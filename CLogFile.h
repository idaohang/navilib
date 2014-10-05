#ifndef __LOGFILE_H__
#define __LOGFILE_H__

#include <log4cpp/Portability.hh>

#ifdef WIN32
#include <windows.h>
#endif
#ifdef LOG4CPP_HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <iostream>

#include <log4cpp/Category.hh>
#include <log4cpp/PropertyConfigurator.hh>

// Wrapper class for logging based on log4cpp

class CLogFile
{
public:

    CLogFile(const char* a_configfile = "log4cpp.ini") 
    {  
       if(!a_configfile || strlen(a_configfile) ==  0)
       {
          m_configfile = "log4cpp.ini";
       }
       else
       {
          m_configfile = a_configfile;
       }

    };
    virtual ~CLogFile() { log4cpp::Category::shutdown(); };
    // Initialization method
    int Instantiate()
    {
       try {
          log4cpp::PropertyConfigurator::configure(m_configfile);
       } catch(log4cpp::ConfigureFailure& f) {
          std::cout << "Configure Problem " << f.what() << std::endl;
          return -1;
       }
       return 0;
    }

private:
   std::string m_configfile;
};

#endif // __LOGFILE_H__