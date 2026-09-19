#pragma once

#include "pch.h"

#include "wood_element_plate.h"
#include "wood_interaction_feature_plate.h"
#include "wood_interaction_feature_fabrication_type.h"

namespace wood_session {

// ═══════════════════════════════════════════════════════════════════════════
// Joint construction
// ═══════════════════════════════════════════════════════════════════════════

/// Moves the volume rectangles of a unit-scale joint to unit_scale_distance apart along the joint line.
void apply_unit_scale(FeaturePlate& joint);

/// Maps the unit-box outlines onto the joint volumes by change of basis, after apply_unit_scale.
void joint_orient_to_connection_area(FeaturePlate& joint);

/// Interleaves the outlines of the joints in linked_joints into this one, following linked_joints_seq.
void merge_linked_joints(FeaturePlate& joint, std::vector<FeaturePlate>& all_joints);

/// Sets divisions from the joint length and division_distance, at least one.
void joint_get_divisions(FeaturePlate& joint, double division_distance);

/// The [width, height, length] extension for a joint type: side-side (11/12/13) reads triple 0, top-side (20) triple 1, top-top (40) triple 2, cross (30) triple 3; a 3-entry list serves every type.
std::array<double, 3> joint_volume_extension(const std::vector<double>& extension, int joint_type);

/// Position of the plate with this guid, or -1.
int index_of(const std::vector<std::shared_ptr<Plate>>& elements, const std::string& guid);

} // namespace wood_session
