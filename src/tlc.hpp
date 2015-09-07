#ifndef PORTED_TLC_HPP
#define PORTED_TLC_HPP

#include <string>
#include <sstream>
#include <vector>

#include <foreign/tcpip/socket.h>
#include <utils/common/SUMOTime.h>
#include <utils/traci/TraCIAPI.h>

class TLC : public TraCIAPI {
public:
  TLC(std::string outputFileName = "tlc.out");
  ~TLC();
  bool run(int port, std::string host = "localhost");

  void getLastStepInductionLoopVehicleNumber(std::string ilID,
					     int& retVal);
  void getLastStepInductionLoopVehicleIDs(std::string ilID, 
					  std::vector<std::string>& retVal);

  void getRedYellowGreenState(std::string tlsID,
			      int& retVal);
  void setRedYellowGreenState(std::string tlsID, 
			      int state);

  void getMinExpectedNumber(int& retVal);
  void getArrivedNumber(int& retVal);
  void getArrivedIDList(std::vector<std::string>& retVal);

protected:
  void commandSimulationStep(SUMOTime time);
  void commandClose();
  void commandGetVariable(int domID, int varID, const std::string& objID, tcpip::Storage* addData = 0);
  void commandSetValue(int domID, int varID, const std::string& objID, std::ifstream& defFile);
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
