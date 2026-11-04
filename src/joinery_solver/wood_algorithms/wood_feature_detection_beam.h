#pragma once

#include "pch.h"

#include "wood_element_edge_straight.h"
#include "wood_interaction_contact_axis.h"
#include "wood_interaction_feature_beam.h"

namespace wood_session {

/// One beam pair to one InteractionFeatureBeam.
bool beam_to_beam(
    const EdgeStraightElement& beam0,
    const EdgeStraightElement& beam1,
    const InteractionContactAxis& contact,
    double volume_length,
    double cross_or_side_to_end,
    int flip_male,
    InteractionFeatureBeam& out
);

}
