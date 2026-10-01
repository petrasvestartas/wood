#pragma once

#include "pch.h"

namespace wood_session {

enum class SolidOperation : int { intersection = 0,
                                  unite = 1,
                                  difference = 2 };

struct SolidCut {
    std::string joint_guid;
    session_cpp::Mesh mesh;
    std::vector<session_cpp::Polyline> profile;
    session_cpp::Vector extrusion;
    SolidOperation operation = SolidOperation::difference;
    double tolerance = 1e-7;
    std::vector<session_cpp::Line> drills; // Round holes this cut also makes, kept as axes so a BRep can make them exact.
    double drill_radius = 0.0; // Radius of every drill.
    double drill_tolerance = 0.05; // Chord tolerance the drills are meshed at.
    SolidCut transformed(const session_cpp::Xform& xform) const;
    std::string pb_dumps() const;
    static SolidCut pb_loads(const std::string& data);
};

}
