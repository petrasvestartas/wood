#include "pch.h"
#include "wood_face_to_face_element.h"
using namespace session_cpp;

namespace wood_session {

ContactElement::ContactElement(const Plate& plate)
    : polylines(plate.polylines), planes(plate.planes), name(plate.name), plate_convention(true) {}

ContactElement::ContactElement(Element& element) : name(element.name) {

    if (const Plate* plate = dynamic_cast<const Plate*>(&element)) {
        polylines = plate->polylines;
        planes = plate->planes;
        plate_convention = true;
        return;
    }

    polylines = element.polylines();
    planes = element.planes();
}

} // namespace wood_session
