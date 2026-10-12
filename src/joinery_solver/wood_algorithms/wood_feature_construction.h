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
void apply_unit_scale(InteractionFeaturePlate& joint);

/// Maps the unit-box outlines onto the joint volumes by change of basis, after apply_unit_scale.
void joint_orient_to_connection_area(InteractionFeaturePlate& joint);

/// Interleaves the outlines of the joints in linked_joints into this one, following linked_joints_seq.
void merge_linked_joints(InteractionFeaturePlate& joint, std::vector<InteractionFeaturePlate>& all_joints);

/// Sets divisions from the joint length and division_distance, at least one.
void joint_get_divisions(InteractionFeaturePlate& joint, double division_distance);

/// The [width, height, length] extension of the joint found joint_id-th, as 2024 indexed it: a 3-entry list serves every joint, a longer one
/// gives triple k to the k-th joint found and its last triple to every later one.
std::array<double, 3> joint_volume_extension(const std::vector<double>& extension, size_t joint_id);

/// Position of the plate with this guid, or -1.
int index_of_plate(const std::vector<std::shared_ptr<Plate>>& elements, const std::string& guid);

/// Position of the joint with this guid in a solver run's joint list, or -1.
int index_of_joint(const std::vector<InteractionFeaturePlate>& joints, const std::string& guid);

} // namespace wood_session
