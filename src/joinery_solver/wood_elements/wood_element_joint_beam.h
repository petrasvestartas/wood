#pragma once

#include "wood_element_joint.h"
#include "wood_element_beam.h"
#include "wood_interaction_feature_beam.h"
#include "wood_interaction_contact_axis.h"

namespace wood_session {

class InteractionContactFace;

/// A joint between beams: either a beam-to-beam joint carrying the feature volumes detection found, or a connector, a part of its own with cutters per target and dowels, nested in the tree as one child element per part and per dowel.
class JointBeam : public Joint {
public:
    InteractionFeatureBeam feature; // The beam-to-beam feature: the four volume rectangles of the male and female corners; empty for a connector.
    std::vector<std::array<session_cpp::Polyline, 2>> parts; // A connector's own solids, each lofted between a bottom and a top loop; empty for a beam-to-beam joint.
    std::vector<std::vector<std::array<session_cpp::Polyline, 2>>> cutters; // A connector's cutters per target in targets order, lofted like parts; the drill lines cut every target too.
    double drill_overshoot = 0.0; // How far a target's holes run past the dowels at an end where the dowel leaves the target; a blind hole stops at its dowel.
    std::vector<SolidCut> solid_cuts; // Cuts into the connector's own parts, a cross lap's slot say, in the connector's frame like an element's.

    JointBeam();

    /// A beam-to-beam joint from its feature.
    explicit JointBeam(const InteractionFeatureBeam& feature);

    /// A beam-to-beam joint whose feature the builder fills.
    JointBeam(InteractionFeatureBeam feature, const std::function<void(InteractionFeatureBeam&)>& builder);

    /// A beam-to-beam joint on the axis contact of two beams; throws when none fits.
    JointBeam(const Beam& source, const Beam& target, const InteractionContactAxis& contact, double volume_length, double cross_or_side_to_end, int flip_male = 0);

    // ═══════════════════════════════════════════════════════════════════════════
    // Static constructors
    // ═══════════════════════════════════════════════════════════════════════════

    /// A beam-to-beam joint on the axis contact of two beams; null when none fits.
    static std::shared_ptr<JointBeam> from_contact(const Beam& source, const Beam& target, const InteractionContactAxis& contact, double volume_length, double cross_or_side_to_end, int flip_male = 0);

    /// The wedge connector of compas_tf ConnectorWedgeElement on the face contact of two members: a prism of the profile along the contact's top edge with horizontal dowels, and a pocket in each member.
    static std::shared_ptr<JointBeam> wedge(
        const session_cpp::Element& a,
        const session_cpp::Element& b,
        const InteractionContactFace& contact,
        double length_margin,
        double pocket_depth,
        double dowel_radius = 10.0,
        double dowel_spacing = 320.0,
        int dowel_sides = 8,
        double overshoot = 20.0,
        const std::array<std::array<double, 2>, 3>& profile = {{{0.0, -197.0}, {-31.75593, 11.530606}, {31.75593, 11.530606}}},
        const std::array<double, 2>& dowel_offset = {80.0, -100.0}
    );

    /// The column-to-rib connector of compas_tf ConnectorElement on their face contact: a plate into both along the contact normal with four dowels across it, cut as its box and the dowel holes out of both.
    static std::shared_ptr<JointBeam> rectangle_plate(
        const session_cpp::Element& column,
        const session_cpp::Element& rib,
        const InteractionContactFace& contact,
        double dowel_length,
        double width = 30.0,
        double back = 220.0,
        double front = 265.0,
        double height = 250.0,
        double dowel_radius = 25.0,
        double margin_x = 6.05,
        double margin_z = 3.0,
        double overshoot = 25.0,
        int dowel_sides = 16
    );

    /// The seam connector of compas_tf OuterRibConnectorElement made parametric: a bow-tie key across the end-to-end contact of two members, with a flat-bottomed pocket in each.
    static std::shared_ptr<JointBeam> tie(
        const session_cpp::Element& a,
        const session_cpp::Element& b,
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

    /// Assembly dowels on the face contact of two members: four round dowels at the corners of the contact inset by offset, half into each member; null when the inset leaves nothing.
    static std::shared_ptr<JointBeam> dowels(
        const session_cpp::Element& a,
        const session_cpp::Element& b,
        const InteractionContactFace& contact,
        double radius = 4.0,
        double length = 30.0,
        double offset = 50.0,
        double overshoot = 10.0,
        int dowel_sides = 16
    );

    /// The half-lap cross joint of two connectors whose box parts cross: a slot through each where the other passes, a's from share of their common height up, b's from the bottom up to there.
    static std::shared_ptr<JointBeam> cross_lap(const JointBeam& a, const JointBeam& b, double share = 0.5, double margin = 1.0);

    // ═══════════════════════════════════════════════════════════════════════════
    // Geometry
    // ═══════════════════════════════════════════════════════════════════════════

    /// Whether this is a connector, with parts or cutters of its own, rather than a beam-to-beam joint.
    bool is_connector() const;

    /// One part as a closed mesh, its loops lofted, before any cut.
    session_cpp::Mesh part_mesh(size_t index) const;

    /// The cuts into one part: the connector's stored cuts and the bores of its own dowels passing through the part.
    std::vector<SolidCut> part_cuts(size_t index) const;

    /// One part as a BRep with its cuts applied and its dowel bores exact.
    session_cpp::BRep part_brep(size_t index) const;

    /// The connector's parts and dowels as elements to nest under it: a ConnectorPart per part named <name>_part, numbered when there are several, then a Dowel per drill line named <name>_dowel_<i>.
    std::vector<std::shared_ptr<Joint>> children() const;

    /// A beam-to-beam joint's feature volumes; a connector draws nothing itself, its children carry its parts and dowels.
    const session_cpp::Mesh& element_geometry_mesh() const override;

    /// A beam-to-beam joint's feature volumes; a connector draws nothing itself, its children carry its parts and dowels.
    const session_cpp::BRep& element_geometry_brep() const override;

    std::string element_type_name() const override {
        return "JointBeam";
    }

    std::shared_ptr<session_cpp::Element> clone() const override {
        return std::make_shared<JointBeam>(*this);
    }

    // ═══════════════════════════════════════════════════════════════════════════
    // Transformation
    // ═══════════════════════════════════════════════════════════════════════════

    /// Moves the joint, its feature volumes, parts, cutters and cuts.
    void place(const session_cpp::Xform& xform) override;

    // ═══════════════════════════════════════════════════════════════════════════
    // Protobuf
    // ═══════════════════════════════════════════════════════════════════════════

protected:
    std::vector<std::array<session_cpp::Polyline, 2>> bodies() const override;
    void write_proto(wood_proto::Joint& proto) const override;
    void read_proto(const wood_proto::Joint& proto) override;
};

}
