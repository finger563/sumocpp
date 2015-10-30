#include <config.h>

#include <iostream>
#include <string>
#include <cstdlib>
#include "sumo_client.hpp"

#include <unistd.h>

SUMO_CLIENT client;

int main(int argc, char* argv[]) {
    std::string outFileName = "testclient_out.txt";
    int port = -1;
    int microsec_step_size = -1;
    std::string host = "localhost";

    if (argc < 5) {
        std::cout << "Usage: sim_stepper -p <remote port> -s <step size in us>"
                  << " [-h <remote host>] [-o <outputfile name>]" << std::endl;
        return 0;
    }

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
	if (arg.compare("-o") == 0) {
            outFileName = argv[i + 1];
            i++;
        } else if (arg.compare("-p") == 0) {
            port = atoi(argv[i + 1]);
            i++;
        } else if (arg.compare("-s") == 0) {
            microsec_step_size = atoi(argv[i + 1]);
            i++;
        } else if (arg.compare("-h") == 0) {
            host = argv[i + 1];
            i++;
        } else {
            std::cout << "unknown parameter: " << argv[i] << std::endl;
            return 1;
        }
    }

    if (port == -1) {
        std::cout << "Missing port" << std::endl;
        return 1;
    }

    client.create_connection(port,host);

    while (true)
      {
	unsigned int tmp_occupancy = 0;
	std::string tmp_laneid;
	try
	  {
	    client.commandSimulationStep(0);

	    tmp_laneid = client.inductionloop.getLaneID("V1");
	    std::cout << "V1 Lane ID: " << tmp_laneid << std::endl;

	    tmp_occupancy = client.inductionloop.getLastStepVehicleNumber("V1");
	    std::cout << "V1 last step vehicle number: " << tmp_occupancy << std::endl;
	  }
	catch ( tcpip::SocketException& e )
	  {
	    std::cout << "Caught exception: " << e.what() << std::endl;
	  }
	usleep(microsec_step_size);
      }
    client.close_connection();
    return 0;
}
