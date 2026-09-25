#pragma once

#include "pch.h"

#include "wood_element_beam.h"
#include "wood_interaction_contact_axis.h"
#include "wood_interaction_feature_beam.h"

namespace wood_session {

/// One beam pair to one InteractionFeatureBeam: the end types at the closest points (`cross_or_side_to_end` separates a crossing from an end contact), four volume rectangles of `volume_length` from each beam's radius and up direction (`flip_male` rotates the male corners), trimmed by the cut planes of a crossing or a side-to-end. False when the end types are not allowed by either beam, a radius is missing, or a frame is degenerate.
bool beam_to_beam(const Beam& beam0, const Beam& beam1, const InteractionContactAxis& contact, double volume_length, double cross_or_side_to_end, int flip_male, InteractionFeatureBeam& out);

} // namespace wood_session
