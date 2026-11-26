#pragma once
#include "src/templates/folding/folding_guide.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_folding {

// ═══════════════════════════════════════════════════════════════════════════
// TruncatedPyramid
// ═══════════════════════════════════════════════════════════════════════════

/// The pyramid and truncated pyramid on a square plan, its sides folded at edge and corner points.
class TruncatedPyramid : public WoodSession {
public:
    const TruncatedPyramidGuide guide; // What it is built from; every element is in the session.

    /// The pyramid of the guide as the session named name.
    explicit TruncatedPyramid(
        const TruncatedPyramidGuide& guide = TruncatedPyramidGuide(),
        const std::string& name = "truncated_pyramid"
    );

    /// Not copied, as a session is not.
    TruncatedPyramid(const TruncatedPyramid&) = delete;

    /// Not assigned, as it is not copied.
    TruncatedPyramid& operator=(const TruncatedPyramid&) = delete;

    /// The folded mesh, the plates' outer faces, one face per plate.
    const Mesh& mesh() const;

private:
    Mesh _mesh; // The folded mesh, the plates' outer faces.
};

}
