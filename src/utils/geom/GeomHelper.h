/****************************************************************************/
/// @file    GeomHelper.h
/// @author  Daniel Krajzewicz
/// @author  Friedemann Wesner
/// @author  Jakob Erdmann
/// @author  Michael Behrisch
/// @date    Sept 2002
/// @version $Id: GeomHelper.h 18095 2015-03-17 09:39:00Z behrisch $
///
// Some static methods performing geometrical operations
/****************************************************************************/
// SUMO, Simulation of Urban MObility; see http://sumo.dlr.de/
// Copyright (C) 2001-2015 DLR (http://www.dlr.de/) and contributors
/****************************************************************************/
//
//   This file is part of SUMO.
//   SUMO is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
/****************************************************************************/
#ifndef GeomHelper_h
#define GeomHelper_h


// ===========================================================================
// included modules
// ===========================================================================
#ifdef _MSC_VER
#include <windows_config.h>
#else
#include <config.h>
#endif

#include <cmath>
#include "Position.h"
#include "PositionVector.h"
#include <utils/common/UtilExceptions.h>

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

#define DEG2RAD(x) static_cast<SUMOReal>((x) * M_PI / 180.)
#define RAD2DEG(x) static_cast<SUMOReal>((x) * 180. / M_PI)


// ===========================================================================
// class definitions
// ===========================================================================
/** @class GeomHelper
 * @brief Some static methods performing geometrical operations
 */
class GeomHelper {

public:
    /// @brief a value to signify offsets outside the range of [0, Line.length()]
    static const SUMOReal INVALID_OFFSET;

    /** @brief return whether given lines intersect
     * @param[in] p11 The begin position of the first line
     * @param[in] p12 The end position of the first line
     * @param[in] p21 The begin position of the second line
     * @param[in] p22 The end position of the second line
     * @return Whether both lines intersect
     */
    static bool intersects(const Position& p11, const Position& p12,
                           const Position& p21, const Position& p22);


    /** @brief Returns whether the given point lies on the given line
     * @param[in] p The position
     * @param[in] from The begin position of the line
     * @param[in] to The end position of the line
     * @return Whether the point lies on the line
     */
    static bool pointOnLine(const Position& p, const Position& from, const Position& to);


    /** @brief Returns the positions the given circle is crossed by the given line
     * @param[in] c The center position of the circle
     * @param[in] radius The radius of the circle
     * @param[in] p1 The begin of the line
     * @param[in] p2 The end of the line
     * @param[filled] into The list of crossing positions (0-1 along the line's length)
     * @see http://blog.csharphelper.com/2010/03/28/determine-where-a-line-intersects-a-circle-in-c.aspx
     * @see http://gamedev.stackexchange.com/questions/18333/circle-line-collision-detection-problem (jazzdawg)
     */
    static void FindLineCircleIntersections(const Position& c, SUMOReal radius, const Position& p1, const Position& p2,
                                            std::vector<SUMOReal>& into);


    /** @brief returns the intersection point
     * of the (infinite) lines p11,p12 and p21,p22.
     * If the given lines are parallel the result will contain NAN-values
     */
    static Position intersection_position2D(
        const Position& p11, const Position& p12,
        const Position& p21, const Position& p22);

    static SUMOReal Angle2D(SUMOReal x1, SUMOReal y1, SUMOReal x2, SUMOReal y2);

    static Position interpolate(const Position& p1,
                                const Position& p2, SUMOReal length);

    static Position extrapolate_first(const Position& p1,
                                      const Position& p2, SUMOReal length);

    static Position extrapolate_second(const Position& p1,
                                       const Position& p2, SUMOReal length);

    static SUMOReal nearest_offset_on_line_to_point2D(
        const Position& l1, const Position& l2,
        const Position& p, bool perpendicular = true);

    /** by Damian Coventry */
    static SUMOReal distancePointLine(const Position& point,
                                      const Position& lineStart, const Position& lineEnd);

    /**
     * Return the distance from point to line as well as the intersection point.
     * If intersection does not lie within the line segment, the  start or end point of the segment is returned
     */
    static SUMOReal closestDistancePointLine2D(const Position& point,
            const Position& lineStart, const Position& lineEnd,
            Position& outIntersection);

    static Position transfer_to_side(Position& p,
                                     const Position& lineBeg, const Position& lineEnd,
                                     SUMOReal amount);


    static Position crossPoint(const Boundary& b,
                               const PositionVector& v);

    static std::pair<SUMOReal, SUMOReal> getNormal90D_CW(const Position& beg,
            const Position& end, SUMOReal length, SUMOReal wanted_offset);

    static std::pair<SUMOReal, SUMOReal> getNormal90D_CW(const Position& beg,
            const Position& end, SUMOReal wanted_offset);

    /** @brief Returns the distance of second angle from first angle counter-clockwise
     * @param[in] angle1 The first angle
     * @param[in] angle2 The second angle
     * @return Angle (counter-clockwise) starting from first to second angle
     */
    static SUMOReal getCCWAngleDiff(SUMOReal angle1, SUMOReal angle2);


    /** @brief Returns the distance of second angle from first angle clockwise
     * @param[in] angle1 The first angle
     * @param[in] angle2 The second angle
     * @return Angle (clockwise) starting from first to second angle
     */
    static SUMOReal getCWAngleDiff(SUMOReal angle1, SUMOReal angle2);


    /** @brief Returns the minimum distance (clockwise/counter-clockwise) between both angles
     * @param[in] angle1 The first angle
     * @param[in] angle2 The second angle
     * @return The minimum distance between both angles
     */
    static SUMOReal getMinAngleDiff(SUMOReal angle1, SUMOReal angle2);


    /** @brief Returns the maximum distance (clockwise/counter-clockwise) between both angles
     * @param[in] angle1 The first angle
     * @param[in] angle2 The second angle
     * @return The maximum distance between both angles
     */
    static SUMOReal getMaxAngleDiff(SUMOReal angle1, SUMOReal angle2);


private:
    /** @brief return whether the line segments defined by Line (x1,y1),(x2,y2)
     * and Line (x3,y3),(x4,y4) intersect
     */
    static bool intersects(
        const SUMOReal x1, const SUMOReal y1, const SUMOReal x2, const SUMOReal y2,
        const SUMOReal x3, const SUMOReal y3, const SUMOReal x4, const SUMOReal y4,
        SUMOReal* x, SUMOReal* y, SUMOReal* mu);

};


#endif

/****************************************************************************/
