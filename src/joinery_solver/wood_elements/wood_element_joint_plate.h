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
    // side_side_inplane
    // ═══════════════════════════════════════════════════════════════════════════

    // Side-to-side in plane, contact 12, ids 1-9 as 2024 numbered them: 1 side_side_inplane_dovetails (the family default), 2 side_side_inplane_three_fingers,
    // 3 side_side_inplane_butterfly_keys, 4 side_side_inplane_splayed_scarf, 5 side_side_inplane_cross_splayed_scarf, 6 side_side_inplane_tooth_keys, 7 jupiter_scarf (not 2024's), 8 side_removal, 9 side_side_inplane_custom. A zero division count takes
    // the geometric count, the joint line's length over the family's 300 mm; the shift default is the family's 0.5.

    static std::shared_ptr<JointPlate> side_side_inplane_three_fingers();

    static std::shared_ptr<JointPlate> side_side_inplane_dovetails(int divisions = 0, double shift = 0.5);

    static std::shared_ptr<JointPlate> side_side_inplane_butterfly_keys(int divisions = 0);

    static std::shared_ptr<JointPlate> side_side_inplane_splayed_scarf();

    static std::shared_ptr<JointPlate> side_side_inplane_cross_splayed_scarf();

    static std::shared_ptr<JointPlate> side_side_inplane_tooth_keys(int divisions = 0);

    static std::shared_ptr<JointPlate> side_side_inplane_custom(const std::vector<Polyline>& male, const std::vector<Polyline>& female);

    // ═══════════════════════════════════════════════════════════════════════════
    // side_side_outplane
    // ═══════════════════════════════════════════════════════════════════════════

    // Side-to-side out of plane, contact 11, ids 10-19 as 2024 numbered them: 10 side_side_outplane_fingers, 11 side_side_outplane_graded_fingers, 12 side_side_outplane_three_fingers,
    // 13 side_side_outplane_miter_tenon, 14 side_side_outplane_tenons, 15 side_side_outplane_linked_tenons (the family default id), 16 side_side_outplane_linked_tenons_wall, 18 side_removal, 19 side_side_outplane_custom;
    // an id without an entry, such as 17, takes side_side_outplane_fingers. A zero division count takes the geometric count, the joint
    // line's length over the family's 450 mm; the shift default is the family's 0.64. side_side_outplane_capped_fingers and side_side_outplane_notch
    // are designs of this port, reached by name only.

    static std::shared_ptr<JointPlate> side_side_outplane_three_fingers();

    static std::shared_ptr<JointPlate> side_side_outplane_fingers(int divisions = 0, double shift = 0.64);

    static std::shared_ptr<JointPlate> side_side_outplane_graded_fingers(int divisions = 0, double shift = 0.64);

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

    static std::shared_ptr<JointPlate> side_side_outplane_linked_tenons(int divisions = 0, bool disable_divisions = false);

    static std::shared_ptr<JointPlate> side_side_outplane_linked_tenons_wall(int divisions = 0);

    static std::shared_ptr<JointPlate> side_side_outplane_capped_fingers(int divisions = 4);

    static std::shared_ptr<JointPlate> side_side_outplane_notch();

    static std::shared_ptr<JointPlate> side_side_outplane_custom(const std::vector<Polyline>& male, const std::vector<Polyline>& female);

    // ═══════════════════════════════════════════════════════════════════════════
    // top_side
    // ═══════════════════════════════════════════════════════════════════════════

    // Top to side, contact 20, ids 20-29 as 2024 numbered them: 20 top_side_tenons (the family default id), 21 top_side_dense_tenons, 22 top_side_tenons,
    // 23 and 24 top_side_milled_wedge (2024's case 23 ran top_side_three_tenons and fell through into top_side_milled_wedge), 25 top_side_snap_fit, 28 side_removal,
    // 29 top_side_custom; an id without an entry takes top_side_tenons; top_side_three_tenons and top_side_annen_tenons are reached by name only. A zero division
    // count takes the geometric count, the joint line's length over the family's 450 mm; the shift default is the family's 0.5.
    // The unit box puts the upright's thickness along x, the base's along y and the joint line along z: the male is the upright,
    // its tenons merged into its outline, the female the base, its mortises holes through it, the last female outline the
    // rectangle that bounds them. top_side_snap_fit is unit scale: oriented, its unit z keeps the upright's thickness, the distance 2024
    // pinned every joint to, the thickness of its first plate times scale[2].

    static std::shared_ptr<JointPlate> top_side_three_tenons();

    static std::shared_ptr<JointPlate> top_side_annen_tenons();

    static std::shared_ptr<JointPlate> top_side_dense_tenons(int divisions = 0, double shift = 0.5);

    static std::shared_ptr<JointPlate> top_side_tenons(int divisions = 0, double shift = 0.5);

    static std::shared_ptr<JointPlate> top_side_milled_wedge();

    static std::shared_ptr<JointPlate> top_side_snap_fit(int divisions = 0);

    static std::shared_ptr<JointPlate> top_side_custom(const std::vector<Polyline>& male, const std::vector<Polyline>& female);

    // ═══════════════════════════════════════════════════════════════════════════
    // side_side_rotated
    // ═══════════════════════════════════════════════════════════════════════════

    // Side-to-side rotated, contact 13, ids 50-59 as 2024 numbered them: 54 side_side_rotated_keys, 55 side_side_rotated_hooks, 56 side_side_rotated_miter,
    // 57 side_removal, 58 side_removal merged with the joint (the family default id), 59 side_side_rotated_custom; an id without
    // an entry takes side_removal. A zero division count takes the geometric count, the joint line's length over the
    // family's 300 mm; the shift default is the family's 0.5. side_side_rotated_hooks and side_side_rotated_keys are key designs: each plate is
    // milled a pocket on its side of the seam and the joint owns the loose key that fills both.

    static std::shared_ptr<JointPlate> side_side_rotated_miter();

    static std::shared_ptr<JointPlate> side_side_rotated_hooks(int divisions = 0, double shift = 0.5);

    static std::shared_ptr<JointPlate> side_side_rotated_keys(int divisions = 0, double shift = 0.5);

    static std::shared_ptr<JointPlate> side_side_rotated_custom(const std::vector<Polyline>& male, const std::vector<Polyline>& female);

    /// The Hilti connector of the in-plane family (side_side_inplane) across the straight seam of two CLT slabs side to side, flat or folded a little:
    /// the 2024 Hilti joint (side_side_rotated_hooks, id 55) with its
    /// key. Each slab gets its half of the bow-tie pocket milled from the top: the neck across the seam widening into the wing, straight
    /// to its end, the far corners rounded by the router, depth under the seam's top edge. The key is a straight bow-tie of two plywood
    /// halves with a steel disc recessed in each end and the bolt from disc to disc, inside the pockets: the same solids at
    /// every fold, so a folded pair, its tops falling away from the ridge, leaves the wings standing out. The defaults are the product's
    /// (240 x 90 x 93 mm cutout, a 27.7 x 40 neck, a 40 mm router) on CLT of at least 120 mm. Null when the contact is not side to side
    /// or has no straight seam.
    static std::shared_ptr<JointPlate> side_side_inplane_hilti(
        const Plate& a,
        const Plate& b,
        const InteractionContactFace& contact,
        double half_length = 120.0,     // the seam to the pocket's end, half the 240 mm cutout
        double neck_length = 27.7,      // the neck across the seam, half in each pocket
        double neck_width = 40.0,       // the neck
        double taper_end = 74.4,        // the seam to where the wing reaches its width
        double wing_width = 90.0,       // the wing, the cutout's width
        double depth = 93.0,            // the pocket, from the top down
        double height = 90.0,           // the key, its top depth - height under the top
        double rod_diameter = 12.0,     // the bolt
        double disc_diameter = 50.0,    // the steel disc under each nut, recessed in its wing's end
        double disc_thickness = 6.0,
        double router_radius = 20.0,    // the far corners of each pocket, a 40 mm router
        int sides = 32                  // the discs', the rounded corners' and the bolt's polygon
    );

    // ═══════════════════════════════════════════════════════════════════════════
    // cross
    // ═══════════════════════════════════════════════════════════════════════════

    // Cross in-plane, contact 30, ids 30-39 as 2024 numbered them: 30 cross_slot (the family default id), 31 cross_sliced_half_lap,
    // 32 cross_half_lap, 33 cross_half_lap_diagonal_drills, 34 cross_half_lap_drill, 35 cross_brussels_half_lap, 38 side_removal, 39 cross_custom; an id without an entry
    // takes cross_slot. The family has no divisions; the shift default is the family's 0.5, and it sets the width of the
    // half-lap's centre square in cross_sliced_half_lap to cross_brussels_half_lap: 0 the widest, 1 the narrowest. The unit box puts the first
    // plate's thickness along x, the second's along y and the cross's depth along z: cross_slot is the plain slot, a
    // rectangle merged into each plate's outline over half its depth; cross_sliced_half_lap to cross_brussels_half_lap are the milled half-lap,
    // every outline a solid taken from the plate, with 2024's drills: two diagonal in cross_half_lap_diagonal_drills, one vertical in
    // cross_half_lap_drill, a vertical 50 mm and a horizontal 10 mm bit in cross_brussels_half_lap, whose bottom sides are extended 0.27 on one
    // segment and shortened 0.075 on the other as 2024 built the Brussels sports tower.


    static std::shared_ptr<JointPlate> cross_slot();
    static std::shared_ptr<JointPlate> cross_sliced_half_lap(double shift = 0.5);

    static std::shared_ptr<JointPlate> cross_half_lap(double shift = 0.5);

    static std::shared_ptr<JointPlate> cross_half_lap_diagonal_drills(double shift = 0.5);

    static std::shared_ptr<JointPlate> cross_half_lap_drill(double shift = 0.5);

    static std::shared_ptr<JointPlate> cross_brussels_half_lap(double shift = 0.5);

    static std::shared_ptr<JointPlate> cross_custom(const std::vector<Polyline>& male, const std::vector<Polyline>& female);

    // ═══════════════════════════════════════════════════════════════════════════
    // b
    // ═══════════════════════════════════════════════════════════════════════════

    static std::shared_ptr<JointPlate> border();

    static std::shared_ptr<JointPlate> border_custom(const std::vector<Polyline>& male, const std::vector<Polyline>& female);

    // ═══════════════════════════════════════════════════════════════════════════
    // top_top
    // ═══════════════════════════════════════════════════════════════════════════

    // Top to top, contact 40, ids 40-49 as 2024 numbered them: 40 top_top_centre_pin (the family default id), 41 top_top_polylabel_pin, 42 top_top_circle_pins,
    // 43 top_top_ring_pins, 44 top_top_grid_pins, 45 top_top_rectangle_pins, 48 side_removal, 49 top_top_custom; an id without an entry takes top_top_centre_pin. Every design
    // drills: a two-point line per hole from the contact, one plate thickness deep, twice on each face of each side, every copy of
    // type drill, in world space (no orient); the family's 2024 defaults are division length 6 and shift 0.95, and each design reads
    // them its own way. top_top_centre_pin drills the centre of the contact, top_top_polylabel_pin its polylabel, the centre of its largest inscribed circle;
    // top_top_circle_pins puts `divisions` holes on that circle with its radius scaled by the shift, from 45 degrees on along the edge nearest
    // the centre; top_top_ring_pins offsets the contact inward by the shift (millimetres) and drills its ring every division length; top_top_grid_pins
    // fills that offset ring with a lattice of the division length, from its first corner on a rectangle, about the centre of its
    // bounding rectangle otherwise; top_top_rectangle_pins inscribes 2024's largest empty rectangle in the contact, inset by 1 - shift of its
    // shorter extent, and drills its edges every division length, or fills it with a grid of that step when the length is negative.
    // `radius` is the pin's, a property of the hole the port bores and no 2024 parameter. Each side records the line through the
    // other plate, as 2024 wrote it and the 2025 reference lists it; the session bores each plate with the other side's lines.

    static std::shared_ptr<JointPlate> top_top_centre_pin(double radius = 1.0);

    static std::shared_ptr<JointPlate> top_top_polylabel_pin(double radius = 1.0);

    static std::shared_ptr<JointPlate> top_top_circle_pins(int divisions = 6, double shift = 0.95, double radius = 1.0);

    static std::shared_ptr<JointPlate> top_top_ring_pins(double division_length = 6.0, double shift = 0.95, double radius = 1.0);

    static std::shared_ptr<JointPlate> top_top_grid_pins(double division_length = 6.0, double shift = 0.95, double radius = 1.0);

    static std::shared_ptr<JointPlate> top_top_rectangle_pins(double division_length = 6.0, double shift = 0.95, double radius = 1.0);

    static std::shared_ptr<JointPlate> top_top_custom(const std::vector<Polyline>& male, const std::vector<Polyline>& female);

    // ═══════════════════════════════════════════════════════════════════════════
    // side_removal
    // ═══════════════════════════════════════════════════════════════════════════

    // The side faces of a pair milled off by the joint scale, on every side-side, side-top and cross family (ids x8).

    static std::shared_ptr<JointPlate> side_removal(bool merge_with_joint = false, double shift = 0.5);

    // The trait de Jupiter of two plates in line (contact 12): a scarf through the thickness, an about at each end of the joint
    // line, two slopes and the redan between them, where a key drives through a slot both plates are notched around; the key
    // is the joint's own. 2024's jupiter_scarf, the side removal with the rotated_1 arc tile, removed no side and was
    // reached by no table, so the name carries this design.

    static std::shared_ptr<JointPlate> jupiter_scarf();

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

    /// The joint id 2024's tables give a design by its name, the id a *_joints_types.txt sidecar or a point's type holds: "side_side_inplane_dovetails" 1,
    /// "top_side_tenons" 20, "cross_slot" 30, "side_side_rotated_hooks" 55, "border" 60, ...; a side removal by its family, "side_side_inplane/side_removal" 8 to
    /// "side_side_rotated/side_removal_merged" 58; "" 0, no joint. Throws for an unknown name and for a design 2024 reaches by name only (top_side_three_tenons,
    /// top_side_annen_tenons, side_side_outplane_capped_fingers, side_side_outplane_notch).
    static int library_id(const std::string& name);
    void orient(const std::shared_ptr<InteractionContactFace>& contact, const Settings& settings = Settings());
    void orient(const std::shared_ptr<InteractionContactFace>& contact, const std::vector<std::shared_ptr<Plate>>& elements, const Settings& settings = Settings());
    void orient(const std::shared_ptr<InteractionContactCross>& contact, const Settings& settings = Settings());
    void orient(const std::shared_ptr<InteractionContactCross>& contact, const std::vector<std::shared_ptr<Plate>>& elements, const Settings& settings = Settings());
    void construct(std::vector<InteractionFeaturePlate> connections, const std::vector<std::shared_ptr<Plate>>& elements, const Settings& settings = Settings());
    void construct(InteractionFeaturePlate connection, const Settings& settings,
                   std::vector<std::shared_ptr<Plate>> elements = {});
    void construct(InteractionFeaturePlate connection, const std::function<void(InteractionFeaturePlate&)>& builder);
    std::shared_ptr<InteractionFeaturePlate> interaction_feature(int side, size_t connection = 0) const;

    /// The interaction this joint puts on its target i: side i % 2 of connection i / 2, so a one-pair joint's targets are its male and its female
    /// plate and a joint of grouped connections (Annen, Vidy) lists both sides of every connection in order.
    std::shared_ptr<Interaction> interaction(size_t target) const override;

    /// How many interactions the joint puts on plates: both sides of every connection.
    size_t interaction_count() const;

    /// The plate interaction(i) acts on, by guid: side i % 2 of connection i / 2.
    const std::string& interaction_target(size_t target) const;
    static void build_geometry(std::vector<InteractionFeaturePlate>& connections,
                               std::vector<std::shared_ptr<Plate>>& elements,
                               const std::vector<std::vector<int>>& types, const Settings& settings);
    void place(const Xform& xform) override;
    std::vector<Line> drill_axes() const override;

    /// The solids one side of a connection takes out of its plate, in the joint's frame: every outline pair of a solid type (slice, mill,
    /// cut, conic) lofted, a pair its builder repeats once, a loft Manifold does not take as a solid left out.
    static std::vector<Mesh> side_solids(const InteractionFeaturePlate& connection, int side);

    /// The drill lines of one side of a connection, a pair its builder repeats once.
    static std::vector<Line> side_drills(const InteractionFeaturePlate& connection, int side);

    /// The loose key of side_side_rotated_hooks, side_side_rotated_keys and top_top_custom, both sides' pocket prisms united as 2024 milled them, straight through the fold; of side_side_inplane_splayed_scarf
    /// and side_side_inplane_cross_splayed_scarf, what each plate loses to its side's solids united with the notches of the in-plane runs; empty for every other design. The
    /// constructors and the solver call it once the connections are built, with the plates they join.
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
    void compute_parameters(InteractionFeaturePlate& connection, const std::vector<std::shared_ptr<Plate>>& elements, const Settings& settings) const;
    bool compute_side_side_inplane(InteractionFeaturePlate& connection, const std::vector<std::shared_ptr<Plate>>& elements, const Settings& settings) const;
    bool compute_side_side_outplane(InteractionFeaturePlate& connection, std::vector<InteractionFeaturePlate>& connections, const Settings& settings) const;
    bool compute_top_side(InteractionFeaturePlate& connection, const Settings& settings) const;
    bool compute_side_side_rotated(InteractionFeaturePlate& connection, const std::vector<std::shared_ptr<Plate>>& elements, const Settings& settings) const;
    bool compute_cross(InteractionFeaturePlate& connection, const Settings& settings) const;
    bool compute_b(InteractionFeaturePlate& connection, const Settings& settings) const;
    bool compute_top_top(InteractionFeaturePlate& connection, const std::vector<std::shared_ptr<Plate>>& elements, const Settings& settings) const;
    bool compute_side_removal(InteractionFeaturePlate& connection, const std::vector<std::shared_ptr<Plate>>& elements) const;

protected:
    Mesh key; // The loose key of a key design, what compute_key() cut from the plates; empty for every other design.
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
