#pragma once
#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_folding {

// ═══════════════════════════════════════════════════════════════════════════
// Fold strips
// ═══════════════════════════════════════════════════════════════════════════

/// The folded mesh through stations: per strip one planar quad, else two triangles on the folding's diagonal (0 ridge to ridge, 1 ridge to valley, 2 alternating), facing -z.
Mesh fold_strips(
    const std::vector<Polyline>& stations,
    const std::vector<std::vector<double>>& heights,
    int folding
);

/// The mesh with every two coplanar faces sharing an edge joined, again and again, and the points no face uses left out.
Mesh merge_coplanar(const Mesh& mesh);

}
