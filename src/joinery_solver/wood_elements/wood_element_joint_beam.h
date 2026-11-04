#pragma once

#include "wood_element_joint.h"
#include "wood_element_beam.h"
#include "wood_element_column.h"
#include "wood_element_plate.h"
#include "wood_interaction_feature_beam.h"
#include "wood_interaction_contact_axis.h"
#include "wood_interaction_feature_plate.h"
#include "wood_settings.h"

using namespace session_cpp;

namespace wood_session {

class InteractionContactFace;

/// Where pins stand on a contact: at its inset corners, in a column across its height, or in a row along it.
enum class PinLayout {
    corners,
    vertical,
    horizontal,
};

/// How two members meet, read from their ends, named by the faces that meet.
enum class ButtConfiguration {
    none,
    cross,
    top_side,
    side_side_outplane,
    side_side_inplane,
};

/// Element that represents a joint between beams: a beam-to-beam joint with feature volumes, or a connector with parts, cutters and pins.
class JointBeam : public Joint {
public:
    static inline const Color CONNECTOR_COLOR = Color(
        33.0f / 255.0f, 150.0f / 255.0f, 234.0f / 255.0f, 1.0f, "brg_blue"
    ); // A connector's node and every part and pin node under it: the Block Research Group's primary blue.
    static constexpr std::array<std::array<double, 2>, 3> WEDGE_PROFILE = {
        {{0.0, -197.0}, {-31.75593, 11.530606}, {31.75593, 11.530606}}
    }; // The wedge's cross-section across and below the contact's top edge: the apex, then the two top corners.

    InteractionFeatureBeam
        feature; // The beam-to-beam feature: the four volume rectangles of the male and female corners; empty for a connector.
    InteractionFeaturePlate joinery; // A beam-to-beam joint's plate joint.
    std::array<int, 2> joinery_sides = {
        -1, -1
    }; // Which side of the plate joint each target is, 0 its male and 1 its female; -1 without a joint.
    std::array<Mesh, 2>
        member_cuts; // What each target loses to a beam-to-beam joint inside the zone of both boxes.
    std::array<std::vector<Line>, 2>
        member_drills; // The plate joint's drills each target is bored along, its box's side of them; empty without any.
    std::vector<std::array<Polyline, 2>>
        parts; // A connector's own solids, each lofted between a bottom and a top loop; empty for a beam-to-beam joint.
    std::vector<std::vector<std::array<Polyline, 2>>>
        cutters; // A connector's cutters per target in targets order, lofted like parts; the drill lines cut every target too.
    double drill_overshoot =
        0.0; // How far a target's holes run past the pins at an end where the pin leaves the target.
    std::vector<InteractionFeatureSolid>
        solid_features; // Cuts into the connector's own parts, in the connector's frame like an element's.
    bool pre_drill =
        false; // A connector of pins: its drill lines are the pre-drilled holes of both targets, stored once here and never cut.
    std::vector<Vector>
        insertions; // A connector's direction each target slides off it, in targets order, then the way any keys of it come out; a beam-to-beam joint reads its feature instead.
    std::vector<Mesh>
        gains; // What a connector adds to each target, in targets order, before it cuts; empty for most. Hosted on the target by add_interaction, not written with the joint.
    std::array<InteractionFeatureSolid, 2> butt_extensions; // A butt's.
    std::array<InteractionFeatureSolid, 2> butt_solids; // A butt's.
    std::array<std::optional<Plane>, 2>
        butt_planes; // A butt's: each target's end plane, its normal out of the member; none for a target the butt leaves whole.
    std::array<int, 2> butt_ends = {-1, -1}; // A butt's.
    ButtConfiguration configuration =
        ButtConfiguration::none; // A butt's: how the two members meet, read from their ends.
    bool is_butt =
        false; // A butt: each target takes its butt_extensions and butt_solids entries, nothing else.

    JointBeam();

    /// A beam-to-beam joint from its feature.
    explicit JointBeam(const InteractionFeatureBeam& feature);

    /// A beam-to-beam joint whose feature the builder fills.
    JointBeam(
        InteractionFeatureBeam feature, const std::function<void(InteractionFeatureBeam&)>& builder
    );

    /// A beam-to-beam joint on the axis contact of two beams; throws when none fits.
    JointBeam(
        const Beam& source,
        const Beam& target,
        const InteractionContactAxis& contact,
        double volume_length,
        double cross_or_side_to_end,
        int flip_male = 0
    );

    // ═══════════════════════════════════════════════════════════════════════════
    // Static constructors
    // ═══════════════════════════════════════════════════════════════════════════

    /// A beam-to-beam joint on the axis contact of two beams, null when no volume fits.
    static std::shared_ptr<JointBeam> from_contact(
        const Beam& source,
        const Beam& target,
        const InteractionContactAxis& contact,
        double volume_length,
        double cross_or_side_to_end,
        int flip_male = 0,
        const Settings& settings = Settings()
    );

    /// The wedge connector on the face contact of two members: a profile prism with horizontal pins and a pocket in each member.
    static std::shared_ptr<JointBeam> wedge(
        const Element& a,
        const Element& b,
        const InteractionContactFace& contact,
        double length_margin,
        double pocket_depth,
        const std::optional<Plane>& end = std::nullopt,
        double pin_radius = 10.0,
        double pin_spacing = 320.0,
        int pin_sides = 8,
        double overshoot = 20.0,
        const std::array<std::array<double, 2>, 3>& profile = WEDGE_PROFILE,
        const std::array<double, 2>& pin_offset = {80.0, -100.0}
    );

    /// A plate on the face contact of a column and a rib.
    static std::shared_ptr<Plate> let_in_plate(
        const Element& rib,
        const InteractionContactFace& contact,
        double width = 30.0,
        double back = 220.0,
        double front = 265.0,
        double height = 250.0
    );

    /// The joint that lets a plate into a column and a rib.
    static std::shared_ptr<JointBeam> rectangle_plate(
        const Element& column,
        const Element& rib,
        const Plate& plate,
        const InteractionContactFace& contact,
        double pin_radius = 25.0,
        double margin_x = 6.05,
        double margin_z = 3.0,
        double overshoot = 25.0,
        int pin_sides = 16
    );

    /// A butt of two members drawn as lines and polygons, beams, columns or plates.
    static std::shared_ptr<JointBeam> butt(const Element& a, const Element& b, double reach = 0.0);

    /// What a plate's edge `end` gains and loses to stop on its plane in `planes`.
    static std::array<Mesh, 2> plate_end(
        const Plate& plate, size_t end, const std::map<size_t, Plane>& planes, double overshoot
    );

    /// A butt whose corner, square or oblique, is side_side_outplane.
    static std::shared_ptr<JointBeam>
    butt_side_side(const Element& a, const Element& b, double reach = 0.0);

    /// A butt top_side whose second member's side.
    static std::shared_ptr<JointBeam>
    butt_top_side(const Element& top, const Element& side, double reach = 0.0);

    /// A butt top_side at a corner of any angle, square or oblique.
    static std::shared_ptr<JointBeam>
    butt_top_side_end(const Element& top, const Element& side, double reach = 0.0);

    /// A butt cross whatever the ends.
    static std::shared_ptr<JointBeam> butt_cross(const Element& a, const Element& b);

    /// A butt cross on a given lap plane.
    static std::shared_ptr<JointBeam>
    butt_cross(const Element& a, const Element& b, const Plane& lap);

    /// A butterfly key across the end-to-end contact of two members: two heads and a neck let into a pocket in each.
    static std::shared_ptr<JointBeam> butterfly_key(
        const Element& a,
        const Element& b,
        const InteractionContactFace& contact,
        double top = 138.5,
        double length = 800.0,
        double head_length = 200.0,
        double head_width = 40.0,
        double neck_width = 20.0,
        double depth = 58.5,
        double end_depth = 58.5 + 302.0 / 25.4,
        double pocket_depth = 80.0,
        double overshoot = 10.0
    );

    /// One round column and four beams, given in any order, into one node: the two beams in line meet in a scarf of two tongues in a slot across the column's head, each wider at its tip, and
    /// the other two, one each side at any angle, end on those tongues in one dovetail each, all walls along the column, so each beam drops in along it. Everything is read from the members:
    /// each beam's near end, side and direction, its section, the seat's level. A beam's tenon, tail and shoulder follow its own direction, so the posts come out unequal; a beam too long is
    /// cut back and one too short gains stock. Sizes are shares of a beam's own width, the shoulder of the column's radius. Throws naming the beam and the reason when no valid joint exists.
    static std::shared_ptr<JointBeam> column_head(
        const Column& column,
        const Beam& a,
        const Beam& b,
        const Beam& c,
        const Beam& d,
        double tenon = 0.7,
        double shoulder = 0.8,
        double seam_slope = 0.1,
        double dovetail_offset = 0.145,
        double dovetail_root = 0.286,
        double dovetail_depth = 0.146,
        double dovetail_flare = 0.176,
        double min_wood = 0.08,
        double min_post = 0.05
    );

    /// Two plates held face to face by a box through a mortise in both, locked by two wedge keys in one long mortise across the box: the first key stops the box on the second plate, the
    /// second, driven in once the plates meet, pinches them together; its parts are the box and the first and second keys, its last two insertions the way the keys come out. Sizes are
    /// shares of the first plate's thickness; key_width is a key's width where it enters the box, key_taper its loss per unit of depth; null when the sizes leave nothing, the keys do not taper or the box leaves the plates.
    static std::shared_ptr<JointBeam> keyed_box(
        const Plate& a,
        const Plate& b,
        const InteractionContactFace& contact,
        double box_width = 2.4,
        double box_depth = 1.6,
        double mortise = 0.6,
        double key_width = 1.5,
        double key_taper = 0.125,
        double key_proud = 0.5,
        double head = 1.2
    );

    /// Pins centred across the face contact at its inset corners, bored into both members; null when the inset leaves nothing.
    static std::shared_ptr<JointBeam> centred_pins(
        const Element& a,
        const Element& b,
        const InteractionContactFace& contact,
        double radius = 4.0,
        double length = 30.0,
        double offset = 50.0,
        double overshoot = 10.0,
        int pin_sides = 16
    );

    /// Headed pins laid out on the face contact of a and b, pre-drilled into both.
    static std::shared_ptr<JointBeam> headed_pins(
        const Element& a,
        const Element& b,
        const InteractionContactFace& contact,
        PinLayout layout,
        size_t count = 2,
        double offset = 20.0,
        double shift = 0.0,
        double radius = 2.0,
        double length = 200.0,
        int sides = 16
    );

    // ═══════════════════════════════════════════════════════════════════════════
    // Geometry
    // ═══════════════════════════════════════════════════════════════════════════

    /// Whether this is a connector, with parts, cutters or pre-drilled pins of its own, rather than a beam-to-beam joint.
    bool is_connector() const;

    /// The direction target i slides off the joint and the other members, unit.
    Vector insertion(size_t target) const;

    /// The interaction this joint puts on its target i: a connector's cutters and bores, pre-drilled holes, or feature volumes.
    std::shared_ptr<Interaction> interaction(size_t target) const override;

    /// One part as a closed mesh, its loops lofted, before any cut.
    Mesh part_mesh(size_t index) const;

    /// The cuts into one part: the connector's stored cuts and the bores of its own pins passing through the part.
    std::vector<InteractionFeatureSolid> part_features(size_t index) const;

    /// One part as a BRep with its cuts applied and its pin bores exact.
    BRep part_brep(size_t index) const;

    /// The connector's parts and pins as ConnectorPart and Pin elements to nest under it.
    std::vector<std::shared_ptr<Joint>> children() const;

    /// A beam-to-beam joint's feature volumes; a connector draws nothing itself, its children carry its parts and pins.
    const Mesh& element_geometry_mesh() const override;

    /// A beam-to-beam joint's feature volumes; a connector draws nothing itself, its children carry its parts and pins.
    const BRep& element_geometry_brep() const override;

    std::string element_type_name() const override {
        return "JointBeam";
    }

    std::shared_ptr<Element> clone() const override {
        return std::make_shared<JointBeam>(*this);
    }

    // ═══════════════════════════════════════════════════════════════════════════
    // Transformation
    // ═══════════════════════════════════════════════════════════════════════════

    /// Moves the joint, its feature volumes, parts, cutters and cuts.
    void place(const Xform& xform) override;

    // ═══════════════════════════════════════════════════════════════════════════
    // Protobuf
    // ═══════════════════════════════════════════════════════════════════════════
protected:
    std::vector<std::array<Polyline, 2>> bodies() const override;
    void write_proto(wood_proto::Joint& proto) const override;
    void read_proto(const wood_proto::Joint& proto) override;
};

}
