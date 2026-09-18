#pragma once

#include "pch.h"

namespace wood_session {

/// Tallies from one element-pair scan; face_to_face_wood reports them as dbg_coplanar / dbg_boolean.
struct PairScanStats {
    int coplanar = 0; // Face pairs that passed the coplanarity test.
    int overlapping = 0; // Of those, the ones with a real overlap area.
    int empty_i = -1; // Face of the first element in the last pair whose boolean came back empty, -1 when none.
    int empty_j = -1; // Face of the second element in that pair, -1 when none.
};

} // namespace wood_session
