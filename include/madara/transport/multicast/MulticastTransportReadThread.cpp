#include "madara/transport/multicast/MulticastTransportReadThread.h"

#include "madara/transport/ReducedMessageHeader.h"
#include "madara/transport/Fragmentation.h"
#include "ace/Time_Value.h"
#include "madara/utility/Utility.h"


#include <iostream>
#include <algorithm>

madara::transport::MulticastTransportReadThread::MulticastTransportReadThread (
  const TransportSettings & settings,
  const std::string & id,
  const ACE_INET_Addr & address,
  ACE_SOCK_Dgram & write_socket,
  ACE_SOCK_Dgram_Mcast & read_socket,
  BandwidthMonitor & send_monitor,
  BandwidthMonitor & receive_monitor,
  PacketScheduler & packet_scheduler)
  : settings_ (settings), id_ (id), context_ (0), address_ (address),
    read_socket_ (read_socket),
    write_socket_ (write_socket),
    send_monitor_ (send_monitor),
    receive_monitor_ (receive_monitor),
    packet_scheduler_ (packet_scheduler)
{
}

void
madara::transport::MulticastTransportReadThread::init (
  knowledge::KnowledgeBase & knowledge)
{
  context_ = &(knowledge.get_context ());
  
  // setup the receive buffer
  if (settings_.queue_length > 0)
    buffer_ = new char [settings_.queue_length];

  if (context_)
  {
    // check for an on_data_received ruleset
    if (settings_.on_data_received_logic.length () != 0)
    {
      madara_logger_log (this->context_->get_logger (), logger::LOG_MAJOR,
        "MulticastTransportReadThread::init:" \
        " setting rules to %s\n",
        settings_.on_data_received_logic.c_str ());


#ifndef _MADARA_NO_KARL_
      madara::expression::Interpreter interpreter;
      on_data_received_ = context_->compile (settings_.on_data_received_logic);
#endif // _MADARA_NO_KARL_
    }
    else
    {
      madara_logger_log (this->context_->get_logger (), logger::LOG_MAJOR,
        "MulticastTransportReadThread::init:" \
        " no permanent rules were set\n");
    }
  }
}

void
madara::transport::MulticastTransportReadThread::cleanup (void)
{
  // Unsubscribe
  if (-1 == read_socket_.leave (address_))
  {
    madara_logger_log (this->context_->get_logger (), logger::LOG_MAJOR,
      "MulticastTransportReadThread::close:" \
      " Error unsubscribing to multicast address\n");
  }

  read_socket_.close ();
}

void
madara::transport::MulticastTransportReadThread::rebroadcast (
  const char * print_prefix,
  MessageHeader * header,
  const knowledge::KnowledgeMap & records)
{
  int64_t buffer_remaining = (int64_t) settings_.queue_length;
  char * buffer = buffer_.get_ptr ();
  int result (0);

  if (!settings_.no_sending)
  {
    result = prep_rebroadcast (*context_, buffer, buffer_remaining,
                                   settings_, print_prefix,
                                   header, records,
                                   packet_scheduler_);

    if (result > 0)
    {
      ssize_t bytes_sent = 0;
      uint64_t packet_size = (uint64_t)result;

      if (packet_size > settings_.max_fragment_size)
      {
        FragmentMap map;

        madara_logger_log (this->context_->get_logger (), logger::LOG_MAJOR,
          "%s:" \
          " fragmenting %d byte packet (%" PRIu64 " bytes is max fragment size)\n",
          print_prefix, packet_size, settings_.max_fragment_size);

        // fragment the message
        frag (buffer_.get_ptr (), settings_.max_fragment_size, map);

        int j (0);
        for (FragmentMap::iterator i = map.begin (); i != map.end ();
             ++i, ++j)
        {
          madara_logger_log (this->context_->get_logger (), logger::LOG_MAJOR,
            "%s:" \
            " Sending fragment %d\n",
            print_prefix, j);

          int send_attempts = -1;
          ssize_t actual_sent = -1;

          while (actual_sent < 0 &&
            (settings_.resend_attempts < 0 ||
            send_attempts < settings_.resend_attempts))
          {
            // send the fragment
            actual_sent = write_socket_.send (
            i->second,
            (ssize_t)MessageHeader::get_size (i->second),
            address_);

            ++send_attempts;

            if (actual_sent > 0)
            {
              bytes_sent = (uint64_t)actual_sent;

              madara_logger_log (context_->get_logger (), logger::LOG_MAJOR,
                "%s:" \
                " Sent packet of size %" PRIu64 "\n",
                print_prefix, bytes_sent);

              send_monitor_.add ((uint32_t)actual_sent);
            }
            else if (actual_sent == ECONNRESET)
            {
              madara_logger_log (context_->get_logger (), logger::LOG_WARNING,
                "%s:" \
                " WARNING: Remote socket disappeared during send (ECONNRESET)\n",
                print_prefix);
            }
            else if (actual_sent == EINTR)
            {
              madara_logger_log (context_->get_logger (), logger::LOG_WARNING,
                "%s:" \
                " Local socket was interrupted during send (EINTR)\n",
                print_prefix);
            }
            else if (actual_sent == EWOULDBLOCK)
            {
              madara_logger_log (context_->get_logger (), logger::LOG_WARNING,
                "%s:" \
                " Send would have blocked (EWOULDBLOCK)\n",
                print_prefix);
            }
            else if (actual_sent == ENOTCONN)
            {
              madara_logger_log (context_->get_logger (), logger::LOG_WARNING,
                "%s:" \
                " Send reports socket is not connected (ENOTCONN)\n",
                print_prefix);
            }
            else if (actual_sent == EADDRINUSE)
            {
              madara_logger_log (context_->get_logger (), logger::LOG_WARNING,
                "%s:" \
                " Send reports the interface is busy (EADDRINUSE)\n",
                print_prefix);
            }
            else if (actual_sent == EBADF)
            {
              madara_logger_log (context_->get_logger (), logger::LOG_WARNING,
                "%s:" \
                " Send socket is invalid (EBADF)\n",
                print_prefix);
            }
            else
            {
              madara_logger_log (context_->get_logger (), logger::LOG_WARNING,
                "%s:" \
                " Packet was not sent due to unknown error (%d)\n",
                print_prefix, (int)actual_sent);
            }
          }

          // sleep between fragments, if such a slack time is specified
          if (settings_.slack_time > 0)
            madara::utility::sleep (settings_.slack_time);
        }
      
        send_monitor_.add ((uint32_t)bytes_sent);

        madara_logger_log (this->context_->get_logger (), logger::LOG_MAJOR,
          "%s:" \
          " Sent fragments totalling %" PRIu64 " bytes\n",
          print_prefix,
          bytes_sent);

        delete_fragments (map);
      }
      else
      {
        madara_logger_log (this->context_->get_logger (),
          logger::LOG_MAJOR,
          "%s:" \
          " Sending packet of size %ld\n",
          print_prefix, result);

        int send_attempts = -1;
        ssize_t actual_sent = -1;

        while (actual_sent < 0 &&
          (settings_.resend_attempts < 0 ||
          send_attempts < settings_.resend_attempts))
        {

          // send the fragment
          actual_sent = write_socket_.send (
            buffer_.get_ptr (), (ssize_t)result, address_);

          ++send_attempts;

          if (actual_sent > 0)
          {
            bytes_sent = (uint64_t)actual_sent;

            madara_logger_log (this->context_->get_logger (),
              logger::LOG_MAJOR,
              "%s:" \
              " Sent packet of size %" PRIu64 "\n",
              print_prefix, bytes_sent);

            send_monitor_.add ((uint32_t)actual_sent);
          }
          else if (actual_sent == ECONNRESET)
          {
            madara_logger_log (this->context_->get_logger (),
              logger::LOG_WARNING,
              "%s:" \
              " WARNING: Remote socket disappeared during send (ECONNRESET)\n",
              print_prefix);
          }
          else if (actual_sent == EINTR)
          {
            madara_logger_log (this->context_->get_logger (),
              logger::LOG_WARNING,
              "%s:" \
              " Local socket was interrupted during send (EINTR)\n",
              print_prefix);
          }
          else if (actual_sent == EWOULDBLOCK)
          {
            madara_logger_log (this->context_->get_logger (),
              logger::LOG_WARNING,
              "%s:" \
              " Send would have blocked (EWOULDBLOCK)\n",
              print_prefix);
          }
          else if (actual_sent == ENOTCONN)
          {
            madara_logger_log (this->context_->get_logger (),
              logger::LOG_WARNING,
              "%s:" \
              " Send reports socket is not connected (ENOTCONN)\n",
              print_prefix);
          }
          else if (actual_sent == EADDRINUSE)
          {
            madara_logger_log (this->context_->get_logger (),
              logger::LOG_WARNING,
              "%s:" \
              " Send reports the interface is busy (EADDRINUSE)\n",
              print_prefix);
          }
          else if (actual_sent == EBADF)
          {
            madara_logger_log (this->context_->get_logger (),
              logger::LOG_WARNING,
              "%s:" \
              " Send socket is invalid (EBADF)\n",
              print_prefix);
          }
          else
          {
            madara_logger_log (this->context_->get_logger (),
              logger::LOG_WARNING,
              "%s:" \
              " Packet was not sent due to unknown error (%d)\n",
              print_prefix, (int)actual_sent);
          }
        }
      }

      madara_logger_log (this->context_->get_logger (), logger::LOG_MAJOR,
        "%s:" \
        " Send bandwidth = %" PRIu64 " B/s\n",
        print_prefix,
        send_monitor_.get_bytes_per_second ());
    }
  }
}

void
madara::transport::MulticastTransportReadThread::run (void)
{
  if (!settings_.no_receiving)
  {
    ACE_Time_Value wait_time (1);
    ACE_INET_Addr  remote;
  
    // allocate a buffer to send
    char * buffer = buffer_.get_ptr ();
    const char * print_prefix = "MulticastTransportReadThread::run";
    int64_t buffer_remaining = settings_.queue_length;

    madara_logger_log (this->context_->get_logger (), logger::LOG_MINOR,
      "%s:" \
      " entering main service loop.\n", print_prefix);
    
    knowledge::KnowledgeMap rebroadcast_records;

    if (buffer == 0)
    {
      madara_logger_log (this->context_->get_logger (), logger::LOG_EMERGENCY,
        "%s:" \
        " Unable to allocate buffer of size " PRIu32 ". Exiting thread.\n",
        print_prefix,
        settings_.queue_length);
    
      return;
    }

    madara_logger_log (this->context_->get_logger (), logger::LOG_MINOR,
      "%s:" \
      " entering a recv on the socket.\n",
      print_prefix);
    
    // read the message
    ssize_t bytes_read = read_socket_.recv ((void *)buffer, 
      (size_t) settings_.queue_length, remote, 0, &wait_time);
 
    if (bytes_read > 0)
    {
      if (remote.get_host_addr () != 0)
      {
        madara_logger_ptr_log (logger::global_logger.get (),
          logger::LOG_MAJOR,
          "%s:" \
          " received a message header of %lld bytes from %s:%d\n",
          print_prefix,
          (long long)bytes_read,
          remote.get_host_addr (), (int)remote.get_port_number ());
      }
      else
      {
        madara_logger_ptr_log (logger::global_logger.get (),
          logger::LOG_WARNING,
          "%s:" \
          " received %lld bytes from unknown host\n",
          print_prefix,
          (long long)bytes_read);
      }
      MessageHeader * header = 0;

      std::stringstream remote_host;
      remote_host << remote.get_host_addr ();
      remote_host << ":";
      remote_host << remote.get_port_number ();

      process_received_update (buffer, bytes_read, id_, *context_,
        settings_, send_monitor_, receive_monitor_, rebroadcast_records,
  #ifndef _MADARA_NO_KARL_
        on_data_received_,
  #endif // _MADARA_NO_KARL_
        print_prefix,
        remote_host.str ().c_str (), header);
      
      if (header)
      {
        if (header->ttl > 0 && rebroadcast_records.size () > 0 &&
            settings_.get_participant_ttl () > 0)
        {
          --header->ttl;
          header->ttl = std::min (
            settings_.get_participant_ttl (), header->ttl);

          rebroadcast (print_prefix, header, rebroadcast_records);
        }

        // delete header
        delete header;
      }
    }
    else
    {
      madara_logger_log (this->context_->get_logger (), logger::LOG_MAJOR,
        "%s:" \
        " wait timeout on new messages. Proceeding to next wait\n",
        print_prefix);
    }

    madara_logger_log (this->context_->get_logger (), logger::LOG_MAJOR,
      "%s:" \
      " finished iteration.\n",
      print_prefix);
  }
}
