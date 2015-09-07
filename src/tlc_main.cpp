#include <config.h>

#include <iostream>
#include <string>
#include <cstdlib>
#include "sumo_client.hpp"


int main(int argc, char* argv[]) {
    std::string outFileName = "testclient_out.txt";
    int port = -1;
    std::string host = "localhost";

    if (argc < 1) {
        std::cout << "Usage: tlc -p <remote port>"
                  << "[-h <remote host>] [-o <outputfile name>]" << std::endl;
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

    SUMO_CLIENT client(outFileName);
    client.create_connection(port,host);
    client.close_connection();
    return 0;
}
