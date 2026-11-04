#pragma once

#include "pch.h"
#include "wood_element.h"

using namespace session_cpp;

namespace wood_session {

/// Element that represents a node: a block or a support, where the edges and faces of a model meet.
class NodeElement : public WoodElement {
public:
    using WoodElement::WoodElement;
};

}
