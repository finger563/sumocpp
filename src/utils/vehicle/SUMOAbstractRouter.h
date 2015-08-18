/****************************************************************************/
/// @file    SUMOAbstractRouter.h
/// @author  Daniel Krajzewicz
/// @author  Michael Behrisch
/// @author  Jakob Erdmann
/// @date    25.Jan 2006
/// @version $Id: SUMOAbstractRouter.h 18095 2015-03-17 09:39:00Z behrisch $
///
// The dijkstra-router
/****************************************************************************/
// SUMO, Simulation of Urban MObility; see http://sumo.dlr.de/
// Copyright (C) 2006-2015 DLR (http://www.dlr.de/) and contributors
/****************************************************************************/
//
//   This file is part of SUMO.
//   SUMO is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
/****************************************************************************/
#ifndef SUMOAbstractRouter_h
#define SUMOAbstractRouter_h


// ===========================================================================
// included modules
// ===========================================================================
#ifdef _MSC_VER
#include <windows_config.h>
#else
#include <config.h>
#endif

#include <string>
#include <vector>
#include <algorithm>
#include <assert.h>
#include <utils/common/SysUtils.h>
#include <utils/common/MsgHandler.h>
#include <utils/common/SUMOTime.h>
#include <utils/common/ToString.h>


// ===========================================================================
// class definitions
// ===========================================================================
/**
 * @class SUMOAbstractRouter
 * The interface for routing the vehicles over the network.
 */
template<class E, class V>
class SUMOAbstractRouter {
public:
    /// Type of the function that is used to retrieve the edge effort.
    typedef SUMOReal(* Operation)(const E* const, const V* const, SUMOReal);

    /// Constructor
    SUMOAbstractRouter(Operation operation, const std::string& type):
        myOperation(operation),
        myType(type),
        myQueryVisits(0),
        myNumQueries(0),
        myQueryStartTime(0),
        myQueryTimeSum(0)
    { }

    /// Destructor
    virtual ~SUMOAbstractRouter() {
        if (myNumQueries > 0) {
            WRITE_MESSAGE(myType + " answered " + toString(myNumQueries) + " queries and explored " + toString(double(myQueryVisits) / myNumQueries) +  " edges on average.");
            WRITE_MESSAGE(myType + " spent " + toString(myQueryTimeSum) + "ms answering queries (" + toString(double(myQueryTimeSum) / myNumQueries) +  "ms on average).");
        }
    }

    virtual SUMOAbstractRouter* clone() const = 0;

    /** @brief Builds the route between the given edges using the minimum effort at the given time
        The definition of the effort depends on the wished routing scheme */
    virtual void compute(const E* from, const E* to, const V* const vehicle,
                         SUMOTime msTime, std::vector<const E*>& into) = 0;

    virtual SUMOReal recomputeCosts(const std::vector<const E*>& edges,
                                    const V* const v, SUMOTime msTime) const = 0;

    // interface extension for BulkStarRouter
    virtual void prepare(const E*, const V*, bool) {
        assert(false);
    }

    inline SUMOReal getEffort(const E* const e, const V* const v, SUMOReal t) const {
        return (*myOperation)(e, v, t);
    }

    inline void startQuery() {
        myNumQueries++;
        myQueryStartTime = SysUtils::getCurrentMillis();
    }

    inline void endQuery(int visits) {
        myQueryVisits += visits;
        myQueryTimeSum += (SysUtils::getCurrentMillis() - myQueryStartTime);
    }

protected:
    /// @brief The object's operation to perform.
    Operation myOperation;

private:
    /// @brief the type of this router
    const std::string myType;

    /// @brief counters for performance logging
    SUMOLong myQueryVisits;
    SUMOLong myNumQueries;
    /// @brief the time spent querying in milliseconds
    SUMOLong myQueryStartTime;
    SUMOLong myQueryTimeSum;
private:
    /// @brief Invalidated assignment operator
    SUMOAbstractRouter& operator=(const SUMOAbstractRouter& s);
};


template<class E, class V>
struct prohibited_withRestrictions {
public:
    inline bool operator()(const E* edge, const V* vehicle) const {
        if (std::find(myProhibited.begin(), myProhibited.end(), edge) != myProhibited.end()) {
            return true;
        }
        return edge->prohibits(vehicle);
    }

    void prohibit(const std::vector<E*>& toProhibit) {
        myProhibited = toProhibit;
    }

protected:
    std::vector<E*> myProhibited;

};

template<class E, class V>
struct prohibited_noRestrictions {
public:
    inline bool operator()(const E*, const V*) const {
        return false;
    }
};




#endif

/****************************************************************************/

