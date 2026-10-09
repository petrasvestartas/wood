#include "pch.h"
#include "wood_element_pin.h"

namespace wood_session {

using namespace session_cpp;

Pin::Pin() {
    name = "pin";
    is_visible = true;
}

Pin::Pin(const Line& axis, double radius, double chord_tolerance) {

    if (axis.length() <= 0.0 || radius <= 0.0)
        throw std::invalid_argument("A pin needs a positive length and radius");

    name = "pin";
    is_visible = true;
    drill_lines = {axis};
    line_radius = radius;
    this->chord_tolerance = chord_tolerance;
}

}
