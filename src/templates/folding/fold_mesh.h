#pragma once
#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_folding {

// ═══════════════════════════════════════════════════════════════════════════
// FoldMesh
// ═══════════════════════════════════════════════════════════════════════════

/// The corners of a regular polygon around the origin at height z, its edge at -y level.
std::vector<Point> regular_polygon(size_t sides, double radius, double z);

/// A folded surface gathered face by face, points within a hundredth of a millimetre welded to one vertex.
class FoldMesh {
private:
    std::vector<Point> _points; // Every point gathered, one per vertex.
    std::vector<std::vector<size_t>> _faces; // Every face as vertex indices, in the order given.
    std::map<std::array<long long, 3>, size_t>
        _welded; // Every point's rounded position to its vertex.

public:
    /// The index of point, the one already gathered when a point lies within a hundredth of a millimetre of it.
    size_t add(const Point& point);

    /// A face through points, in the order given.
    void face(const std::vector<Point>& points);

    /// The faces as one mesh, every face wound the same way and the sheet facing down.
    Mesh mesh() const;

    /// Every point gathered so far.
    const std::vector<Point>& points() const;

    /// Every face gathered so far, in the order given.
    const std::vector<std::vector<size_t>>& faces() const;
};

}
