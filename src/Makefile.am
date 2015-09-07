SUBDIRS = utils foreign

bin_PROGRAMS = TraCITestClient tlc

TraCITestClient_SOURCES = tracitestclient_main.cpp TraCITestClient.cpp TraCITestClient.h

TraCITestClient_LDADD = utils/common/libcommon.a \
utils/traci/libtraci.a foreign/tcpip/libtcpip.a

tlc_SOURCES = tlc_main.cpp sumo_client.cpp sumo_client.hpp

tlc_LDADD = utils/common/libcommon.a \
utils/traci/libtraci.a foreign/tcpip/libtcpip.a
