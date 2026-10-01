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
