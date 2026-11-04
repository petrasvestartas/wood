#pragma once

#include "wood_element_joint.h"

using namespace session_cpp;

namespace wood_session {

/// Element that represents a pin: every pin, screw or pin, one cylinder along its axis under its connector.
class JointPinElement : public JointElement {
public:
    JointPinElement();

    /// The pin along axis, radius thick, meshed at chord_tolerance.
    JointPinElement(const Line& axis, double radius, double chord_tolerance);

    /// The pin's axis.
    const Line& axis() const {
        return drill_lines.front();
    }

    std::string element_type_name() const override {
        return "JointPinElement";
    }

    std::shared_ptr<Element> clone() const override {
        return std::make_shared<JointPinElement>(*this);
    }
};

}
