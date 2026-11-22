#pragma once

using namespace wood_session;

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4505)
#endif

/// Includes the plate-joint designs in the consuming translation unit.
#include "wood_interaction_feature_plate_joints/custom_outlines.h"
#include "wood_interaction_feature_plate_joints/custom_pairs.h"
#include "wood_interaction_feature_plate_joints/unit_scale_distance.h"

// ═══════════════════════════════════════════════════════════════════════════
// side_side_inplane: side-side in-plane, type 12
// ═══════════════════════════════════════════════════════════════════════════
#include "wood_interaction_feature_plate_joints/side_side_inplane_custom.h"

// ═══════════════════════════════════════════════════════════════════════════
// side_side_outplane: side-side out-of-plane, type 11 (side_side_outplane_linked_tenons calls side_side_outplane_tenons, side_side_outplane_linked_tenons_wall calls side_side_outplane_linked_tenons)
// ═══════════════════════════════════════════════════════════════════════════
#include "wood_interaction_feature_plate_joints/side_side_outplane_linked_tenons.h"
#include "wood_interaction_feature_plate_joints/side_side_outplane_linked_tenons_wall.h"
#include "wood_interaction_feature_plate_joints/side_side_outplane_annen_fingers.h"
#include "wood_interaction_feature_plate_joints/side_side_outplane_custom.h"

// ═══════════════════════════════════════════════════════════════════════════
// top_side: top-side, type 20
// ═══════════════════════════════════════════════════════════════════════════
#include "wood_interaction_feature_plate_joints/top_side_annen_tenons.h"
#include "wood_interaction_feature_plate_joints/top_side_annen_shifted_tenons.h"
#include "wood_interaction_feature_plate_joints/top_side_milled_wedge.h"
#include "wood_interaction_feature_plate_joints/top_side_snap_fit.h"
#include "wood_interaction_feature_plate_joints/top_side_bridle.h"
#include "wood_interaction_feature_plate_joints/top_side_custom.h"

// ═══════════════════════════════════════════════════════════════════════════
// top_top: top-top drills, type 40
// ═══════════════════════════════════════════════════════════════════════════
#include "wood_interaction_feature_plate_joints/stacked_drills.h"
#include "wood_interaction_feature_plate_joints/top_top_centre_pin.h"
#include "wood_interaction_feature_plate_joints/top_top_polylabel_pin.h"
#include "wood_interaction_feature_plate_joints/top_top_circle_pins.h"
#include "wood_interaction_feature_plate_joints/top_top_ring_pins.h"
#include "wood_interaction_feature_plate_joints/top_top_grid_pins.h"
#include "wood_interaction_feature_plate_joints/top_top_rectangle_pins.h"
#include "wood_interaction_feature_plate_joints/top_top_custom.h"

// ═══════════════════════════════════════════════════════════════════════════
// cross: cross in-plane, type 30
// ═══════════════════════════════════════════════════════════════════════════
#include "wood_interaction_feature_plate_joints/cross_custom.h"

// ═══════════════════════════════════════════════════════════════════════════
// side_side_rotated: side-side rotated, type 13 (jupiter_scarf is the trait de Jupiter of two plates in line)
// ═══════════════════════════════════════════════════════════════════════════
#include "wood_interaction_feature_plate_joints/side_side_rotated_miter.h"
#include "wood_interaction_feature_plate_joints/side_side_rotated_custom.h"
#include "wood_interaction_feature_plate_joints/side_removal.h"
#include "wood_interaction_feature_plate_joints/jupiter_scarf.h"

// ═══════════════════════════════════════════════════════════════════════════
// b: beam, type 60
// ═══════════════════════════════════════════════════════════════════════════
#include "wood_interaction_feature_plate_joints/border.h"
#include "wood_interaction_feature_plate_joints/border_custom.h"

#ifdef _MSC_VER
#pragma warning(pop)
#endif
