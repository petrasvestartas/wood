#pragma once

#include "wood_element_joint.h"

using namespace session_cpp;

namespace wood_session {

/// Element that represents a cutting plane: a joint with one target side, drawn as a square.
class JointCutPlaneElement : public JointElement {
public:
    double size = 1000.0; // The side of the square it is drawn as.

    JointCutPlaneElement();

    /// A cutting plane drawn as a square of size.
    explicit JointCutPlaneElement(
        const Plane& plane, double size = 1000.0, const std::string& name = "cut_plane"
    );

    /// The cutting plane; what it cuts keeps the side its normal points to.
    const Plane& plane() const {
        return cuts.front();
    }

    /// The plane feature it cuts a target by.
    std::shared_ptr<Interaction> interaction(size_t target) const override;

    /// The square on the plane, one face.
    const Mesh& element_geometry_mesh() const override;

    /// The square on the plane, one face.
    const BRep& element_geometry_brep() const override;

    /// The square: a plane is not cut.
    const Mesh& model_geometry_mesh() const override;

    /// The square: a plane is not cut.
    const BRep& model_geometry_brep() const override;

    std::string element_type_name() const override {
        return "JointCutPlaneElement";
    }

    std::shared_ptr<Element> clone() const override {
        return std::make_shared<JointCutPlaneElement>(*this);
    }

protected:
    void write_proto(wood_proto::JointElement& proto) const override;
    void read_proto(const wood_proto::JointElement& proto) override;
};

}
