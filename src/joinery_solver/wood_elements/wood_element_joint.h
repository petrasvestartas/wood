#pragma once

#include "pch.h"
#include "wood_element_geometry.h"

namespace wood_proto {
class Joint;
}

namespace wood_session {

class Support;
class Column;

class Joint : public session_cpp::Element {
public:
    static constexpr std::string_view ELEMENT_TYPE = "Joint";
    std::vector<session_cpp::Polyline> loops;
    std::vector<session_cpp::Plane> cuts;
    std::vector<std::string> targets;
    double line_radius = 1.0;
    bool generated = false;
    double chord_tolerance = 0.05;
    std::vector<session_cpp::Polyline> cutter_profile;
    session_cpp::Vector cutter_extrusion;
    std::vector<session_cpp::Line> drill_lines;
    SolidOperation operation = SolidOperation::difference;

protected:
    mutable std::optional<session_cpp::Mesh> mesh_;
    mutable std::optional<session_cpp::BRep> brep_;
    mutable std::optional<session_cpp::Mesh> model_mesh_;
    mutable std::optional<session_cpp::BRep> model_brep_;

public:
    Joint();
    explicit Joint(const std::vector<session_cpp::Polyline>& loops, const std::string& name = "Joint");
    explicit Joint(const session_cpp::Plane& cutter);
    explicit Joint(const session_cpp::Mesh& cutter, SolidOperation operation = SolidOperation::difference);
    /// Keep the interior of a closed profile extruded along direction.
    Joint(const session_cpp::Polyline& profile, const session_cpp::Vector& direction);
    Joint(const std::vector<session_cpp::Polyline>& profile, const session_cpp::Vector& direction,
          SolidOperation operation = SolidOperation::intersection);
    static std::shared_ptr<Joint> drill(const session_cpp::Line& axis, double radius, double chord_tolerance = 0.05);

    /// The joint of a support and the column standing on it, named "support": the head plate disc let up into the column end by the recess, and the column screws drilled from the head plate underside, so each hole opens into the pocket; aimed at the column.
    static std::shared_ptr<Joint> support(const Support& support, const Column& column);
    virtual std::vector<session_cpp::Line> drill_axes() const;
    static std::shared_ptr<Joint> from_element(session_cpp::Element element);
    static void register_type();

    /// The joint's solid without its drills: its loops lofted in pairs, else its bodies; empty for a joint that only drills.
    session_cpp::Mesh body_mesh() const;

    const session_cpp::Mesh& element_geometry_mesh() const override;
    const session_cpp::BRep& element_geometry_brep() const override;
    const session_cpp::Mesh& model_geometry_mesh() const override;
    const session_cpp::BRep& model_geometry_brep() const override;
    std::vector<session_cpp::Plane> compute_planes() const override;
    void invalidate_geometry() override;
    void place(const session_cpp::Xform& xform) override;
    std::shared_ptr<Joint> transformed(const session_cpp::Xform& xform) const;
    using session_cpp::Element::aabb;
    session_cpp::AABB aabb(double inflate) const;
    nlohmann::ordered_json element_data_jsondump() const;
    std::string element_data_dumps() const override;
    std::string element_type_name() const override {
        return std::string(ELEMENT_TYPE);
    }
    std::shared_ptr<session_cpp::Element> clone() const override {
        return std::make_shared<Joint>(*this);
    }
    std::string str() const override;

protected:
    virtual std::vector<std::array<session_cpp::Polyline, 2>> bodies() const;
    virtual void write_proto(wood_proto::Joint& proto) const;
    virtual void read_proto(const wood_proto::Joint& proto);
    void compute_geometry_mesh_impl() override;
    void compute_geometry_brep_impl() override;
};

}
