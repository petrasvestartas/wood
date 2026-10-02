#pragma once

#include "wood_element_joint.h"
#include "wood_element_beam.h"
#include "wood_interaction_feature_beam.h"
#include "wood_interaction_contact_axis.h"

namespace wood_session {

class InteractionContactFace;

class JointBeam : public Joint {
public:
    InteractionFeatureBeam feature;
    std::vector<std::array<session_cpp::Polyline, 2>> parts; // A connector's own solids, each lofted between a bottom and a top loop; empty for a beam-to-beam joint.
    std::vector<std::vector<std::array<session_cpp::Polyline, 2>>> cutters; // A connector's cutters per target in targets order, lofted like parts; the drill lines, a connector's dowels flush with the members they pass through, cut every target.
    double drill_overshoot = 0.0; // How far a target's holes run past the dowels at an end where the dowel leaves the target, tested just beyond the dowel's end; a blind hole stops at its dowel.
    std::vector<SolidCut> solid_cuts; // Cuts into the connector's own parts, a cross lap's slot say, in the connector's frame like an element's; its dowels bore the parts they pass through without one.
    bool nested = false; // The connector's parts and dowels are its children in the tree, each an element of its own, so the connector draws nothing itself; WoodSession::add_connector sets it.
    std::string part_label = "part"; // What a part child is called: plate, wedge, key; the factories set it.
    JointBeam();
    JointBeam(const Beam& source, const Beam& target, const InteractionContactAxis& contact,
              double volume_length, double cross_or_side_to_end, int flip_male = 0);
    static std::shared_ptr<JointBeam> from_contact(const Beam& source, const Beam& target,
                                                   const InteractionContactAxis& contact, double volume_length, double cross_or_side_to_end, int flip_male = 0);
    explicit JointBeam(const InteractionFeatureBeam& feature);

    /// The wedge connector of compas_tf ConnectorWedgeElement on the face contact of two members: a triangular prism along the contact's longest top edge, shortened by length_margin at both ends, horizontal dowels every dowel_spacing as dowel_sides-sided prisms, flush with the members' outer faces, their holes overshoot past them, and in each member a box pocket pocket_depth deep under the wedge face on its side; aimed at a then b.
    static std::shared_ptr<JointBeam> wedge(
        const session_cpp::Element& a,
        const session_cpp::Element& b,
        const InteractionContactFace& contact,
        double length_margin,
        double pocket_depth,
        double dowel_radius = 10.0,
        double dowel_spacing = 320.0,
        int dowel_sides = 8,
        double overshoot = 20.0
    );

    /// The column-to-rib connector of compas_tf ConnectorElement on their face contact: a plate width thick, back into the column and front into the rib along the horizontal contact normal, height down from the contact's top edge, with four dowels across it, two per side, margin_x and margin_z radii in from its ends and its top and bottom, dowel_length long but flush with the member they pass through; it cuts its box, top raised by overshoot, and the dowel holes, overshoot past every face a dowel leaves, out of both; aimed at the column then the rib.
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

    /// The seam connector of compas_tf OuterRibConnectorElement, made parametric: a key across the end-to-end contact of two members, length long along the contact normal, top below the contact's top edge, its heads head_width wide over head_length at both ends and its neck neck_width wide between, depth deep at the seam deepening straight to end_depth at its ends; each member gets a flat-bottomed pocket pocket_depth deep, its head and neck boxes, the neck overshoot past the seam; aimed at a then b. The defaults are the OBJ template's.
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

    /// Assembly dowels on the face contact of two members: four round dowels of radius, length long, centred on the contact along its normal so half goes into each member, one exactly at every corner of the contact polygon inset by offset, the four extreme corners of a longer inset; nothing else moves them. They cut their holes out of both, overshoot past every face a dowel leaves; aimed at a then b. Null when the inset leaves nothing.
    static std::shared_ptr<JointBeam> dowels(
        const session_cpp::Element& a,
        const session_cpp::Element& b,
        const InteractionContactFace& contact,
        double radius = 5.0,
        double length = 30.0,
        double offset = 50.0,
        double overshoot = 10.0,
        int dowel_sides = 16
    );

    /// The half-lap cross joint of two connectors whose box parts cross, two rectangle plates in one column head say: a slot through each part where the other passes, margin wider than the part's thickness and longer than its height so the cut is through, a's from share of their common height up, b's from the bottom up to there, so the two slide together; stored on the connectors as their solid cuts when added. Aimed at a then b, hidden: it is a relation, not a part.
    static std::shared_ptr<JointBeam> cross_lap(const JointBeam& a, const JointBeam& b, double share = 0.5, double margin = 1.0);

    /// One part as a closed mesh, its loops lofted, before any cut.
    session_cpp::Mesh part_mesh(size_t index) const;

    /// The cuts into one part: the connector's stored cuts and, as one more, its own dowels where they pass through the part, so the part carries their bores.
    std::vector<SolidCut> part_cuts(size_t index) const;

    /// The connector's parts and dowels as elements of their own, to nest under it in the tree: a ConnectorPart per part, named part_label, numbered when there are several, with its cuts and bores, then a Dowel per drill line, dowel_0 on; the connector keeps the relation, they carry the solids.
    std::vector<std::shared_ptr<Joint>> children() const;

    /// One part as a BRep with its solid cuts applied and the bores of the dowels passing through it exact, cylinders with circle or ellipse loops.
    session_cpp::BRep part_brep(size_t index) const;

    /// A connector's parts cut and bored, then its dowels as cylinders; nothing once it is nested, its children carrying them; a beam joint's feature volumes.
    const session_cpp::Mesh& element_geometry_mesh() const override;
    const session_cpp::BRep& element_geometry_brep() const override;

    JointBeam(InteractionFeatureBeam feature, const std::function<void(InteractionFeatureBeam&)>& builder);
    void place(const session_cpp::Xform& xform) override;
    std::string element_type_name() const override {
        return "JointBeam";
    }
    std::shared_ptr<session_cpp::Element> clone() const override {
        return std::make_shared<JointBeam>(*this);
    }

protected:
    std::vector<std::array<session_cpp::Polyline, 2>> bodies() const override;
    void write_proto(wood_proto::Joint& proto) const override;
    void read_proto(const wood_proto::Joint& proto) override;
};

}
