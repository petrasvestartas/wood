#pragma once

#include "pch.h"

#include "wood_face_to_face_contact.h"

namespace wood_session {

/// face_contacts() output: one element pair as positions in the vector it was given, and every overlap between them.
struct ContactPair {
    int element_a = -1; // Position of the first element.
    int element_b = -1; // Position of the second element.
    std::vector<FaceContact> faces; // Every face pair in contact between the two.
};

} // namespace wood_session
