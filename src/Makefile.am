SUBDIRS = utils

bin_PROGRAMS = TraCITestClient

TraCITestClient_SOURCES = tracitestclient_main.cpp TraCITestClient.cpp TraCITestClient.h

TraCITestClient_LDADD = utils/common/libcommon.a \
../utils/traci/libtraci.a foreign/tcpip/libtcpip.a
