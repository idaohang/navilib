
#include "CSerialPort.h"
#include <string>
#include <algorithm>
#include <iostream>
#include <boost/bind.hpp>

using namespace std;
using namespace boost;

CUart::CUart(): io(), port(io), timer(io),
        timeout(posix_time::milliseconds(0)) {}

CUart::CUart(const std::string& devname, unsigned int baud_rate,
        asio::serial_port_base::parity opt_parity,
        asio::serial_port_base::character_size opt_csize,
        asio::serial_port_base::flow_control opt_flow,
        asio::serial_port_base::stop_bits opt_stop)
        : io(), port(io), timer(io), timeout(posix_time::milliseconds(0))
//       , readData(m_binremove_filter)
{
    open(devname,baud_rate,opt_parity,opt_csize,opt_flow,opt_stop);
}

void CUart::open(const std::string& devname, unsigned int baud_rate,
        asio::serial_port_base::parity opt_parity,
        asio::serial_port_base::character_size opt_csize,
        asio::serial_port_base::flow_control opt_flow,
        asio::serial_port_base::stop_bits opt_stop)
{
    if(isOpen()) close();
    port.open(devname);
    port.set_option(asio::serial_port_base::baud_rate(baud_rate));
    port.set_option(opt_parity);
    port.set_option(opt_csize);
    port.set_option(opt_flow);
    port.set_option(opt_stop);
}

void CUart::open(const std::string& devname, 
        unsigned int baud_rate,
        boost::system::error_code & errcode,
        asio::serial_port_base::parity opt_parity,
        asio::serial_port_base::character_size opt_csize,
        asio::serial_port_base::flow_control opt_flow,
        asio::serial_port_base::stop_bits opt_stop)
{
    if(isOpen()) close();
    port.open(devname, errcode);
    if(errcode.value())
    {
        return;
    }
    port.set_option(asio::serial_port_base::baud_rate(baud_rate));
    port.set_option(opt_parity);
    port.set_option(opt_csize);
    port.set_option(opt_flow);
    port.set_option(opt_stop);
}

bool CUart::isOpen() const
{
    return port.is_open();
}

void CUart::close()
{
    if(isOpen()==false) return;
    port.close();
}

void CUart::setTimeout(const posix_time::time_duration& t)
{
    timeout = t;
}

void CUart::write(const char *data, size_t size)
{
    asio::write(port,asio::buffer(data,size));
}

void CUart::write(const std::vector<char>& data)
{
    asio::write(port,asio::buffer(&data[0],data.size()));
}

void CUart::writeString(const std::string& s)
{
    asio::write(port,asio::buffer(s.c_str(),s.size()));
}

int CUart::read(char *data, size_t size)
{
    if(readData.size()>0)//If there is some data from a previous read
    {
        istream is(&readData);
        size_t toRead=min(readData.size(),size);//How many bytes to read?
        is.read(data,toRead);
        data+=toRead;
        size-=toRead;
        if(size>=0) return resultSuccess;//If read data was enough, just return
    }
    
    setupParameters=ReadSetupParameters(data,size);
    performReadSetup(setupParameters);

    //For this code to work, there should always be a timeout, so the
    //request for no timeout is translated into a very long timeout
    if(timeout != posix_time::milliseconds(0)) timer.expires_from_now(timeout);
    else timer.expires_from_now(posix_time::hours(100000));
    
    timer.async_wait(boost::bind(&CUart::timeoutExpired,this,
                asio::placeholders::error));
    
    result=resultInProgress;
    bytesTransferred=0;
    for(;;)
    {
        io.run_one();
        switch(result)
        {
            case resultSuccess:
                timer.cancel();
                return result;
            case resultTimeoutExpired:
                port.cancel();
                //throw(timeout_exception("Timeout expired"));
                return result;
            case resultError:
                timer.cancel();
                port.cancel();
                //throw(boost::system::system_error(boost::system::error_code(), "Error while reading"));
                return result;
            default:
                //if resultInProgress remain in the loop
                break;
        }
    }
}

std::vector<char> CUart::read(size_t size)
{
    vector<char> out(size,'\0');//Allocate a vector with the desired size
    read(&out[0],size);//Fill it with values
    return out;
}

int CUart::readString(std::string &out)
{
   out.assign(out.capacity(), '\0');
    return read(&out[0], out.size());//Fill it with values
}

int CUart::readStringUntil(std::string &out, const std::string& delim)
{
    // Note: if readData contains some previously read data, the call to
    // async_read_until (which is done in performReadSetup) correctly handles
    // it. If the data is enough it will also immediately call readCompleted()
    setupParameters=ReadSetupParameters(delim);
    performReadSetup(setupParameters);

    //For this code to work, there should always be a timeout, so the
    //request for no timeout is translated into a very long timeout
    if(timeout != posix_time::milliseconds(0)) timer.expires_from_now(timeout);
    else timer.expires_from_now(posix_time::hours(100000));

    timer.async_wait(boost::bind(&CUart::timeoutExpired,this,
                asio::placeholders::error));

    result=resultInProgress;
    bytesTransferred=0;
    for(;;)
    {
        io.run_one();
        switch(result)
        {
            case resultSuccess:
                {
                    timer.cancel();
                    bytesTransferred-=delim.size();//Don't count delim
                    istream is(&readData);
#if 0
                    is.read(&out[0],bytesTransferred);//Fill values
#else
                    // Read to string and remove non-printables from result string
                    for(std::size_t idx = 0; idx < bytesTransferred; idx++)
                    {
                        char c = static_cast<char>(is.get());
                        if( ! is_binary(c))
                        {
                            out += c;
                        }
                    }
#endif
                    is.ignore(delim.size());//Remove delimiter from stream
                    return result;
                }
            case resultTimeoutExpired:
                port.cancel();
                //throw(timeout_exception("Timeout expired"));
                return result;
            case resultError:
                timer.cancel();
                port.cancel();
                //throw(boost::system::system_error(boost::system::error_code(), "Error while reading"));
                return result;
            default:
                //if resultInProgress remain in the loop
                break;
        }
    }
}

CUart::~CUart() {}

void CUart::performReadSetup(const ReadSetupParameters& param)
{
    if(param.fixedSize)
    {
        asio::async_read(port,asio::buffer(param.data,param.size),boost::bind(
                &CUart::readCompleted,this,asio::placeholders::error,
                asio::placeholders::bytes_transferred));
    } else {
        asio::async_read_until(port,readData,param.delim,boost::bind(
                &CUart::readCompleted,this,asio::placeholders::error,
                asio::placeholders::bytes_transferred));
    }
}

void CUart::timeoutExpired(const boost::system::error_code& error)
{
     if(!error && result == resultInProgress) result = resultTimeoutExpired;
}

void CUart::readCompleted(const boost::system::error_code& error,
        const size_t bytesTransferred)
{
    if(!error)
    {
        result=resultSuccess;
        this->bytesTransferred=bytesTransferred;
        return;
    }

    #ifdef __APPLE__
    if(error.value()==45)
    {
        //Bug on OS X, it might be necessary to repeat the setup
        //http://osdir.com/ml/lib.boost.asio.user/2008-08/msg00004.html
        performReadSetup(setupParameters);
        return;
    }
    #endif //__APPLE__

    result=resultError;
}
