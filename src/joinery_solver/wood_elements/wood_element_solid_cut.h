#pragma once

#include "pch.h"

using namespace session_cpp;

namespace wood_session {

enum class SolidOperation : int { intersection = 0,
                                  unite = 1,
                                  difference = 2 };

struct SolidCut {
    std::string joint_guid;
    Mesh mesh;
    std::vector<Polyline> profile;
    Vector extrusion;
    SolidOperation operation = SolidOperation::difference;
    double tolerance = 1e-7;
    std::vector<Line> drills; // Round holes this cut also makes, kept as axes so a BRep can make them exact.
    double drill_radius = 0.0; // Radius of every drill.
    double drill_tolerance = 0.05; // Chord tolerance the drills are meshed at.

    /// A cut that removes the closed mesh from the element.
    static SolidCut difference(const Mesh& mesh);

    /// A feature that adds the closed mesh to the element: a glued block.
    static SolidCut unite(const Mesh& mesh);

    SolidCut transformed(const Xform& xform) const;
    std::string pb_dumps() const;
    static SolidCut pb_loads(const std::string& data);
};

}
