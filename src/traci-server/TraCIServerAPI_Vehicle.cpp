/****************************************************************************/
/// @file    TraCIServerAPI_Vehicle.cpp
/// @author  Daniel Krajzewicz
/// @author  Laura Bieker
/// @author  Christoph Sommer
/// @author  Michael Behrisch
/// @author  Bjoern Hendriks
/// @author  Mario Krumnow
/// @author  Jakob Erdmann
/// @date    07.05.2009
/// @version $Id: TraCIServerAPI_Vehicle.cpp 18096 2015-03-17 09:50:59Z behrisch $
///
// APIs for getting/setting vehicle values via TraCI
/****************************************************************************/
// SUMO, Simulation of Urban MObility; see http://sumo.dlr.de/
// Copyright (C) 2009-2015 DLR (http://www.dlr.de/) and contributors
/****************************************************************************/
//
//   This file is part of SUMO.
//   SUMO is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
/****************************************************************************/


// ===========================================================================
// included modules
// ===========================================================================
#ifdef _MSC_VER
#include <windows_config.h>
#else
#include <config.h>
#endif

#ifndef NO_TRACI

#include <microsim/MSNet.h>
#include <microsim/MSInsertionControl.h>
#include <microsim/MSVehicle.h>
#include <microsim/MSLane.h>
#include <microsim/MSEdge.h>
#include <microsim/MSEdgeWeightsStorage.h>
#include <microsim/lcmodels/MSAbstractLaneChangeModel.h>
#include <utils/geom/PositionVector.h>
#include <utils/vehicle/DijkstraRouterTT.h>
#include <utils/vehicle/DijkstraRouterEffort.h>
#include <utils/emissions/PollutantsInterface.h>
#include <utils/emissions/HelpersHarmonoise.h>
#include <utils/vehicle/SUMOVehicleParameter.h>
#include "TraCIConstants.h"
#include "TraCIServerAPI_Simulation.h"
#include "TraCIServerAPI_Vehicle.h"
#include "TraCIServerAPI_VehicleType.h"

#ifdef CHECK_MEMORY_LEAKS
#include <foreign/nvwa/debug_new.h>
#endif // CHECK_MEMORY_LEAKS

//#define DEBUG_VTD 1
//#define DEBUG_VTD_ANGLE 1


// ===========================================================================
// static member variables
// ===========================================================================
std::map<std::string, std::vector<MSLane*> > TraCIServerAPI_Vehicle::gVTDMap;


// ===========================================================================
// method definitions
// ===========================================================================
bool
TraCIServerAPI_Vehicle::processGet(TraCIServer& server, tcpip::Storage& inputStorage,
                                   tcpip::Storage& outputStorage) {
    // variable & id
    int variable = inputStorage.readUnsignedByte();
    std::string id = inputStorage.readString();
    // check variable
    if (variable != ID_LIST && variable != VAR_SPEED && variable != VAR_SPEED_WITHOUT_TRACI
            && variable != VAR_POSITION && variable != VAR_ANGLE && variable != VAR_POSITION3D
            && variable != VAR_ROAD_ID && variable != VAR_LANE_ID && variable != VAR_LANE_INDEX
            && variable != VAR_TYPE && variable != VAR_ROUTE_ID && variable != VAR_COLOR
            && variable != VAR_LANEPOSITION
            && variable != VAR_CO2EMISSION && variable != VAR_COEMISSION
            && variable != VAR_HCEMISSION && variable != VAR_PMXEMISSION
            && variable != VAR_NOXEMISSION && variable != VAR_FUELCONSUMPTION && variable != VAR_NOISEEMISSION
            && variable != VAR_PERSON_NUMBER && variable != VAR_LEADER
            && variable != VAR_EDGE_TRAVELTIME && variable != VAR_EDGE_EFFORT
            && variable != VAR_ROUTE_VALID && variable != VAR_EDGES
            && variable != VAR_SIGNALS && variable != VAR_DISTANCE
            && variable != VAR_LENGTH && variable != VAR_MAXSPEED && variable != VAR_VEHICLECLASS
            && variable != VAR_SPEED_FACTOR && variable != VAR_SPEED_DEVIATION
            && variable != VAR_ALLOWED_SPEED && variable != VAR_EMISSIONCLASS
            && variable != VAR_WIDTH && variable != VAR_MINGAP && variable != VAR_SHAPECLASS
            && variable != VAR_ACCEL && variable != VAR_DECEL && variable != VAR_IMPERFECTION
            && variable != VAR_TAU && variable != VAR_BEST_LANES && variable != DISTANCE_REQUEST
            && variable != ID_COUNT && variable != VAR_STOPSTATE && variable !=  VAR_WAITING_TIME
            && variable != VAR_PARAMETER
       ) {
        return server.writeErrorStatusCmd(CMD_GET_VEHICLE_VARIABLE, "Get Vehicle Variable: unsupported variable specified", outputStorage);
    }
    // begin response building
    tcpip::Storage tempMsg;
    //  response-code, variableID, objectID
    tempMsg.writeUnsignedByte(RESPONSE_GET_VEHICLE_VARIABLE);
    tempMsg.writeUnsignedByte(variable);
    tempMsg.writeString(id);
    // process request
    if (variable == ID_LIST || variable == ID_COUNT) {
        std::vector<std::string> ids;
        MSVehicleControl& c = MSNet::getInstance()->getVehicleControl();
        for (MSVehicleControl::constVehIt i = c.loadedVehBegin(); i != c.loadedVehEnd(); ++i) {
            if ((*i).second->isOnRoad()) {
                ids.push_back((*i).first);
            }
        }
        if (variable == ID_LIST) {
            tempMsg.writeUnsignedByte(TYPE_STRINGLIST);
            tempMsg.writeStringList(ids);
        } else {
            tempMsg.writeUnsignedByte(TYPE_INTEGER);
            tempMsg.writeInt((int) ids.size());
        }
    } else {
        SUMOVehicle* sumoVehicle = MSNet::getInstance()->getVehicleControl().getVehicle(id);
        if (sumoVehicle == 0) {
            return server.writeErrorStatusCmd(CMD_GET_VEHICLE_VARIABLE, "Vehicle '" + id + "' is not known", outputStorage);
        }
        MSVehicle* v = dynamic_cast<MSVehicle*>(sumoVehicle);
        if (v == 0) {
            return server.writeErrorStatusCmd(CMD_GET_VEHICLE_VARIABLE, "Vehicle '" + id + "' is not a micro-simulation vehicle", outputStorage);
        }
        const bool onRoad = v->isOnRoad();
        switch (variable) {
            case VAR_SPEED:
                tempMsg.writeUnsignedByte(TYPE_DOUBLE);
                tempMsg.writeDouble(onRoad ? v->getSpeed() : INVALID_DOUBLE_VALUE);
                break;
            case VAR_SPEED_WITHOUT_TRACI:
                tempMsg.writeUnsignedByte(TYPE_DOUBLE);
                tempMsg.writeDouble(onRoad ? v->getSpeedWithoutTraciInfluence() : INVALID_DOUBLE_VALUE);
                break;
            case VAR_POSITION:
                tempMsg.writeUnsignedByte(POSITION_2D);
                tempMsg.writeDouble(onRoad ? v->getPosition().x() : INVALID_DOUBLE_VALUE);
                tempMsg.writeDouble(onRoad ? v->getPosition().y() : INVALID_DOUBLE_VALUE);
                break;
            case VAR_POSITION3D:
                tempMsg.writeUnsignedByte(POSITION_3D);
                tempMsg.writeDouble(onRoad ? v->getPosition().x() : INVALID_DOUBLE_VALUE);
                tempMsg.writeDouble(onRoad ? v->getPosition().y() : INVALID_DOUBLE_VALUE);
                tempMsg.writeDouble(onRoad ? v->getPosition().z() : INVALID_DOUBLE_VALUE);
                break;
            case VAR_ANGLE:
                tempMsg.writeUnsignedByte(TYPE_DOUBLE);
                tempMsg.writeDouble(onRoad ? v->getAngle() : INVALID_DOUBLE_VALUE);
                break;
            case VAR_ROAD_ID:
                tempMsg.writeUnsignedByte(TYPE_STRING);
                tempMsg.writeString(onRoad ? v->getLane()->getEdge().getID() : "");
                break;
            case VAR_LANE_ID:
                tempMsg.writeUnsignedByte(TYPE_STRING);
                tempMsg.writeString(onRoad ? v->getLane()->getID() : "");
                break;
            case VAR_LANE_INDEX:
                tempMsg.writeUnsignedByte(TYPE_INTEGER);
                if (onRoad) {
                    const std::vector<MSLane*>& lanes = v->getLane()->getEdge().getLanes();
                    tempMsg.writeInt((int)std::distance(lanes.begin(), std::find(lanes.begin(), lanes.end(), v->getLane())));
                } else {
                    tempMsg.writeInt(INVALID_INT_VALUE);
                }
                break;
            case VAR_TYPE:
                tempMsg.writeUnsignedByte(TYPE_STRING);
                tempMsg.writeString(v->getVehicleType().getID());
                break;
            case VAR_ROUTE_ID:
                tempMsg.writeUnsignedByte(TYPE_STRING);
                tempMsg.writeString(v->getRoute().getID());
                break;
            case VAR_COLOR:
                tempMsg.writeUnsignedByte(TYPE_COLOR);
                tempMsg.writeUnsignedByte(v->getParameter().color.red());
                tempMsg.writeUnsignedByte(v->getParameter().color.green());
                tempMsg.writeUnsignedByte(v->getParameter().color.blue());
                tempMsg.writeUnsignedByte(v->getParameter().color.alpha());
                break;
            case VAR_LANEPOSITION:
                tempMsg.writeUnsignedByte(TYPE_DOUBLE);
                tempMsg.writeDouble(onRoad ? v->getPositionOnLane() : INVALID_DOUBLE_VALUE);
                break;
            case VAR_CO2EMISSION:
                tempMsg.writeUnsignedByte(TYPE_DOUBLE);
                tempMsg.writeDouble(onRoad ? v->getCO2Emissions() : INVALID_DOUBLE_VALUE);
                break;
            case VAR_COEMISSION:
                tempMsg.writeUnsignedByte(TYPE_DOUBLE);
                tempMsg.writeDouble(onRoad ? v->getCOEmissions() : INVALID_DOUBLE_VALUE);
                break;
            case VAR_HCEMISSION:
                tempMsg.writeUnsignedByte(TYPE_DOUBLE);
                tempMsg.writeDouble(onRoad ? v->getHCEmissions() : INVALID_DOUBLE_VALUE);
                break;
            case VAR_PMXEMISSION:
                tempMsg.writeUnsignedByte(TYPE_DOUBLE);
                tempMsg.writeDouble(onRoad ? v->getPMxEmissions() : INVALID_DOUBLE_VALUE);
                break;
            case VAR_NOXEMISSION:
                tempMsg.writeUnsignedByte(TYPE_DOUBLE);
                tempMsg.writeDouble(onRoad ? v->getNOxEmissions() : INVALID_DOUBLE_VALUE);
                break;
            case VAR_FUELCONSUMPTION:
                tempMsg.writeUnsignedByte(TYPE_DOUBLE);
                tempMsg.writeDouble(onRoad ? v->getFuelConsumption() : INVALID_DOUBLE_VALUE);
                break;
            case VAR_NOISEEMISSION:
                tempMsg.writeUnsignedByte(TYPE_DOUBLE);
                tempMsg.writeDouble(onRoad ? v->getHarmonoise_NoiseEmissions() : INVALID_DOUBLE_VALUE);
                break;
            case VAR_PERSON_NUMBER:
                tempMsg.writeUnsignedByte(TYPE_INTEGER);
                tempMsg.writeInt(v->getPersonNumber());
                break;
            case VAR_LEADER: {
                double dist = 0;
                if (!server.readTypeCheckingDouble(inputStorage, dist)) {
                    return server.writeErrorStatusCmd(CMD_GET_VEHICLE_VARIABLE, "Leader retrieval requires a double.", outputStorage);
                }
                std::pair<const MSVehicle* const, SUMOReal> leaderInfo = v->getLeader(dist);
                tempMsg.writeUnsignedByte(TYPE_COMPOUND);
                tempMsg.writeInt(2);
                tempMsg.writeUnsignedByte(TYPE_STRING);
                tempMsg.writeString(leaderInfo.first != 0 ? leaderInfo.first->getID() : "");
                tempMsg.writeUnsignedByte(TYPE_DOUBLE);
                tempMsg.writeDouble(leaderInfo.second);
            }
            break;
            case VAR_WAITING_TIME:
                tempMsg.writeUnsignedByte(TYPE_DOUBLE);
                tempMsg.writeDouble(v->getWaitingSeconds());
                break;
            case VAR_EDGE_TRAVELTIME: {
                if (inputStorage.readUnsignedByte() != TYPE_COMPOUND) {
                    return server.writeErrorStatusCmd(CMD_GET_VEHICLE_VARIABLE, "Retrieval of travel time requires a compound object.", outputStorage);
                }
                if (inputStorage.readInt() != 2) {
                    return server.writeErrorStatusCmd(CMD_GET_VEHICLE_VARIABLE, "Retrieval of travel time requires time, and edge as parameter.", outputStorage);
                }
                // time
                SUMOTime time = 0;
                if (!server.readTypeCheckingInt(inputStorage, time)) {
                    return server.writeErrorStatusCmd(CMD_GET_VEHICLE_VARIABLE, "Retrieval of travel time requires the referenced time as first parameter.", outputStorage);
                }
                // edge
                std::string edgeID;
                if (!server.readTypeCheckingString(inputStorage, edgeID)) {
                    return server.writeErrorStatusCmd(CMD_GET_VEHICLE_VARIABLE, "Retrieval of travel time requires the referenced edge as second parameter.", outputStorage);
                }
                MSEdge* edge = MSEdge::dictionary(edgeID);
                if (edge == 0) {
                    return server.writeErrorStatusCmd(CMD_GET_VEHICLE_VARIABLE, "Referenced edge '" + edgeID + "' is not known.", outputStorage);
                }
                // retrieve
                tempMsg.writeUnsignedByte(TYPE_DOUBLE);
                SUMOReal value;
                if (!v->getWeightsStorage().retrieveExistingTravelTime(edge, time, value)) {
                    tempMsg.writeDouble(INVALID_DOUBLE_VALUE);
                } else {
                    tempMsg.writeDouble(value);
                }

            }
            break;
            case VAR_EDGE_EFFORT: {
                if (inputStorage.readUnsignedByte() != TYPE_COMPOUND) {
                    return server.writeErrorStatusCmd(CMD_GET_VEHICLE_VARIABLE, "Retrieval of travel time requires a compound object.", outputStorage);
                }
                if (inputStorage.readInt() != 2) {
                    return server.writeErrorStatusCmd(CMD_GET_VEHICLE_VARIABLE, "Retrieval of travel time requires time, and edge as parameter.", outputStorage);
                }
                // time
                SUMOTime time = 0;
                if (!server.readTypeCheckingInt(inputStorage, time)) {
                    return server.writeErrorStatusCmd(CMD_GET_VEHICLE_VARIABLE, "Retrieval of effort requires the referenced time as first parameter.", outputStorage);
                }
                // edge
                std::string edgeID;
                if (!server.readTypeCheckingString(inputStorage, edgeID)) {
                    return server.writeErrorStatusCmd(CMD_GET_VEHICLE_VARIABLE, "Retrieval of effort requires the referenced edge as second parameter.", outputStorage);
                }
                MSEdge* edge = MSEdge::dictionary(edgeID);
                if (edge == 0) {
                    return server.writeErrorStatusCmd(CMD_GET_VEHICLE_VARIABLE, "Referenced edge '" + edgeID + "' is not known.", outputStorage);
                }
                // retrieve
                tempMsg.writeUnsignedByte(TYPE_DOUBLE);
                SUMOReal value;
                if (!v->getWeightsStorage().retrieveExistingEffort(edge, time, value)) {
                    tempMsg.writeDouble(INVALID_DOUBLE_VALUE);
                } else {
                    tempMsg.writeDouble(value);
                }

            }
            break;
            case VAR_ROUTE_VALID: {
                std::string msg;
                tempMsg.writeUnsignedByte(TYPE_UBYTE);
                tempMsg.writeUnsignedByte(v->hasValidRoute(msg));
            }
            break;
            case VAR_EDGES: {
                const MSRoute& r = v->getRoute();
                tempMsg.writeUnsignedByte(TYPE_STRINGLIST);
                tempMsg.writeInt(r.size());
                for (MSRouteIterator i = r.begin(); i != r.end(); ++i) {
                    tempMsg.writeString((*i)->getID());
                }
            }
            break;
            case VAR_SIGNALS:
                tempMsg.writeUnsignedByte(TYPE_INTEGER);
                tempMsg.writeInt(v->getSignals());
                break;
            case VAR_BEST_LANES: {
                tempMsg.writeUnsignedByte(TYPE_COMPOUND);
                tcpip::Storage tempContent;
                unsigned int cnt = 0;
                tempContent.writeUnsignedByte(TYPE_INTEGER);
                const std::vector<MSVehicle::LaneQ>& bestLanes = onRoad ? v->getBestLanes() : std::vector<MSVehicle::LaneQ>();
                tempContent.writeInt((int) bestLanes.size());
                ++cnt;
                for (std::vector<MSVehicle::LaneQ>::const_iterator i = bestLanes.begin(); i != bestLanes.end(); ++i) {
                    const MSVehicle::LaneQ& lq = *i;
                    tempContent.writeUnsignedByte(TYPE_STRING);
                    tempContent.writeString(lq.lane->getID());
                    ++cnt;
                    tempContent.writeUnsignedByte(TYPE_DOUBLE);
                    tempContent.writeDouble(lq.length);
                    ++cnt;
                    tempContent.writeUnsignedByte(TYPE_DOUBLE);
                    tempContent.writeDouble(lq.nextOccupation);
                    ++cnt;
                    tempContent.writeUnsignedByte(TYPE_BYTE);
                    tempContent.writeByte(lq.bestLaneOffset);
                    ++cnt;
                    tempContent.writeUnsignedByte(TYPE_UBYTE);
                    lq.allowsContinuation ? tempContent.writeUnsignedByte(1) : tempContent.writeUnsignedByte(0);
                    ++cnt;
                    std::vector<std::string> bestContIDs;
                    for (std::vector<MSLane*>::const_iterator j = lq.bestContinuations.begin(); j != lq.bestContinuations.end(); ++j) {
                        if ((*j) != 0) {
                            bestContIDs.push_back((*j)->getID());
                        }
                    }
                    tempContent.writeUnsignedByte(TYPE_STRINGLIST);
                    tempContent.writeStringList(bestContIDs);
                    ++cnt;
                }
                tempMsg.writeInt((int) cnt);
                tempMsg.writeStorage(tempContent);
            }
            break;
            case VAR_STOPSTATE: {
                char b = (
                             1 * (v->isStopped() ? 1 : 0) +
                             2 * (v->isParking() ? 1 : 0) +
                             4 * (v->isStoppedTriggered() ? 1 : 0));
                tempMsg.writeUnsignedByte(TYPE_UBYTE);
                tempMsg.writeUnsignedByte(b);
            }
            break;
            case VAR_DISTANCE: {
                tempMsg.writeUnsignedByte(TYPE_DOUBLE);
                SUMOReal distance = onRoad ? v->getRoute().getDistanceBetween(0, v->getPositionOnLane(), v->getRoute().getEdges()[0],  &v->getLane()->getEdge()) : INVALID_DOUBLE_VALUE;
                if (distance == std::numeric_limits<SUMOReal>::max()) {
                    distance = INVALID_DOUBLE_VALUE;
                }
                tempMsg.writeDouble(distance);
            }
            break;
            case DISTANCE_REQUEST:
                if (!commandDistanceRequest(server, inputStorage, tempMsg, v)) {
                    return false;
                }
                break;
            case VAR_ALLOWED_SPEED:
                tempMsg.writeUnsignedByte(TYPE_DOUBLE);
                tempMsg.writeDouble(onRoad ? v->getLane()->getVehicleMaxSpeed(v) : INVALID_DOUBLE_VALUE);
                break;
            case VAR_SPEED_FACTOR:
                tempMsg.writeUnsignedByte(TYPE_DOUBLE);
                tempMsg.writeDouble(v->getChosenSpeedFactor());
                break;
            case VAR_PARAMETER: {
                std::string paramName = "";
                if (!server.readTypeCheckingString(inputStorage, paramName)) {
                    return server.writeErrorStatusCmd(CMD_GET_VEHICLE_VARIABLE, "Retrieval of a parameter requires its name.", outputStorage);
                }
                tempMsg.writeUnsignedByte(TYPE_STRING);
                tempMsg.writeString(v->getParameter().getParameter(paramName, ""));
            }
            break;
            default:
                TraCIServerAPI_VehicleType::getVariable(variable, v->getVehicleType(), tempMsg);
                break;
        }
    }
    server.writeStatusCmd(CMD_GET_VEHICLE_VARIABLE, RTYPE_OK, "", outputStorage);
    server.writeResponseWithLength(outputStorage, tempMsg);
    return true;
}


bool
TraCIServerAPI_Vehicle::processSet(TraCIServer& server, tcpip::Storage& inputStorage,
                                   tcpip::Storage& outputStorage) {
    std::string warning = ""; // additional description for response
    // variable
    int variable = inputStorage.readUnsignedByte();
    if (variable != CMD_STOP && variable != CMD_CHANGELANE
            && variable != CMD_SLOWDOWN && variable != CMD_CHANGETARGET && variable != CMD_RESUME
            && variable != VAR_TYPE && variable != VAR_ROUTE_ID && variable != VAR_ROUTE
            && variable != VAR_EDGE_TRAVELTIME && variable != VAR_EDGE_EFFORT
            && variable != CMD_REROUTE_TRAVELTIME && variable != CMD_REROUTE_EFFORT
            && variable != VAR_SIGNALS && variable != VAR_MOVE_TO
            && variable != VAR_LENGTH && variable != VAR_MAXSPEED && variable != VAR_VEHICLECLASS
            && variable != VAR_SPEED_FACTOR && variable != VAR_EMISSIONCLASS
            && variable != VAR_WIDTH && variable != VAR_MINGAP && variable != VAR_SHAPECLASS
            && variable != VAR_ACCEL && variable != VAR_DECEL && variable != VAR_IMPERFECTION
            && variable != VAR_TAU && variable != VAR_LANECHANGE_MODE
            && variable != VAR_SPEED && variable != VAR_SPEEDSETMODE && variable != VAR_COLOR
            && variable != ADD && variable != ADD_FULL && variable != REMOVE
            && variable != VAR_MOVE_TO_VTD && variable != VAR_PARAMETER/* && variable != VAR_SPEED_TIME_LINE && variable != VAR_LANE_TIME_LINE*/
       ) {
        return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Change Vehicle State: unsupported variable specified", outputStorage);
    }
    // id
    std::string id = inputStorage.readString();
    const bool shouldExist = variable != ADD && variable != ADD_FULL;
    SUMOVehicle* sumoVehicle = MSNet::getInstance()->getVehicleControl().getVehicle(id);
    if (sumoVehicle == 0) {
        if (shouldExist) {
            return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Vehicle '" + id + "' is not known", outputStorage);
        }
    }
    MSVehicle* v = dynamic_cast<MSVehicle*>(sumoVehicle);
    if (v == 0 && shouldExist) {
        return server.writeErrorStatusCmd(CMD_GET_VEHICLE_VARIABLE, "Vehicle '" + id + "' is not a micro-simulation vehicle", outputStorage);
    }
    switch (variable) {
        case CMD_STOP: {
            if (inputStorage.readUnsignedByte() != TYPE_COMPOUND) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Stop needs a compound object description.", outputStorage);
            }
            int compoundSize = inputStorage.readInt();
            if (compoundSize != 4 && compoundSize != 5) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Stop needs a compound object description of four of five items.", outputStorage);
            }
            // read road map position
            std::string roadId;
            if (!server.readTypeCheckingString(inputStorage, roadId)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "The first stop parameter must be the edge id given as a string.", outputStorage);
            }
            double pos = 0;
            if (!server.readTypeCheckingDouble(inputStorage, pos)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "The second stop parameter must be the position along the edge given as a double.", outputStorage);
            }
            int laneIndex = 0;
            if (!server.readTypeCheckingByte(inputStorage, laneIndex)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "The third stop parameter must be the lane index given as a byte.", outputStorage);
            }
            // waitTime
            SUMOTime waitTime = 0;
            if (!server.readTypeCheckingInt(inputStorage, waitTime)) {
                return server.writeErrorStatusCmd(CMD_GET_VEHICLE_VARIABLE, "The fourth stop parameter must be the waiting time given as an integer.", outputStorage);
            }
            // optional stop flags
            bool parking = false;
            bool triggered = false;
            bool containerTriggered = false;
            if (compoundSize == 5) {
                int stopFlags;
                if (!server.readTypeCheckingByte(inputStorage, stopFlags)) {
                    return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "The fifth stop parameter must be a byte indicating its parking/triggered status.", outputStorage);
                }
                parking = ((stopFlags & 1) != 0);
                triggered = ((stopFlags & 2) != 0);
            }
            // check
            if (pos < 0) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Position on lane must not be negative.", outputStorage);
            }
            // get the actual lane that is referenced by laneIndex
            MSEdge* road = MSEdge::dictionary(roadId);
            if (road == 0) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Unable to retrieve road with given id.", outputStorage);
            }
            const std::vector<MSLane*>& allLanes = road->getLanes();
            if ((laneIndex < 0) || laneIndex >= (int)(allLanes.size())) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "No lane with index '" + toString(laneIndex) + "' on road '" + roadId + "'.", outputStorage);
            }
            // Forward command to vehicle
            std::string error;
            if (!v->addTraciStop(allLanes[laneIndex], pos, 0, waitTime, parking, triggered, containerTriggered, error)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, error, outputStorage);
            }
        }
        break;
        case CMD_RESUME: {
            if (inputStorage.readUnsignedByte() != TYPE_COMPOUND) {
                server.writeStatusCmd(CMD_SET_VEHICLE_VARIABLE, RTYPE_ERR, "Resuming requires a compound object.", outputStorage);
                return false;
            }
            if (inputStorage.readInt() != 0) {
                server.writeStatusCmd(CMD_SET_VEHICLE_VARIABLE, RTYPE_ERR, "Resuming should obtain an empty compound object.", outputStorage);
                return false;
            }
            if (!static_cast<MSVehicle*>(v)->hasStops()) {
                server.writeStatusCmd(CMD_SET_VEHICLE_VARIABLE, RTYPE_ERR, "Failed to resume vehicle '" + v->getID() + "', it has no stops.", outputStorage);
                return false;
            }
            if (!static_cast<MSVehicle*>(v)->resumeFromStopping()) {
                MSVehicle::Stop& sto = (static_cast<MSVehicle*>(v))->getNextStop();
                std::ostringstream strs;
                strs << "reached: " << sto.reached;
                strs << ", duration:" << sto.duration;
                strs << ", edge:" << (*sto.edge)->getID();
                strs << ", startPos: " << sto.startPos;
                std::string posStr = strs.str();
                server.writeStatusCmd(CMD_SET_VEHICLE_VARIABLE, RTYPE_ERR, "Failed to resume a non parking vehicle '" + v->getID() + "', " + posStr, outputStorage);
                return false;
            }
        }
        break;
        case CMD_CHANGELANE: {
            if (inputStorage.readUnsignedByte() != TYPE_COMPOUND) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Lane change needs a compound object description.", outputStorage);
            }
            if (inputStorage.readInt() != 2) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Lane change needs a compound object description of two items.", outputStorage);
            }
            // Lane ID
            int laneIndex = 0;
            if (!server.readTypeCheckingByte(inputStorage, laneIndex)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "The first lane change parameter must be the lane index given as a byte.", outputStorage);
            }
            // stickyTime
            SUMOTime stickyTime = 0;
            if (!server.readTypeCheckingInt(inputStorage, stickyTime)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "The second lane change parameter must be the duration given as an integer.", outputStorage);
            }
            if ((laneIndex < 0) || (laneIndex >= (int)(v->getEdge()->getLanes().size()))) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "No lane with index '" + toString(laneIndex) + "' on road '" + v->getEdge()->getID() + "'.", outputStorage);
            }
            // Forward command to vehicle
            std::vector<std::pair<SUMOTime, unsigned int> > laneTimeLine;
            laneTimeLine.push_back(std::make_pair(MSNet::getInstance()->getCurrentTimeStep(), laneIndex));
            laneTimeLine.push_back(std::make_pair(MSNet::getInstance()->getCurrentTimeStep() + stickyTime, laneIndex));
            v->getInfluencer().setLaneTimeLine(laneTimeLine);
        }
        break;
        /*
        case VAR_LANE_TIME_LINE: {
            if (inputStorage.readUnsignedByte() != TYPE_COMPOUND) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Lane change needs a compound object description.", outputStorage);
            }
            if (inputStorage.readInt() != 2) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Lane change needs a compound object description of two items.", outputStorage);
            }
            // Lane ID
            int laneIndex = 0;
            if (!server.readTypeCheckingByte(inputStorage, laneIndex)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "The first lane change parameter must be the lane index given as a byte.", outputStorage);
            }
            // stickyTime
            SUMOTime stickyTime = 0;
            if (!server.readTypeCheckingInt(inputStorage, stickyTime)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "The second lane change parameter must be the duration given as an integer.", outputStorage);
            }
            if ((laneIndex < 0) || (laneIndex >= (int)(v->getEdge()->getLanes().size()))) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "No lane existing with given id on the current road", outputStorage);
            }
            // Forward command to vehicle
            std::vector<std::pair<SUMOTime, unsigned int> > laneTimeLine;
            laneTimeLine.push_back(std::make_pair(MSNet::getInstance()->getCurrentTimeStep(), laneIndex));
            laneTimeLine.push_back(std::make_pair(MSNet::getInstance()->getCurrentTimeStep() + stickyTime, laneIndex));
            v->getInfluencer().setLaneTimeLine(laneTimeLine);
            MSVehicle::ChangeRequest req = v->getInfluencer().checkForLaneChanges(MSNet::getInstance()->getCurrentTimeStep(),
                                           *v->getEdge(), v->getLaneIndex());
            v->getLaneChangeModel().requestLaneChange(req);
        }
        break;
        */
        case CMD_SLOWDOWN: {
            if (inputStorage.readUnsignedByte() != TYPE_COMPOUND) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Slow down needs a compound object description.", outputStorage);
            }
            if (inputStorage.readInt() != 2) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Slow down needs a compound object description of two items.", outputStorage);
            }
            double newSpeed = 0;
            if (!server.readTypeCheckingDouble(inputStorage, newSpeed)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "The first slow down parameter must be the speed given as a double.", outputStorage);
            }
            if (newSpeed < 0) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Speed must not be negative", outputStorage);
            }
            SUMOTime duration = 0;
            if (!server.readTypeCheckingInt(inputStorage, duration)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "The second slow down parameter must be the duration given as an integer.", outputStorage);
            }
            if (duration < 0 || STEPS2TIME(MSNet::getInstance()->getCurrentTimeStep()) + STEPS2TIME(duration) > STEPS2TIME(SUMOTime_MAX - DELTA_T)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Invalid time interval", outputStorage);
            }
            std::vector<std::pair<SUMOTime, SUMOReal> > speedTimeLine;
            speedTimeLine.push_back(std::make_pair(MSNet::getInstance()->getCurrentTimeStep(), v->getSpeed()));
            speedTimeLine.push_back(std::make_pair(MSNet::getInstance()->getCurrentTimeStep() + duration, newSpeed));
            v->getInfluencer().setSpeedTimeLine(speedTimeLine);
        }
        break;
        case CMD_CHANGETARGET: {
            std::string edgeID;
            if (!server.readTypeCheckingString(inputStorage, edgeID)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Change target requires a string containing the id of the new destination edge as parameter.", outputStorage);
            }
            const MSEdge* destEdge = MSEdge::dictionary(edgeID);
            if (destEdge == 0) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Can not retrieve road with ID " + edgeID, outputStorage);
            }
            // build a new route between the vehicle's current edge and destination edge
            ConstMSEdgeVector newRoute;
            const MSEdge* currentEdge = v->getRerouteOrigin();
            MSNet::getInstance()->getRouterTT().compute(
                currentEdge, destEdge, (const MSVehicle * const) v, MSNet::getInstance()->getCurrentTimeStep(), newRoute);
            // replace the vehicle's route by the new one
            if (!v->replaceRouteEdges(newRoute, v->getLane() == 0)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Route replacement failed for " + v->getID(), outputStorage);
            }
        }
        break;
        case VAR_TYPE: {
            std::string vTypeID;
            if (!server.readTypeCheckingString(inputStorage, vTypeID)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "The vehicle type id must be given as a string.", outputStorage);
            }
            MSVehicleType* vehicleType = MSNet::getInstance()->getVehicleControl().getVType(vTypeID);
            if (vehicleType == 0) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "The vehicle type '" + vTypeID + "' is not known.", outputStorage);
            }
            v->replaceVehicleType(vehicleType);
        }
        break;
        case VAR_ROUTE_ID: {
            std::string rid;
            if (!server.readTypeCheckingString(inputStorage, rid)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "The route id must be given as a string.", outputStorage);
            }
            const MSRoute* r = MSRoute::dictionary(rid);
            if (r == 0) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "The route '" + rid + "' is not known.", outputStorage);
            }
            if (!v->replaceRoute(r, v->getLane() == 0)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Route replacement failed for " + v->getID(), outputStorage);
            }
        }
        break;
        case VAR_ROUTE: {
            std::vector<std::string> edgeIDs;
            if (!server.readTypeCheckingStringList(inputStorage, edgeIDs)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "A route must be defined as a list of edge ids.", outputStorage);
            }
            ConstMSEdgeVector edges;
            MSEdge::parseEdgesList(edgeIDs, edges, "<unknown>");
            if (!v->replaceRouteEdges(edges, v->getLane() == 0)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Route replacement failed for " + v->getID(), outputStorage);
            }
        }
        break;
        case VAR_EDGE_TRAVELTIME: {
            if (inputStorage.readUnsignedByte() != TYPE_COMPOUND) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Setting travel time requires a compound object.", outputStorage);
            }
            int parameterCount = inputStorage.readInt();
            if (parameterCount == 4) {
                // begin time
                SUMOTime begTime = 0, endTime = 0;
                if (!server.readTypeCheckingInt(inputStorage, begTime)) {
                    return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Setting travel time using 4 parameters requires the begin time as first parameter.", outputStorage);
                }
                // begin time
                if (!server.readTypeCheckingInt(inputStorage, endTime)) {
                    return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Setting travel time using 4 parameters requires the end time as second parameter.", outputStorage);
                }
                // edge
                std::string edgeID;
                if (!server.readTypeCheckingString(inputStorage, edgeID)) {
                    return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Setting travel time using 4 parameters requires the referenced edge as third parameter.", outputStorage);
                }
                MSEdge* edge = MSEdge::dictionary(edgeID);
                if (edge == 0) {
                    return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Referenced edge '" + edgeID + "' is not known.", outputStorage);
                }
                // value
                double value = 0;
                if (!server.readTypeCheckingDouble(inputStorage, value)) {
                    return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Setting travel time using 4 parameters requires the travel time as double as fourth parameter.", outputStorage);
                }
                // retrieve
                v->getWeightsStorage().addTravelTime(edge, begTime, endTime, value);
            } else if (parameterCount == 2) {
                // edge
                std::string edgeID;
                if (!server.readTypeCheckingString(inputStorage, edgeID)) {
                    return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Setting travel time using 2 parameters requires the referenced edge as first parameter.", outputStorage);
                }
                MSEdge* edge = MSEdge::dictionary(edgeID);
                if (edge == 0) {
                    return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Referenced edge '" + edgeID + "' is not known.", outputStorage);
                }
                // value
                double value = 0;
                if (!server.readTypeCheckingDouble(inputStorage, value)) {
                    return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Setting travel time using 2 parameters requires the travel time as second parameter.", outputStorage);
                }
                // retrieve
                while (v->getWeightsStorage().knowsTravelTime(edge)) {
                    v->getWeightsStorage().removeTravelTime(edge);
                }
                v->getWeightsStorage().addTravelTime(edge, 0, SUMOTime_MAX, value);
            } else if (parameterCount == 1) {
                // edge
                std::string edgeID;
                if (!server.readTypeCheckingString(inputStorage, edgeID)) {
                    return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Setting travel time using 1 parameter requires the referenced edge as first parameter.", outputStorage);
                }
                MSEdge* edge = MSEdge::dictionary(edgeID);
                if (edge == 0) {
                    return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Referenced edge '" + edgeID + "' is not known.", outputStorage);
                }
                // retrieve
                while (v->getWeightsStorage().knowsTravelTime(edge)) {
                    v->getWeightsStorage().removeTravelTime(edge);
                }
            } else {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Setting travel time requires 1, 2, or 4 parameters.", outputStorage);
            }
        }
        break;
        case VAR_EDGE_EFFORT: {
            if (inputStorage.readUnsignedByte() != TYPE_COMPOUND) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Setting effort requires a compound object.", outputStorage);
            }
            int parameterCount = inputStorage.readInt();
            if (parameterCount == 4) {
                // begin time
                SUMOTime begTime = 0, endTime = 0;
                if (!server.readTypeCheckingInt(inputStorage, begTime)) {
                    return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Setting effort using 4 parameters requires the begin time as first parameter.", outputStorage);
                }
                // begin time
                if (!server.readTypeCheckingInt(inputStorage, endTime)) {
                    return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Setting effort using 4 parameters requires the end time as second parameter.", outputStorage);
                }
                // edge
                std::string edgeID;
                if (!server.readTypeCheckingString(inputStorage, edgeID)) {
                    return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Setting effort using 4 parameters requires the referenced edge as third parameter.", outputStorage);
                }
                MSEdge* edge = MSEdge::dictionary(edgeID);
                if (edge == 0) {
                    return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Referenced edge '" + edgeID + "' is not known.", outputStorage);
                }
                // value
                double value = 0;
                if (!server.readTypeCheckingDouble(inputStorage, value)) {
                    return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Setting effort using 4 parameters requires the travel time as fourth parameter.", outputStorage);
                }
                // retrieve
                v->getWeightsStorage().addEffort(edge, begTime, endTime, value);
            } else if (parameterCount == 2) {
                // edge
                std::string edgeID;
                if (!server.readTypeCheckingString(inputStorage, edgeID)) {
                    return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Setting effort using 2 parameters requires the referenced edge as first parameter.", outputStorage);
                }
                MSEdge* edge = MSEdge::dictionary(edgeID);
                if (edge == 0) {
                    return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Referenced edge '" + edgeID + "' is not known.", outputStorage);
                }
                // value
                double value = 0;
                if (!server.readTypeCheckingDouble(inputStorage, value)) {
                    return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Setting effort using 2 parameters requires the travel time as second parameter.", outputStorage);
                }
                // retrieve
                while (v->getWeightsStorage().knowsEffort(edge)) {
                    v->getWeightsStorage().removeEffort(edge);
                }
                v->getWeightsStorage().addEffort(edge, 0, SUMOTime_MAX, value);
            } else if (parameterCount == 1) {
                // edge
                std::string edgeID;
                if (!server.readTypeCheckingString(inputStorage, edgeID)) {
                    return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Setting effort using 1 parameter requires the referenced edge as first parameter.", outputStorage);
                }
                MSEdge* edge = MSEdge::dictionary(edgeID);
                if (edge == 0) {
                    return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Referenced edge '" + edgeID + "' is not known.", outputStorage);
                }
                // retrieve
                while (v->getWeightsStorage().knowsEffort(edge)) {
                    v->getWeightsStorage().removeEffort(edge);
                }
            } else {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Setting effort requires 1, 2, or 4 parameters.", outputStorage);
            }
        }
        break;
        case CMD_REROUTE_TRAVELTIME: {
            if (inputStorage.readUnsignedByte() != TYPE_COMPOUND) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Rerouting requires a compound object.", outputStorage);
            }
            if (inputStorage.readInt() != 0) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Rerouting should obtain an empty compound object.", outputStorage);
            }
            v->reroute(MSNet::getInstance()->getCurrentTimeStep(), MSNet::getInstance()->getRouterTT());
        }
        break;
        case CMD_REROUTE_EFFORT: {
            if (inputStorage.readUnsignedByte() != TYPE_COMPOUND) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Rerouting requires a compound object.", outputStorage);
            }
            if (inputStorage.readInt() != 0) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Rerouting should obtain an empty compound object.", outputStorage);
            }
            v->reroute(MSNet::getInstance()->getCurrentTimeStep(), MSNet::getInstance()->getRouterEffort());
        }
        break;
        case VAR_SIGNALS: {
            int signals = 0;
            if (!server.readTypeCheckingInt(inputStorage, signals)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Setting signals requires an integer.", outputStorage);
            }
            v->switchOffSignal(0x0fffffff);
            v->switchOnSignal(signals);
        }
        break;
        case VAR_MOVE_TO: {
            if (inputStorage.readUnsignedByte() != TYPE_COMPOUND) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Setting position requires a compound object.", outputStorage);
            }
            if (inputStorage.readInt() != 2) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Setting position should obtain the lane id and the position.", outputStorage);
            }
            // lane ID
            std::string laneID;
            if (!server.readTypeCheckingString(inputStorage, laneID)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "The first parameter for setting a position must be the lane ID given as a string.", outputStorage);
            }
            // position on lane
            double position = 0;
            if (!server.readTypeCheckingDouble(inputStorage, position)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "The second parameter for setting a position must be the position given as a double.", outputStorage);
            }
            // process
            MSLane* l = MSLane::dictionary(laneID);
            if (l == 0) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Unknown lane '" + laneID + "'.", outputStorage);
            }
            MSEdge& destinationEdge = l->getEdge();
            if (!v->willPass(&destinationEdge)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Vehicle '" + laneID + "' may be set onto an edge to pass only.", outputStorage);
            }
            v->onRemovalFromNet(MSMoveReminder::NOTIFICATION_TELEPORT);
            v->getLane()->removeVehicle(v, MSMoveReminder::NOTIFICATION_TELEPORT);
            while (v->getEdge() != &destinationEdge) {
                const MSEdge* nextEdge = v->succEdge(1);
                // let the vehicle move to the next edge
                if (v->enterLaneAtMove(nextEdge->getLanes()[0], true)) {
                    MSNet::getInstance()->getVehicleControl().scheduleVehicleRemoval(v);
                    continue;
                }
            }
            l->forceVehicleInsertion(v, position);
        }
        break;
        case VAR_SPEED: {
            double speed = 0;
            if (!server.readTypeCheckingDouble(inputStorage, speed)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Setting speed requires a double.", outputStorage);
            }
            std::vector<std::pair<SUMOTime, SUMOReal> > speedTimeLine;
            if (speed >= 0) {
                speedTimeLine.push_back(std::make_pair(MSNet::getInstance()->getCurrentTimeStep(), speed));
                speedTimeLine.push_back(std::make_pair(SUMOTime_MAX - DELTA_T, speed));
            }
            v->getInfluencer().setSpeedTimeLine(speedTimeLine);
        }
        break;
        case VAR_SPEEDSETMODE: {
            int speedMode = 0;
            if (!server.readTypeCheckingInt(inputStorage, speedMode)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Setting speed mode requires an integer.", outputStorage);
            }
            v->getInfluencer().setConsiderSafeVelocity((speedMode & 1) != 0);
            v->getInfluencer().setConsiderMaxAcceleration((speedMode & 2) != 0);
            v->getInfluencer().setConsiderMaxDeceleration((speedMode & 4) != 0);
            v->getInfluencer().setRespectJunctionPriority((speedMode & 8) != 0);
            v->getInfluencer().setEmergencyBrakeRedLight((speedMode & 16) != 0);
        }
        break;
        case VAR_LANECHANGE_MODE: {
            int laneChangeMode = 0;
            if (!server.readTypeCheckingInt(inputStorage, laneChangeMode)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Setting lane change mode requires an integer.", outputStorage);
            }
            v->getInfluencer().setLaneChangeMode(laneChangeMode);
        }
        break;
        case VAR_COLOR: {
            RGBColor col;
            if (!server.readTypeCheckingColor(inputStorage, col)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "The color must be given using the according type.", outputStorage);
            }
            v->getParameter().color.set(col.red(), col.green(), col.blue(), col.alpha());
            v->getParameter().setParameter |= VEHPARS_COLOR_SET;
        }
        break;
        case ADD: {
            if (v != 0) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "The vehicle " + id + " to add already exists.", outputStorage);
            }
            if (inputStorage.readUnsignedByte() != TYPE_COMPOUND) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Adding a vehicle requires a compound object.", outputStorage);
            }
            if (inputStorage.readInt() != 6) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Adding a vehicle needs six parameters.", outputStorage);
            }
            SUMOVehicleParameter vehicleParams;
            vehicleParams.id = id;

            std::string vTypeID;
            if (!server.readTypeCheckingString(inputStorage, vTypeID)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "First parameter (type) requires a string.", outputStorage);
            }
            MSVehicleType* vehicleType = MSNet::getInstance()->getVehicleControl().getVType(vTypeID);
            if (!vehicleType) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Invalid type '" + vTypeID + "' for vehicle '" + id + "'", outputStorage);
            }

            std::string routeID;
            if (!server.readTypeCheckingString(inputStorage, routeID)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Second parameter (route) requires a string.", outputStorage);
            }
            const MSRoute* route = MSRoute::dictionary(routeID);
            if (!route) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Invalid route '" + routeID + "' for vehicle: '" + id + "'", outputStorage);
            }

            if (!server.readTypeCheckingInt(inputStorage, vehicleParams.depart)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Third parameter (depart) requires an integer.", outputStorage);
            }
            if (vehicleParams.depart < 0) {
                const int proc = static_cast<int>(-vehicleParams.depart);
                if (proc >= static_cast<int>(DEPART_DEF_MAX)) {
                    return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Invalid departure time.", outputStorage);
                }
                vehicleParams.departProcedure = (DepartDefinition)proc;
            } else if (vehicleParams.depart < MSNet::getInstance()->getCurrentTimeStep()) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Departure time in the past.", outputStorage);
            }

            double pos;
            if (!server.readTypeCheckingDouble(inputStorage, pos)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Fourth parameter (position) requires a double.", outputStorage);
            }
            vehicleParams.departPos = pos;
            if (vehicleParams.departPos < 0) {
                const int proc = static_cast<int>(-vehicleParams.departPos);
                if (fabs(proc + vehicleParams.departPos) > NUMERICAL_EPS || proc >= static_cast<int>(DEPART_POS_DEF_MAX) || proc == static_cast<int>(DEPART_POS_GIVEN)) {
                    return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Invalid departure position.", outputStorage);
                }
                vehicleParams.departPosProcedure = (DepartPosDefinition)proc;
            } else {
                vehicleParams.departPosProcedure = DEPART_POS_GIVEN;
            }

            double speed;
            if (!server.readTypeCheckingDouble(inputStorage, speed)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Fifth parameter (speed) requires a double.", outputStorage);
            }
            vehicleParams.departSpeed = speed;
            if (vehicleParams.departSpeed < 0) {
                const int proc = static_cast<int>(-vehicleParams.departSpeed);
                if (proc >= static_cast<int>(DEPART_SPEED_DEF_MAX)) {
                    return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Invalid departure speed.", outputStorage);
                }
                vehicleParams.departSpeedProcedure = (DepartSpeedDefinition)proc;
            } else {
                vehicleParams.departSpeedProcedure = DEPART_SPEED_GIVEN;
            }

            if (!server.readTypeCheckingByte(inputStorage, vehicleParams.departLane)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Sixth parameter (lane) requires a byte.", outputStorage);
            }

            if (vehicleParams.departLane < 0) {
                const int proc = static_cast<int>(-vehicleParams.departLane);
                if (proc >= static_cast<int>(DEPART_LANE_DEF_MAX)) {
                    return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Invalid departure lane.", outputStorage);
                }
                vehicleParams.departLaneProcedure = (DepartLaneDefinition)proc;
            } else {
                vehicleParams.departLaneProcedure = DEPART_LANE_GIVEN;
            }

            SUMOVehicleParameter* params = new SUMOVehicleParameter(vehicleParams);
            try {
                SUMOVehicle* vehicle = MSNet::getInstance()->getVehicleControl().buildVehicle(params, route, vehicleType, true, false);
                MSNet::getInstance()->getVehicleControl().addVehicle(vehicleParams.id, vehicle);
                MSNet::getInstance()->getInsertionControl().add(vehicle);
            } catch (ProcessError& e) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, e.what(), outputStorage);
            }
        }
        break;
        case ADD_FULL: {
            if (v != 0) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "The vehicle " + id + " to add already exists.", outputStorage);
            }
            if (inputStorage.readUnsignedByte() != TYPE_COMPOUND) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Adding a vehicle requires a compound object.", outputStorage);
            }
            if (inputStorage.readInt() != 14) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Adding a fully specified vehicle needs fourteen parameters.", outputStorage);
            }
            SUMOVehicleParameter vehicleParams;
            vehicleParams.id = id;

            std::string routeID;
            if (!server.readTypeCheckingString(inputStorage, routeID)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Second parameter (route) requires a string.", outputStorage);
            }
            const MSRoute* route = MSRoute::dictionary(routeID);
            if (!route) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Invalid route '" + routeID + "' for vehicle: '" + id + "'", outputStorage);
            }

            std::string vTypeID;
            if (!server.readTypeCheckingString(inputStorage, vTypeID)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "First parameter (type) requires a string.", outputStorage);
            }
            MSVehicleType* vehicleType = MSNet::getInstance()->getVehicleControl().getVType(vTypeID);
            if (!vehicleType) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Invalid type '" + vTypeID + "' for vehicle '" + id + "'", outputStorage);
            }

            std::string helper;
            std::string error;
            if (!server.readTypeCheckingString(inputStorage, helper)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Third parameter (depart) requires an string.", outputStorage);
            }
            if (!SUMOVehicleParameter::parseDepart(helper, "vehicle", id, vehicleParams.depart, vehicleParams.departProcedure, error)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, error, outputStorage);
            }
            if (vehicleParams.departProcedure == DEPART_GIVEN && vehicleParams.depart < MSNet::getInstance()->getCurrentTimeStep()) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Departure time in the past.", outputStorage);
            }

            if (!server.readTypeCheckingString(inputStorage, helper)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Fourth parameter (depart lane) requires a string.", outputStorage);
            }
            if (!SUMOVehicleParameter::parseDepartLane(helper, "vehicle", id, vehicleParams.departLane, vehicleParams.departLaneProcedure, error)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, error, outputStorage);
            }
            if (!server.readTypeCheckingString(inputStorage, helper)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Fifth parameter (depart position) requires a string.", outputStorage);
            }
            if (!SUMOVehicleParameter::parseDepartPos(helper, "vehicle", id, vehicleParams.departPos, vehicleParams.departPosProcedure, error)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, error, outputStorage);
            }
            if (!server.readTypeCheckingString(inputStorage, helper)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Sixth parameter (depart speed) requires a string.", outputStorage);
            }
            if (!SUMOVehicleParameter::parseDepartSpeed(helper, "vehicle", id, vehicleParams.departSpeed, vehicleParams.departSpeedProcedure, error)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, error, outputStorage);
            }

            if (!server.readTypeCheckingString(inputStorage, helper)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Seventh parameter (arrival lane) requires a string.", outputStorage);
            }
            if (!SUMOVehicleParameter::parseArrivalLane(helper, "vehicle", id, vehicleParams.arrivalLane, vehicleParams.arrivalLaneProcedure, error)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, error, outputStorage);
            }
            if (!server.readTypeCheckingString(inputStorage, helper)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Eighth parameter (arrival position) requires a string.", outputStorage);
            }
            if (!SUMOVehicleParameter::parseArrivalPos(helper, "vehicle", id, vehicleParams.arrivalPos, vehicleParams.arrivalPosProcedure, error)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, error, outputStorage);
            }
            if (!server.readTypeCheckingString(inputStorage, helper)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Ninth parameter (arrival speed) requires a string.", outputStorage);
            }
            if (!SUMOVehicleParameter::parseArrivalSpeed(helper, "vehicle", id, vehicleParams.arrivalSpeed, vehicleParams.arrivalSpeedProcedure, error)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, error, outputStorage);
            }

            if (!server.readTypeCheckingString(inputStorage, vehicleParams.fromTaz)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Tenth parameter (from taz) requires a string.", outputStorage);
            }
            if (!server.readTypeCheckingString(inputStorage, vehicleParams.toTaz)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Eleventh parameter (to taz) requires a string.", outputStorage);
            }
            if (!server.readTypeCheckingString(inputStorage, vehicleParams.line)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Twelth parameter (line) requires a string.", outputStorage);
            }

            int num;
            if (!server.readTypeCheckingInt(inputStorage, num)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "13th parameter (person capacity) requires an int.", outputStorage);
            }
            vehicleParams.personCapacity = num;
            if (!server.readTypeCheckingInt(inputStorage, num)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "14th parameter (person number) requires an int.", outputStorage);
            }
            vehicleParams.personNumber = num;

            SUMOVehicleParameter* params = new SUMOVehicleParameter(vehicleParams);
            try {
                SUMOVehicle* vehicle = MSNet::getInstance()->getVehicleControl().buildVehicle(params, route, vehicleType, true, false);
                MSNet::getInstance()->getVehicleControl().addVehicle(vehicleParams.id, vehicle);
                MSNet::getInstance()->getInsertionControl().add(vehicle);
            } catch (ProcessError& e) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, e.what(), outputStorage);
            }
        }
        break;
        case REMOVE: {
            int why = 0;
            if (!server.readTypeCheckingByte(inputStorage, why)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Removing a vehicle requires a byte.", outputStorage);
            }
            MSMoveReminder::Notification n = MSMoveReminder::NOTIFICATION_ARRIVED;
            switch (why) {
                case REMOVE_TELEPORT:
                    // XXX semantics unclear
                    // n = MSMoveReminder::NOTIFICATION_TELEPORT;
                    n = MSMoveReminder::NOTIFICATION_TELEPORT_ARRIVED;
                    break;
                case REMOVE_PARKING:
                    // XXX semantics unclear
                    // n = MSMoveReminder::NOTIFICATION_PARKING;
                    n = MSMoveReminder::NOTIFICATION_ARRIVED;
                    break;
                case REMOVE_ARRIVED:
                    n = MSMoveReminder::NOTIFICATION_ARRIVED;
                    break;
                case REMOVE_VAPORIZED:
                    n = MSMoveReminder::NOTIFICATION_VAPORIZED;
                    break;
                case REMOVE_TELEPORT_ARRIVED:
                    n = MSMoveReminder::NOTIFICATION_TELEPORT_ARRIVED;
                    break;
                default:
                    return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Unknown removal status.", outputStorage);
            }
            if (v->hasDeparted()) {
                v->onRemovalFromNet(n);
                if (v->getLane() != 0) {
                    v->getLane()->removeVehicle(v, n);
                }
                MSNet::getInstance()->getVehicleControl().scheduleVehicleRemoval(v);
            }
        }
        break;
        case VAR_MOVE_TO_VTD: {
            if (inputStorage.readUnsignedByte() != TYPE_COMPOUND) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Setting VTD vehicle requires a compound object.", outputStorage);
            }
            if (inputStorage.readInt() != 5) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Setting VTD vehicle should obtain: edgeID, lane, x, y, angle.", outputStorage);
            }
            // edge ID
            std::string edgeID;
            if (!server.readTypeCheckingString(inputStorage, edgeID)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "The first parameter for setting a VTD vehicle must be the edge ID given as a string.", outputStorage);
            }
            // lane index
            int laneNum = 0;
            if (!server.readTypeCheckingInt(inputStorage, laneNum)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "The second parameter for setting a VTD vehicle must be lane given as an int.", outputStorage);
            }
            // x
            double x = 0, y = 0, angle = 0;
            if (!server.readTypeCheckingDouble(inputStorage, x)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "The third parameter for setting a VTD vehicle must be the x-position given as a double.", outputStorage);
            }
            // y
            if (!server.readTypeCheckingDouble(inputStorage, y)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "The fourth parameter for setting a VTD vehicle must be the y-position given as a double.", outputStorage);
            }
            // angle
            if (!server.readTypeCheckingDouble(inputStorage, angle)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "The fifth parameter for setting a VTD vehicle must be the angle given as a double.", outputStorage);
            }
            // process
            if (!v->isOnRoad()) {
                break;
            }
            std::string origID = edgeID + " " + toString(laneNum);
            if (laneNum < 0) {
                edgeID = '-' + edgeID;
                laneNum = -laneNum;
            }
            Position pos(x, y);
            angle *= -1.;
            if (fabs(angle) > 180.) {
                angle = 180. - angle;
            }

            Position vehPos = v->getPosition();
            v->getBestLanes();
#ifdef DEBUG_VTD
            std::cout << std::endl << "begin vehicle " << v->getID() << " vehPos:" << vehPos << " lane:" << v->getLane()->getID() << std::endl;
            std::cout << " want pos:" << pos << " edge:" << edgeID << " laneNum:" << laneNum << " angle:" << angle << std::endl;
#endif

            ConstMSEdgeVector edges;
            MSLane* lane = 0;
            SUMOReal lanePos;
            SUMOReal bestDistance = std::numeric_limits<SUMOReal>::max();
            int routeOffset = 0;
            // case a): edge/lane is known and matches route
            bool dFound = vtdMap(pos, origID, angle, *v, server, bestDistance, &lane, lanePos, routeOffset, edges);
            //
            SUMOReal maxRouteDistance = 100;
            /*
            if (cFound && (bestDistanceA > maxRouteDistance && bestDistanceC > maxRouteDistance)) {
                // both route-based approach yield in a position too far away from the submitted --> new route!?
                server.setVTDControlled(v, laneC, lanePosC, routeOffsetC, edgesC);
            } else {
            */
            // use the best we have
            if (dFound && maxRouteDistance > bestDistance) {
                server.setVTDControlled(v, lane, lanePos, routeOffset, edges, MSNet::getInstance()->getCurrentTimeStep());
            } else {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Could not map vehicle '" + id + "'.", outputStorage);
            }
            //}
        }
        break;
        case VAR_SPEED_FACTOR: {
            double factor = 0;
            if (!server.readTypeCheckingDouble(inputStorage, factor)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "Setting speed factor requires a double.", outputStorage);
            }
            v->setChosenSpeedFactor(factor);
        }
        break;
        case VAR_PARAMETER: {
            if (inputStorage.readUnsignedByte() != TYPE_COMPOUND) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "A compound object is needed for setting a parameter.", outputStorage);
            }
            //readt itemNo
            inputStorage.readInt();
            std::string name;
            if (!server.readTypeCheckingString(inputStorage, name)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "The name of the parameter must be given as a string.", outputStorage);
            }
            std::string value;
            if (!server.readTypeCheckingString(inputStorage, value)) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, "The value of the parameter must be given as a string.", outputStorage);
            }
            ((SUMOVehicleParameter&) v->getParameter()).addParameter(name, value);
        }
        break;
        default:
            try {
                if (!TraCIServerAPI_VehicleType::setVariable(CMD_SET_VEHICLE_VARIABLE, variable, getSingularType(v), server, inputStorage, outputStorage)) {
                    return false;
                }
            } catch (ProcessError& e) {
                return server.writeErrorStatusCmd(CMD_SET_VEHICLE_VARIABLE, e.what(), outputStorage);
            }
            break;
    }
    server.writeStatusCmd(CMD_SET_VEHICLE_VARIABLE, RTYPE_OK, warning, outputStorage);
    return true;
}


bool
TraCIServerAPI_Vehicle::vtdMap(const Position& pos, const std::string& origID, const SUMOReal angle,  MSVehicle& v, TraCIServer& server,
                               SUMOReal& bestDistance, MSLane** lane, SUMOReal& lanePos, int& routeOffset, ConstMSEdgeVector& edges) {
    SUMOReal speed = pos.distanceTo2D(v.getPosition()); // !!!v.getSpeed();
    std::set<std::string> into;
    PositionVector shape;
    shape.push_back(pos);
    server.collectObjectsInRange(CMD_GET_EDGE_VARIABLE, shape, speed * 2, into);
    SUMOReal maxDist = 0;
    std::map<MSLane*, LaneUtility> lane2utility;
    for (std::set<std::string>::const_iterator j = into.begin(); j != into.end(); ++j) {
        MSEdge* e = MSEdge::dictionary(*j);
        const MSEdge* prevEdge = 0;
        const MSEdge* nextEdge = 0;
        MSEdge::EdgeBasicFunction ef = e->getPurpose();
        bool onRoute = false;
        if (ef != MSEdge::EDGEFUNCTION_INTERNAL) {
            const ConstMSEdgeVector& ev = v.getRoute().getEdges();
            unsigned int routePosition = v.getRoutePosition();
            if (v.getLane()->getEdge().getPurpose() == MSEdge::EDGEFUNCTION_INTERNAL) {
                ++routePosition;
            }
            ConstMSEdgeVector::const_iterator edgePos = std::find(ev.begin() + routePosition, ev.end(), e);
            onRoute = edgePos != ev.end();
            if (edgePos == ev.end() - 1 && v.getEdge() == e) {
                onRoute &= v.getEdge()->getLanes()[0]->getLength() > v.getPositionOnLane() + SPEED2DIST(speed);
            }
            prevEdge = e;
            nextEdge = !onRoute || edgePos == ev.end() - 1 ? 0 : *(edgePos + 1);
        } else {
            prevEdge = e;
            while (prevEdge != 0 && prevEdge->getPurpose() == MSEdge::EDGEFUNCTION_INTERNAL) {
                MSLane* l = prevEdge->getLanes()[0];
                l = l->getLogicalPredecessorLane();
                prevEdge = l == 0 ? 0 : &l->getEdge();
            }
            const ConstMSEdgeVector& ev = v.getRoute().getEdges();
            ConstMSEdgeVector::const_iterator prevEdgePos = std::find(ev.begin() + v.getRoutePosition(), ev.end(), prevEdge);
            if (prevEdgePos != ev.end() && ev.size() > 1 && prevEdgePos != ev.end() - 1) {
                const MSJunction* junction = e->getFromJunction();
                const ConstMSEdgeVector& outgoing = junction->getOutgoing();
                ConstMSEdgeVector::const_iterator nextEdgePos = std::find(outgoing.begin(), outgoing.end(), *(ev.begin() + v.getRoutePosition() + 1));
                if (nextEdgePos != outgoing.end()) {
                    nextEdge = *nextEdgePos;
                    onRoute = true;
                }
            }
        }


        const std::vector<MSLane*>& lanes = e->getLanes();
        for (std::vector<MSLane*>::const_iterator k = lanes.begin(); k != lanes.end(); ++k) {
            MSLane* lane = *k;
            SUMOReal off = lane->getShape().nearest_offset_to_point2D(pos);
            SUMOReal langle = 180.;
            SUMOReal dist = 1000.;
            if (off >= 0) {
                dist = lane->getShape().distance(pos);
                if (dist > lane->getLength()) { // this is a workaround
                    // a SmartDB, running at :49_2 delivers off=~9.24 while dist>24.?
                    dist = 1000.;
                } else {
                    langle = lane->getShape().rotationDegreeAtOffset(off);
                }
            }
            maxDist = MAX2(maxDist, dist);
            bool sameEdge = &lane->getEdge() == &v.getLane()->getEdge() && v.getEdge()->getLanes()[0]->getLength() > v.getPositionOnLane() + SPEED2DIST(speed);
            const MSEdge* rNextEdge = nextEdge;
            if (rNextEdge == 0 && lane->getEdge().getPurpose() == MSEdge::EDGEFUNCTION_INTERNAL) {
                MSLane* next = lane->getLinkCont()[0]->getLane();
                rNextEdge = next == 0 ? 0 : &next->getEdge();
            }
#ifdef DEBUG_VTD_ANGLE
            std::cout << lane->getID() << ": " << langle << " " << off << std::endl;
#endif
            lane2utility[lane] = LaneUtility(
                                     dist, GeomHelper::getMinAngleDiff(angle, langle),
                                     lane->getParameter("origId", "") == origID,
                                     onRoute, sameEdge, prevEdge, rNextEdge);
        }
    }

    SUMOReal bestValue = 0;
    MSLane* bestLane = 0;
    for (std::map<MSLane*, LaneUtility>::iterator i = lane2utility.begin(); i != lane2utility.end(); ++i) {
        MSLane* l = (*i).first;
        const LaneUtility& u = (*i).second;
        SUMOReal distN = u.dist > 999 ? -10 : 1. - (u.dist / maxDist);
        SUMOReal angleDiffN = 1. - (u.angleDiff / 180.);
        SUMOReal idN = u.ID ? 1 : 0;
        SUMOReal onRouteN = u.onRoute ? 1 : 0;
        SUMOReal sameEdgeN = u.sameEdge ? MIN2(v.getEdge()->getLength() / speed, (SUMOReal)1.) : 0;
        SUMOReal value = distN * .5
                         + angleDiffN * 0 /*.5 */
                         + idN * .5
                         + onRouteN * 0.5
                         + sameEdgeN * 0.5
                         ;
#ifdef DEBUG_VTD
        std::cout << " x; l:" << l->getID() << " d:" << u.dist << " dN:" << distN << " aD:" << angleDiffN <<
                  " ID:" << idN << " oRN:" << onRouteN << " sEN:" << sameEdgeN << " value:" << value << std::endl;
#endif
        if (value > bestValue || bestLane == 0) {
            bestValue = value;
            bestLane = l;
        }
    }
    if (bestLane == 0) {
        return false;
    }
    const LaneUtility& u = lane2utility.find(bestLane)->second;
    bestDistance = u.dist;
    *lane = bestLane;
    lanePos = bestLane->getShape().nearest_offset_to_point2D(pos);
    const MSEdge* prevEdge = u.prevEdge;
    if (u.onRoute) {
        const ConstMSEdgeVector& ev = v.getRoute().getEdges();
        ConstMSEdgeVector::const_iterator prevEdgePos = std::find(ev.begin() + v.getRoutePosition(), ev.end(), prevEdge);
        routeOffset = (int)std::distance(ev.begin(), prevEdgePos) - v.getRoutePosition();
    } else {
        edges.push_back(prevEdge);
        if (u.nextEdge != 0) {
            edges.push_back(u.nextEdge);
        }
        routeOffset = 0;
    }
    return true;
}

bool
TraCIServerAPI_Vehicle::commandDistanceRequest(TraCIServer& server, tcpip::Storage& inputStorage,
        tcpip::Storage& outputStorage, const MSVehicle* v) {
    if (inputStorage.readUnsignedByte() != TYPE_COMPOUND) {
        return server.writeErrorStatusCmd(CMD_GET_VEHICLE_VARIABLE, "Retrieval of distance requires a compound object.", outputStorage);
    }
    if (inputStorage.readInt() != 2) {
        return server.writeErrorStatusCmd(CMD_GET_VEHICLE_VARIABLE, "Retrieval of distance requires position and distance type as parameter.", outputStorage);
    }

    Position pos;
    std::pair<const MSLane*, SUMOReal> roadPos;

    // read position
    int posType = inputStorage.readUnsignedByte();
    switch (posType) {
        case POSITION_ROADMAP:
            try {
                std::string roadID = inputStorage.readString();
                roadPos.second = inputStorage.readDouble();
                roadPos.first = TraCIServerAPI_Simulation::getLaneChecking(roadID, inputStorage.readUnsignedByte(), roadPos.second);
                pos = roadPos.first->getShape().positionAtOffset(roadPos.second);
            } catch (TraCIException& e) {
                return server.writeErrorStatusCmd(CMD_GET_VEHICLE_VARIABLE, e.what(), outputStorage);
            }
            break;
        case POSITION_2D:
        case POSITION_3D: {
            const double p1x = inputStorage.readDouble();
            const double p1y = inputStorage.readDouble();
            pos.set(p1x, p1y);
        }
        if (posType == POSITION_3D) {
            inputStorage.readDouble();        // z value is ignored
        }
        roadPos = TraCIServerAPI_Simulation::convertCartesianToRoadMap(pos);
        break;
        default:
            return server.writeErrorStatusCmd(CMD_GET_VEHICLE_VARIABLE, "Unknown position format used for distance request", outputStorage);
    }

    // read distance type
    int distType = inputStorage.readUnsignedByte();

    SUMOReal distance = INVALID_DOUBLE_VALUE;
    if (v->isOnRoad()) {
        if (distType == REQUEST_DRIVINGDIST) {
            distance = v->getRoute().getDistanceBetween(v->getPositionOnLane(), roadPos.second,
                       v->getEdge(), &roadPos.first->getEdge());
            if (distance == std::numeric_limits<SUMOReal>::max()) {
                distance = INVALID_DOUBLE_VALUE;
            }
        } else {
            // compute air distance (default)
            distance = v->getPosition().distanceTo(pos);
        }
    }
    // write response command
    outputStorage.writeUnsignedByte(TYPE_DOUBLE);
    outputStorage.writeDouble(distance);
    return true;
}


// ------ helper functions ------
bool
TraCIServerAPI_Vehicle::getPosition(const std::string& id, Position& p) {
    MSVehicle* v = dynamic_cast<MSVehicle*>(MSNet::getInstance()->getVehicleControl().getVehicle(id));
    if (v == 0) {
        return false;
    }
    p = v->getPosition();
    return true;
}


MSVehicleType&
TraCIServerAPI_Vehicle::getSingularType(SUMOVehicle* const veh) {
    const MSVehicleType& oType = veh->getVehicleType();
    std::string newID = oType.getID().find('@') == std::string::npos ? oType.getID() + "@" + veh->getID() : oType.getID();
    MSVehicleType* type = MSVehicleType::build(newID, &oType);
    static_cast<MSVehicle*>(veh)->replaceVehicleType(type);
    return *type;
}


#include <microsim/MSEdgeControl.h>

const std::map<std::string, std::vector<MSLane*> >&
TraCIServerAPI_Vehicle::getOrBuildVTDMap() {
    if (gVTDMap.size() == 0) {
        const MSEdgeVector& edges = MSNet::getInstance()->getEdgeControl().getEdges();
        for (MSEdgeVector::const_iterator i = edges.begin(); i != edges.end(); ++i) {
            const std::vector<MSLane*>& lanes = (*i)->getLanes();
            for (std::vector<MSLane*>::const_iterator j = lanes.begin(); j != lanes.end(); ++j) {
                if ((*j)->knowsParameter("origId")) {
                    std::string origID = (*j)->getParameter("origId", "");
                    if (gVTDMap.find(origID) == gVTDMap.end()) {
                        gVTDMap[origID] = std::vector<MSLane*>();
                    }
                    gVTDMap[origID].push_back(*j);
                }
            }
        }
        if (gVTDMap.size() == 0) {
            gVTDMap["unknown"] = std::vector<MSLane*>();
        }
    }
    return gVTDMap;
}


#endif


/****************************************************************************/

