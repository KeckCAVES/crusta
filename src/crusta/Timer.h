#ifndef _TIMER_H_
#define _TIMER_H_

//--------------------------------------------- windows performance counter ----
#if defined(_WIN32) && (defined(_MSC_VER) || defined(__INTEL_COMPILER))

#include <windows.h>

class TimerImpl
{
protected:
   LARGE_INTEGER freq;
   LARGE_INTEGER count;
   LARGE_INTEGER startTime;

public:
   TimerImpl()
   {
       if (QueryPerformanceFrequency(&freq)==FALSE)
          throw std::runtime_error("Performance counter unavailable");
       reset();
   }

   void reset()
   {
       memset(&count,    0, sizeof(count));
       memset(&startTime,0, sizeof(count));
   }
   void start()
   {
       reset();
       QueryPerformanceCounter(&startTime);
   }
   void stop()
   {
       LARGE_INTEGER stopTime;

       QueryPerformanceCounter(&stopTime);
       count.QuadPart += stopTime.QuadPart - startTime.QuadPart;
   }
   void resume()
   {
       QueryPerformanceCounter(&startTime);
   }
   double seconds() const
   {
       return (double)count.QuadPart/(double)freq.QuadPart;
   }
};


//-------------------------------------------------------------- posix time ----
#elif defined(__GNUC__) && defined(__POSIX__)

#include <ctime>

class TimerImpl
{
protected:
   clockid_t id;
   double    elapsedTime;
   timespec  startTime;

public:
    TimerImpl(clockid_t iId) :
        id(iId)
    {
        reset();
    }

    void reset()
    {
        elapsedTime = 0.0;
    }
    void start()
    {
        reset();
        clock_gettime(id, &startTime);
    }
    void stop()
    {
        timespec stop;
        clock_gettime(id, &stopTime);
        elapsedTime += stopTime.tv_sec - startTime.tv_sec;
        elapsedTime += double(stopTime.tv_nsec-startTime.tv_nsec) * 1e-9;
    }
    void resume()
    {
        clock_gettime(id, &startTime);
    }
    double seconds() const
    {
        return elapsedTime;
    }
};

//------------------------------------------------------------ gettimeofday ----
#elif defined(__GNUC__) || (defined(__INTEL_COMPILER) && !defined(_WIN32))

#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>

class TimerImpl
{
protected:
  struct timeval  startTime;
  struct timeval  stopTime;
  struct timezone zone;

  double elapsedTime;

public:
    TimerImpl()
    {
        reset();
    }

    void reset()
    {
        elapsedTime = 0.0;
    }
    void start()
    {
        reset();
        gettimeofday(&startTime, &zone);
    }
    void stop()
    {
        gettimeofday(&stopTime, &zone);

        elapsedTime += double(stopTime.tv_sec  - startTime.tv_sec);
        elapsedTime += double(stopTime.tv_usec - startTime.tv_usec) * 1e-6;
    }
    void resume()
    {
        gettimeofday(&startTime, &zone);
    }
    double seconds() const
    {
        return elapsedTime;
    }
};


//------------------------------------------------- standard implementation ----
#else

#include <ctime>

static const unsigned long clockticks = CLOCKS_PER_SEC;

class TimerImpl
{
protected:
   unsigned long freq;
   unsigned long count;
   unsigned long startTime;

public:
    TimerImplStd() :
        freq(clockticks)
    {
        reset();
    }

    void reset()
    {
        count = 0;
    }
    void start()
    {
        reset();
        startTime = clock();
    }
    void stop()
    {
        unsigned long stopTime = clock();
        count += stopTime - startTime;
    }
    void resume()
    {
        startTime = clock();
    }
    double seconds() const
    {
        return (double)count / (double)freq;
    }
};

#endif //Timer platform specific implementations





//--------------------------------------------------------------- Timer API ----

/**
 A timer class based almost entirely on the one from OpenMesh
 */
class Timer
{
protected:
    enum State
    {
        Invalid = -1,
        Stopped =  0,
        Running =  1
    };

    TimerImpl impl;
    State state;

public:
    Timer();

    bool is_valid() const;
    bool is_stopped() const;

    void reset();
    void start();
    void stop();
    void resume();

    //@{ (if the timer is in the state 'Stopped')
    /// Returns measured time in seconds
    double seconds() const;
    /// Returns measured time in hundredth seconds
    double hseconds() const;
    /// Returns measured time in milli-seconds
    double mseconds() const;
    /// Returns measured time in micro seconds
    double useconds() const;
    //@}

    //@{ Compare timer values
    bool operator  <(const Timer& t2) const;
    bool operator  >(const Timer& t2) const;
    bool operator ==(const Timer& t2) const;
    bool operator <=(const Timer& t2) const;
    bool operator >=(const Timer& t2) const;
    //@}
};


//------------------------------------------------ Timer API implementation ----

#include <cassert>

inline
Timer::
Timer() :
#if defined(_WIN32) && defined(_MSC_VER)
    impl(),
#elif defined(__GNUC__) && defined(__POSIX__)
// CLOCK_REALTIME
// CLOCK_MONOTONIC     - ?
// CLOCK_REALTIME_HR   - RTlinux
// CLOCK_MONOTONIC_HR  - ?
  #if defined(CLOCK_REALTIME_HR)
    impl(CLOCK_REALTIME_HR),
  #else
    impl(CLOCK_REALTIME),
  #endif
#elif defined(__GNUC__) || (defined(__INTEL_COMPILER) && !defined(_WIN32))
    impl(),
#else
    impl(),
#endif
    state(Stopped)
{}

inline
bool Timer::
is_valid() const
{
    return state!=Invalid;
}

inline
bool Timer::
is_stopped() const
{
    return state==Stopped;
}

inline
void Timer::
reset()
{
    impl.reset();
    state = Stopped;
}

inline
void Timer::
start()
{
    impl.start();
    state = Running;
}

inline
void Timer::
stop()
{
    if (state == Running)
    {
        impl.stop();
        state = Stopped;
    }
}

inline
void Timer::
resume()
{
    if (state == Stopped)
    {
        impl.resume();
        state = Running;
    }
}

inline
double Timer::
seconds() const
{
    return state==Stopped ? impl.seconds() : 0.0;
}

inline
double Timer::
hseconds() const
{
    return seconds() * 1e2;
}

inline
double Timer::
mseconds() const
{
    return seconds() * 1e3;
}

inline
double Timer::
useconds() const
{
    return seconds() * 1e6;
}


inline
bool Timer::
operator <(const Timer& t2) const
{
    assert(is_stopped() && t2.is_stopped());
    return seconds()<t2.seconds();
}

inline
bool Timer::
operator >(const Timer& t2) const
{
    assert(is_stopped() && t2.is_stopped());
    return seconds()>t2.seconds();
}

inline
bool Timer::
operator == (const Timer& t2) const
{
    assert(is_stopped() && t2.is_stopped());
    return seconds()==t2.seconds();
}

inline
bool Timer::
operator <=(const Timer& t2) const
{
    assert(is_stopped() && t2.is_stopped());
    return seconds()<=t2.seconds();
}

inline
bool Timer::
operator >=(const Timer& t2) const
{
    assert(is_stopped() && t2.is_stopped());
    return seconds()>=t2.seconds();
}

#endif //_TIMER_H_
