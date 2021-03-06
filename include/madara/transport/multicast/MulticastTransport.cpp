
#include "madara/transport/multicast/MulticastTransport.h"
#include "madara/transport/multicast/MulticastTransportReadThread.h"
#include "madara/transport/TransportContext.h"

#include "madara/transport/ReducedMessageHeader.h"
#include "madara/utility/Utility.h"
#include "madara/expression/ExpressionTree.h"
#include "madara/expression/Interpreter.h"
#include "madara/transport/Fragmentation.h"

#include <iostream>
#include "madara/utility/inttypes.h"

madara::transport::MulticastTransport::MulticastTransport (const std::string & id,
        madara::knowledge::ThreadSafeContext & context, 
        TransportSettings & config, bool launch_transport)
: Base (id, config, context),
  write_socket_ (ACE_sap_any_cast (ACE_INET_Addr &), PF_INET, 0, 1)
{
  // create a reference to the knowledge base for threading
  knowledge_.use (context);

  // set the data plane for the read threads
  read_threads_.set_data_plane (knowledge_);

  if (launch_transport)
    setup ();
}

madara::transport::MulticastTransport::~MulticastTransport ()
{
  close ();
}

void
madara::transport::MulticastTransport::close (void)
{
  this->invalidate_transport ();

  read_threads_.terminate ();

  read_threads_.wait ();
  
  write_socket_.close ();
}

int
madara::transport::MulticastTransport::reliability (void) const
{
  return madara::transport::BEST_EFFORT;
}

int
madara::transport::MulticastTransport::reliability (const int &)
{
  return madara::transport::BEST_EFFORT;
}

int
madara::transport::MulticastTransport::setup (void)
{
  // call base setup method to initialize certain common variables
  Base::setup ();

  // resize addresses to be the size of the list of hosts
  addresses_.resize (this->settings_.hosts.size ());

  int ttl = 1;
  int send_buff_size = 0, tar_buff_size (settings_.queue_length);
  int rcv_buff_size = 0;
  int opt_len = sizeof (int);

  write_socket_.set_option (IPPROTO_IP,
                     IP_MULTICAST_TTL,
                     (void *) &ttl,
                     sizeof (ttl));

  write_socket_.get_option (SOL_SOCKET, SO_SNDBUF,
    (void *)&send_buff_size, &opt_len);

  write_socket_.get_option (SOL_SOCKET, SO_RCVBUF,
    (void *)&rcv_buff_size, &opt_len);
  
  madara_logger_log (context_.get_logger (), logger::LOG_MAJOR,
    "MulticastTransport::setup:" \
    " default socket buff size is send=%d, rcv=%d\n",
    send_buff_size, rcv_buff_size);
  
  if (send_buff_size < tar_buff_size)
  {
    madara_logger_log (context_.get_logger (), logger::LOG_MAJOR,
      "MulticastTransport::setup:" \
      " setting send buff size to settings.queue_length (%d)\n",
      tar_buff_size);
  
    write_socket_.set_option (SOL_SOCKET, SO_SNDBUF,
      (void *) &tar_buff_size, opt_len);
    
    write_socket_.get_option (SOL_SOCKET, SO_SNDBUF,
      (void *)&send_buff_size, &opt_len);

    madara_logger_log (context_.get_logger (), logger::LOG_MAJOR,
      "MulticastTransport::setup:" \
      " current socket buff size is send=%d, rcv=%d\n",
      send_buff_size, rcv_buff_size);
  }
  
  if (rcv_buff_size < tar_buff_size)
  {
    madara_logger_log (context_.get_logger (), logger::LOG_MAJOR,
      "MulticastTransport::setup:" \
      " setting rcv buff size to settings.queue_length (%d)\n",
      tar_buff_size);
  
    write_socket_.set_option (SOL_SOCKET, SO_SNDBUF,
      (void *) &tar_buff_size, opt_len);
    
    write_socket_.get_option (SOL_SOCKET, SO_SNDBUF,
      (void *)&rcv_buff_size, &opt_len);

    madara_logger_log (context_.get_logger (), logger::LOG_MAJOR,
      "MulticastTransport::setup:" \
      " current socket buff size is send=%d, rcv=%d\n",
      send_buff_size, rcv_buff_size);
  }
    
  if (addresses_.size () > 0)
  {
    // convert the string host:port into an ACE address
    for (unsigned int i = 0; i < addresses_.size (); ++i)
    {
      addresses_[i].set (settings_.hosts[i].c_str ());

      madara_logger_log (context_.get_logger (), logger::LOG_MAJOR,
        "MulticastTransport::setup:" \
        " settings address[%d] to %s:%d\n", i,
        addresses_[i].get_host_addr (), addresses_[i].get_port_number ());
    }
    
    int port = addresses_[0].get_port_number ();
    const char * host = addresses_[0].get_host_addr ();

    if (-1 == read_socket_.join (addresses_[0], 1))
    {
      madara_logger_log (context_.get_logger (), logger::LOG_MAJOR,
        "MulticastTransport::setup:" \
        " Error subscribing to multicast address %s:%d\n", host, port);
    } 
    else
    {
      madara_logger_log (context_.get_logger (), logger::LOG_MAJOR,
        "MulticastTransport::setup:" \
        " Success subscribing to multicast address %s:%d\n", host, port);
    
      int send_buff_size = 0, tar_buff_size (settings_.queue_length);
      int rcv_buff_size = 0;
      int opt_len = sizeof (int);
    
      ACE_SOCK_Dgram & bare_socket = read_socket_;

      bare_socket.get_option (SOL_SOCKET, SO_RCVBUF,
        (void *)&rcv_buff_size, &opt_len);

      madara_logger_log (context_.get_logger (), logger::LOG_MAJOR,
        "MulticastTransport::setup:" \
        " default socket buff size is send=%d, rcv=%d\n",
        send_buff_size, rcv_buff_size);
  
      if (send_buff_size < tar_buff_size)
      {
        madara_logger_log (context_.get_logger (), logger::LOG_MAJOR,
          "MulticastTransport::setup:" \
          " setting send buff size to settings.queue_length (%d)\n",
          tar_buff_size);
  
        bare_socket.set_option (SOL_SOCKET, SO_SNDBUF,
          (void *)&tar_buff_size, opt_len);
    
        bare_socket.get_option (SOL_SOCKET, SO_SNDBUF,
          (void *)&send_buff_size, &opt_len);

        madara_logger_log (context_.get_logger (), logger::LOG_MAJOR,
          "MulticastTransport::setup:" \
          " current socket buff size is send=%d, rcv=%d\n",
          send_buff_size, rcv_buff_size);
      }
  
      if (rcv_buff_size < tar_buff_size)
      {
        madara_logger_log (context_.get_logger (), logger::LOG_MAJOR,
          "MulticastTransport::setup:" \
          " setting rcv buff size to settings.queue_length (%d)\n",
          tar_buff_size);
  
        bare_socket.set_option (SOL_SOCKET, SO_RCVBUF,
          (void *)&tar_buff_size, opt_len);
    
        bare_socket.get_option (SOL_SOCKET, SO_RCVBUF,
          (void *)&rcv_buff_size, &opt_len);

        madara_logger_log (context_.get_logger (), logger::LOG_MAJOR,
          "MulticastTransport::setup:" \
          " current socket buff size is send=%d, rcv=%d\n",
          send_buff_size, rcv_buff_size);
      }
    }

    if (!settings_.no_receiving)
    {
      double hertz = settings_.read_thread_hertz;
      if (hertz < 0.0)
      {
        hertz = 0.0;
      }

      madara_logger_log (context_.get_logger (), logger::LOG_MAJOR,
        "MulticastTransport::setup:" \
        " starting %d threads at %f hertz\n", settings_.read_threads,
        hertz);

      for (uint32_t i = 0; i < settings_.read_threads; ++i)
      {
        std::stringstream thread_name;
        thread_name << "read";
        thread_name << i;

        read_threads_.run (hertz, thread_name.str (),
          new MulticastTransportReadThread (
            settings_, id_, addresses_[0], write_socket_, read_socket_,
            send_monitor_, receive_monitor_, packet_scheduler_));
      }
    }
  }

  

  return this->validate_transport ();
}

long
madara::transport::MulticastTransport::send_data (
  const madara::knowledge::KnowledgeRecords & orig_updates)
{
  long result (0);
  const char * print_prefix = "MulticastTransport::send_data";

  if (!settings_.no_sending)
  {
    result = prep_send (orig_updates, print_prefix);

    if (addresses_.size () > 0 && result > 0)
    {
      uint64_t bytes_sent = 0;
      uint64_t packet_size = (uint64_t)result;

      if (packet_size > settings_.max_fragment_size)
      {
        FragmentMap map;

        madara_logger_log (context_.get_logger (), logger::LOG_MAJOR,
          "%s:" \
          " fragmenting %" PRIu64 " byte packet (%" PRIu32 " bytes is max fragment size)\n",
          print_prefix, packet_size, settings_.max_fragment_size);

        // fragment the message
        frag (buffer_.get_ptr (), settings_.max_fragment_size, map);

        int j (0);
        for (FragmentMap::iterator i = map.begin (); i != map.end (); ++i)
        {
          madara_logger_log (context_.get_logger (), logger::LOG_MAJOR,
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
              addresses_[0]);

            ++send_attempts;

            if (actual_sent > 0)
            {
              madara_logger_log (context_.get_logger (), logger::LOG_MAJOR,
                "%s:" \
                " Sent %d byte fragment\n",
                print_prefix, (int)actual_sent);

              bytes_sent += actual_sent;
            }
          }

          // sleep between fragments, if such a slack time is specified
          if (settings_.slack_time > 0)
            madara::utility::sleep (settings_.slack_time);
        }

        if (bytes_sent > 0)
        {
          send_monitor_.add ((uint32_t)bytes_sent);
        }

        madara_logger_log (context_.get_logger (), logger::LOG_MAJOR,
          "%s:" \
          " Sent fragments totalling %" PRIu64 " bytes\n",
          print_prefix, bytes_sent);

        delete_fragments (map);
      }
      else
      {
        madara_logger_log (context_.get_logger (), logger::LOG_MAJOR,
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
            buffer_.get_ptr (), (ssize_t)result, addresses_[0]);

          ++send_attempts;

          if (actual_sent > 0)
          {
            bytes_sent = (uint64_t)actual_sent;

            madara_logger_log (context_.get_logger (), logger::LOG_MAJOR,
              "%s:" \
              " Sent packet of size %" PRIu64 "\n",
              print_prefix, bytes_sent);

            send_monitor_.add ((uint32_t)actual_sent);
          }
          else if (actual_sent == ECONNRESET)
          {
            madara_logger_log (context_.get_logger (), logger::LOG_WARNING,
              "%s:" \
              " WARNING: Remote socket disappeared during send (ECONNRESET)\n",
              print_prefix);
          }
          else if (actual_sent == EINTR)
          {
            madara_logger_log (context_.get_logger (), logger::LOG_WARNING,
              "%s:" \
              " Local socket was interrupted during send (EINTR)\n",
              print_prefix);
          }
          else if (actual_sent == EWOULDBLOCK)
          {
            madara_logger_log (context_.get_logger (), logger::LOG_WARNING,
              "%s:" \
              " Send would have blocked (EWOULDBLOCK)\n",
              print_prefix);
          }
          else if (actual_sent == ENOTCONN)
          {
            madara_logger_log (context_.get_logger (), logger::LOG_WARNING,
              "%s:" \
              " Send reports socket is not connected (ENOTCONN)\n",
              print_prefix);
          }
          else if (actual_sent == EADDRINUSE)
          {
            madara_logger_log (context_.get_logger (), logger::LOG_WARNING,
              "%s:" \
              " Send reports the interface is busy (EADDRINUSE)\n",
              print_prefix);
          }
          else if (actual_sent == EBADF)
          {
            madara_logger_log (context_.get_logger (), logger::LOG_WARNING,
              "%s:" \
              " Send socket is invalid (EBADF)\n",
              print_prefix);
          }
          else
          {
            madara_logger_log (context_.get_logger (), logger::LOG_WARNING,
              "%s:" \
              " Packet was not sent due to unknown error (%d)\n",
              print_prefix, (int)actual_sent);
          }
        }
      }


      madara_logger_log (context_.get_logger (), logger::LOG_MAJOR,
        "%s:" \
        " Send bandwidth = %" PRIu64 " B/s\n",
        print_prefix, send_monitor_.get_bytes_per_second ());

      result = (long) bytes_sent;
    }
  }

  return result;
}
