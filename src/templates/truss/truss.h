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
/// - At a kink, the two runs side_side on the miter.
/// - Where chords meet at their ends, a heel, JointBeam::butt reads the case; a chord's end on another chord's run stops top_side on it.
/// - A web's or post's end on a run, or where several runs end, stops top_side on each run there; at a run's lone end the chord runs on over
///   it, top_side at the end.
/// - A web's or post's end on another web's or post's middle, as a K's on its vertical, stops top_side on it.
/// - A chord's end on a web's or post's middle, as a tie on a column, stops top_side on it.
/// - Any two members whose lines cross inside both, chords, webs or posts, get a half lap, JointBeam::butt_cross; one member can be crossed
///   many times.
/// - At a node, the webs and posts ending there, sorted by angle, side_side with each neighbour: on a chord never across the widest gap, at a
///   node clear of the chords all the way round.
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
