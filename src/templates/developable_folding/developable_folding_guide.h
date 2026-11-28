#pragma once
#include "wood_session.h"
#include "primitives.h"

using namespace laika_cpp;
using namespace wood_session;

namespace wood_developable_folding {

// ═══════════════════════════════════════════════════════════════════════════
// CurvedFoldGuide
// ═══════════════════════════════════════════════════════════════════════════

/// The geometry of a curved folded tent roof in mm: fold strips between ridges and valleys rising into a tower.
struct CurvedFoldGuide {
    size_t sectors = 12; // Folds around the axis, a ridge and a valley each.
    double radius = 16000.0; // The eaves' radius at the ridges.
    double top_radius = 3500.0; // The tower's radius at the top.
    double eave = 4000.0; // The valleys' height at the eaves, on the columns.
    double height = 26000.0; // The tower's top.
    double gable = 4000.0; // The ridges' rise over the eaves.
    double inset = 0.1; // The valleys' radius inside the ridges', a fraction of the radius.
    size_t rings = 14; // Rings from the eaves to the top the meridians are interpolated through.
    size_t samples = 32; // Rulings per strip.
    double thickness = 120.0; // Every lamella under the outer surface, 0 for the surface alone.

    /// The tent roof of 32 m rising into a tower 26 m high: the defaults.
    static CurvedFoldGuide tent_roof();
};

// ═══════════════════════════════════════════════════════════════════════════
// CurvedCreaseUnfoldingGuide
// ═══════════════════════════════════════════════════════════════════════════

/// The geometry of a curved-crease unfolding (CCU) in mm: a planar crease mirrored from plane to plane, a plate between every two.
struct CurvedCreaseUnfoldingGuide {
    NurbsCurve crease = Primitives::arc(
        Point(-4500.0, 0.0, 0.0), Point(0.0, 0.0, 4500.0), Point(4500.0, 0.0, 0.0)
    ); // The target crease, on one of the planes.
    double opening = 55.0 * Tolerance::TO_RADIANS; // Rulings to the crease's plane, 0 to 90.
    std::vector<Plane> planes = vault(
        Point(0.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0), Vector(1.0, 0.0, 0.0), 12, 450.0, 0.0
    ); // The reflection planes in order.
    std::vector<double> deployments = {1.0}; // Per stage w = alpha / opening, 0 closed, 1 target.
    double thickness = 30.0; // Every plate, 0 for the surfaces alone.
    size_t samples = 48; // Rulings per plate.

    // ═══════════════════════════════════════════════════════════════════════════
    // Creases and planes
    // ═══════════════════════════════════════════════════════════════════════════

    /// Funicular catenary arch of span and rise over the x axis in the xz plane, centred on the origin.
    static NurbsCurve catenary(double span, double rise, size_t points = 48);

    /// Parallel planes spacing apart along along; step > 0 curves the vault synclastic, step < 0 anticlastic.
    static std::vector<Plane> vault(
        const Point& origin,
        const Vector& along,
        const Vector& axis,
        size_t count,
        double spacing,
        double step = 0.0
    );

    /// Radial meridian planes around axis through origin, turned by angle from one to the next, the middle containing first.
    static std::vector<Plane>
    fan(const Point& origin, const Vector& axis, const Vector& first, size_t count, double angle);

    /// Planes spacing apart along along, every other pair inclined by +-tilt about axis.
    static std::vector<Plane> zigzag(
        const Point& origin,
        const Vector& along,
        const Vector& axis,
        size_t count,
        double spacing,
        double tilt
    );

    /// Planes spacing apart along along, the i-th from the middle inclined by i * lean about axis.
    static std::vector<Plane> leaning(
        const Point& origin,
        const Vector& along,
        const Vector& axis,
        size_t count,
        double spacing,
        double lean
    );

    // ═══════════════════════════════════════════════════════════════════════════
    // Presets: the shape set by the crease's profile, the planes' distance and their inclination
    // ═══════════════════════════════════════════════════════════════════════════

    /// Monoclastic corrugated arch: a 9 m semicircle on thirteen parallel planes 450 apart, the defaults.
    static CurvedCreaseUnfoldingGuide monoclastic();

    /// Synclastic vault: the plane distances stepping by 95 towards the middle.
    static CurvedCreaseUnfoldingGuide synclastic();

    /// Anticlastic vault: the plane distances stepping by 95 away from the middle.
    static CurvedCreaseUnfoldingGuide anticlastic();

    /// Catenary crease of 6 m span and 2.8 m rise on nine parallel planes 420 apart.
    static CurvedCreaseUnfoldingGuide catenary_vault();

    /// The catenary vault unfolding from its closed stack through w = 0.35 and 0.70 to the target.
    static CurvedCreaseUnfoldingGuide unfolding_sequence();

    /// Every other plane pair inclined by 5 degrees around the actuation axis at mid-height.
    static CurvedCreaseUnfoldingGuide alternating();

    /// The planes inclined progressively by 3 degrees around the actuation axis at the crown, the crown kept level.
    static CurvedCreaseUnfoldingGuide level_crown();

    /// Every other plane pair inclined by 3 degrees perpendicular to the actuation axis, articulating to the sides.
    static CurvedCreaseUnfoldingGuide asymmetric();

    /// Corrugated dome: a meridian arc of a 6 m sphere in 25 radial planes 15 degrees apart.
    static CurvedCreaseUnfoldingGuide dome();

    /// Corrugated funnel column: a flaring quarter circle of 5 m in 33 radial planes 11.25 degrees apart.
    static CurvedCreaseUnfoldingGuide funnel();
};

// ═══════════════════════════════════════════════════════════════════════════
// CreaseQuadrantsGuide
// ═══════════════════════════════════════════════════════════════════════════

/// The geometry of the four quadrants of one planar curved crease in mm, two strips each.
struct CreaseQuadrantsGuide {
    NurbsCurve crease = Primitives::arc(
        Point(-2000.0, 0.0, 0.0), Point(0.0, 0.0, 1500.0), Point(2000.0, 0.0, 0.0)
    ); // The planar crease.
    double opening = 40.0 * Tolerance::TO_RADIANS; // Rulings to the crease's plane, 0 to 90.
    double width = 1200.0; // Strip width along its rulings.
    double deployment = 1.0; // w = alpha / opening, 0 flat or closed, 1 the target.
    double thickness = 40.0; // Every strip, on its body side, 0 for the surfaces alone.
    size_t samples = 48; // Rulings per strip.

    /// An arc 4 m wide and 1.5 m high, strips 1200 wide at 40 degrees: the defaults.
    static CreaseQuadrantsGuide arc_crease();
};

// ═══════════════════════════════════════════════════════════════════════════
// CurvedCreaseTubeGuide
// ═══════════════════════════════════════════════════════════════════════════

/// The geometry of a cellular curved-crease tube vault in mm (Tachi 2013): parallelogram tubes along a space curve.
struct CurvedCreaseTubeGuide {
    double span = 8000.0; // Horizontal chord of the reference space curve at the target opening.
    double torsion_ratio = 0.30; // Harmonic torsion amplitude a = tau_max / kappa_0.
    double opening = 45.0 * Tolerance::TO_RADIANS; // Half folding angle alpha_0, 0 to 90.
    double width_left = 480.0; // Strip width w_L(0) across the left walls at the supports.
    double width_right = 360.0; // Strip width w_R(0) across the right walls at the supports.
    size_t tubes = 6; // Tubes chained from corner (b, b) to (b + 1, b + 1).
    std::vector<double> deployments = {1.0}; // Per stage w = alpha / opening > 0, 1 target.
    double thickness = 24.0; // Every wall, centred on its surface, 0 for the surfaces alone.
    size_t samples = 48; // Stations along the arch.

    /// Six tubes over 8 m, a = 0.30, walls 480 and 360 wide at 45 degrees: the defaults.
    static CurvedCreaseTubeGuide cellular_vault();
};

// ═══════════════════════════════════════════════════════════════════════════
// SpaceCurveFoldGuide
// ═══════════════════════════════════════════════════════════════════════════

/// The geometry of one constant-angle curved fold along a space polyline in mm (Tachi 2011): a V ridge of planar quads, a ruling per vertex.
struct SpaceCurveFoldGuide {
    Polyline curve = helix(2000.0, 3000.0, 1.5, 36); // The space curve the crease follows, a ruling at every vertex.
    double width = 600.0; // The ribbon across, w, each arm w / 2 from the crease.
    double angle = 60.0 * Tolerance::TO_RADIANS; // rho, the opening between the arms, 0 folded flat to 180 a flat sheet.
    double zigzag = 0.0; // mm, every other vertex moved along its curvature vector and the others against it, 0 for the curve itself.
    std::vector<double> deployments = {1.0}; // Per stage the opening as a fraction of angle, 0 folded flat.
    double thickness = 18.0; // Every plate, 0 for the quads alone.
    bool smooth = false; // Every arm one curved plate over its crease and edge, not a mitered plate per quad.

    // ═══════════════════════════════════════════════════════════════════════════
    // Curves
    // ═══════════════════════════════════════════════════════════════════════════

    /// Helix of radius around z rising pitch per turn, turns long, in points segments.
    static Polyline helix(double radius, double pitch, double turns, size_t points);

    /// Arc of radius in plan sweeping sweep radians, rising rise and falling again waves times, in points segments.
    static Polyline wave(double radius, double sweep, double rise, double waves, size_t points);

    // ═══════════════════════════════════════════════════════════════════════════
    // Presets
    // ═══════════════════════════════════════════════════════════════════════════

    /// A helix of 2 m radius rising 3 m per turn, 1.5 turns in 36 segments, opened 60 degrees: the defaults.
    static SpaceCurveFoldGuide helix_fold();

    /// An arc of 3 m radius over 172 degrees rising 0.8 m and falling again, in 40 segments.
    static SpaceCurveFoldGuide wave_fold();

    /// The helix zig-zagged by 150 along its curvature vectors, folding from 60 degrees to 1.2, all but flat, without plates.
    static SpaceCurveFoldGuide zigzag_fold();

    /// The helix in 180 segments, every arm one curved plate, folding from 60 to 6 degrees before its arms collide.
    static SpaceCurveFoldGuide smooth_helix_fold();
};

// ═══════════════════════════════════════════════════════════════════════════
// CurvedMiuraGuide
// ═══════════════════════════════════════════════════════════════════════════

/// The geometry of a generalized Miura-ori bent and twisted along a space polyline in mm (Tachi 2011): strips of the V ridge repeated across.
struct CurvedMiuraGuide {
    Polyline curve = SpaceCurveFoldGuide::helix(2000.0, 3000.0, 1.5, 36); // The space curve the middle crease follows.
    double width = 300.0; // Every strip across at the middle of the curve.
    double angle = 60.0 * Tolerance::TO_RADIANS; // rho, the opening at every crease, 0 folded flat to 180 a flat sheet.
    double zigzag = 0.0; // mm, every other vertex moved along its curvature vector and the others against it, 0 for the curve itself.
    size_t strips = 6; // Strips across, half on either side of the middle crease.
    std::vector<double> deployments = {1.0}; // Per stage the opening as a fraction of angle, 0 folded flat.
    double thickness = 18.0; // Every plate, 0 for the quads alone.

    /// Six strips 300 wide along the helix of SpaceCurveFoldGuide, opened 60 degrees: the defaults.
    static CurvedMiuraGuide helix_miura();

    /// Six strips along the wave of SpaceCurveFoldGuide::wave_fold.
    static CurvedMiuraGuide wave_miura();

    /// Six strips along the zig-zagged helix, folding from 60 degrees to 1.2, all but flat, without plates.
    static CurvedMiuraGuide zigzag_miura();
};

}
