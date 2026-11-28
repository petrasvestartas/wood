#pragma once
#include "src/templates/folding/folding_guide.h"

using namespace laika_cpp;
using namespace wood_session;

namespace wood_folding {

// ═══════════════════════════════════════════════════════════════════════════
// SingleFold
// ═══════════════════════════════════════════════════════════════════════════

/// Singly folded surfaces over a regular polygon plan, every sector folded once on its ridge.
class SingleFold : public WoodSession {
public:
    const SingleFoldGuide guide; // What it is built from; every element is in the session.

    /// The fold over the polygon plan of the guide as the session named name.
    explicit SingleFold(
        const SingleFoldGuide& guide = SingleFoldGuide(), const std::string& name = "single_fold"
    );

    /// Not copied, as a session is not.
    SingleFold(const SingleFold&) = delete;

    /// Not assigned, as it is not copied.
    SingleFold& operator=(const SingleFold&) = delete;

    /// The folded mesh, the plates' outer faces, one face per plate.
    const Mesh& mesh() const;

private:
    Mesh _mesh; // The folded mesh, the plates' outer faces.
};

}
