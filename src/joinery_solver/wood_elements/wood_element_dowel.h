#pragma once

#include "wood_element_joint.h"

using namespace session_cpp;

namespace wood_session {

/// Element that represents one round dowel of a connector: its axis and radius, drawn as an exact cylinder, flush with the members it joins; a child of the connector in the tree, carrying no relation of its own.
class Dowel : public Joint {
public:
    Dowel();

    /// The dowel along axis, radius thick, meshed at chord_tolerance; visible, named "dowel".
    Dowel(const Line& axis, double radius, double chord_tolerance);

    /// The dowel's axis.
    const Line& axis() const {
        return drill_lines.front();
    }

    std::string element_type_name() const override {
        return "Dowel";
    }
    std::shared_ptr<Element> clone() const override {
        return std::make_shared<Dowel>(*this);
    }
};

}
