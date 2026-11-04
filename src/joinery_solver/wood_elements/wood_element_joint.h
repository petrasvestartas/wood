#pragma once

#include "pch.h"
#include "wood_element_geometry.h"

using namespace session_cpp;

namespace wood_proto {
class JointElement;
}

namespace wood_session {

class NodeSupportElement;
class EdgeStraightElement;

/// Element that represents a joint: what it cuts and drills in its targets; the base of JointBeamElement, JointPlateElement and JointPinElement.
class JointElement : public Element {
public:
    static constexpr std::string_view ELEMENT_TYPE = "JointElement";
    std::vector<Polyline> loops;
    std::vector<Plane> cuts;
    std::vector<std::string> targets;
    double line_radius = 1.0;
    bool generated = false;
    double chord_tolerance = 0.05;
    std::vector<Polyline> cutter_profile;
    Vector cutter_extrusion;
    std::vector<Line> drill_lines;
    SolidOperation operation = SolidOperation::subtract;

protected:
    mutable std::optional<Mesh> mesh_;
    mutable std::optional<BRep> brep_;
    mutable std::optional<Mesh> model_mesh_;
    mutable std::optional<BRep> model_brep_;

public:
    JointElement();
    explicit JointElement(const std::vector<Polyline>& loops, const std::string& name = "JointElement");
    explicit JointElement(const Plane& cutter);
    explicit JointElement(const Mesh& cutter, SolidOperation operation = SolidOperation::subtract);

    /// Keep the interior of a closed profile extruded along direction.
    JointElement(const Polyline& profile, const Vector& direction);
    JointElement(
        const std::vector<Polyline>& profile,
        const Vector& direction,
        SolidOperation operation = SolidOperation::intersect
    );
    static std::shared_ptr<JointElement>
    drill(const Line& axis, double radius, double chord_tolerance = 0.05);

    /// The joint of a support and the column standing on it, named "support".
    static std::shared_ptr<JointElement> support(const NodeSupportElement& support, const EdgeStraightElement& column);
    virtual std::vector<Line> drill_axes() const;

    /// The interaction this joint puts on its target i, for WoodSession::add_interaction.
    virtual std::shared_ptr<Interaction> interaction(size_t target) const;

    static std::shared_ptr<JointElement> from_element(Element element);
    static void register_type();

    /// The joint's solid without its drills: its loops lofted in pairs, else its bodies; empty for a joint that only drills.
    Mesh body_mesh() const;

    const Mesh& element_geometry_mesh() const override;
    const BRep& element_geometry_brep() const override;
    const Mesh& model_geometry_mesh() const override;
    const BRep& model_geometry_brep() const override;
    std::vector<Plane> compute_planes() const override;
    void invalidate_geometry() override;
    void place(const Xform& xform) override;
    std::shared_ptr<JointElement> transformed(const Xform& xform) const;
    using Element::aabb;
    AABB aabb(double inflate) const;
    nlohmann::ordered_json element_data_jsondump() const;
    std::string element_data_dumps() const override;

    std::string element_type_name() const override {
        return std::string(ELEMENT_TYPE);
    }

    std::shared_ptr<Element> clone() const override {
        return std::make_shared<JointElement>(*this);
    }

    std::string str() const override;

    /// The joint's own solids.
    virtual std::vector<std::array<Polyline, 2>> bodies() const;

    /// The joint's own solid when it is no loft of loops.
    virtual Mesh key_mesh() const;

protected:
    virtual void write_proto(wood_proto::JointElement& proto) const;
    virtual void read_proto(const wood_proto::JointElement& proto);
    void compute_geometry_mesh_impl() override;
    void compute_geometry_brep_impl() override;
};

}
