#pragma once

#include "pch.h"
#include "wood_element_geometry.h"
#include "wood_joint_category.h"
#include "wood_joint_cut_step.h"
#include "wood_joint_move.h"

using namespace laika_cpp;

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
    std::string design; // Its compendium design: "notched_oblique_thrust"; empty for an older joint.
    JointCategory category = JointCategory::none; // What meets in it.
    JointFamily family = JointFamily::none; // Its compendium family.
    std::vector<JointCutStep> steps; // Its cutting procedure in order; a target loses its steps' removals in one boolean.
    std::vector<std::vector<Vector>> target_insertions; // Per target, the unit directions it goes in along; it comes out the opposite way.
    std::vector<std::vector<std::vector<JointMove>>> target_legs; // Per target and way in (as target_insertions), the moves after the approach in assembly order, slides and turns; none for a straight way in.
    std::vector<std::vector<Line>> target_posts; // Per target, the round posts its cut keeps (a Chidori stick's neck), exact cylinders.
    double post_radius = 0.0; // Radius of every post.

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

    /// The interaction this joint puts on its target i, for WoodSession::add_interaction: a joint with a procedure its removals on that target in one cut.
    virtual std::shared_ptr<Interaction> interaction(size_t target) const;

    /// The steps of the procedure on target i, in order.
    std::vector<JointCutStep> target_steps(size_t target) const;

    /// What target i loses to the first count steps of the procedure on it, every removal side by side in one mesh for one boolean; all of them by default.
    Mesh removal(size_t target, size_t count = std::numeric_limits<size_t>::max()) const;

    /// The unit directions target i can be inserted along into the joint and its other targets, none when the joint does not say.
    virtual std::vector<Vector> insertion_directions(size_t target) const;

    /// The moves target i makes after approaching along its way in number way, in assembly order: in, then along the slot; none for a straight way in.
    std::vector<JointMove> insertion_legs(size_t target, size_t way) const;

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

    /// The joint's own solid when it is no loft of loops: a joint with a procedure every removal of it.
    virtual Mesh key_mesh() const;

protected:
    virtual void write_proto(wood_proto::JointElement& proto) const;
    virtual void read_proto(const wood_proto::JointElement& proto);
    void compute_geometry_mesh_impl() override;
    void compute_geometry_brep_impl() override;
};

}
