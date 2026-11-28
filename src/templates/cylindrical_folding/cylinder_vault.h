#pragma once
#include "src/templates/cylindrical_folding/cylindrical_folding_guide.h"
#include "src/templates/development.h"

using namespace laika_cpp;
using namespace wood_session;

namespace wood_cylindrical_folding {

// ═══════════════════════════════════════════════════════════════════════════
// CylinderVault
// ═══════════════════════════════════════════════════════════════════════════

/// Equal circular cylinders over a regular polygon.
class CylinderVault : public WoodSession {
public:
    const CylinderVaultGuide
        guide; // The polygon and cylinders the vault is built from; every element is in the session.

    /// The vault of the guide as the session named name.
    explicit CylinderVault(
        const CylinderVaultGuide& guide = CylinderVaultGuide(),
        const std::string& name = "cylinder_vault"
    );

    /// Not copied, as a session is not.
    CylinderVault(const CylinderVault&) = delete;

    /// Not assigned, as it is not copied.
    CylinderVault& operator=(const CylinderVault&) = delete;

    /// The largest relative length error of any panel's development, the proof every panel is cut flat.
    double distortion() const;

    /// The largest twist of any panel's generatrices out of the tangent planes, 0 on a cylinder.
    double twist() const;

private:
    /// One cylinder segment: its circle, its generatrices, the angles it spans and the vertical planes around its footprint.
    struct Segment {
        Point centre; // The circle's centre, on the cylinder's axis.
        Vector x; // The circle's direction at angle 90 degrees.
        Vector y; // The circle's direction at angle 0.
        Vector axis; // The generatrices' unit direction.
        double radius; // mm, the cylinder.
        double start; // radians, the first generatrix.
        double end; // radians, the last generatrix.
        std::vector<Plane>
            footprint; // The vertical planes around the segment, normals pointing in.
        std::vector<Point> corners; // The footprint's corners on the ground, counter-clockwise.
        Point from; // The side's first corner on the ground, the wall under the segment.
        Point to; // The side's second corner.
    };

    double _distortion = 0.0; // The largest relative length error of any panel's development.
    double _twist = 0.0; // The largest twist of any panel's generatrices out of the tangent planes.

    /// The polygon's corner i on the ground, counter-clockwise from the x axis, around the origin.
    Point corner(size_t i) const;

    /// The vertical plane through two points on the ground, its normal pointing toward a third.
    static Plane wall_plane(const Point& from, const Point& to, const Point& inside);

    /// Every segment of the kind over the polygon, lifted onto the walls.
    std::vector<Segment> compute_segments() const;

    /// The point of the segment's circle at the angle.
    static Point on_circle(const Segment& segment, double radius, double angle);

    /// Where the generatrix at the angle enters and leaves the footprint: the distances along the axis and the planes.
    std::array<std::pair<double, size_t>, 2>
    clip(const Segment& segment, double radius, double angle) const;

    /// mm, the generatrix at the angle inside the footprint.
    double generatrix_length(const Segment& segment, double radius, double angle) const;

    /// The planes the generatrix at the angle enters and leaves the footprint by.
    std::pair<size_t, size_t>
    generatrix_planes(const Segment& segment, double radius, double angle) const;

    /// The angle between end and other where the generatrix is a millimetre long, by bisection.
    double drawn_in(const Segment& segment, double radius, double end, double other) const;

    /// The angles the meridians of the cylinder of the radius change their plane at, the spans of the panel's multi-span arcs.
    std::vector<double> compute_spans(const Segment& segment, double radius) const;

    /// The meridian at the given end (0 the entry, 1 the exit).
    NurbsCurve compute_meridian(
        const Segment& segment, const std::vector<double>& spans, double radius, size_t end
    ) const;

    /// The footprints, in the group plan.
    void add_plan(const std::vector<Segment>& segments);

    /// Every segment's panel, lamella_<segment>, in the group lamellas; its development returned for the flat pattern.
    std::vector<Development> add_panels(const std::vector<Segment>& segments);

    /// The flat walls under the cloister vault's sides, wall_<segment>, in the group walls.
    void add_walls(const std::vector<Segment>& segments);

    /// The flat cutting pattern beside the model, a row per segment, in the group flat.
    void add_flat(const std::vector<Development>& developments, double beside);
};

}
