/*
  Simple *working* example of sending an OSC message using oscpack.
*/

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <csignal>
#include <iostream>
// Sorry windows.
#include <unistd.h>
#include "oscpack/osc/OscOutboundPacketStream.h"
#include "oscpack/ip/UdpSocket.h"
#include "emokit/emokit.h"

#define ADDRESS "127.0.0.1"
#define PORT 7000

#define OUTPUT_BUFFER_SIZE 4096

#define FREQ 1000000/128

void sigproc(int i)
{
	std::cout << "\nClosing emokit and quitting." << std::endl;
	exit(0);
}

int main(int argc, char* argv[])
{
	signal(SIGINT, sigproc);
#ifndef WIN32
	signal(SIGQUIT, sigproc);
#endif

	bool noHelmet = false;
	if((argc > 1) && (argv[1] == std::string("-n"))) noHelmet = true;

  UdpTransmitSocket transmitSocket( IpEndpointName( ADDRESS, PORT ) );

  char buffer[OUTPUT_BUFFER_SIZE];

	struct emokit_device* d;

	d = emokit_create();
	printf("Current epoc devices connected: %d\n", emokit_get_count(d, EMOKIT_VID, EMOKIT_PID));
  int r = emokit_open(d, EMOKIT_VID, EMOKIT_PID, 1);
	if(r != 0 && !noHelmet)
	{
		emokit_close(d);
		emokit_delete(d);
		d = emokit_create();
		r = emokit_open(d, EMOKIT_VID, EMOKIT_PID, 0);
		if (r!=0) {
			printf("CANNOT CONNECT: %d\n", r);
			return 1;
		}
		return 1;
	} else if(noHelmet) {
		std::cout << "Sending random data" << std::endl;
	}

	if (!noHelmet) {
    if (emokit_read_data(d)<=0) {
      printf("Error reading from headset\n");
      emokit_close(d);
      emokit_delete(d);
      return 1;
    }

		while(true)
		{
			if(emokit_read_data(d) > 0)
			{
				emokit_frame frame = emokit_get_next_frame(d);

				std::cout << "\r\33[2K" << "gyroX: " << (int)frame.gyroX
                  << "; gyroY: " << (int)frame.gyroY
                  << "; F3: " << frame.F3
                  << "; FC6: " << frame.FC6
                  << "; battery: " << (int)frame.battery << "%";

				flush(std::cout);

				osc::OutboundPacketStream channels( buffer, OUTPUT_BUFFER_SIZE );
        osc::OutboundPacketStream channel_quality( buffer, OUTPUT_BUFFER_SIZE );
				osc::OutboundPacketStream gyro( buffer, OUTPUT_BUFFER_SIZE );
				osc::OutboundPacketStream battery( buffer, OUTPUT_BUFFER_SIZE );

				channels << osc::BeginMessage( "/emokit/channels" )
                 << frame.F3
                 << frame.FC6
                 << frame.P7
                 << frame.T8
                 << frame.F7
                 << frame.F8
                 << frame.T7
                 << frame.P8
                 << frame.AF4
                 << frame.F4
                 << frame.AF3
                 << frame.O2
                 << frame.O1
                 << frame.FC5
                 << osc::EndMessage;
				transmitSocket.Send( channels.Data(), channels.Size() );

				gyro << osc::BeginMessage( "/emokit/gyro" )
             << (int)frame.gyroX
             << (int)frame.gyroY
             << osc::EndMessage;
				transmitSocket.Send( gyro.Data(), gyro.Size() );

				battery << osc::BeginMessage( "/emokit/battery" )
                << (int)frame.battery
                << osc::EndMessage;
        transmitSocket.Send( battery.Data(), battery.Size() );

				channel_quality << osc::BeginMessage( "/emokit/channel_quality" )
                        << frame.cq.F3
                        << frame.cq.FC6
                        << frame.cq.P7
                        << frame.cq.T8
                        << frame.cq.F7
                        << frame.cq.F8
                        << frame.cq.T7
                        << frame.cq.P8
                        << frame.cq.AF4
                        << frame.cq.F4
                        << frame.cq.AF3
                        << frame.cq.O2
                        << frame.cq.O1
                        << frame.cq.FC5
                        << osc::EndMessage;
 				transmitSocket.Send( channel_quality.Data(), channel_quality.Size() );
			}
		}
	} else {
		while (true) {
			usleep(FREQ);

			osc::OutboundPacketStream channels( buffer, OUTPUT_BUFFER_SIZE );
			osc::OutboundPacketStream gyro( buffer, OUTPUT_BUFFER_SIZE );
			osc::OutboundPacketStream battery( buffer, OUTPUT_BUFFER_SIZE );
      osc::OutboundPacketStream channel_quality( buffer, OUTPUT_BUFFER_SIZE );

			channels << osc::BeginMessage( "/emokit/channels" );
			for (int i=0 ; i < 14 ; i++) channels << rand() % 10000;
			channels << osc::EndMessage;
			transmitSocket.Send( channels.Data(), channels.Size() );

			gyro << osc::BeginMessage( "/emokit/gyro" )
           << rand() % 100 << rand() % 100 << osc::EndMessage;
			transmitSocket.Send( gyro.Data(), gyro.Size() );

			battery << osc::BeginMessage( "/emokit/battery" )
              << rand() % 100 << osc::EndMessage;
			transmitSocket.Send( battery.Data(), battery.Size() );

      channel_quality << osc::BeginMessage( "/emokit/channel_quality" );
			for (int i=0 ; i < 14 ; i++) channel_quality << rand() % 10000;
			channel_quality << osc::EndMessage;
			transmitSocket.Send( channel_quality.Data(), channel_quality.Size() );
		}
	}

	emokit_close(d);
	emokit_delete(d);
	return 0;

}
