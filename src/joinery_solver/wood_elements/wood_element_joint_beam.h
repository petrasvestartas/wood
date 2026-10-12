#pragma once

#include "wood_element_joint.h"
#include "wood_element_beam.h"
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
    corners, // The inset contact's extreme corners.
    vertical, // A column of count pins across the contact's height.
    horizontal, // A row of count pins along the contact.
};

/// How two members meet, read from their ends, named by the faces that meet: the case a butt makes, and the joint family that fits it.
enum class ButtConfiguration {
    none, // They do not touch.
    cross, // They cross, no end reaching the other: a cross joint, nothing butted.
    top_side, // One member's side, its end, on the other's top, its long face: at the other's middle, or at its end where the first runs on.
    side_side_outplane, // Both ends, out of plane, cut on the plane halfway: a miter.
    side_side_inplane, // Both ends, in line or in plane, cut on the plane halfway: a splice.
};

/// Element that represents a joint between beams: a beam-to-beam joint with feature volumes, or a connector with parts, cutters and pins.
class JointBeam : public Joint {
public:
    static inline const Color CONNECTOR_COLOR = Color(33.0f / 255.0f, 150.0f / 255.0f, 234.0f / 255.0f, 1.0f, "brg_blue"); // A connector's node and every part and pin node under it: the Block Research Group's primary blue.
    static constexpr std::array<std::array<double, 2>, 3> WEDGE_PROFILE = {{{0.0, -197.0}, {-31.75593, 11.530606}, {31.75593, 11.530606}}}; // The wedge's cross-section across and below the contact's top edge: the apex, then the two top corners.

    InteractionFeatureBeam feature; // The beam-to-beam feature: the four volume rectangles of the male and female corners; empty for a connector.
    InteractionFeaturePlate joinery; // A beam-to-beam joint's plate joint, as 2024 built it on the two boxes its volumes make: [0] and [1] on the first beam's box, [2] and [3] on the second's; empty when the boxes take no joint.
    std::array<int, 2> joinery_sides = {-1, -1}; // Which side of the plate joint each target is, 0 its male and 1 its female; -1 without a joint.
    std::array<Mesh, 2> member_cuts; // What each target loses to a beam-to-beam joint inside the zone of both boxes: all but what its box keeps of the plate joint; empty without one.
    std::array<std::vector<Line>, 2> member_drills; // The plate joint's drills each target is bored along, its box's side of them; empty without any.
    std::vector<std::array<Polyline, 2>> parts; // A connector's own solids, each lofted between a bottom and a top loop; empty for a beam-to-beam joint.
    std::vector<std::vector<std::array<Polyline, 2>>> cutters; // A connector's cutters per target in targets order, lofted like parts; the drill lines cut every target too.
    double drill_overshoot = 0.0; // How far a target's holes run past the pins at an end where the pin leaves the target; a blind hole stops at its pin.
    std::vector<InteractionFeatureSolid> solid_features; // Cuts into the connector's own parts, in the connector's frame like an element's.
    bool pre_drill = false; // A connector of pins: its drill lines are the pre-drilled holes of both targets, stored once here and never cut.
    std::vector<Vector> insertions; // A connector's direction each target slides off it, in targets order; a beam-to-beam joint reads its feature instead.
    std::array<InteractionFeatureSolid, 2> butt_extensions; // A butt's: what each target's end gains, run on past its end plane; empty for a target the butt leaves whole.
    std::array<InteractionFeatureSolid, 2> butt_solids; // A butt's: what each target loses beyond its end plane, so the run-on stops on it; empty for a target the butt leaves whole.
    std::array<std::optional<Plane>, 2> butt_planes; // A butt's: each target's end plane, its normal out of the member; none for a target the butt leaves whole.
    std::array<int, 2> butt_ends = {-1, -1}; // A butt's: which end of each target it stops, a beam's 0 or 1, a plate's edge; -1 for a target it leaves whole.
    ButtConfiguration configuration = ButtConfiguration::none; // A butt's: how the two members meet, read from their ends.
    bool is_butt = false; // A butt: each target takes its butt_extensions and butt_solids entries, nothing else.

    JointBeam();

    /// A beam-to-beam joint from its feature.
    explicit JointBeam(const InteractionFeatureBeam& feature);

    /// A beam-to-beam joint whose feature the builder fills.
    JointBeam(InteractionFeatureBeam feature, const std::function<void(InteractionFeatureBeam&)>& builder);

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

    /// A beam-to-beam joint on the axis contact of two beams, null when no volume fits: its two volumes made boxes and the plate joint the
    /// settings' rows give them, as 2024 built beam joints, each beam then cut to the shape its box takes inside the volume.
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

    /// A plate on the face contact of a column and a rib: width thick, back into the column, front into the rib, height down from the contact's top edge.
    static std::shared_ptr<Plate> let_in_plate(
        const Element& rib,
        const InteractionContactFace& contact,
        double width = 30.0,
        double back = 220.0,
        double front = 265.0,
        double height = 250.0
    );

    /// The joint that lets a plate into a column and a rib: a pocket in both and four pins through all three, each pin across its member face to face.
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

    /// A butt of two members drawn as lines and polygons, beams, columns or plates: each member end that reaches the other, an end face within
    /// `reach` of it short or past, is run on and cut on one plane, so the two meet face to face. top_side: one end on the other's face, or a
    /// corner square within 10 degrees, a running on to b's outer face and b stopping on a's inner face. side_side_outplane: an oblique corner,
    /// both cut on the miter. side_side_inplane: both on the plane halfway. cross, no end reaching: nothing cut. `reach` 0 takes the larger
    /// member depth. The drawn, uncut members decide, so the order butts are added in does not matter; nullptr when they do not touch.
    static std::shared_ptr<JointBeam> butt(const Element& a, const Element& b, double reach = 0.0);

    /// What a plate's edge `end` gains and loses to stop on its plane in `planes`, the planes of all its butted edges by edge: the run-on out to
    /// the plane and `overshoot` past it, between its neighbours' edges run out to theirs too, so two run-on edges fill their corner, empty
    /// when the edge runs in; and the wood beyond the plane, at a concave corner only what lies beyond the butted neighbour's plane too, so a
    /// valley keeps what lies under the neighbour. The session calls it on every butt of a plate, so all its edges agree.
    static std::array<Mesh, 2> plate_end(const Plate& plate, size_t end, const std::map<size_t, Plane>& planes, double overshoot);

    /// A butt whose corner, square or oblique, is side_side_outplane, a miter: both ends cut on the plane through the outer and the inner
    /// meeting edges.
    static std::shared_ptr<JointBeam> butt_side_side(const Element& a, const Element& b, double reach = 0.0);

    /// A butt top_side whose second member's side, its end, stops on the first member's top, the first left whole even where its own end lies
    /// at the same node, as a brace on a post that stands on the sill beside it.
    static std::shared_ptr<JointBeam> butt_top_side(const Element& top, const Element& side, double reach = 0.0);

    /// A butt top_side at a corner of any angle, square or oblique: the first member runs on to the second's outer face and the second's
    /// end stops on the first, as a chord over an end post; a tee where only the second's end reaches.
    static std::shared_ptr<JointBeam> butt_top_side_end(const Element& top, const Element& side, double reach = 0.0);

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

    /// Headed pins laid out on the face contact of a and b, pre-drilled into both: level along the axis of the member that ends on the contact, from the far face of the other.
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

    /// The direction target i slides off the joint and the other members, unit: a crossing along the normal of the two axes, away from the
    /// other beam's box; a side-to-end or end-to-end pair along the ending beam's axis; a connector as its factory recorded it.
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
