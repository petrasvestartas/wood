#pragma once

#include "pch.h"
#include "wood_element_geometry.h"

using namespace session_cpp;

namespace wood_proto {
class Joint;
}

namespace wood_session {

class Support;
class Column;

/// Element that represents a joint: what it cuts and drills in its targets; the base of JointBeam, JointPlate and Pin.
class Joint : public Element {
public:
    static constexpr std::string_view ELEMENT_TYPE = "Joint";
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
    Joint();
    explicit Joint(const std::vector<Polyline>& loops, const std::string& name = "Joint");
    explicit Joint(const Plane& cutter);
    explicit Joint(const Mesh& cutter, SolidOperation operation = SolidOperation::subtract);
    /// Keep the interior of a closed profile extruded along direction.
    Joint(const Polyline& profile, const Vector& direction);
    Joint(const std::vector<Polyline>& profile, const Vector& direction,
          SolidOperation operation = SolidOperation::intersect);
    static std::shared_ptr<Joint> drill(const Line& axis, double radius, double chord_tolerance = 0.05);

    /// The joint of a support and the column standing on it, named "support": a head plate let into the column end and pinned.
    static std::shared_ptr<Joint> support(const Support& support, const Column& column);
    virtual std::vector<Line> drill_axes() const;

    /// The interaction this joint puts on its target i, for WoodSession::add_interaction.
    virtual std::shared_ptr<Interaction> interaction(size_t target) const;

    static std::shared_ptr<Joint> from_element(Element element);
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
    std::shared_ptr<Joint> transformed(const Xform& xform) const;
    using Element::aabb;
    AABB aabb(double inflate) const;
    nlohmann::ordered_json element_data_jsondump() const;
    std::string element_data_dumps() const override;
    std::string element_type_name() const override {
        return std::string(ELEMENT_TYPE);
    }
    std::shared_ptr<Element> clone() const override {
        return std::make_shared<Joint>(*this);
    }
    std::string str() const override;

    /// The joint's own solids, each a bottom and a top loop to loft: a connector's parts, a key design's keys; empty for a joint that owns none.
    virtual std::vector<std::array<Polyline, 2>> bodies() const;

protected:
    virtual void write_proto(wood_proto::Joint& proto) const;
    virtual void read_proto(const wood_proto::Joint& proto);
    void compute_geometry_mesh_impl() override;
    void compute_geometry_brep_impl() override;
};

}
