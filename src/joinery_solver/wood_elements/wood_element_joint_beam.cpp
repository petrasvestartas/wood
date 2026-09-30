#include "pch.h"
#include "element_joint.pb.h"
#include "wood_session.h"
#include "wood_feature_detection_beam.h"

namespace wood_session {

using namespace session_cpp;

JointBeam::JointBeam() {
    name = "JointBeam";
}
JointBeam::JointBeam(const InteractionFeatureBeam& feature) : feature(feature) {
    name = "JointBeam";
}
JointBeam::JointBeam(InteractionFeatureBeam input, const std::function<void(InteractionFeatureBeam&)>& builder)
    : feature(std::move(input)) {
    if (!builder)
        throw std::invalid_argument("A custom beam joint needs a builder");
    name = "JointBeam";
    builder(feature);
}
JointBeam::JointBeam(const Beam& source, const Beam& target, const InteractionContactAxis& contact,
                     double volume_length, double cross_or_side_to_end, int flip_male) {
    name = "JointBeam";
    if (!beam_to_beam(source, target, contact, volume_length, cross_or_side_to_end, flip_male, feature))
        throw std::invalid_argument("Cannot construct joint for the given beam contact");
    targets = {source.guid(), target.guid()};
}
std::shared_ptr<JointBeam> JointBeam::from_contact(const Beam& source, const Beam& target,
                                                   const InteractionContactAxis& contact, double volume_length, double cross_or_side_to_end, int flip_male) {
    const std::shared_ptr<JointBeam> joint = std::make_shared<JointBeam>();
    if (!beam_to_beam(source, target, contact, volume_length, cross_or_side_to_end, flip_male, joint->feature))
        return nullptr;
    joint->targets = {source.guid(), target.guid()};
    return joint;
}
std::vector<std::array<Polyline, 2>> JointBeam::bodies() const {
    std::vector<std::array<Polyline, 2>> result;
    for (int k = 0; k < 4; k += 2)
        if (feature.volumes[k].point_count() >= 4 && feature.volumes[k + 1].point_count() >= 4)
            result.push_back({feature.volumes[k], feature.volumes[k + 1]});
    return result;
}
void JointBeam::place(const Xform& xform) {
    Joint::place(xform);
    for (Polyline& volume : feature.volumes)
        volume = volume.transformed(xform);
}

void JointBeam::write_proto(wood_proto::Joint& proto) const {

    Joint::write_proto(proto);
    if (!proto.mutable_beam_feature()->ParseFromString(feature.pb_dumps()))
        throw std::runtime_error("Invalid beam feature");
}

void JointBeam::read_proto(const wood_proto::Joint& proto) {

    Joint::read_proto(proto);
    if (!proto.has_beam_feature())
        return;

    const std::shared_ptr<InteractionFeatureBeam> feature = std::dynamic_pointer_cast<InteractionFeatureBeam>(Interaction::pb_loads(proto.beam_feature().SerializeAsString()));
    if (!feature)
        throw std::runtime_error("Invalid beam joint feature");
    this->feature = *feature;
}

}
