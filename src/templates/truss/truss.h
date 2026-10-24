#pragma once
#include "src/templates/truss/truss_guide.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_truss {

// ═══════════════════════════════════════════════════════════════════════════
// Truss
// ═══════════════════════════════════════════════════════════════════════════

/// The truss model.
class Truss : public WoodSession {
public:
    static constexpr double NODE_TOLERANCE =
        1.0; // mm two ends lie apart and still meet at one node.
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

    /// Each web's and post's ends on the runs and on other webs, chord ends on posts.
    void add_web_butts();

    /// The butt added, named by its case and its members, and passed to both; nothing when they do not touch.
    void add_butt(
        const std::shared_ptr<JointBeam>& butt,
        const std::shared_ptr<Beam>& a,
        const std::shared_ptr<Beam>& b
    );
};

}
