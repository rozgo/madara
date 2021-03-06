
#include <string>
#include "ace/OS.h"
#include "ace/Log_Msg.h"
#include "ace/SOCK_Connector.h"
#include "AgentMessages.h"
#include "MadaraCommon.h"

/*
  Unlike the previous two Tutorials, we're going to allow the user to provide
  command line options this time.  Still, we need defaults in case that isn't
  done.
 */
static const char *const SERVER_PORT = "49995";
static const char *const SERVER_HOST = "127.0.0.1";
static const int MAX_ITERATIONS = 4;

int main (int argc, char *argv[])
{
  /*
    Accept the users's choice of hosts or use the default.  Then do the same
    for the TCP/IP port at which the server is listening as well as the
    number of iterations to perform.
   */
  const std::string server_host = argc > 1 ? argv[1] : SERVER_HOST;
  std::string port  = argc > 2 ? argv[2] : SERVER_PORT;
  int max_iterations      = argc > 3 ? ACE_OS::atoi (argv[3]) : MAX_ITERATIONS;

  u_short server_port     = ACE_OS::atoi (port.c_str ());
  /*
    Build ourselves a Stream socket. This is a connected socket that provides
    reliable end-to-end communications. We will use the server object to send
    data to the server we connect to.
   */
  ACE_SOCK_Stream server;

  /*
    And we need a connector object to establish that connection. The ACE_SOCK_Connector
    object provides all of the tools we need to establish a connection once we know the
    server's network address... 
   */
  ACE_SOCK_Connector connector;

  /*
    Which we create with an ACE_INET_Addr object. This object is given the TCP/IP port
    and hostname of the server we want to connect to.
   */
  ACE_INET_Addr addr (server_port, server_host.c_str ());

  ACE_DEBUG ((LM_DEBUG, "(%P|%t) Attempting server connect (port %d)\n", server_port));
  /*
    So, we feed the Addr object and the Stream object to the connector's connect() member
    function. Given this information, it will establish the network connection to the
    server and attacht that connection to the server object. 
   */
  if (connector.connect (server, addr) == -1)
  {
    ACE_ERROR_RETURN ((LM_ERROR, "%p\n", "open"), -1);
  }
  
  /*
    Just for grins, we'll send the server several messages.
   */
  for (int i = 0; i < max_iterations; i++)
  {
    port = "30000";

    Madara::AgentPing ping;
    Madara::AgentFile deployment_file;

    strcpy (deployment_file.name, "deployment_example.txt");

    ping.size = sizeof (ping) - sizeof (ping.size);
    ping.type = Madara::BROKER_DEPLOYMENT_OFFER;
    //ping.type = Madara::AGENT_PING;
    strcpy (ping.host, server_host.c_str ()); 
    ping.port = atoi (port.c_str ());

    //continue;

    /*
      Create our message with the message number
     */
//    ACE_OS::sprintf (buf, "message = %d\n", i + 1);

    /*
      Send the message to the server.  We use the server object's send_n() function to
      send all of the data at once. There is also a send() function but it may not send
      all of the data. That is due to network buffer availability and such. If the send()
      doesn't send all of the data, it is up to you to program things such that it will
      keep trying until all of the data is sent or simply give up. The send_n() function
      already does the "keep tyring" option for us, so we use it. 
     */
    //if (server.send_n ( buf, strlen(buf) ) == -1)
    if (server.send_n ( (void *)&ping, sizeof (ping) ) == -1)
    {
      ACE_ERROR_RETURN ((LM_ERROR, "%p\n", "send"), -1);
    }
    else
    {
      if (server.send_n ( (void *)&deployment_file, 
                           sizeof (deployment_file) ) == -1)
        {
          ACE_ERROR_RETURN ((LM_ERROR, "%p\n", "send"), -1);
        }

      ping.port = 30000;
      strcpy(ping.host, "10.0.0.40");
      ACE_Time_Value timeout (0,1000);
     // size_t ret = server.recv ( (void *)&ping, sizeof (ping), &timeout);

      ping.type = Madara::BROKER_DEPLOYMENT_PRINT;
      ping.port = 30000;
      strcpy(ping.host, server_host.c_str ()); 

   //   if (server.send_n ( (void *)&ping, sizeof (ping) ) == -1)
   //   {
   //     ACE_ERROR_RETURN ((LM_ERROR, "%p\n", "send"), -1);
   //   }
      /*
        Pause for a second.
       */

      ACE_OS::sleep (1);

      if (server.send_n ( (void *)&ping, sizeof (ping) ) == -1)
      {
        ACE_ERROR_RETURN ((LM_ERROR, "%p\n", "send"), -1);
      }
    }
  }

  /*
    Close the connection to the server.  The servers we've created so far all are based
    on the ACE_Reactor.  When we close(), the server's reactor will see activity for
    the registered event handler and invoke handle_input().  That, in turn, will try
    to read from the socket but get back zero bytes.  At that point, the server will know
    that we've closed from our side.
   */
  if (server.close () == -1)
  {
    ACE_ERROR_RETURN ((LM_ERROR, "%p\n", "close"), -1);
  }

  return 0;
}