#include "src/templates/reciprocal/reciprocal_guide.h"
#include "src/templates/reciprocal/reciprocal_surface.h"

namespace wood_reciprocal {

// ═══════════════════════════════════════════════════════════════════════════
// Frame ups
// ═══════════════════════════════════════════════════════════════════════════

/// The ups of the frame per side of the surface, or per side at 45 degree bends of the mesh, upright when vertical.
static std::map<std::pair<size_t, size_t>, Vector> side_ups(
    const Mesh& mesh, const std::optional<NurbsSurface>& surface, bool vertical
) {

    if (surface)
        return side_tilt_boundary_ups(mesh, *surface);

    const std::vector<Vector> upright(4, Vector(0, 0, 1));
    return side_tilt_boundary_ups(mesh, 45.0, vertical ? upright : std::vector<Vector>{});
}

/// The sides that run through a butt corner, per side of the surface, or every other side at 45 degree bends.
static std::map<std::pair<size_t, size_t>, int> through_sides(
    const Mesh& mesh, const std::optional<NurbsSurface>& surface
) {

    if (surface)
        return through_side_priority(mesh, *surface);

    return through_side_priority(mesh, 45.0);
}

// ═══════════════════════════════════════════════════════════════════════════
// ReciprocalRotationGuide
// ═══════════════════════════════════════════════════════════════════════════
double ReciprocalRotationGuide::beam_height() const {
    return height > 0.0 ? height : 2.0 * width;
}

ReciprocalRotationGuide ReciprocalRotationGuide::defaults(const Mesh& mesh) {

    ReciprocalRotationGuide guide;
    guide.mesh = mesh;

    return guide;
}

ReciprocalRotationGuide ReciprocalRotationGuide::butted(
    const Mesh& mesh, const std::optional<NurbsSurface>& surface, bool vertical
) {

    ReciprocalRotationGuide guide;
    guide.mesh = mesh;
    guide.height = 400.0;
    guide.frame_ups = side_ups(mesh, surface, vertical);
    guide.butt = true;
    guide.through_priority = through_sides(mesh, surface);

    return guide;
}

// ═══════════════════════════════════════════════════════════════════════════
// ReciprocalMoveGuide
// ═══════════════════════════════════════════════════════════════════════════
double ReciprocalMoveGuide::beam_height() const {
    return height > 0.0 ? height : 2.0 * width;
}

ReciprocalMoveGuide ReciprocalMoveGuide::defaults(const Mesh& mesh) {

    ReciprocalMoveGuide guide;
    guide.mesh = mesh;

    return guide;
}

ReciprocalMoveGuide ReciprocalMoveGuide::butted(
    const Mesh& mesh, const std::optional<NurbsSurface>& surface, bool vertical
) {

    ReciprocalMoveGuide guide;
    guide.mesh = mesh;
    guide.shift = 100.0;
    guide.height = 400.0;
    guide.frame_ups = side_ups(mesh, surface, vertical);
    guide.edge_up = true;
    guide.butt = true;
    guide.through_priority = through_sides(mesh, surface);

    return guide;
}

}
