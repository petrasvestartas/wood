#pragma once
#include "src/templates/folding/folding_guide.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_folding {

// ═══════════════════════════════════════════════════════════════════════════
// TriangleFold
// ═══════════════════════════════════════════════════════════════════════════

/// Folded triangular surfaces over a regular polygon plan, a gable on every edge.
class TriangleFold : public WoodSession {
public:
    const TriangleFoldGuide guide; // What it is built from; every element is in the session.

    /// The triangular fold over the polygon plan of the guide as the session named name.
    explicit TriangleFold(
        const TriangleFoldGuide& guide = TriangleFoldGuide(),
        const std::string& name = "triangle_fold"
    );

    /// Not copied, as a session is not.
    TriangleFold(const TriangleFold&) = delete;

    /// Not assigned, as it is not copied.
    TriangleFold& operator=(const TriangleFold&) = delete;

    /// The folded mesh, the plates' outer faces, one face per plate.
    const Mesh& mesh() const;

private:
    Mesh _mesh; // The folded mesh, the plates' outer faces.
};

}
