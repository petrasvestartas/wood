#pragma once
#include "src/templates/plaited/plaited_guide.h"
#include "src/templates/reciprocal/reciprocal_boundary.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_plaited {

// ═══════════════════════════════════════════════════════════════════════════
// PlaitedStereotomy
// ═══════════════════════════════════════════════════════════════════════════

/// Stereotomic block vault, arch, dome or flat floor built from the shared bevel planes of a PlaitedGuide.
class PlaitedStereotomy : public WoodSession {
public:
    const PlaitedGuide guide; // Underlying cell, seam plane and axis guide.
    const bool with_contacts; // Computes coplanar face-to-face contacts when true.

    /// Builds one planar-faced NodeBlockElement voussoir per plaited cell and detects seam contacts.
    explicit PlaitedStereotomy(const PlaitedGuide& guide, bool with_contacts = true);

    /// Builds the guide and stereotomic block assembly directly from an extrados mesh.
    PlaitedStereotomy(
        const Mesh& mesh,
        PlaitedPattern pattern = PlaitedPattern::Abeille,
        double thickness = 180.0,
        double bevel = 0.45,
        double relief_depth = 0.0,
        bool flip_chirality = false,
        bool bevel_naked_edges = false,
        bool with_contacts = true,
        double opening = 0.0,
        bool flat_base = false
    );

    /// Every stereotomic block in guide cell order.
    const std::vector<std::shared_ptr<NodeBlockElement>>& blocks() const;

private:
    std::vector<std::shared_ptr<NodeBlockElement>> _blocks; // Stereotomic voussoir blocks.

    std::vector<std::shared_ptr<NodeBlockElement>> compute_blocks() const;
    void draw();
};

// ═══════════════════════════════════════════════════════════════════════════
// PlaitedReciprocal
// ═══════════════════════════════════════════════════════════════════════════

/// Leonardo da Vinci reciprocal grillage built from the bar axes of a PlaitedGuide with perimeter frame and pins.
class PlaitedReciprocal : public WoodSession {
public:
    const PlaitedGuide guide; // Underlying plaited cell and bar-axis guide.
    const double beam_width; // Cross-section width across each reciprocal bar.
    const double beam_height; // Cross-section depth normal to the surface.
    const bool add_boundary_frame; // Surrounds naked mesh edges with straight perimeter frame beams.
    const double pin_diameter; // Pin diameter bored across butt contacts when positive.
    const wood_reciprocal::CornerJoint corner_joint; // Perimeter frame corner joint: Butt or Miter.
    const bool with_contacts; // Computes coplanar face-to-face contacts when true.

    /// Builds the reciprocal bars, continuous perimeter frame, optional centred pins, and face contacts.
    explicit PlaitedReciprocal(
        const PlaitedGuide& guide,
        double beam_width = 80.0,
        double beam_height = 0.0,
        bool add_boundary_frame = true,
        double pin_diameter = 0.0,
        wood_reciprocal::CornerJoint corner_joint = wood_reciprocal::CornerJoint::Butt,
        bool with_contacts = true
    );

    /// Every interior reciprocal bar in guide cell order.
    const std::vector<std::shared_ptr<FacePlanarElement>>& beams() const;

    /// Every straight perimeter frame beam on the naked mesh edges.
    const std::vector<std::shared_ptr<FacePlanarElement>>& frame_beams() const;

    /// Every bored pin connector created when pin_diameter is positive.
    const std::vector<std::shared_ptr<JointBeamElement>>& pins() const;

private:
    wood_reciprocal::MeshFaces _mesh_faces; // Mesh topology and face normals for frame layout.
    wood_reciprocal::BoundaryFrame _frame; // Naked boundary half-edges and end planes.
    std::vector<std::shared_ptr<FacePlanarElement>> _frame_beams; // Perimeter frame members.
    std::vector<std::shared_ptr<FacePlanarElement>> _beams; // Reciprocal bar members.
    std::vector<std::shared_ptr<JointBeamElement>> _pins; // Centred cylindrical pin connectors.

    std::vector<std::shared_ptr<FacePlanarElement>> compute_frame_beams();
    std::vector<std::shared_ptr<FacePlanarElement>> compute_beams() const;
    std::vector<std::shared_ptr<JointBeamElement>> compute_pins();
    void draw();
};

} // namespace wood_plaited
