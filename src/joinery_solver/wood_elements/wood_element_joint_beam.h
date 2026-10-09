#pragma once

#include "wood_element_joint.h"
#include "wood_element_beam.h"
#include "wood_interaction_feature_beam.h"
#include "wood_interaction_contact_axis.h"

using namespace session_cpp;

namespace wood_session {

class InteractionContactFace;

/// Where pins stand on a contact: at its inset corners, in a column across its height, or in a row along it.
enum class PinLayout {
    corners, // The inset contact's extreme corners.
    vertical, // A column of count pins across the contact's height.
    horizontal, // A row of count pins along the contact.
};

/// Element that represents a joint between beams: a beam-to-beam joint with feature volumes, or a connector with parts, cutters and pins.
class JointBeam : public Joint {
public:
    static inline const Color CONNECTOR_COLOR = Color(33.0f / 255.0f, 150.0f / 255.0f, 234.0f / 255.0f, 1.0f, "brg_blue"); // A connector's node and every part and pin node under it: the Block Research Group's primary blue.
    static constexpr std::array<std::array<double, 2>, 3> WEDGE_PROFILE = {{{0.0, -197.0}, {-31.75593, 11.530606}, {31.75593, 11.530606}}}; // The wedge's cross-section across and below the contact's top edge: the apex, then the two top corners.

    InteractionFeatureBeam feature; // The beam-to-beam feature: the four volume rectangles of the male and female corners; empty for a connector.
    std::vector<std::array<Polyline, 2>> parts; // A connector's own solids, each lofted between a bottom and a top loop; empty for a beam-to-beam joint.
    std::vector<std::vector<std::array<Polyline, 2>>> cutters; // A connector's cutters per target in targets order, lofted like parts; the drill lines cut every target too.
    double drill_overshoot = 0.0; // How far a target's holes run past the pins at an end where the pin leaves the target; a blind hole stops at its pin.
    std::vector<InteractionFeatureSolid> solid_features; // Cuts into the connector's own parts, a cross lap's slot say, in the connector's frame like an element's.
    bool pre_drill = false; // A connector of pins: its drill lines are the pre-drilled holes of both targets, stored once here and never cut.

    JointBeam();

    /// A beam-to-beam joint from its feature.
    explicit JointBeam(const InteractionFeatureBeam& feature);

    /// A beam-to-beam joint whose feature the builder fills.
    JointBeam(InteractionFeatureBeam feature, const std::function<void(InteractionFeatureBeam&)>& builder);

    /// A beam-to-beam joint on the axis contact of two beams; throws when none fits.
    JointBeam(
        const Beam& source,
        const Beam& target,
        const InteractionContactAxis& contact,
        double volume_length,
        double cross_or_side_to_end,
        int flip_male = 0
    );

    // ═══════════════════════════════════════════════════════════════════════════
    // Static constructors
    // ═══════════════════════════════════════════════════════════════════════════

    /// A beam-to-beam joint on the axis contact of two beams; null when none fits.
    static std::shared_ptr<JointBeam> from_contact(
        const Beam& source,
        const Beam& target,
        const InteractionContactAxis& contact,
        double volume_length,
        double cross_or_side_to_end,
        int flip_male = 0
    );

    /// The wedge connector on the face contact of two members: a profile prism with horizontal pins and a pocket in each member.
    static std::shared_ptr<JointBeam> wedge(
        const Element& a,
        const Element& b,
        const InteractionContactFace& contact,
        double length_margin,
        double pocket_depth,
        const std::optional<Plane>& end = std::nullopt,
        double pin_radius = 10.0,
        double pin_spacing = 320.0,
        int pin_sides = 8,
        double overshoot = 20.0,
        const std::array<std::array<double, 2>, 3>& profile = WEDGE_PROFILE,
        const std::array<double, 2>& pin_offset = {80.0, -100.0}
    );

    /// The column-to-rib connector on the face contact of two members: a plate into both with four pins across it.
    static std::shared_ptr<JointBeam> rectangle_plate(
        const Element& column,
        const Element& rib,
        const InteractionContactFace& contact,
        double pin_length,
        double width = 30.0,
        double back = 220.0,
        double front = 265.0,
        double height = 250.0,
        double pin_radius = 25.0,
        double margin_x = 6.05,
        double margin_z = 3.0,
        double overshoot = 25.0,
        int pin_sides = 16
    );

    /// The seam connector: a bow-tie key across the end-to-end contact of two members, with a pocket in each.
    static std::shared_ptr<JointBeam> tie(
        const Element& a,
        const Element& b,
        const InteractionContactFace& contact,
        double top = 138.5,
        double length = 800.0,
        double head_length = 200.0,
        double head_width = 40.0,
        double neck_width = 20.0,
        double depth = 58.5,
        double end_depth = 58.5 + 302.0 / 25.4,
        double pocket_depth = 80.0,
        double overshoot = 10.0
    );

    /// Pins centred across the face contact at its inset corners, bored into both members; null when the inset leaves nothing.
    static std::shared_ptr<JointBeam> centred_pins(
        const Element& a,
        const Element& b,
        const InteractionContactFace& contact,
        double radius = 4.0,
        double length = 30.0,
        double offset = 50.0,
        double overshoot = 10.0,
        int pin_sides = 16
    );

    /// Headed pins laid out on the face contact, from the far face of through into into, pre-drilled.
    static std::shared_ptr<JointBeam> headed_pins(
        const Element& through,
        const Element& into,
        const InteractionContactFace& contact,
        PinLayout layout,
        size_t count = 2,
        double offset = 20.0,
        double shift = 0.0,
        double radius = 2.0,
        double length = 200.0,
        int sides = 16
    );

    /// The half-lap cross joint of two connectors whose box parts cross: a slot through each where the other passes.
    static std::shared_ptr<JointBeam> cross_lap(
        const JointBeam& a,
        const JointBeam& b,
        double share = 0.5,
        double margin = 1.0
    );

    // ═══════════════════════════════════════════════════════════════════════════
    // Geometry
    // ═══════════════════════════════════════════════════════════════════════════

    /// Whether this is a connector, with parts, cutters or pre-drilled pins of its own, rather than a beam-to-beam joint.
    bool is_connector() const;

    /// The interaction this joint puts on its target i: a connector's cutters and bores, pre-drilled holes, or feature volumes.
    std::shared_ptr<Interaction> interaction(size_t target) const override;

    /// One part as a closed mesh, its loops lofted, before any cut.
    Mesh part_mesh(size_t index) const;

    /// The cuts into one part: the connector's stored cuts and the bores of its own pins passing through the part.
    std::vector<InteractionFeatureSolid> part_features(size_t index) const;

    /// One part as a BRep with its cuts applied and its pin bores exact.
    BRep part_brep(size_t index) const;

    /// The connector's parts and pins as ConnectorPart and Pin elements to nest under it.
    std::vector<std::shared_ptr<Joint>> children() const;

    /// A beam-to-beam joint's feature volumes; a connector draws nothing itself, its children carry its parts and pins.
    const Mesh& element_geometry_mesh() const override;

    /// A beam-to-beam joint's feature volumes; a connector draws nothing itself, its children carry its parts and pins.
    const BRep& element_geometry_brep() const override;

    std::string element_type_name() const override {
        return "JointBeam";
    }

    std::shared_ptr<Element> clone() const override {
        return std::make_shared<JointBeam>(*this);
    }

    // ═══════════════════════════════════════════════════════════════════════════
    // Transformation
    // ═══════════════════════════════════════════════════════════════════════════

    /// Moves the joint, its feature volumes, parts, cutters and cuts.
    void place(const Xform& xform) override;

    // ═══════════════════════════════════════════════════════════════════════════
    // Protobuf
    // ═══════════════════════════════════════════════════════════════════════════

protected:
    std::vector<std::array<Polyline, 2>> bodies() const override;
    void write_proto(wood_proto::Joint& proto) const override;
    void read_proto(const wood_proto::Joint& proto) override;
};

}
