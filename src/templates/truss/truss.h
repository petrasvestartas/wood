#pragma once
#include "src/templates/truss/truss_guide.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_truss {

// ═══════════════════════════════════════════════════════════════════════════
// Truss
// ═══════════════════════════════════════════════════════════════════════════

/// The truss model.
/// What a member ending on another does there, which picks its joint.
enum class JointRole {
    heel, // A sloped chord on the level tie.
    flat_heel, // The same under 30 degrees.
    oblique_push, // A strut bearing at an angle.
    square_push, // A post bearing square.
    hanging_post, // A post that pulls.
    pulling, // Any other member that pulls.
};

class Truss : public WoodSession {
public:
    static constexpr double NODE_TOLERANCE =
        1.0; // mm two ends lie apart and still meet at one node.
    static constexpr double RUN_ON =
        3.0; // Half sections a carrier ending at a node runs on past it (the tie past a heel's notch, the post above the rafters).
    inline static const std::map<JointRole, std::vector<std::string>> ROLE_DESIGNS = {
        {JointRole::heel, {"oblique_thrust_notched_heel", "double_notched_oblique_thrust"}},
        {JointRole::flat_heel, {"double_notched_oblique_thrust", "oblique_thrust_notched_heel"}},
        {JointRole::oblique_push, {"notched_oblique_thrust", "notched_oblique_thrust_right_angled_end", "notched_oblique_thrust_visible_end"}},
        {JointRole::square_push, {"stopped_mortise_tenon", "cogged_lap"}},
        {JointRole::hanging_post, {"wedged_dovetail_half_lap", "stopped_dovetail_half_lap", "housed_dovetail_half_lap", "stopped_housed_dovetail_half_lap"}},
        {JointRole::pulling, {"stopped_dovetail_half_lap", "dovetail_half_lap", "stopped_housed_dovetail_half_lap", "housed_dovetail_half_lap"}},
    }; // Each role's designs, taken in turn (heel pp. 590, 592; push pp. 584, 586, 588, 436, 428; pull pp. 570, 442, 440, 450, 448).
    static constexpr double MITER_ANGLE = 35.0; // Degrees two ends at a plate node meet at, at least, to be mitered; sharper, one bears on the other.
    static constexpr double PLATE_CLEAR_ANGLE = 25.0; // Degrees a node plate's way in keeps from every member of its node.
    static constexpr double PIN_RADIUS = 6.0; // mm, a node plate's pins.
    static constexpr double PLATE_INSET = 15.0; // mm, a node plate kept in from its members' edges.
    const TrussGuide guide; // The lines the truss is drawn with.

    /// The truss: the guide's lines, the members on them, their butts so every end meets, then the book's joinery in place of
    /// every plain butt of an end on a side.
    explicit Truss(const TrussGuide& guide, const std::string& name = "truss");

private:
    /// The guide's chords and webs, red, in input.
    void add_input();

    /// A beam per chord run, chord_<c>_<s>, in chords.
    void add_chords();

    /// A beam per web, web_<w>, in webs.
    void add_webs();

    /// Runs side_side at every kink; at heels the case read by JointBeamElement::butt; a chord's end on another's run top_side.
    void add_chord_butts();

    /// Each web's ends on the runs and on other webs, chord ends on webs.
    void add_web_butts();

    /// The book's joinery at every node, no butt left: each member ending at a node jointed into the member that runs through
    /// it (at a heel the tie, at an apex the post, each running on past the node), a later end bearing on the side of an earlier
    /// one (steepest first), by the node and by whether the member pulls (TrussGuide::bars): a rafter on the tie the oblique
    /// thrust with notched heel (p. 590), a push at an angle the notched
    /// oblique thrust (p. 584), a push square the stopped mortise and tenon (p. 436), a pulling post the wedged dovetail half
    /// lap (p. 570), any other pull the stopped dovetail half lap (p. 442). See docs/truss_joinery_plan.md.
    void add_joinery();

    /// The joint of role for end on side at node, the role's next design in turns that lets end go in along the bench's way
    /// (against the plane normal), its face turned where the record has one; else the first that fits. Returned, null when none.
    std::shared_ptr<JointBeamElement> add_role_joint(
        JointRole role,
        const std::shared_ptr<EdgeStraightElement>& end,
        const std::shared_ptr<EdgeStraightElement>& side,
        const Point& node,
        std::map<JointRole, size_t>& turns
    );

    /// The joint design of end on side at node, built with the end's stock running on through the side; null when it does not fit.
    std::shared_ptr<JointBeamElement> build_library_joint(
        const std::string& design,
        const std::map<std::string, double>& given,
        EdgeStraightElement& end,
        const EdgeStraightElement& side,
        const Point& node
    ) const;

    /// The built joint added and passed to end and side, end run on through the side, named after its design and members.
    std::shared_ptr<JointBeamElement> add_built_joint(
        const std::shared_ptr<JointBeamElement>& joint,
        const std::string& design,
        const std::shared_ptr<EdgeStraightElement>& end,
        const std::shared_ptr<EdgeStraightElement>& side,
        const Point& node
    );

    /// How far end runs on past node to clear side's far face.
    double through_length(const EdgeStraightElement& end, const EdgeStraightElement& side, const Point& node) const;

    /// The member's own section added from its far end to length past node, returned.
    Mesh add_run_on(const std::shared_ptr<EdgeStraightElement>& member, const Point& node, double length);

    /// The axial force of the guide's bar along member at node (positive pulls).
    double pull_at(const std::vector<TrussBar>& bars, const EdgeStraightElement& member, const Point& node) const;

    /// The direction member leaves node along its axis.
    static Vector away_from(const EdgeStraightElement& member, const Point& node);

    /// How steep member's axis is, 0 level to 1 plumb.
    static double steepness(const EdgeStraightElement& member);

    /// A plate let into every member of a crowded node (the first the carrier, running on carried past it), slotted along the path it drops in by, square
    /// to the node's chord away from the others, one row of two pins in each member clear of the others (JointBeamElement::node_plate,
    /// slotted_plate).
    void add_node_plate(const std::vector<std::shared_ptr<EdgeStraightElement>>& members, const Point& node, double carried);

    /// How far along member's axis from node its section is clear of other's, other running on run_on past the node.
    double clear_of(const EdgeStraightElement& member, const EdgeStraightElement& other, const Point& node, double run_on) const;

    /// Whether member's axis ends at node.
    static bool ends_at(const EdgeStraightElement& member, const Point& node);

    /// Every point where three or more members pass through each other split into a node: a chord (else the longest member)
    /// runs on through it, every other is cut in two there, `<name>_0` and `<name>_1`.
    void split_crossings();

    /// The pairs of members whose ways out of node lie next to each other around it (one running on through it leaving both
    /// ways).
    std::set<std::pair<const EdgeStraightElement*, const EdgeStraightElement*>> neighbours(
        const std::vector<std::shared_ptr<EdgeStraightElement>>& members,
        const std::vector<std::shared_ptr<EdgeStraightElement>>& runners,
        const Point& node
    ) const;

    /// A box from the plane through node square to keep, reaching out the other way: a member losing it is cut on that plane.
    Mesh cut_away(const Point& node, const Vector& keep) const;

    /// Whether butt is the miter of a chord's kink: between two consecutive runs of one chord.
    bool is_kink_miter(const JointBeamElement& butt) const;

    /// Whether a and b are runs of one chord.
    static bool same_chord(const EdgeStraightElement& a, const EdgeStraightElement& b);

    /// The run s of a chord named chord_<c>_<s>.
    static size_t run_index(const EdgeStraightElement& chord);

    /// The member guide web w became: its own, or the web it was joined into, in line with it.
    std::shared_ptr<EdgeStraightElement> web_member(size_t w) const;

    /// Half the truss's thickness across its plane, every member's: its deepest chord's half section.
    double half_thickness() const;

    /// The normal of the truss's plane.
    Vector plane_normal() const;

    /// Whether a's axis is steeper than b's.
    static bool steeper(const std::shared_ptr<EdgeStraightElement>& a, const std::shared_ptr<EdgeStraightElement>& b);

    /// The height of member's axis midpoint.
    static double lowness(const EdgeStraightElement& member);

    /// The butt added, named by its case and its members, and passed to both; nothing when they do not touch.
    void add_butt(
        const std::shared_ptr<JointBeamElement>& butt,
        const std::shared_ptr<EdgeStraightElement>& a,
        const std::shared_ptr<EdgeStraightElement>& b
    );
};

}
