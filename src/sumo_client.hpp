#ifndef SUMO_CLIENT_HPP
#define SUMO_CLIENT_HPP

#include <string>
#include <sstream>
#include <vector>

#include <foreign/tcpip/socket.h>
#include <utils/common/SUMOTime.h>
#include <utils/traci/TraCIAPI.h>

class SUMO_CLIENT : public TraCIAPI {
public:
  SUMO_CLIENT(std::string outputFileName = "tlc.out");
  ~SUMO_CLIENT();

  bool create_connection(int port, std::string host = "localhost");
  void close_connection();

  void commandSimulationStep(SUMOTime time);

protected:
  void commandClose();

  tcpip::Storage commandGetVariable(int domID,
				    int varID,
				    const std::string& objID,
				    tcpip::Storage* addData = 0);
  void commandSetValue(int domID,
		       int varID,
		       const std::string& objID,
		       tcpip::Storage& value);
  void commandSubscribeObjectVariable(int domID,
				      const std::string& objID,
				      int beginTime,
				      int endTime,
				      int varNo,
				      std::ifstream& defFile);
  void commandSubscribeContextVariable(int domID,
				       const std::string& objID,
				       int beginTime,
				       int endTime,
				       int domain,
				       SUMOReal range,
				       int varNo,
				       std::ifstream& defFile);

private:
  void writeResult();
  void errorMsg(std::stringstream& msg);

  bool validateSimulationStep2(tcpip::Storage& inMsg);
  bool validateSubscription(tcpip::Storage& inMsg);

  int setValueTypeDependant(tcpip::Storage& into, std::ifstream& defFile, std::stringstream& msg);
  void readAndReportTypeDependent(tcpip::Storage& inMsg, int valueDataType);

private:
  std::string outputFileName;
  std::stringstream answerLog;
};

#endif
