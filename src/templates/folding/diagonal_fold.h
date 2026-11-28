#pragma once
#include "src/templates/folding/folding_guide.h"
#include "src/templates/folding/fold_mesh.h"

using namespace laika_cpp;
using namespace wood_session;

namespace wood_folding {

// ═══════════════════════════════════════════════════════════════════════════
// Cross fold
// ═══════════════════════════════════════════════════════════════════════════

/// The sixteen triangles of one cross-folded square centred on centre, half its side half, gathered into fold.
void add_diagonal_fold(
    FoldMesh& fold, const Point& centre, double half, const DiagonalHeights& heights
);

// ═══════════════════════════════════════════════════════════════════════════
// DiagonalFold
// ═══════════════════════════════════════════════════════════════════════════

/// Cross-folded surfaces spanned diagonally over a square plan, four sectors folded on their diagonals.
class DiagonalFold : public WoodSession {
public:
    const DiagonalFoldGuide guide; // What it is built from; every element is in the session.

    /// The cross fold over the square of the guide as the session named name.
    explicit DiagonalFold(
        const DiagonalFoldGuide& guide = DiagonalFoldGuide(),
        const std::string& name = "diagonal_fold"
    );

    /// Not copied, as a session is not.
    DiagonalFold(const DiagonalFold&) = delete;

    /// Not assigned, as it is not copied.
    DiagonalFold& operator=(const DiagonalFold&) = delete;

    /// The folded triangle mesh, the plates' outer faces, one face per plate.
    const Mesh& mesh() const;

private:
    Mesh _mesh; // The folded mesh, the plates' outer faces.
};

}
