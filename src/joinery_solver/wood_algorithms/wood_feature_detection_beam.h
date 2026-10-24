#pragma once

#include "pch.h"

#include "wood_element_beam.h"
#include "wood_interaction_contact_axis.h"
#include "wood_interaction_feature_beam.h"

namespace wood_session {

/// One beam pair to one InteractionFeatureBeam.
bool beam_to_beam(
    const Beam& beam0,
    const Beam& beam1,
    const InteractionContactAxis& contact,
    double volume_length,
    double cross_or_side_to_end,
    int flip_male,
    InteractionFeatureBeam& out
);

}
