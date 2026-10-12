#pragma once
#include "src/templates/truss/truss_guide.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_truss {

// ═══════════════════════════════════════════════════════════════════════════
// Truss
// ═══════════════════════════════════════════════════════════════════════════

/// The truss model, a session built from a guide: a beam per chord run and per web, then the butts the nodes call for, every one read from
/// how the lines meet there.
///
/// - At a kink, the two runs side_side on the miter.
/// - Where chords meet at their ends, a heel, JointBeam::butt reads the case; a chord's end on another chord's run stops top_side on it.
/// - A web's end on a run or at a kink stops top_side on every run there; at a chord's end where only that chord ends, an end post, the
///   chord runs on over it.
/// - Webs ending at the same node side_side on the plane halfway.
///
/// Members are named `chord_<c>_<s>` and `web_<w>`, read back by name; the guide's lines are drawn red in `input`.
class Truss : public WoodSession {
public:
    static constexpr double NODE_TOLERANCE = 1.0; // mm two ends lie apart and still meet at one node.
    const TrussGuide guide; // The lines the truss is drawn with.

    /// The truss: members, then butts at the chords' kinks and heels, then the webs' butts.
    explicit Truss(const TrussGuide& guide, const std::string& name = "truss");

private:
    /// The guide's chords and webs, red, in input.
    void add_input();

    /// A beam per chord run, chord_<c>_<s>, in chords.
    void add_chords();

    /// A beam per web, web_<w>, in webs.
    void add_webs();

    /// Runs side_side at every kink; at heels the case read by JointBeam::butt; a chord's end on another's run top_side.
    void add_chord_butts();

    /// Each web's ends top_side on the runs they lie on, a chord running on over an end post, and webs at one node side_side.
    void add_web_butts();

    /// The butt added, named by its case and its members, and passed to both; nothing when they do not touch.
    void add_butt(const std::shared_ptr<JointBeam>& butt, const std::shared_ptr<Beam>& a, const std::shared_ptr<Beam>& b);
};

} // namespace wood_truss
