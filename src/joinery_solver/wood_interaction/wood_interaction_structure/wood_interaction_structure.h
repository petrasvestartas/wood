#pragma once

#include "pch.h"

using namespace session_cpp;

namespace wood_session {

/// How forces pass between the edge's two elements; abstract, with no concrete kind until the structural pass exists.
class InteractionStructure : public Interaction {
};

} // namespace wood_session
