#pragma once
#include "src/templates/folding/folding_guide.h"
#include "src/templates/folding/fold_unfolding.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_folding {

// ═══════════════════════════════════════════════════════════════════════════
// ReflexFold
// ═══════════════════════════════════════════════════════════════════════════

/// A folded plate shell: a profile carried along a cross section, folded at every cross-section point, one plate per fold.
class ReflexFold : public WoodSession {
public:
    const ReflexFoldGuide guide; // What it is built from; every element is in the session.

    /// The fold of the profile along the cross section of the guide as the session named name.
    explicit ReflexFold(
        const ReflexFoldGuide& guide = ReflexFoldGuide(), const std::string& name = "reflex_fold"
    );

    /// Not copied, as a session is not.
    ReflexFold(const ReflexFold&) = delete;

    /// Not assigned, as it is not copied.
    ReflexFold& operator=(const ReflexFold&) = delete;

    /// The folded quad mesh, one face per plate.
    const Mesh& mesh() const;

    /// The flat 2D unfolding of the folded quad mesh.
    const FoldUnfolding& unfolding() const;

private:
    Mesh _mesh; // The folded mesh, the plates' outer faces.
    FoldUnfolding _unfolding; // The flat unfolding of the folded mesh.

    /// The bisector plane at every cross-section point, the end planes normal to z.
    std::vector<Plane> compute_fold_planes() const;

    /// Each profile row moved along the cross section onto the next fold plane, the rows joined by quads.
    Mesh compute_mesh(const std::vector<Plane>& fold_planes) const;
};

}
