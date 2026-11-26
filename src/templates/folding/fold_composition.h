#pragma once
#include "src/templates/folding/folding_guide.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_folding {

// ═══════════════════════════════════════════════════════════════════════════
// FoldComposition
// ═══════════════════════════════════════════════════════════════════════════

/// A field of crosswise folded square cells.
class FoldComposition : public WoodSession {
public:
    const FoldCompositionGuide guide; // What it is built from; every element is in the session.

    /// The composition of the guide as the session named name.
    explicit FoldComposition(
        const FoldCompositionGuide& guide = FoldCompositionGuide(),
        const std::string& name = "fold_composition"
    );

    /// Not copied, as a session is not.
    FoldComposition(const FoldComposition&) = delete;

    /// Not assigned, as it is not copied.
    FoldComposition& operator=(const FoldComposition&) = delete;

    /// The folded triangle mesh, the plates' outer faces, one face per plate.
    const Mesh& mesh() const;

private:
    Mesh _mesh; // The folded mesh, the plates' outer faces.

    /// The node at column i and row j of the field's grid, low or high by the chessboard, the block's corners raised.
    Point compute_node(size_t i, size_t j) const;
};

}
