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
    static constexpr double OVERHANG = 6.0; // Half depths a piece hanging past a node is shorter than to be removed.
    static constexpr double MITER_ANGLE = 35.0; // Degrees two ends at a plate node meet at, at least, to be mitered; sharper, one bears on the other.
    static constexpr double PIN_RADIUS = 6.0; // mm, a node plate's pins.
    static constexpr double PROUD = 1.0; // mm a member another bears on is taken past its faces in that cut, so the cut shares no face.
    static constexpr double PLATE_INSET = 15.0; // mm, a node plate kept in from its members' edges.
    static constexpr double PLATE_KEEP_ANGLE = 60.0; // Degrees off every member of its node the plate's way square to the chord is kept at, when clear.
    static constexpr double PLATE_FACE_ANGLE = 45.0; // Degrees off every member of its node a plate's way in the truss's plane keeps, at least; else from the face.
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

    /// The joinery in place of the draft's butts, one of two: the book's joints where every node is one member ending on another
    /// (add_book_joinery), else a plate at every node (add_plated_joinery). See docs/truss_joinery_plan.md.
    void add_joinery();

    /// Every end of a member's axis, once.
    std::vector<Point> nodes() const;

    /// The members ending at node, and those running through it.
    std::array<std::vector<std::shared_ptr<EdgeStraightElement>>, 2> meeting(const Point& node) const;

    /// The member the others at node are jointed into: one running through, else a post ending there, else the lowest chord,
    /// else the steepest web.
    std::shared_ptr<EdgeStraightElement> carrier_at(const Point& node) const;

    /// Whether every node is one member ending on another: no two ending in one, no chord's kink, no three crossing at a point.
    bool book_fits() const;

    /// The book's joint of every end on its carrier at its node (the carrier running on past it when it ends there), by
    /// whether the end pulls (TrussGuide::bars): a heel the notched heel (pp. 590, 592), a push the notched oblique thrusts
    /// (pp. 584, 586, 588) or a mortise and tenon (p. 436), a pull the dovetail half laps (pp. 440-450, wedged p. 570); two
    /// members crossing a cross lap (pp. 646, 672, 676).
    void add_book_joinery();

    /// Every member split at every node on it, a short piece hanging past a node removed; at each node the chords mitered with
    /// each other (a heel: the steeper bearing on the flatter, both cut square to it), the webs mitered with each other and flush
    /// on the chords, one plate let into them all, pinned.
    void add_plated_joinery();

    /// bearing stopping on holding: losing what holding is so far (its drawn stock where that is no closed solid), taken PROUD
    /// past the truss's faces, and past holding's end at joined, the node where the next piece of its chord takes over, so the
    /// two cuts overlap there instead of meeting on one face.
    void bear(
        const std::shared_ptr<EdgeStraightElement>& holding,
        const std::shared_ptr<EdgeStraightElement>& bearing,
        const std::optional<Point>& joined = std::nullopt,
        double past = PROUD
    );

    /// first and second mitered on their bisector at node, true; two pieces of one line cut square on the node, true; meeting
    /// under MITER_ANGLE not mitered, false.
    bool add_miter(const std::shared_ptr<EdgeStraightElement>& first, const std::shared_ptr<EdgeStraightElement>& second, const Point& node);

    /// A box from the plane through node square to keep, reaching out the other way: a member losing it is cut on that plane.
    Mesh cut_away(const Point& node, const Vector& keep) const;

    /// node's chords mitered with each other (a heel: the steeper bearing on the flatter, both cut square to it), its webs
    /// mitered with each other and flush on the chords, one plate let into them all, pinned.
    void add_plated_node(const Point& node);

    /// Every member cut at every node strictly inside it (another's end or a crossing), its pieces <name>_<k> along it.
    void split_at_nodes();

    /// The points where members pass through each other, neither ending there, with who passes each.
    std::vector<std::pair<Point, std::vector<std::shared_ptr<EdgeStraightElement>>>> crossings() const;

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

    /// A plate in the middle of the thickness let into every member ending at node, slotted along the way it drops in by in the
    /// truss's plane (square to the node's chord, else the clear way farthest off its members), at a node too dense for that from
    /// the face; one row of two pins in each member clear of the others (JointBeamElement::node_plate, slotted_plate).
    void add_node_plate(const std::vector<std::shared_ptr<EdgeStraightElement>>& members, const Point& node);

    /// How far along member's axis from node its section is clear of other's, other running on run_on past the node.
    double clear_of(const EdgeStraightElement& member, const EdgeStraightElement& other, const Point& node, double run_on) const;

    /// Whether member's axis ends at node.
    static bool ends_at(const EdgeStraightElement& member, const Point& node);

    /// Whether a and b are runs of one chord.
    static bool same_chord(const EdgeStraightElement& a, const EdgeStraightElement& b);

    /// The member guide web w became: its own, the web it was joined into in line with it, else the web nearest its middle.
    std::shared_ptr<EdgeStraightElement> web_member(size_t w) const;

    /// Half the truss's thickness across its plane, every member's: its deepest chord's half section.
    double half_thickness() const;

    /// The normal of the truss's plane.
    Vector plane_normal() const;

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
