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
    std::vector<std::vector<std::array<session_cpp::Polyline, 2>>> cutters; // A connector's cutters per target in targets order, lofted like parts; the drill lines cut every target.
    JointBeam();
    JointBeam(const Beam& source, const Beam& target, const InteractionContactAxis& contact,
              double volume_length, double cross_or_side_to_end, int flip_male = 0);
    static std::shared_ptr<JointBeam> from_contact(const Beam& source, const Beam& target,
                                                   const InteractionContactAxis& contact, double volume_length, double cross_or_side_to_end, int flip_male = 0);
    explicit JointBeam(const InteractionFeatureBeam& feature);

    /// The wedge connector of compas_tf ConnectorWedgeElement on the face contact of two members: a triangular prism along the contact's longest top edge, shortened by length_margin at both ends, horizontal dowels every dowel_spacing as dowel_sides-sided prisms, and in each member a box pocket pocket_depth deep under the wedge face on its side; aimed at a then b.
    static std::shared_ptr<JointBeam> wedge(
        const session_cpp::Element& a,
        const session_cpp::Element& b,
        const InteractionContactFace& contact,
        double length_margin,
        double pocket_depth,
        double dowel_radius = 10.0,
        double dowel_spacing = 320.0,
        int dowel_sides = 8
    );

    /// The column-to-rib connector of compas_tf ConnectorElement on their face contact: a plate width thick, back into the column and front into the rib along the horizontal contact normal, height down from the contact's top edge, with four dowels across it, two per side, margin_x and margin_z radii in from its ends and its top and bottom; it cuts its box, top raised by overshoot, and the dowel holes, dowel_length plus overshoot at both ends, out of both; aimed at the column then the rib.
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
