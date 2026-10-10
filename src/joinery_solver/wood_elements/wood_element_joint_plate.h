#pragma once

#include "wood_element_joint.h"
#include "wood_element_plate.h"
#include "wood_interaction_feature_plate.h"
#include "wood_settings.h"

using namespace session_cpp;

namespace wood_session {

/// The parameters a library design of a plate joint is made with.
struct JointPlateParameters {
    std::string library;                                        // Library constructor name.
    int contact_type = 0;                                       // Required contact family.
    int divisions = 0;                                          // Explicit count, zero uses division distance.
    double taper = 0.0;                                         // Finger taper.
    bool chamfer = false;                                       // Chamfer finger ends.
    bool modify_outline = true;                                 // Modify the female outline.
    std::array<double, 2> x = {-0.5, 0.5};                      // Finger x extent.
    std::array<double, 2> y = {-0.5, 0.5};                      // Finger y extent.
    std::array<double, 2> z = {-0.5, 0.5};                      // Finger z extent.
    bool disable_divisions = false;                             // Disable divisions on the second linked joint.
    double distance_squared = 0.01;                             // Drill boundary opening tolerance.
    bool merge_with_joint = false;                              // Merge side removal with an existing joint.
    std::array<std::vector<Polyline>, 2> outlines; // Custom male and female outline pairs.

    std::string pb_dumps() const;

    static JointPlateParameters pb_loads(const std::string& data);
};

/// Element that represents a joint between plates: a parametric joint from the joint library placed on plate connections.
class JointPlate : public Joint {
public:
    JointPlateParameters parameters;
    std::vector<InteractionFeaturePlate> connections;
    int variant = 20;
    double division_distance = 450.0;
    double shift = 0.5;

    // ═══════════════════════════════════════════════════════════════════════════
    // ss_e_ip
    // ═══════════════════════════════════════════════════════════════════════════

    // Side-to-side in plane, contact 12, ids 1-9 as 2024 numbered them: 1 ss_e_ip_1 (the family default), 2 ss_e_ip_0,
    // 3 ss_e_ip_2, 4 ss_e_ip_3, 5 ss_e_ip_4, 6 ss_e_ip_5, 8 side_removal, 9 ss_e_ip_custom. A zero division count takes
    // the geometric count, the joint line's length over the family's 300 mm; the shift default is the family's 0.5.

    static std::shared_ptr<JointPlate> ss_e_ip_0();

    static std::shared_ptr<JointPlate> ss_e_ip_1(int divisions = 0, double shift = 0.5);

    static std::shared_ptr<JointPlate> ss_e_ip_2(int divisions = 0);

    static std::shared_ptr<JointPlate> ss_e_ip_3();

    static std::shared_ptr<JointPlate> ss_e_ip_4();

    static std::shared_ptr<JointPlate> ss_e_ip_5(int divisions = 0);

    static std::shared_ptr<JointPlate> ss_e_ip_custom(const std::vector<Polyline>& male, const std::vector<Polyline>& female);

    // ═══════════════════════════════════════════════════════════════════════════
    // ss_e_op
    // ═══════════════════════════════════════════════════════════════════════════

    // Side-to-side out of plane, contact 11, ids 10-19 as 2024 numbered them: 10 ss_e_op_1, 11 ss_e_op_2, 12 ss_e_op_0,
    // 13 ss_e_op_3, 14 ss_e_op_4, 15 ss_e_op_5 (the family default id), 16 ss_e_op_6, 18 side_removal, 19 ss_e_op_custom;
    // an id without an entry, such as 17, takes ss_e_op_1. A zero division count takes the geometric count, the joint
    // line's length over the family's 450 mm; the shift default is the family's 0.64. ss_e_op_17 and ss_e_op_tutorial
    // are designs of this port, reached by name only.

    static std::shared_ptr<JointPlate> ss_e_op_0();

    static std::shared_ptr<JointPlate> ss_e_op_1(int divisions = 0, double shift = 0.64);

    static std::shared_ptr<JointPlate> ss_e_op_2(int divisions = 0, double shift = 0.64);

    static std::shared_ptr<JointPlate> ss_e_op_3();

    static std::shared_ptr<JointPlate> ss_e_op_4(
        int divisions = 0,
        double taper = 0.0,
        bool chamfer = true,
        bool modify_outline = true,
        const std::array<double, 2>& x = {-0.5, 0.5},
        const std::array<double, 2>& y = {-0.5, 0.5},
        const std::array<double, 2>& z = {-0.5, 0.5}
    );

    static std::shared_ptr<JointPlate> ss_e_op_5(int divisions = 0, bool disable_divisions = false);

    static std::shared_ptr<JointPlate> ss_e_op_6(int divisions = 0);

    static std::shared_ptr<JointPlate> ss_e_op_17(int divisions = 4);

    static std::shared_ptr<JointPlate> ss_e_op_tutorial();

    static std::shared_ptr<JointPlate> ss_e_op_custom(const std::vector<Polyline>& male, const std::vector<Polyline>& female);

    // ═══════════════════════════════════════════════════════════════════════════
    // ts_e_p
    // ═══════════════════════════════════════════════════════════════════════════

    static std::shared_ptr<JointPlate> ts_e_p_0();

    static std::shared_ptr<JointPlate> ts_e_p_1();

    static std::shared_ptr<JointPlate> ts_e_p_2(int divisions = 8, double shift = 0.5);

    static std::shared_ptr<JointPlate> ts_e_p_3(int divisions = 8, double shift = 0.5);

    static std::shared_ptr<JointPlate> ts_e_p_5(int divisions = 4);

    static std::shared_ptr<JointPlate> ts_e_p_custom(const std::vector<Polyline>& male, const std::vector<Polyline>& female);

    // ═══════════════════════════════════════════════════════════════════════════
    // ss_e_r
    // ═══════════════════════════════════════════════════════════════════════════

    // Side-to-side rotated, contact 13, ids 50-59 as 2024 numbered them: 54 ss_e_r_3, 55 ss_e_r_2, 56 ss_e_r_0,
    // 57 side_removal, 58 side_removal merged with the joint (the family default id), 59 ss_e_r_custom; an id without
    // an entry takes side_removal. A zero division count takes the geometric count, the joint line's length over the
    // family's 300 mm; the shift default is the family's 0.5. ss_e_r_2 and ss_e_r_3 are key designs: each plate is
    // milled a pocket on its side of the seam and the joint owns the loose key that fills both. ss_e_r_1 is no design
    // of its own: it is the tenon tile side_removal_ss_e_r_1 lays in the side face, with no id and no dispatch in 2024,
    // and alone on a pair its male and female profiles coincide, so it is not exposed.

    static std::shared_ptr<JointPlate> ss_e_r_0();

    static std::shared_ptr<JointPlate> ss_e_r_2(int divisions = 0, double shift = 0.5);

    static std::shared_ptr<JointPlate> ss_e_r_3(int divisions = 0, double shift = 0.5);

    static std::shared_ptr<JointPlate> ss_e_r_custom(const std::vector<Polyline>& male, const std::vector<Polyline>& female);

    // ═══════════════════════════════════════════════════════════════════════════
    // cr_c_ip
    // ═══════════════════════════════════════════════════════════════════════════

    static std::shared_ptr<JointPlate> cr_c_ip_0();

    static std::shared_ptr<JointPlate> cr_c_ip_1(double shift = 0.5);

    static std::shared_ptr<JointPlate> cr_c_ip_2();

    static std::shared_ptr<JointPlate> cr_c_ip_3();

    static std::shared_ptr<JointPlate> cr_c_ip_4();

    static std::shared_ptr<JointPlate> cr_c_ip_5();

    static std::shared_ptr<JointPlate> cr_c_ip_custom(const std::vector<Polyline>& male, const std::vector<Polyline>& female);

    // ═══════════════════════════════════════════════════════════════════════════
    // b
    // ═══════════════════════════════════════════════════════════════════════════

    static std::shared_ptr<JointPlate> b_0();

    static std::shared_ptr<JointPlate> b_custom(const std::vector<Polyline>& male, const std::vector<Polyline>& female);

    // ═══════════════════════════════════════════════════════════════════════════
    // tt_e_p
    // ═══════════════════════════════════════════════════════════════════════════

    static std::shared_ptr<JointPlate> tt_e_p_0(double radius = 1.0, double chord_tolerance = 0.05);

    static std::shared_ptr<JointPlate> tt_e_p_1(double radius = 1.0, double chord_tolerance = 0.05);

    static std::shared_ptr<JointPlate> tt_e_p_2(
        int count = 6,
        double circle_radius = 20.0,
        double radius = 1.0,
        double chord_tolerance = 0.05
    );

    static std::shared_ptr<JointPlate> tt_e_p_3(
        double spacing = 30.0,
        double radius = 1.0,
        double chord_tolerance = 0.05,
        double distance_squared = 0.01
    );

    static std::shared_ptr<JointPlate> tt_e_p_4(double spacing = 30.0, double radius = 1.0, double chord_tolerance = 0.05);

    static std::shared_ptr<JointPlate> tt_e_p_5(double spacing = 30.0, double radius = 1.0, double chord_tolerance = 0.05);

    // ═══════════════════════════════════════════════════════════════════════════
    // side_removal
    // ═══════════════════════════════════════════════════════════════════════════

    // The side faces of a pair milled off by the joint scale, on every side-side, side-top and cross family (ids x8);
    // side_removal_ss_e_r_1 is the rotated family's, merging the ss_e_r_1 tile into the male side under merge_with_joint.
    // 2024 named side_removal_ss_e_r_1 as id 58 but dispatched side_removal(merge) for it, so no 2025 reference holds it;
    // its merged form, kept as 2024 wrote it, swaps the sides last, hands each plate its own side slab outside its stock
    // and so removes no side: only the tile's conic slivers cut. It is the 2024 function, not a design the solver reaches.

    static std::shared_ptr<JointPlate> side_removal(bool merge_with_joint = false, double shift = 0.5);

    static std::shared_ptr<JointPlate> side_removal_ss_e_r_1(bool merge_with_joint = false, double shift = 0.5);

    // ═══════════════════════════════════════════════════════════════════════════
    // Constructors
    // ═══════════════════════════════════════════════════════════════════════════

    JointPlate();
    explicit JointPlate(const InteractionFeaturePlate& connection);
    JointPlate(int variant, double division_distance, double shift = 0.5);
    JointPlate(const std::shared_ptr<Plate>& source, const std::shared_ptr<Plate>& target,
               const InteractionContactFace& contact, int variant, double division_distance,
               double shift = 0.5, const Settings& settings = Settings());
    static std::shared_ptr<JointPlate> side_to_top(int variant = 20, double division_distance = 450.0, double shift = 0.5);
    void orient(const std::shared_ptr<InteractionContactFace>& contact, const Settings& settings = Settings());
    void orient(const std::shared_ptr<InteractionContactFace>& contact, const std::vector<std::shared_ptr<Plate>>& elements, const Settings& settings = Settings());
    void orient(const std::shared_ptr<InteractionContactCross>& contact, const Settings& settings = Settings());
    void orient(const std::shared_ptr<InteractionContactCross>& contact, const std::vector<std::shared_ptr<Plate>>& elements, const Settings& settings = Settings());
    void construct(std::vector<InteractionFeaturePlate> connections, const std::vector<std::shared_ptr<Plate>>& elements, const Settings& settings = Settings());
    void construct(InteractionFeaturePlate connection, const Settings& settings,
                   std::vector<std::shared_ptr<Plate>> elements = {});
    void construct(InteractionFeaturePlate connection, const std::function<void(InteractionFeaturePlate&)>& builder);
    std::shared_ptr<InteractionFeaturePlate> interaction_feature(int side, size_t connection = 0) const;

    /// The interaction this joint puts on its target i, side i of its first connection: interaction_feature(i, 0).
    std::shared_ptr<Interaction> interaction(size_t target) const override;
    static void build_geometry(std::vector<InteractionFeaturePlate>& connections,
                               std::vector<std::shared_ptr<Plate>>& elements,
                               const std::vector<std::vector<int>>& types, const Settings& settings);
    void place(const Xform& xform) override;
    std::vector<Line> drill_axes() const override;
    std::string element_type_name() const override {
        return "JointPlate";
    }
    std::shared_ptr<Element> clone() const override {
        return std::make_shared<JointPlate>(*this);
    }

private:
    static std::shared_ptr<JointPlate> from_library(const std::string& library, int contact_type);
    static std::shared_ptr<JointPlate> ss_e_ip(const std::string& library);
    static std::shared_ptr<JointPlate> ss_e_op(const std::string& library);
    static std::shared_ptr<JointPlate> ss_e_r(const std::string& library);
    void compute_library(
        InteractionFeaturePlate& connection,
        const std::vector<std::shared_ptr<Plate>>& elements,
        std::vector<InteractionFeaturePlate>& connections,
        const Settings& settings
    ) const;
    void compute_parameters(InteractionFeaturePlate& connection, const std::vector<std::shared_ptr<Plate>>& elements, const Settings& settings) const;
    bool compute_ss_e_ip(InteractionFeaturePlate& connection, const std::vector<std::shared_ptr<Plate>>& elements, const Settings& settings) const;
    bool compute_ss_e_op(InteractionFeaturePlate& connection, std::vector<InteractionFeaturePlate>& connections, const Settings& settings) const;
    bool compute_ts_e_p(InteractionFeaturePlate& connection, const Settings& settings) const;
    bool compute_ss_e_r(InteractionFeaturePlate& connection, const std::vector<std::shared_ptr<Plate>>& elements, const Settings& settings) const;
    bool compute_cr_c_ip(InteractionFeaturePlate& connection, const Settings& settings) const;
    bool compute_b(InteractionFeaturePlate& connection, const Settings& settings) const;
    bool compute_tt_e_p(InteractionFeaturePlate& connection, const std::vector<std::shared_ptr<Plate>>& elements) const;
    bool compute_side_removal(InteractionFeaturePlate& connection, const std::vector<std::shared_ptr<Plate>>& elements) const;

protected:
    std::vector<std::array<Polyline, 2>> bodies() const override;
    void write_proto(wood_proto::Joint& proto) const override;
    void read_proto(const wood_proto::Joint& proto) override;
};

/// Element that represents the joints of an Annen dataset: plate joints built from its plates, connections and groups.
class JointAnnen : public JointPlate {
public:
    JointAnnen() = default;
    JointAnnen(std::vector<std::shared_ptr<Plate>> elements, std::vector<InteractionFeaturePlate> connections,
               const std::vector<std::vector<int>>& groups, const Settings& settings = Settings());
    std::string element_type_name() const override {
        return "JointAnnen";
    }
    std::shared_ptr<Element> clone() const override {
        return std::make_shared<JointAnnen>(*this);
    }
};

/// Element that represents the joints of a Vidy dataset: plate joints built from its plates, connections and groups.
class JointVidy : public JointPlate {
public:
    JointVidy() = default;
    JointVidy(std::vector<std::shared_ptr<Plate>> elements, std::vector<InteractionFeaturePlate> connections,
              const std::vector<std::vector<int>>& groups, const Settings& settings = Settings());
    std::string element_type_name() const override {
        return "JointVidy";
    }
    std::shared_ptr<Element> clone() const override {
        return std::make_shared<JointVidy>(*this);
    }
};

}
