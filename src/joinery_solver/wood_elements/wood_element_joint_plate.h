#pragma once

#include "wood_element_joint.h"
#include "wood_element_plate.h"
#include "wood_interaction_feature_plate.h"
#include "wood_settings.h"

using namespace session_cpp;

namespace wood_session {

/// The parameters a library design of a plate joint is made with.
struct JointPlateParameters {
    std::string library; // Library constructor name.
    int contact_type = 0; // Required contact family.
    int divisions = 0; // Explicit count, zero uses division distance.
    double taper = 0.0; // Finger taper.
    bool chamfer = false; // Chamfer finger ends.
    bool modify_outline = true; // Modify the female outline.
    std::array<double, 2> x = {-0.5, 0.5}; // Finger x extent.
    std::array<double, 2> y = {-0.5, 0.5}; // Finger y extent.
    std::array<double, 2> z = {-0.5, 0.5}; // Finger z extent.
    bool disable_divisions = false; // Disable divisions on the second linked joint.
    double distance_squared = 0.01; // Drill boundary opening tolerance.
    bool merge_with_joint = false; // Merge side removal with an existing joint.
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
    // side_side_inplane
    // ═══════════════════════════════════════════════════════════════════════════
    // Side-to-side in plane, contact 12, ids 1-9 as 2024 numbered them: 1 side_side_inplane_dovetails (the family default), 2 side_side_inplane_three_fingers,

    static std::shared_ptr<JointPlate> side_side_inplane_three_fingers();

    static std::shared_ptr<JointPlate>
    side_side_inplane_dovetails(int divisions = 0, double shift = 0.5);

    static std::shared_ptr<JointPlate> side_side_inplane_butterfly_keys(int divisions = 0);

    static std::shared_ptr<JointPlate> side_side_inplane_splayed_scarf();

    static std::shared_ptr<JointPlate> side_side_inplane_cross_splayed_scarf();

    static std::shared_ptr<JointPlate> side_side_inplane_tooth_keys(int divisions = 0);

    static std::shared_ptr<JointPlate> side_side_inplane_custom(
        const std::vector<Polyline>& male, const std::vector<Polyline>& female
    );

    // ═══════════════════════════════════════════════════════════════════════════
    // side_side_outplane
    // ═══════════════════════════════════════════════════════════════════════════
    // Side-to-side out of plane, contact 11, ids 10-19 as 2024 numbered them: 10 side_side_outplane_fingers, 11 side_side_outplane_graded_fingers, 12 side_side_outplane_three_fingers,

    static std::shared_ptr<JointPlate> side_side_outplane_three_fingers();

    static std::shared_ptr<JointPlate>
    side_side_outplane_fingers(int divisions = 0, double shift = 0.64);

    static std::shared_ptr<JointPlate>
    side_side_outplane_graded_fingers(int divisions = 0, double shift = 0.64);

    static std::shared_ptr<JointPlate> side_side_outplane_miter_tenon();

    static std::shared_ptr<JointPlate> side_side_outplane_tenons(
        int divisions = 0,
        double taper = 0.0,
        bool chamfer = true,
        bool modify_outline = true,
        const std::array<double, 2>& x = {-0.5, 0.5},
        const std::array<double, 2>& y = {-0.5, 0.5},
        const std::array<double, 2>& z = {-0.5, 0.5}
    );

    static std::shared_ptr<JointPlate>
    side_side_outplane_linked_tenons(int divisions = 0, bool disable_divisions = false);

    static std::shared_ptr<JointPlate> side_side_outplane_linked_tenons_wall(int divisions = 0);

    static std::shared_ptr<JointPlate> side_side_outplane_capped_fingers(int divisions = 4);

    static std::shared_ptr<JointPlate> side_side_outplane_notch();

    static std::shared_ptr<JointPlate> side_side_outplane_custom(
        const std::vector<Polyline>& male, const std::vector<Polyline>& female
    );

    // ═══════════════════════════════════════════════════════════════════════════
    // top_side
    // ═══════════════════════════════════════════════════════════════════════════
    // Top to side, contact 20, ids 20-29 as 2024 numbered them: 20 top_side_tenons (the family default id), 21 top_side_dense_tenons, 22 top_side_tenons,

    static std::shared_ptr<JointPlate> top_side_three_tenons();

    static std::shared_ptr<JointPlate> top_side_annen_tenons();

    static std::shared_ptr<JointPlate> top_side_dense_tenons(int divisions = 0, double shift = 0.5);

    static std::shared_ptr<JointPlate> top_side_tenons(int divisions = 0, double shift = 0.5);

    static std::shared_ptr<JointPlate> top_side_milled_wedge();

    static std::shared_ptr<JointPlate> top_side_snap_fit(int divisions = 0);

    static std::shared_ptr<JointPlate>
    top_side_custom(const std::vector<Polyline>& male, const std::vector<Polyline>& female);

    // ═══════════════════════════════════════════════════════════════════════════
    // side_side_rotated
    // ═══════════════════════════════════════════════════════════════════════════
    // Side-to-side rotated, contact 13, ids 50-59 as 2024 numbered them: 54 side_side_rotated_keys, 55 side_side_rotated_hooks, 56 side_side_rotated_miter,

    static std::shared_ptr<JointPlate> side_side_rotated_miter();

    static std::shared_ptr<JointPlate>
    side_side_rotated_hooks(int divisions = 0, double shift = 0.5);

    static std::shared_ptr<JointPlate>
    side_side_rotated_keys(int divisions = 0, double shift = 0.5);

    static std::shared_ptr<JointPlate> side_side_rotated_custom(
        const std::vector<Polyline>& male, const std::vector<Polyline>& female
    );

    /// Creates the keyed Hilti connector across the straight seam between two slabs.
    static std::shared_ptr<JointPlate> side_side_inplane_hilti(
        const Plate& a,
        const Plate& b,
        const InteractionContactFace& contact,
        double half_length = 120.0,
        double neck_length = 27.7,
        double neck_width = 40.0,
        double taper_end = 74.4,
        double wing_width = 90.0,
        double depth = 93.0,
        double height = 90.0,
        double rod_diameter = 12.0,
        double disc_diameter = 50.0,
        double disc_thickness = 6.0,
        double router_radius = 20.0,
        int sides = 32
    );

    // ═══════════════════════════════════════════════════════════════════════════
    // cross
    // ═══════════════════════════════════════════════════════════════════════════
    // Cross in-plane, contact 30, ids 30-39 as 2024 numbered them: 30 cross_slot (the family default id), 31 cross_sliced_half_lap,

    static std::shared_ptr<JointPlate> cross_slot();
    static std::shared_ptr<JointPlate> cross_sliced_half_lap(double shift = 0.5);

    static std::shared_ptr<JointPlate> cross_half_lap(double shift = 0.5);

    static std::shared_ptr<JointPlate> cross_half_lap_diagonal_drills(double shift = 0.5);

    static std::shared_ptr<JointPlate> cross_half_lap_drill(double shift = 0.5);

    static std::shared_ptr<JointPlate> cross_brussels_half_lap(double shift = 0.5);

    static std::shared_ptr<JointPlate>
    cross_custom(const std::vector<Polyline>& male, const std::vector<Polyline>& female);

    // ═══════════════════════════════════════════════════════════════════════════
    // b
    // ═══════════════════════════════════════════════════════════════════════════
    static std::shared_ptr<JointPlate> border();

    static std::shared_ptr<JointPlate>
    border_custom(const std::vector<Polyline>& male, const std::vector<Polyline>& female);

    // ═══════════════════════════════════════════════════════════════════════════
    // top_top
    // ═══════════════════════════════════════════════════════════════════════════
    // Top to top, contact 40, ids 40-49 as 2024 numbered them: 40 top_top_centre_pin (the family default id), 41 top_top_polylabel_pin, 42 top_top_circle_pins,
    // drills: a two-point line per hole from the contact, one plate thickness deep, twice on each face of each side, every copy of

    static std::shared_ptr<JointPlate> top_top_centre_pin(double radius = 1.0);

    static std::shared_ptr<JointPlate> top_top_polylabel_pin(double radius = 1.0);

    static std::shared_ptr<JointPlate>
    top_top_circle_pins(int divisions = 6, double shift = 0.95, double radius = 1.0);

    static std::shared_ptr<JointPlate>
    top_top_ring_pins(double division_length = 6.0, double shift = 0.95, double radius = 1.0);

    static std::shared_ptr<JointPlate>
    top_top_grid_pins(double division_length = 6.0, double shift = 0.95, double radius = 1.0);

    static std::shared_ptr<JointPlate>
    top_top_rectangle_pins(double division_length = 6.0, double shift = 0.95, double radius = 1.0);

    static std::shared_ptr<JointPlate>
    top_top_custom(const std::vector<Polyline>& male, const std::vector<Polyline>& female);

    // ═══════════════════════════════════════════════════════════════════════════
    // side_removal
    // ═══════════════════════════════════════════════════════════════════════════

    static std::shared_ptr<JointPlate>
    side_removal(bool merge_with_joint = false, double shift = 0.5);

    static std::shared_ptr<JointPlate> jupiter_scarf();

    // ═══════════════════════════════════════════════════════════════════════════
    // Constructors
    // ═══════════════════════════════════════════════════════════════════════════
    JointPlate();
    explicit JointPlate(const InteractionFeaturePlate& connection);
    JointPlate(int variant, double division_distance, double shift = 0.5);
    JointPlate(
        const std::shared_ptr<Plate>& source,
        const std::shared_ptr<Plate>& target,
        const InteractionContactFace& contact,
        int variant,
        double division_distance,
        double shift = 0.5,
        const Settings& settings = Settings()
    );
    static std::shared_ptr<JointPlate>
    side_to_top(int variant = 20, double division_distance = 450.0, double shift = 0.5);

    /// The joint id 2024's tables give a design by its name.
    static int library_id(const std::string& name);
    void orient(
        const std::shared_ptr<InteractionContactFace>& contact,
        const Settings& settings = Settings()
    );
    void orient(
        const std::shared_ptr<InteractionContactFace>& contact,
        const std::vector<std::shared_ptr<Plate>>& elements,
        const Settings& settings = Settings()
    );
    void orient(
        const std::shared_ptr<InteractionContactCross>& contact,
        const Settings& settings = Settings()
    );
    void orient(
        const std::shared_ptr<InteractionContactCross>& contact,
        const std::vector<std::shared_ptr<Plate>>& elements,
        const Settings& settings = Settings()
    );
    void construct(
        std::vector<InteractionFeaturePlate> connections,
        const std::vector<std::shared_ptr<Plate>>& elements,
        const Settings& settings = Settings()
    );
    void construct(
        InteractionFeaturePlate connection,
        const Settings& settings,
        std::vector<std::shared_ptr<Plate>> elements = {}
    );
    void construct(
        InteractionFeaturePlate connection,
        const std::function<void(InteractionFeaturePlate&)>& builder
    );
    std::shared_ptr<InteractionFeaturePlate>
    interaction_feature(int side, size_t connection = 0) const;

    /// The interaction this joint puts on its target i.
    std::shared_ptr<Interaction> interaction(size_t target) const override;

    /// How many interactions the joint puts on plates: both sides of every connection.
    size_t interaction_count() const;

    /// The plate interaction(i) acts on, by guid: side i % 2 of connection i / 2.
    const std::string& interaction_target(size_t target) const;
    static void build_geometry(
        std::vector<InteractionFeaturePlate>& connections,
        std::vector<std::shared_ptr<Plate>>& elements,
        const std::vector<std::vector<int>>& types,
        const Settings& settings
    );
    void place(const Xform& xform) override;
    std::vector<Line> drill_axes() const override;

    /// The solids one side of a connection takes out of its plate, in the joint's frame.
    static std::vector<Mesh> side_solids(const InteractionFeaturePlate& connection, int side);

    /// The drill lines of one side of a connection, a pair its builder repeats once.
    static std::vector<Line> side_drills(const InteractionFeaturePlate& connection, int side);

    /// Returns the loose key joining both pocket prisms through the fold.
    void compute_key(const std::vector<std::shared_ptr<Plate>>& elements);

    Mesh key_mesh() const override;

    std::string element_type_name() const override {
        return "JointPlate";
    }

    std::shared_ptr<Element> clone() const override {
        return std::make_shared<JointPlate>(*this);
    }

private:
    static std::shared_ptr<JointPlate> from_library(const std::string& library, int contact_type);
    static std::shared_ptr<JointPlate> side_side_inplane(const std::string& library);
    static std::shared_ptr<JointPlate> side_side_outplane(const std::string& library);
    static std::shared_ptr<JointPlate> side_side_rotated(const std::string& library);
    void compute_library(
        InteractionFeaturePlate& connection,
        const std::vector<std::shared_ptr<Plate>>& elements,
        std::vector<InteractionFeaturePlate>& connections,
        const Settings& settings
    ) const;
    void compute_parameters(
        InteractionFeaturePlate& connection,
        const std::vector<std::shared_ptr<Plate>>& elements,
        const Settings& settings
    ) const;
    bool compute_side_side_inplane(
        InteractionFeaturePlate& connection,
        const std::vector<std::shared_ptr<Plate>>& elements,
        const Settings& settings
    ) const;
    bool compute_side_side_outplane(
        InteractionFeaturePlate& connection,
        std::vector<InteractionFeaturePlate>& connections,
        const Settings& settings
    ) const;
    bool compute_top_side(InteractionFeaturePlate& connection, const Settings& settings) const;
    bool compute_side_side_rotated(
        InteractionFeaturePlate& connection,
        const std::vector<std::shared_ptr<Plate>>& elements,
        const Settings& settings
    ) const;
    bool compute_cross(InteractionFeaturePlate& connection, const Settings& settings) const;
    bool compute_b(InteractionFeaturePlate& connection, const Settings& settings) const;
    bool compute_top_top(
        InteractionFeaturePlate& connection,
        const std::vector<std::shared_ptr<Plate>>& elements,
        const Settings& settings
    ) const;
    bool compute_side_removal(
        InteractionFeaturePlate& connection, const std::vector<std::shared_ptr<Plate>>& elements
    ) const;

protected:
    Mesh
        key; // The loose key of a key design, what compute_key() cut from the plates; empty for every other design.
    std::vector<std::array<Polyline, 2>> bodies() const override;
    void write_proto(wood_proto::Joint& proto) const override;
    void read_proto(const wood_proto::Joint& proto) override;
};

/// Element that represents the joints of an Annen dataset: plate joints built from its plates, connections and groups.
class JointAnnen : public JointPlate {
public:
    JointAnnen() = default;
    JointAnnen(
        std::vector<std::shared_ptr<Plate>> elements,
        std::vector<InteractionFeaturePlate> connections,
        const std::vector<std::vector<int>>& groups,
        const Settings& settings = Settings()
    );

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
    JointVidy(
        std::vector<std::shared_ptr<Plate>> elements,
        std::vector<InteractionFeaturePlate> connections,
        const std::vector<std::vector<int>>& groups,
        const Settings& settings = Settings()
    );

    std::string element_type_name() const override {
        return "JointVidy";
    }

    std::shared_ptr<Element> clone() const override {
        return std::make_shared<JointVidy>(*this);
    }
};

}
