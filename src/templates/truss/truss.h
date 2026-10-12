#pragma once
#include "src/templates/truss/truss_guide.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_truss {

// ═══════════════════════════════════════════════════════════════════════════
// Truss
// ═══════════════════════════════════════════════════════════════════════════

/// The truss model, a session built from a guide: a beam per chord run, per web and per end post, then the butts the nodes call for, every
/// one read from how the lines meet there.
///
/// The members are ranked as a graph is pruned, the outer boundary of the lines first, then the boundary of what is left; boundary corners are
/// made first and an inner member is let into the outer ones last, losing only the wood they have once cut.
///
/// - At a kink, the two runs side_side on the miter.
/// - Where chords meet at their ends, a heel, JointBeam::butt reads the case; a chord's end on another chord's run stops top_side on it.
/// - A web's or post's end on a run, or where several runs end, is let into each run there; at a run's lone end an outer chord runs on over
///   it, top_side at the end, an inner member is let in.
/// - A web's or post's end on another member's middle, as a K's on its vertical or a tie on a column, is let into it.
/// - Members whose lines cross inside each get laps, JointBeam::butt_cross, each keeping a layer of the depth where several cross at a point.
/// - At a node, an inner member is let into every outer one; those of one rank, sorted by angle, side_side with each neighbour, never across a
///   member passing through: on a chord never across the widest gap, at a node clear of the chords all the way round.
///
/// Members are named `chord_<c>_<s>`, `web_<w>` and `post_<p>`, read back by name; the guide's lines are drawn red in `input`.
class Truss : public WoodSession {
public:
    static constexpr double NODE_TOLERANCE = 1.0; // mm two ends lie apart and still meet at one node.
    const TrussGuide guide; // The lines the truss is drawn with.

    /// The truss: members, then butts at the chords' kinks and heels, then the webs' and posts' butts.
    explicit Truss(const TrussGuide& guide, const std::string& name = "truss");

private:
    /// The guide's chords, webs and posts, red, in input.
    void add_input();

    /// A beam per chord run, chord_<c>_<s>, in chords.
    void add_chords();

    /// A beam per web, web_<w>, and per post, post_<p>, in webs.
    void add_webs();

    /// Runs side_side at every kink; at heels the case read by JointBeam::butt; a chord's end on another's run top_side.
    void add_chord_butts();

    /// Each web's and post's ends on the runs and on other webs, chord ends on posts, the half laps wherever members cross, and the side_side miters between
    /// neighbours at every node.
    void add_web_butts();

    /// The butt added, named by its case and its members, and passed to both; nothing when they do not touch.
    void add_butt(const std::shared_ptr<JointBeam>& butt, const std::shared_ptr<Beam>& a, const std::shared_ptr<Beam>& b);
};

} // namespace wood_truss
