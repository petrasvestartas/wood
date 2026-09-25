#pragma once

#include "pch.h"

namespace wood_session {

/// How forces pass between the edge's two elements; abstract, with no concrete kind until the structural pass exists.
class InteractionStructure : public session_cpp::Interaction {
};

} // namespace wood_session
