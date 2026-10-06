#include "pch.h"
#include "wood_interaction_feature_cut.h"
using namespace session_cpp;

namespace wood_session {

InteractionFeatureCut::InteractionFeatureCut(const SolidCut& cut) : cut(cut) {
}

// ═══════════════════════════════════════════════════════════════════════════
// InteractionFeatureCut - Geometry
// ═══════════════════════════════════════════════════════════════════════════

std::string_view InteractionFeatureCut::kind() const {
    return "cut";
}

// ═══════════════════════════════════════════════════════════════════════════
// InteractionFeatureCut - Protobuf
// ═══════════════════════════════════════════════════════════════════════════

std::string InteractionFeatureCut::interaction_type_name() const {
    return std::string(INTERACTION_TYPE);
}

std::string InteractionFeatureCut::interaction_data_dumps() const {
    return cut.pb_dumps();
}

InteractionFeatureCut InteractionFeatureCut::interaction_data_loads(const std::string& data) {
    return InteractionFeatureCut(SolidCut::pb_loads(data));
}

std::shared_ptr<Interaction> InteractionFeatureCut::clone() const {
    return std::make_shared<InteractionFeatureCut>(*this);
}

/// The registered factory: interaction_data bytes to a cut.
static std::shared_ptr<Interaction> feature_cut_from_protobuf(const std::string& data) {
    return std::make_shared<InteractionFeatureCut>(InteractionFeatureCut::interaction_data_loads(data));
}

void InteractionFeatureCut::register_type() {
    Interaction::register_type(std::string(INTERACTION_TYPE), feature_cut_from_protobuf);
}

// ═══════════════════════════════════════════════════════════════════════════
// InteractionFeatureCut - String
// ═══════════════════════════════════════════════════════════════════════════

std::string InteractionFeatureCut::str() const {
    return fmt::format("InteractionFeatureCut(faces={}, drills={})", cut.mesh.number_of_faces(), cut.drills.size());
}

} // namespace wood_session
