#pragma once
#include "wood_session.h"
#include "primitives.h"

using namespace session_cpp;
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
    // Presets: the paper's rules (Scheder-Bieschin, Van Mele and Block, AAG 2023, 5.4 and 5.5)
    // ═══════════════════════════════════════════════════════════════════════════

    /// Monoclastic corrugated arch: a 9 m semicircle on thirteen parallel planes 450 apart (Fig. 10a), the defaults.
    static CurvedCreaseUnfoldingGuide monoclastic();

    /// Synclastic vault: the plane distances stepping by 95 towards the middle (Fig. 10b).
    static CurvedCreaseUnfoldingGuide synclastic();

    /// Anticlastic vault: the plane distances stepping by 95 away from the middle (Fig. 10c).
    static CurvedCreaseUnfoldingGuide anticlastic();

    /// Catenary crease of 6 m span and 2.8 m rise on nine parallel planes 420 apart (Fig. 9c).
    static CurvedCreaseUnfoldingGuide catenary_vault();

    /// The catenary vault unfolding from its closed stack through w = 0.35 and 0.70 to the target.
    static CurvedCreaseUnfoldingGuide unfolding_sequence();

    /// Every other plane pair inclined by 5 degrees around the actuation axis at mid-height (Fig. 11).
    static CurvedCreaseUnfoldingGuide alternating();

    /// The planes inclined progressively by 3 degrees around the actuation axis at the crown, the crown kept level (Fig. 11).
    static CurvedCreaseUnfoldingGuide level_crown();

    /// Every other plane pair inclined by 3 degrees perpendicular to the actuation axis, articulating to the sides (Fig. 12a).
    static CurvedCreaseUnfoldingGuide asymmetric();

    /// Corrugated dome: a meridian arc of a 6 m sphere in 25 radial planes 15 degrees apart (Fig. 12b).
    static CurvedCreaseUnfoldingGuide dome();

    /// Corrugated funnel column: a flaring quarter circle of 5 m in 33 radial planes 11.25 degrees apart (Fig. 12c).
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

}
