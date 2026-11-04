#pragma once

#include "pch.h"
#include "wood_element.h"

using namespace session_cpp;

namespace wood_session {

/// Element that represents a face: a planar or a curved plate.
class FaceElement : public WoodElement {
public:
    using WoodElement::WoodElement;
};

}
