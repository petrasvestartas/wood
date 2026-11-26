#pragma once
#include "wood_session.h"
#include "src/templates/folding/fold_unfolding.h"

using namespace session_cpp;
using namespace wood_session;

// ═══════════════════════════════════════════════════════════════════════════
// Ruled and curved-crease strip development
// ═══════════════════════════════════════════════════════════════════════════

/// Isometric triangle-strip development of one ruled curved plate or a continuous multi-strip curved-crease sheet.
class Development {
public:
    size_t samples = 64; // Rulings per strip used to unfold the meridians.
    Polyline outline; // Closed 2D outer cutting outline in the xy plane.
    Polyline inner_outline; // Closed 2D inner-face outline when built from a FaceCurvedElement.
    Polyline bottom; // Closed 2D bottom-face outline (z = 0) of an unfolded FaceCurvedElement.
    Polyline
        top; // Closed 2D top-face outline (projected to z = 0) of an unfolded FaceCurvedElement.
    std::vector<Line> rulings; // Straight 2D ruling lines across the unfolded strip.
    std::vector<Polyline>
        creases; // Interior 2D curved creases when built from three or more meridians.
    double thickness = 0.0; // mm, recovered uniform plate thickness of a FaceCurvedElement.
    double bevel_a = 0.0; // mm, miter bevel offset |t * cot(alpha)| along crease a.
    double bevel_b = 0.0; // mm, miter bevel offset |t * cot(alpha)| along crease b.
    double distortion =
        0.0; // Largest relative quad diagonal change, 0 on an exact developable sheet.
    double twist =
        0.0; // Largest |ruling . (tangent_a x tangent_b)| of unit vectors, 0 on a developable strip.
    std::shared_ptr<FaceCurvedElement>
        piece; // Closed 6-patch BRep flat solid piece on z in [0, thickness].

    // ═══════════════════════════════════════════════════════════════════════════
    // Constructors
    // ═══════════════════════════════════════════════════════════════════════════

    /// Construct an empty development.
    Development() = default;

    /// Unfold the ruled strip between curves a and b with the given number of rulings.
    Development(const NurbsCurve& a, const NurbsCurve& b, size_t samples = 64);

    /// Unfold the zero-strain neutral mid-surface and 3D miter bevels of a curved timber plate into a flat solid piece.
    explicit Development(
        const FaceCurvedElement& plate, size_t samples = 64, const std::string& name = "flat_piece"
    );

    /// Unfold a continuous single-sheet chain of ruled strips across shared curved meridians.
    explicit Development(const std::vector<NurbsCurve>& meridians, size_t samples = 64);

    // ═══════════════════════════════════════════════════════════════════════════
    // Scene export
    // ═══════════════════════════════════════════════════════════════════════════

    /// Every development's piece on a cutting sheet beside the session's model, group flat: turned so its longest chord runs along x and
    /// its arc bulges the same way, in rows of count, gap apart.
    static void add_flat(
        WoodSession& session, const std::vector<Development>& developments, size_t count = 4, double gap = 300.0
    );

    /// Lay the outlines of developments flat in group "pattern" under box, in rows of columns gap apart, named pattern_i.
    static void add_pattern(
        WoodSession& session,
        const std::vector<Development>& developments,
        const AABB& box,
        size_t columns,
        double gap
    );

    /// The zero-thickness ruled strip from a to b as a quad mesh, ruling k from a[k] to b[k].
    static Mesh
    strip(const std::vector<Point>& a, const std::vector<Point>& b, const std::string& name);

private:
    /// Point at given distances from two flat points, on the side of their line away from a third point.
    static Point third_corner(
        const Point& from,
        const Point& to,
        double from_distance,
        double to_distance,
        const Point& away
    );
};
