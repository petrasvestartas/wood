#include "pch.h"
#include "wood_element_dowel.h"

namespace wood_session {

using namespace session_cpp;

Dowel::Dowel() {
    name = "dowel";
    is_visible = true;
}

Dowel::Dowel(const Line& axis, double radius, double chord_tolerance) {

    if (axis.length() <= 0.0 || radius <= 0.0)
        throw std::invalid_argument("A dowel needs a positive length and radius");

    name = "dowel";
    is_visible = true;
    drill_lines = {axis};
    line_radius = radius;
    this->chord_tolerance = chord_tolerance;
}

}
