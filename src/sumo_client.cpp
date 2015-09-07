#include <config.h>

#include <vector>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <ctime>
#include <cstdlib>

#define BUILD_TCPIP
#include <foreign/tcpip/storage.h>
#include <foreign/tcpip/socket.h>

#include <traci-server/TraCIConstants.h>
#include <utils/common/SUMOTime.h>
#include "sumo_client.hpp"

SUMO_CLIENT::SUMO_CLIENT(std::string outputFileName)
  : outputFileName(outputFileName), answerLog("") {
  answerLog.setf(std::ios::fixed , std::ios::floatfield); // use decimal format
  answerLog.setf(std::ios::showpoint); // print decimal point
  answerLog << std::setprecision(2);
}

SUMO_CLIENT::~SUMO_CLIENT() {
  writeResult();
}

bool
SUMO_CLIENT::run(int port, std::string host) {
  
  std::stringstream msg;

  // try to connect
  try {
    TraCIAPI::connect(host, port);
  } catch (tcpip::SocketException& e) {
    std::stringstream msg;
    msg << "#Error while connecting: " << e.what();
    errorMsg(msg);
    return false;
  }

  // CHANGE THIS TO RUN THE TRAFFIC LIGHT CONTROLLER CODE

  // FIGURE OUT WHEN TO CLOSE HERE (FROM HIS PYTHON CODE)
  commandClose();
  close();
  return true;
}

// ---------- Helper commands: scoped

void
SUMO_CLIENT::getLastStepInductionLoopVehicleNumber(std::string ilID,
					   int& retVal)
{
  commandGetVariable(CMD_GET_INDUCTIONLOOP_VARIABLE,
		     LAST_STEP_VEHICLE_NUMBER,
		     ilID);
}

void
SUMO_CLIENT::getLastStepInductionLoopVehicleIDs(std::string ilID,
					std::vector<std::string>& retVal)
{
  commandGetVariable(CMD_GET_INDUCTIONLOOP_VARIABLE,
		     LAST_STEP_VEHICLE_ID_LIST,
		     ilID);
}

void
SUMO_CLIENT::getRedYellowGreenState(std::string tlsID,
			    int& retVal)
{
  commandGetVariable(CMD_GET_TL_VARIABLE,
		     TL_RED_YELLOW_GREEN_STATE,
		     tlsID);
}

void
SUMO_CLIENT::setRedYellowGreenState(std::string tlsID,
			    int state)
{
  std::ifstream tmp;
  commandSetValue(CMD_SET_TL_VARIABLE,
		  TL_RED_YELLOW_GREEN_STATE,
		  tlsID,
		  tmp);
}

void
SUMO_CLIENT::getMinExpectedNumber(int& retVal)
{
  commandGetVariable(CMD_GET_SIM_VARIABLE,
		     VAR_MIN_EXPECTED_VEHICLES,
		     "");
}

void
SUMO_CLIENT::getArrivedNumber(int& retVal)
{
  commandGetVariable(CMD_GET_SIM_VARIABLE,
		     VAR_ARRIVED_VEHICLES_NUMBER,
		     "");
}

void
SUMO_CLIENT::getArrivedIDList(std::vector<std::string>& retVal)
{
  commandGetVariable(CMD_GET_SIM_VARIABLE,
		     VAR_ARRIVED_VEHICLES_IDS,
		     "");
}

// ---------- Commands handling
void
SUMO_CLIENT::commandSimulationStep(SUMOTime time) {
  send_commandSimulationStep(time);
  answerLog << std::endl << "-> Command sent: <SimulationStep2>:" << std::endl;
  tcpip::Storage inMsg;
  try {
    std::string acknowledgement;
    check_resultState(inMsg, CMD_SIMSTEP2, false, &acknowledgement);
    answerLog << acknowledgement << std::endl;
    validateSimulationStep2(inMsg);
  } catch (tcpip::SocketException& e) {
    answerLog << e.what() << std::endl;
  }
}


void
SUMO_CLIENT::commandClose() {
  send_commandClose();
  answerLog << std::endl << "-> Command sent: <Close>:" << std::endl;
  try {
    tcpip::Storage inMsg;
    std::string acknowledgement;
    check_resultState(inMsg, CMD_CLOSE, false, &acknowledgement);
    answerLog << acknowledgement << std::endl;
  } catch (tcpip::SocketException& e) {
    answerLog << e.what() << std::endl;
  }
}


void
SUMO_CLIENT::commandGetVariable(int domID, int varID, const std::string& objID, tcpip::Storage* addData) {
  send_commandGetVariable(domID, varID, objID, addData);
  answerLog << std::endl << "-> Command sent: <GetVariable>:" << std::endl
	    << "  domID=" << domID << " varID=" << varID
	    << " objID=" << objID << std::endl;
  tcpip::Storage inMsg;
  try {
    std::string acknowledgement;
    check_resultState(inMsg, domID, false, &acknowledgement);
    answerLog << acknowledgement << std::endl;
  } catch (tcpip::SocketException& e) {
    answerLog << e.what() << std::endl;
    return;
  }
  check_commandGetResult(inMsg, domID, -1, false);
  // report result state
  try {
    int variableID = inMsg.readUnsignedByte();
    std::string objectID = inMsg.readString();
    answerLog <<  "  CommandID=" << (domID + 0x10) << "  VariableID=" << variableID << "  ObjectID=" << objectID;
    int valueDataType = inMsg.readUnsignedByte();
    answerLog << " valueDataType=" << valueDataType;
    readAndReportTypeDependent(inMsg, valueDataType);
  } catch (tcpip::SocketException& e) {
    std::stringstream msg;
    msg << "Error while receiving command: " << e.what();
    errorMsg(msg);
    return;
  }
}


void
SUMO_CLIENT::commandSetValue(int domID, int varID, const std::string& objID, std::ifstream& defFile) {
  std::stringstream msg;
  tcpip::Storage inMsg, tmp;
  setValueTypeDependant(tmp, defFile, msg);
  std::string msgS = msg.str();
  if (msgS != "") {
    errorMsg(msg);
  }
  send_commandSetValue(domID, varID, objID, tmp);
  answerLog << std::endl << "-> Command sent: <SetValue>:" << std::endl
	    << "  domID=" << domID << " varID=" << varID
	    << " objID=" << objID << std::endl;
  try {
    std::string acknowledgement;
    check_resultState(inMsg, domID, false, &acknowledgement);
    answerLog << acknowledgement << std::endl;
  } catch (tcpip::SocketException& e) {
    answerLog << e.what() << std::endl;
  }
}


void
SUMO_CLIENT::commandSubscribeObjectVariable(int domID, const std::string& objID, int beginTime, int endTime, int varNo, std::ifstream& defFile) {
  std::vector<int> vars;
  for (int i = 0; i < varNo; ++i) {
    int var;
    defFile >> var;
    // variable id
    vars.push_back(var);
  }
  send_commandSubscribeObjectVariable(domID, objID, beginTime, endTime, vars);
  answerLog << std::endl << "-> Command sent: <SubscribeVariable>:" << std::endl
	    << "  domID=" << domID << " objID=" << objID << " with " << varNo << " variables" << std::endl;
  tcpip::Storage inMsg;
  try {
    std::string acknowledgement;
    check_resultState(inMsg, domID, false, &acknowledgement);
    answerLog << acknowledgement << std::endl;
    validateSubscription(inMsg);
  } catch (tcpip::SocketException& e) {
    answerLog << e.what() << std::endl;
  }
}


void
SUMO_CLIENT::commandSubscribeContextVariable(int domID, const std::string& objID, int beginTime, int endTime,
				     int domain, SUMOReal range, int varNo, std::ifstream& defFile) {
  std::vector<int> vars;
  for (int i = 0; i < varNo; ++i) {
    int var;
    defFile >> var;
    // variable id
    vars.push_back(var);
  }
  send_commandSubscribeObjectContext(domID, objID, beginTime, endTime, domain, range, vars);
  answerLog << std::endl << "-> Command sent: <SubscribeContext>:" << std::endl
	    << "  domID=" << domID << " objID=" << objID << " domain=" << domain << " range=" << range
	    << " with " << varNo << " variables" << std::endl;
  tcpip::Storage inMsg;
  try {
    std::string acknowledgement;
    check_resultState(inMsg, domID, false, &acknowledgement);
    answerLog << acknowledgement << std::endl;
    validateSubscription(inMsg);
  } catch (tcpip::SocketException& e) {
    answerLog << e.what() << std::endl;
  }
}


// ---------- Report helper
void
SUMO_CLIENT::writeResult() {
  time_t seconds;
  tm* locTime;
  std::ofstream outFile(outputFileName.c_str());
  if (!outFile) {
    std::cerr << "Unable to write result file" << std::endl;
  }
  time(&seconds);
  locTime = localtime(&seconds);
  outFile << "SUMO_CLIENT output file. Date: " << asctime(locTime) << std::endl;
  outFile << answerLog.str();
  outFile.close();
}


void
SUMO_CLIENT::errorMsg(std::stringstream& msg) {
  std::cerr << msg.str() << std::endl;
  answerLog << "----" << std::endl << msg.str() << std::endl;
}

bool
SUMO_CLIENT::validateSimulationStep2(tcpip::Storage& inMsg) {
  try {
    int noSubscriptions = inMsg.readInt();
    for (int s = 0; s < noSubscriptions; ++s) {
      if (!validateSubscription(inMsg)) {
	return false;
      }
    }
  } catch (std::invalid_argument& e) {
    answerLog << "#Error while reading message:" << e.what() << std::endl;
    return false;
  }
  return true;
}


bool
SUMO_CLIENT::validateSubscription(tcpip::Storage& inMsg) {
  try {
    int length = inMsg.readUnsignedByte();
    if (length == 0) {
      length = inMsg.readInt();
    }
    int cmdId = inMsg.readUnsignedByte();
    if (cmdId >= RESPONSE_SUBSCRIBE_INDUCTIONLOOP_VARIABLE && cmdId <= RESPONSE_SUBSCRIBE_GUI_VARIABLE) {
      answerLog << "  CommandID=" << cmdId;
      answerLog << "  ObjectID=" << inMsg.readString();
      unsigned int varNo = inMsg.readUnsignedByte();
      answerLog << "  #variables=" << varNo << std::endl;
      for (unsigned int i = 0; i < varNo; ++i) {
	answerLog << "      VariableID=" << inMsg.readUnsignedByte();
	bool ok = inMsg.readUnsignedByte() == RTYPE_OK;
	answerLog << "      ok=" << ok;
	int valueDataType = inMsg.readUnsignedByte();
	answerLog << " valueDataType=" << valueDataType;
	readAndReportTypeDependent(inMsg, valueDataType);
      }
    } else if (cmdId >= RESPONSE_SUBSCRIBE_INDUCTIONLOOP_CONTEXT && cmdId <= RESPONSE_SUBSCRIBE_GUI_CONTEXT) {
      answerLog << "  CommandID=" << cmdId;
      answerLog << "  ObjectID=" << inMsg.readString();
      answerLog << "  Domain=" << inMsg.readUnsignedByte();
      unsigned int varNo = inMsg.readUnsignedByte();
      answerLog << "  #variables=" << varNo << std::endl;
      unsigned int objNo = inMsg.readInt();
      answerLog << "  #objects=" << objNo << std::endl;
      for (unsigned int j = 0; j < objNo; ++j) {
	answerLog << "   ObjectID=" << inMsg.readString() << std::endl;
	for (unsigned int i = 0; i < varNo; ++i) {
	  answerLog << "      VariableID=" << inMsg.readUnsignedByte();
	  bool ok = inMsg.readUnsignedByte() == RTYPE_OK;
	  answerLog << "      ok=" << ok;
	  int valueDataType = inMsg.readUnsignedByte();
	  answerLog << " valueDataType=" << valueDataType;
	  readAndReportTypeDependent(inMsg, valueDataType);
	}
      }
    } else {
      answerLog << "#Error: received response with command id: " << cmdId << " but expected a subscription response (0xe0-0xef / 0x90-0x9f)" << std::endl;
      return false;
    }
  } catch (std::invalid_argument& e) {
    answerLog << "#Error while reading message:" << e.what() << std::endl;
    return false;
  }
  return true;
}

// ---------- Conversion helper
int
SUMO_CLIENT::setValueTypeDependant(tcpip::Storage& into, std::ifstream& defFile, std::stringstream& msg) {
  std::string dataTypeS;
  defFile >> dataTypeS;
  if (dataTypeS == "<airDist>") {
    into.writeUnsignedByte(REQUEST_AIRDIST);
    return 1;
  } else if (dataTypeS == "<drivingDist>") {
    into.writeUnsignedByte(REQUEST_DRIVINGDIST);
    return 1;
  } else if (dataTypeS == "<objSubscription>") {
    int beginTime, endTime, numVars;
    defFile >> beginTime >> endTime >> numVars;
    into.writeInt(beginTime);
    into.writeInt(endTime);
    into.writeInt(numVars);
    for (int i = 0; i < numVars; ++i) {
      int var;
      defFile >> var;
      into.writeUnsignedByte(var);
    }
    return 4 + 4 + 4 + numVars;
  }
  int valI;
  double valF;
  if (dataTypeS == "<int>") {
    defFile >> valI;
    into.writeUnsignedByte(TYPE_INTEGER);
    into.writeInt(valI);
    return 4 + 1;
  } else if (dataTypeS == "<byte>") {
    defFile >> valI;
    into.writeUnsignedByte(TYPE_BYTE);
    into.writeByte(valI);
    return 1 + 1;
  }  else if (dataTypeS == "<ubyte>") {
    defFile >> valI;
    into.writeUnsignedByte(TYPE_UBYTE);
    into.writeUnsignedByte(valI);
    return 1 + 1;
  } else if (dataTypeS == "<float>") {
    defFile >> valF;
    into.writeUnsignedByte(TYPE_FLOAT);
    into.writeFloat(float(valF));
    return 4 + 1;
  } else if (dataTypeS == "<double>") {
    defFile >> valF;
    into.writeUnsignedByte(TYPE_DOUBLE);
    into.writeDouble(valF);
    return 8 + 1;
  } else if (dataTypeS == "<string>") {
    std::string valueS;
    defFile >> valueS;
    if (valueS == "\"\"") {
      valueS = "";
    }
    into.writeUnsignedByte(TYPE_STRING);
    into.writeString(valueS);
    return 4 + 1 + (int) valueS.length();
  } else if (dataTypeS == "<string*>") {
    std::vector<std::string> slValue;
    defFile >> valI;
    int length = 1 + 4;
    for (int i = 0; i < valI; ++i) {
      std::string tmp;
      defFile >> tmp;
      slValue.push_back(tmp);
      length += 4 + int(tmp.length());
    }
    into.writeUnsignedByte(TYPE_STRINGLIST);
    into.writeStringList(slValue);
    return length;
  } else if (dataTypeS == "<compound>") {
    defFile >> valI;
    into.writeUnsignedByte(TYPE_COMPOUND);
    into.writeInt(valI);
    int length = 1 + 4;
    for (int i = 0; i < valI; ++i) {
      length += setValueTypeDependant(into, defFile, msg);
    }
    return length;
  } else if (dataTypeS == "<color>") {
    defFile >> valI;
    into.writeUnsignedByte(TYPE_COLOR);
    into.writeUnsignedByte(valI);
    for (int i = 0; i < 3; ++i) {
      defFile >> valI;
      into.writeUnsignedByte(valI);
    }
    return 1 + 4;
  } else if (dataTypeS == "<position2D>") {
    defFile >> valF;
    into.writeUnsignedByte(POSITION_2D);
    into.writeDouble(valF);
    defFile >> valF;
    into.writeDouble(valF);
    return 1 + 8 + 8;
  } else if (dataTypeS == "<position3D>") {
    defFile >> valF;
    into.writeUnsignedByte(POSITION_3D);
    into.writeDouble(valF);
    defFile >> valF;
    into.writeDouble(valF);
    defFile >> valF;
    into.writeDouble(valF);
    return 1 + 8 + 8 + 8;
  } else if (dataTypeS == "<positionRoadmap>") {
    std::string valueS;
    defFile >> valueS;
    into.writeUnsignedByte(POSITION_ROADMAP);
    into.writeString(valueS);
    int length = 1 + 8 + (int) valueS.length();
    defFile >> valF;
    into.writeDouble(valF);
    defFile >> valI;
    into.writeUnsignedByte(valI);
    return length + 4 + 1;
  } else if (dataTypeS == "<shape>") {
    defFile >> valI;
    into.writeUnsignedByte(TYPE_POLYGON);
    into.writeUnsignedByte(valI);
    int length = 1 + 1;
    for (int i = 0; i < valI; ++i) {
      double x, y;
      defFile >> x >> y;
      into.writeDouble(x);
      into.writeDouble(y);
      length += 8 + 8;
    }
    return length;
  }
  msg << "## Unknown data type: " << dataTypeS;
  return 0;
}


void
SUMO_CLIENT::readAndReportTypeDependent(tcpip::Storage& inMsg, int valueDataType) {
  if (valueDataType == TYPE_UBYTE) {
    int ubyte = inMsg.readUnsignedByte();
    answerLog << " Unsigned Byte Value: " << ubyte << std::endl;
  } else if (valueDataType == TYPE_BYTE) {
    int byte = inMsg.readByte();
    answerLog << " Byte value: " << byte << std::endl;
  } else if (valueDataType == TYPE_INTEGER) {
    int integer = inMsg.readInt();
    answerLog << " Int value: " << integer << std::endl;
  } else if (valueDataType == TYPE_FLOAT) {
    float floatv = inMsg.readFloat();
    if (floatv < 0.1 && floatv > 0) {
      answerLog.setf(std::ios::scientific, std::ios::floatfield);
    }
    answerLog << " float value: " << floatv << std::endl;
    answerLog.setf(std::ios::fixed , std::ios::floatfield); // use decimal format
    answerLog.setf(std::ios::showpoint); // print decimal point
    answerLog << std::setprecision(2);
  } else if (valueDataType == TYPE_DOUBLE) {
    double doublev = inMsg.readDouble();
    answerLog << " Double value: " << doublev << std::endl;
  } else if (valueDataType == TYPE_BOUNDINGBOX) {
    SUMOReal lowerLeftX = inMsg.readDouble();
    SUMOReal lowerLeftY = inMsg.readDouble();
    SUMOReal upperRightX = inMsg.readDouble();
    SUMOReal upperRightY = inMsg.readDouble();
    answerLog << " BoundaryBoxValue: lowerLeft x=" << lowerLeftX
	      << " y=" << lowerLeftY << " upperRight x=" << upperRightX
	      << " y=" << upperRightY << std::endl;
  } else if (valueDataType == TYPE_POLYGON) {
    int length = inMsg.readUnsignedByte();
    answerLog << " PolygonValue: ";
    for (int i = 0; i < length; i++) {
      SUMOReal x = inMsg.readDouble();
      SUMOReal y = inMsg.readDouble();
      answerLog << "(" << x << "," << y << ") ";
    }
    answerLog << std::endl;
  } else if (valueDataType == POSITION_3D) {
    SUMOReal x = inMsg.readDouble();
    SUMOReal y = inMsg.readDouble();
    SUMOReal z = inMsg.readDouble();
    answerLog << " Position3DValue: " << std::endl;
    answerLog << " x: " << x << " y: " << y
	      << " z: " << z << std::endl;
  } else if (valueDataType == POSITION_ROADMAP) {
    std::string roadId = inMsg.readString();
    SUMOReal pos = inMsg.readDouble();
    int laneId = inMsg.readUnsignedByte();
    answerLog << " RoadMapPositionValue: roadId=" << roadId
	      << " pos=" << pos
	      << " laneId=" << laneId << std::endl;
  } else if (valueDataType == TYPE_TLPHASELIST) {
    int length = inMsg.readUnsignedByte();
    answerLog << " TLPhaseListValue: length=" << length << std::endl;
    for (int i = 0; i < length; i++) {
      std::string pred = inMsg.readString();
      std::string succ = inMsg.readString();
      int phase = inMsg.readUnsignedByte();
      answerLog << " precRoad=" << pred << " succRoad=" << succ
		<< " phase=";
      switch (phase) {
      case TLPHASE_RED:
	answerLog << "red" << std::endl;
	break;
      case TLPHASE_YELLOW:
	answerLog << "yellow" << std::endl;
	break;
      case TLPHASE_GREEN:
	answerLog << "green" << std::endl;
	break;
      default:
	answerLog << "#Error: unknown phase value" << (int)phase << std::endl;
	return;
      }
    }
  } else if (valueDataType == TYPE_STRING) {
    std::string s = inMsg.readString();
    answerLog << " string value: " << s << std::endl;
  } else if (valueDataType == TYPE_STRINGLIST) {
    std::vector<std::string> s = inMsg.readStringList();
    answerLog << " string list value: [ " << std::endl;
    for (std::vector<std::string>::iterator i = s.begin(); i != s.end(); ++i) {
      if (i != s.begin()) {
	answerLog << ", ";
      }
      answerLog << '"' << *i << '"';
    }
    answerLog << " ]" << std::endl;
  } else if (valueDataType == TYPE_COMPOUND) {
    int no = inMsg.readInt();
    answerLog << " compound value with " << no << " members: [ " << std::endl;
    for (int i = 0; i < no; ++i) {
      int currentValueDataType = inMsg.readUnsignedByte();
      answerLog << " valueDataType=" << currentValueDataType;
      readAndReportTypeDependent(inMsg, currentValueDataType);
    }
    answerLog << " ]" << std::endl;
  } else if (valueDataType == POSITION_2D) {
    SUMOReal xv = inMsg.readDouble();
    SUMOReal yv = inMsg.readDouble();
    answerLog << " position value: (" << xv << "," << yv << ")" << std::endl;
  } else if (valueDataType == TYPE_COLOR) {
    int r = inMsg.readUnsignedByte();
    int g = inMsg.readUnsignedByte();
    int b = inMsg.readUnsignedByte();
    int a = inMsg.readUnsignedByte();
    answerLog << " color value: (" << r << "," << g << "," << b << "," << a << ")" << std::endl;
  } else {
    answerLog << "#Error: unknown valueDataType!" << std::endl;
  }
}


