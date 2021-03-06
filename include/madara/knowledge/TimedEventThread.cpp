#include "TimedEventThread.h"

#include "madara/transport/ReducedMessageHeader.h"
#include "madara/transport/Fragmentation.h"
#include "ace/Time_Value.h"
#include "madara/utility/Utility.h"
#include "madara/logger/GlobalLogger.h"

#ifdef WIN32

#include <process.h>

unsigned __stdcall timed_thread_windows_glue (void * param)
{
  madara::knowledge::TimedEventThread * caller = 
    static_cast <madara::knowledge::TimedEventThread *> (
      param);

  if (caller)
    return (unsigned) caller->svc ();
  else
    return 0;
}

#endif


#include <iostream>
#include <algorithm>

madara::knowledge::TimedEventThread::TimedEventThread (
  TimedEventThreadInfo & info)
  : info_ (info)
{
  int result;

//#ifndef WIN32
    result = this->activate ();
/*#else
    result = 0;
    _beginthreadex(NULL, 0, timed_thread_windows_glue, (void*)this, 0, 0);
    
#endif*/

  if (result != -1)
  {
    madara_logger_ptr_log (logger::global_logger.get(), logger::LOG_MAJOR,
      "TimedEventThread::TimedEventThread:" \
      " thread started (result = %d)\n", result);
  }
  else
  {
    madara_logger_ptr_log (logger::global_logger.get(), logger::LOG_MAJOR,
      "TimedEventThread::TimedEventThread:" \
      " failed to create thread\n");
  }

  std::stringstream expression;
  expression << "thread.";
  expression << info_.id;
  expression << ".closed = 1";
  
#ifndef _MADARA_NO_KARL_

  thread_closed_ = info_.control_plane->compile (expression.str ());
  queued_or_terminated_ = info_.control_plane->compile (
    "queued > 0 || terminated");

#endif // _MADARA_NO_KARL_

}

madara::knowledge::TimedEventThread::~TimedEventThread ()
{
}


int
madara::knowledge::TimedEventThread::svc (void)
{
  ACE_Time_Value sleep_time;
  ACE_Time_Value zero_time;
  TimedEvent cur_event;
  
  zero_time.set (0.0);

  std::stringstream buffer;
  buffer << "TimedEventThread[";
  buffer << info_.id;
  buffer << "]::svc";
  std::string print_prefix (buffer.str ());

  madara_logger_ptr_log (logger::global_logger.get(), logger::LOG_MAJOR,
    "%s: " \
    "Service thread started.\n",
    print_prefix.c_str ());

  while (!info_.executor->is_shutdown ())
  {
    madara_logger_ptr_log (logger::global_logger.get(), logger::LOG_MAJOR,
      "%s: " \
      "Attempting to remove an event.\n",
      print_prefix.c_str ());

    /**
     * remove returns in one of three conditions
     * 1) an event has a timeout, so cur_event.second is valid
     * 2) an event is in the queue but not timed out, so
     *    sleep_time is a valid sleep time (>0.0)
     * 3) there were no events in queue, so we slept on a condition,
     *    and we've now been signaled. It is possible that someone
     *    else has serviced the event since we were signaled. The
     *    result will be 0.0 in sleep_time and a need to try to
     *    remove again.
     **/
    sleep_time = info_.executor->remove (cur_event);
    Event * valid = cur_event.second;

    // if we had a timeout, we'll have a non-zero Event *
    if (valid > 0)
    {
      ++valid->executions;
      knowledge::KnowledgeRecord result (
#ifndef _MADARA_NO_KARL_
        valid->knowledge->evaluate (valid->root)
#else
        knowledge::KnowledgeRecord::Integer (0)
#endif
        );

      if (valid->intended_executions >= 0 &&
          valid->intended_executions == valid->executions)
      {
        madara_logger_ptr_log (logger::global_logger.get(), logger::LOG_MAJOR,
          "%s: " \
          "Timed Event has expired. Deleting.\n",
          print_prefix.c_str ());

        delete valid;
      }
      else if (valid->cancel_on_false && result.is_false ())
      {
        madara_logger_ptr_log (logger::global_logger.get(), logger::LOG_MAJOR,
          "%s: " \
          "Timed Event returned false. User requested a cancel. Deleting.\n",
          print_prefix.c_str ());

        delete valid;
      }
      else
      {
        madara_logger_ptr_log (logger::global_logger.get(), logger::LOG_MAJOR,
          "%s: " \
          "Timed Event added back to queue.\n",
          print_prefix.c_str ());

        cur_event.first += valid->period;
        info_.executor->add (cur_event);
      }
    }
    // otherwise, wait on changes to the queue
    else
    {
      if (sleep_time > zero_time)
      {
        madara_logger_ptr_log (logger::global_logger.get(), logger::LOG_MAJOR,
          "%s: " \
          "Not ready for timeout. Thread sleeping.\n",
          print_prefix.c_str ());
        
        WaitSettings wait_settings;
        wait_settings.poll_frequency = 0.5;
        wait_settings.max_wait_time = 5.0;
    
#ifndef _MADARA_NO_KARL_

        // inform sleeping threads of new queued events
        info_.control_plane->wait (queued_or_terminated_, wait_settings);
#endif // _MADARA_NO_KARL_

      }
    }
  }
  
#ifndef _MADARA_NO_KARL_

  info_.control_plane->evaluate (thread_closed_);
#endif // _MADARA_NO_KARL_


  return 0;
}

