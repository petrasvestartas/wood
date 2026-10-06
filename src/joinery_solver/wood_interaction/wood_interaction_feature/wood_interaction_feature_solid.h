#pragma once

#include "pch.h"

#include "wood_interaction_feature.h"

using namespace session_cpp;

namespace wood_session {

/// What a solid feature does to its element.
enum class SolidOperation : int {
    intersect = 0, // Keep only what the solid and the element share.
    add = 1, // Add the solid: a glued block.
    subtract = 2, // Take the solid away: a cut, a pocket, a hole.
};

/// A solid feature: a solid added to or taken away from an element, the solid counterpart of a plate's merged outline feature. An element's own features have no source; another element puts one on it with WoodSession::add_interaction(source, target, feature), the edge keeping it in the source's frame and the target a copy in its own frame. The element's stock is its shape with every add, its model the stock with every subtract.
class InteractionFeatureSolid : public InteractionFeature {
public:
    static constexpr std::string_view INTERACTION_TYPE = "InteractionFeatureSolid"; // The tag the kernel writes and the registry reads.
    std::string source; // The guid of the element that put it there, so its next feature replaces it; empty for the element's own.
    Mesh mesh; // The closed solid; empty for a feature of drills alone.
    std::vector<Polyline> profile; // A profile feature's closed loops, extruded along extrusion.
    Vector extrusion; // The profile's extrusion.
    SolidOperation operation = SolidOperation::subtract; // Added or taken away.
    double tolerance = 1e-7; // Boolean tolerance.
    std::vector<Line> drills; // Round holes it also makes, kept as axes so a BRep can make them exact.
    double drill_radius = 0.0; // Radius of every drill.
    double drill_tolerance = 0.05; // Chord tolerance the drills are meshed at.

    // ═══════════════════════════════════════════════════════════════════════════
    // Constructors
    // ═══════════════════════════════════════════════════════════════════════════

    /// An empty feature, filled field by field.
    InteractionFeatureSolid() = default;

    /// A feature of the closed mesh: added, taken away or intersected.
    InteractionFeatureSolid(const Mesh& mesh, SolidOperation operation);

    // ═══════════════════════════════════════════════════════════════════════════
    // Geometry
    // ═══════════════════════════════════════════════════════════════════════════

    /// "solid".
    std::string_view kind() const override;

    /// A copy with the solid, the profile, the extrusion and the drills moved by xform.
    InteractionFeatureSolid transformed(const Xform& xform) const;

    // ═══════════════════════════════════════════════════════════════════════════
    // Protobuf
    // ═══════════════════════════════════════════════════════════════════════════

    /// INTERACTION_TYPE.
    std::string interaction_type_name() const override;

    /// The feature as wood_proto.InteractionFeatureSolid bytes.
    std::string interaction_data_dumps() const override;

    /// The feature as wood_proto.InteractionFeatureSolid bytes, the same as interaction_data_dumps.
    std::string pb_dumps() const;

    /// A feature from wood_proto.InteractionFeatureSolid bytes.
    static InteractionFeatureSolid pb_loads(const std::string& data);

    /// A copy with the same guid, the polymorphic copy a Session makes.
    std::shared_ptr<Interaction> clone() const override;

    /// Registers the INTERACTION_TYPE factory with the kernel.
    static void register_type();

    // ═══════════════════════════════════════════════════════════════════════════
    // String
    // ═══════════════════════════════════════════════════════════════════════════

    /// "InteractionFeatureSolid(operation=..., faces=..., drills=...)".
    std::string str() const override;
};

} // namespace wood_session
