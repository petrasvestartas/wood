#pragma once

#include "wood_element_joint.h"
#include "wood_element_beam.h"
#include "wood_interaction_feature_beam.h"
#include "wood_interaction_contact_axis.h"

namespace wood_session {

class JointBeam : public Joint {
public:
    InteractionFeatureBeam feature;
    JointBeam();
    JointBeam(const Beam& source, const Beam& target, const InteractionContactAxis& contact,
              double volume_length, double cross_or_side_to_end, int flip_male = 0);
    static std::shared_ptr<JointBeam> from_contact(const Beam& source, const Beam& target,
                                                   const InteractionContactAxis& contact, double volume_length, double cross_or_side_to_end, int flip_male = 0);
    explicit JointBeam(const InteractionFeatureBeam& feature);
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
