#pragma once

#include "pch.h"

using namespace session_cpp;

namespace wood_session {

/// One step of a joint's cutting procedure, as the Joinery Compendium numbers it: A1, A2 on the first target, B1 on the second.
struct JointCutStep {
    std::string label; // "A1", "B2".
    size_t target = 0; // Which target of the joint, in targets order.
    std::vector<Plane> planes; // The saw or chisel planes, normals towards the wood taken away.
    Mesh removal; // The wood the step takes away, closed; empty for a kerf that frees nothing yet.
};

}
