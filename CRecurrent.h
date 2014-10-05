#ifndef __CRECURRENT_H__
#define __CRECURRENT_H__

#include <cstdlib>

// --------------------------------------------
/// Simple recurrent event handler class
/// All preiodically polled devices inherit it
class CRecurrent
{
public:
    CRecurrent(int poll_interval) : m_poll_interval(poll_interval), m_next_poll_time(0) {};
    virtual ~CRecurrent() {};

    /// Virtual function should be re-implemented in inherited classes
    virtual int Poll()
    {
        int rc = 0;
        if(isPollTime())
        {
            printf("%s: next poll at %s\n", typeid(*this).name(), ctime( &m_next_poll_time));
            // Do real Poll() here...
            rc = 1;
        }
        return rc;
    };

    /// Check if time interval expired and sets time (absolute) of new poll    
    bool isPollTime()
    {
        time(& m_last_poll_time);
        if(m_last_poll_time < m_next_poll_time)
        {
            return false;
        }
        m_next_poll_time = m_last_poll_time + m_poll_interval;
        return true;
    };
    /// Get time of next poll
    time_t getNextPollTime()
    {
        return m_next_poll_time;
    };

private:
    // Poll interval, sec
    int m_poll_interval;
    // Next and last poll time, secs (since 1-jan-1970 as time() returns)
    time_t m_next_poll_time;
    time_t m_last_poll_time;
};


#endif // __CRECURRENT_H__
