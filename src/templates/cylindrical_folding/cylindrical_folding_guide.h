#pragma once
#include "wood_session.h"
#include "primitives.h"

using namespace laika_cpp;
using namespace wood_session;

namespace wood_cylindrical_folding {

// ═══════════════════════════════════════════════════════════════════════════
// Fold
// ═══════════════════════════════════════════════════════════════════════════

/// One fold of the sheet: where the rulings turn, by how much, and how.
struct Fold {
    double station; // mm along the rulings from the sheet's start, at the profile's first point.
    double angle; // radians, the rulings' turn about the axis, positive by the right hand.
    bool reflected = true; // The sheet turned over when true; mirrored, its bulge kept, when false.
};

// ═══════════════════════════════════════════════════════════════════════════
// ReflectionFoldGuide
// ═══════════════════════════════════════════════════════════════════════════

/// The sheet of a curved folding of a cylindrical sheet in mm.
struct ReflectionFoldGuide {
    std::vector<NurbsCurve> profile = {Primitives::arc(
        Point(500.0, -866.0, 3000.0), Point(1000.0, 0.0, 3000.0), Point(500.0, 866.0, 3000.0)
    )}; // The sheet's section: planar curves end to end, each a row of lamellas.
    Vector direction = Vector(0.0, 0.0, -1.0); // The rulings' direction before the first fold.
    Vector axis = Vector(0.0, -1.0, 0.0); // The folds' axis, in every fold plane.
    double length = 3500.0; // The sheet along the rulings, when no symmetry plane ends it.
    std::vector<Fold> folds = {
        {1500.0, 60.0 * Tolerance::TO_RADIANS}
    }; // The folds, in order along the rulings.
    std::optional<Plane>
        symmetry; // The plane the sheet ends and is mirrored in; none for two ends.
    double thickness = 120.0; // Every lamella, on the sheet's underside, kept through every fold.
    size_t samples = 24; // Rulings per profile curve, 16 times as many for the offsets.

    /// A profile of count circular arcs side by side from start along across, each width wide and bulging rise toward up.
    static std::vector<NurbsCurve> arcs(
        const Point& start,
        const Vector& across,
        const Vector& up,
        size_t count,
        double width,
        double rise
    ) {

        const Vector a = across.normalized();
        const Vector u = up.normalized();
        std::vector<NurbsCurve> curves;

        for (size_t i = 0; i < count; i++) {
            const Point from = start + a * (width * static_cast<double>(i));
            const Point top = from + a * (0.5 * width) + u * rise;
            const Point to = from + a * width;
            curves.push_back(Primitives::arc(from, top, to));
        }

        return curves;
    }

    /// A corrugated profile from start along across: half_waves half sine waves half_wavelength long and amplitude high, first up.
    static std::vector<NurbsCurve> waves(
        const Point& start,
        const Vector& across,
        const Vector& up,
        size_t half_waves,
        double half_wavelength,
        double amplitude
    ) {

        const Vector a = across.normalized();
        const Vector u = up.normalized();
        const size_t points = 12;
        std::vector<NurbsCurve> curves;

        for (size_t i = 0; i < half_waves; i++) {
            const double sign = i % 2 == 0 ? 1.0 : -1.0;
            std::vector<Point> wave;

            for (size_t k = 0; k <= points; k++) {
                const double s = static_cast<double>(k) / static_cast<double>(points);
                const Vector run = a * (half_wavelength * (static_cast<double>(i) + s));
                const Vector lift = u * (sign * amplitude * std::sin(Tolerance::PI * s));
                wave.push_back(start + run + lift);
            }

            curves.push_back(NurbsCurve::create_interpolated(wave));
        }

        return curves;
    }

    /// The IBOIS reflection principle: a 120 degree arc reflected once by 120 degrees, the defaults at 6 rulings per curve.
    static ReflectionFoldGuide reflection() {

        ReflectionFoldGuide guide;
        guide.samples = 6; // A smooth barrel, the curves within 0.05 mm of 24, a smaller scene.

        return guide;
    }

    /// A curved folded plate: six half sine waves climbing at 60 degrees, folded back four times by 30 degrees.
    static ReflectionFoldGuide curved_folded_plate() {

        const double climb = 60.0 * Tolerance::TO_RADIANS;
        const Vector along(0.0, std::cos(climb), std::sin(climb));
        const Vector up(0.0, -std::sin(climb), std::cos(climb));

        ReflectionFoldGuide guide;
        guide.profile =
            waves(Point(-4500.0, 0.0, 0.0), Vector(1.0, 0.0, 0.0), up, 6, 1500.0, 400.0);
        guide.direction = along.normalized();
        guide.axis = Vector(1.0, 0.0, 0.0);
        guide.length = 18100.0;
        guide.folds = {
            {3620.0, -30.0 * Tolerance::TO_RADIANS},
            {7240.0, -30.0 * Tolerance::TO_RADIANS},
            {10860.0, -30.0 * Tolerance::TO_RADIANS},
            {14480.0, -30.0 * Tolerance::TO_RADIANS}
        };
        guide.samples = 6; // At 24 the viewer takes 25 minutes, at 6 one; within 0.03 mm.

        return guide;
    }

    /// The Mendrisio pavilion: a shallow barrel reflected at the eave and at the ridge, mirrored in its symmetry plane.
    static ReflectionFoldGuide mendrisio_pavilion() {

        ReflectionFoldGuide guide;
        guide.profile = {Primitives::arc(
            Point(6000.0, -2250.0, 0.0), Point(6400.0, 0.0, 0.0), Point(6000.0, 2250.0, 0.0)
        )};
        guide.direction = Vector(0.1719, 0.0, 0.9851).normalized();
        guide.length = 12000.0;
        guide.folds = {
            {2923.0, 89.9 * Tolerance::TO_RADIANS, true},
            {3623.0, 10.0 * Tolerance::TO_RADIANS, true}
        };
        guide.symmetry = Plane::from_point_normal(Point(0.0, 0.0, 0.0), Vector(1.0, 0.0, 0.0));
        guide.thickness = 77.0;
        guide.samples = 6; // A smooth barrel, the curves within 0.05 mm of 24, a smaller scene.

        return guide;
    }

    /// A two-hinged arch: four barrels climbing at 105 degrees, two mirror folds of 50 degrees, mirrored at the crown.
    static ReflectionFoldGuide two_hinged_arch() {

        const double climb = 105.0 * Tolerance::TO_RADIANS;
        const Vector along(std::cos(climb), 0.0, std::sin(climb));
        const Vector up(-std::sin(climb), 0.0, std::cos(climb));

        ReflectionFoldGuide guide;
        guide.profile =
            arcs(Point(0.0, 12000.0, 0.0), Vector(0.0, -1.0, 0.0), up, 4, 6000.0, 2400.0);
        guide.direction = along.normalized();
        guide.length = 40000.0;
        guide.folds = {
            {11000.0, -50.0 * Tolerance::TO_RADIANS, false},
            {22000.0, -50.0 * Tolerance::TO_RADIANS, false}
        };
        guide.symmetry = Plane::from_point_normal(Point(11000.0, 0.0, 0.0), Vector(1.0, 0.0, 0.0));
        guide.samples = 6; // A smooth barrel, the curves within 0.05 mm of 24, a smaller scene.

        return guide;
    }

    /// A two-hinged A-frame: four barrels climbing at 110 degrees, one reflection of 60 degrees, mirrored at the ridge.
    static ReflectionFoldGuide a_frame() {

        const double climb = 110.0 * Tolerance::TO_RADIANS;
        const Vector along(std::cos(climb), 0.0, std::sin(climb));
        const Vector up(-std::sin(climb), 0.0, std::cos(climb));

        ReflectionFoldGuide guide;
        guide.profile =
            arcs(Point(0.0, 12000.0, 0.0), Vector(0.0, -1.0, 0.0), up, 4, 6000.0, 1400.0);
        guide.direction = along.normalized();
        guide.length = 40000.0;
        guide.folds = {{5600.0, -60.0 * Tolerance::TO_RADIANS, true}};
        guide.symmetry = Plane::from_point_normal(Point(12500.0, 0.0, 0.0), Vector(1.0, 0.0, 0.0));
        guide.samples = 6; // A smooth barrel, the curves within 0.05 mm of 24, a smaller scene.

        return guide;
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// CylinderVaultGuide
// ═══════════════════════════════════════════════════════════════════════════

/// Equal circular cylinders over a regular polygon in mm.
struct CylinderVaultGuide {
    size_t sides = 4; // The regular polygon's sides, at least 3.
    double edge = 12000.0; // The polygon's side.
    double radius = 6000.0; // Every cylinder; at least half the side or the apothem.
    bool cloister = false; // The cylinders along the sides when true, across them when false.
    double wall = 0.0; // The walls the cloister vault stands on, a panel per side; 0 for none.
    double thickness = 120.0; // Every panel, under the cylinder.

    /// The groin vault: two half barrels of 12 m span crossing over a square, the defaults.
    static CylinderVaultGuide groin() {
        return CylinderVaultGuide();
    }

    /// The pavilion vault: four cylinder segments on 4 m walls, rising along the walls to the peak over the centre.
    static CylinderVaultGuide pavilion() {

        CylinderVaultGuide guide;
        guide.cloister = true;
        guide.wall = 4000.0;

        return guide;
    }

    /// The octagon groin vault: eight half barrels of 6 m span and 3 m radius over a regular octagon.
    static CylinderVaultGuide octagon() {

        CylinderVaultGuide guide;
        guide.sides = 8;
        guide.edge = 6000.0;
        guide.radius = 3000.0;

        return guide;
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// CurvedFoldedBeamGuide
// ═══════════════════════════════════════════════════════════════════════════

/// Closed curved-folded beams side by side in mm.
struct CurvedFoldedBeamGuide {
    double span = 12000.0; // Along x between the supports.
    double width = 800.0; // The section's width at the supports, its height half that.
    double rise = 250.0; // How far the top and bottom plates bulge outward at mid-span.
    double sag = 80.0; // How far the side plates sag inward at mid-span, well inside the width.
    double thickness = 27.0; // Every plate and web.
    size_t webs = 5; // Flat webs across every beam at even stations along the span.
    size_t count = 6; // Beams side by side along y, width apart.
    size_t samples = 48; // Rulings each plate is interpolated through.

    /// Six beams of 12 m span side by side, the defaults.
    static CurvedFoldedBeamGuide six_beams() {
        return CurvedFoldedBeamGuide();
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// CornerFanGuide
// ═══════════════════════════════════════════════════════════════════════════

/// A fan of developable strips folded from one bottom corner of a square in mm.
struct CornerFanGuide {
    double size = 6000.0; // The square's side, the corner at the origin.
    double height = 4000.0; // Where the high folds end above the corner.
    size_t strips = 6; // Strips dividing the two far sides, half on each, even.
    double lift = 1200.0; // Every parabola's middle control point above its chord.
    double drop = 800.0; // How much lower every second fold ends.
    double foot = 0.15; // The share of the way up where the strips start, cut short of the corner.
    double thickness = 40.0; // Every strip.
    Point corner = Point(0.0, 0.0, 0.0); // The square's bottom corner the folds start from.
    size_t samples = 64; // Points every inner crease is interpolated through.

    /// Six strips from one corner of a 6 m square, the defaults.
    static CornerFanGuide six_strips() {
        return CornerFanGuide();
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// Cylinder
// ═══════════════════════════════════════════════════════════════════════════

/// One cylinder: a point on its axis, the axis direction and the radius.
struct Cylinder {
    Point point; // A point on the axis.
    Vector direction; // The axis direction.
    double radius; // mm, the cylinder.
};

// ═══════════════════════════════════════════════════════════════════════════
// CylinderCarvingGuide
// ═══════════════════════════════════════════════════════════════════════════

/// A cylinder carved by cylinders in sequence, ((a - b) - c) - d, in mm.
struct CylinderCarvingGuide {
    Cylinder stock = {
        Point(0.0, 0.0, 0.0), Vector(0.0, 0.0, 1.0), 2000.0
    }; // The carved cylinder a, from its point along its direction for length.
    std::vector<Cylinder> carvers = {
        {Point(-1799.8, 2144.9, 6000.0), Vector(-0.046, -0.0386, 1.0), 1500.0}
    }; // The cylinders b, c, d and so on that carve it, in sequence; one cutting 700 mm deep by default.
    double length = 12000.0; // The stock from its point along its axis.
    double thickness = 60.0; // Every plate, behind the carved surface.
    double min_angle = 10.0 * Tolerance::TO_RADIANS; // The least angle two patches meet at.
    bool closed = false; // Whether the carvers must carve all the stock's surface, no gap.
    size_t samples = 128; // Generators every plate is interpolated through.

    /// The stock standing on z moved by shift, carved by the first count of three carvers 40 degrees apart round one side, cutting 700 mm deep, turned off its axis.
    static CylinderCarvingGuide column(size_t count, const Vector& shift) {

        const std::array<double, 3> radii = {1500.0, 1800.0, 1300.0};
        const std::array<double, 3> tilts = {0.06, -0.05, 0.04};

        CylinderCarvingGuide guide;
        guide.stock = {Point(0.0, 0.0, 0.0) + shift, Vector(0.0, 0.0, 1.0), 2000.0};
        guide.carvers.clear();

        for (size_t i = 0; i < count; i++) {
            const double around = (130.0 - 40.0 * static_cast<double>(i)) * Tolerance::TO_RADIANS;
            const Vector out(std::cos(around), std::sin(around), 0.0);
            const Vector across(-std::sin(around), std::cos(around), 0.0);
            const Point axis =
                Point(0.0, 0.0, 6000.0) + shift + out * (guide.stock.radius + radii[i] - 700.0);
            guide.carvers.push_back({axis, Vector(0.0, 0.0, 1.0) + across * tilts[i], radii[i]});
        }

        return guide;
    }

    /// The stock standing on z moved by shift, carved all round by count carvers of the radius at equal steps, cutting depth deep, turned off its axis one way and the other in turn.
    static CylinderCarvingGuide
    ring(size_t count, double radius, double depth, const Vector& shift) {

        CylinderCarvingGuide guide;
        guide.stock = {Point(0.0, 0.0, 0.0) + shift, Vector(0.0, 0.0, 1.0), 2000.0};
        guide.carvers.clear();
        guide.closed = true;

        for (size_t i = 0; i < count; i++) {
            const double around =
                2.0 * Tolerance::PI * static_cast<double>(i) / static_cast<double>(count);
            const Vector out(std::cos(around), std::sin(around), 0.0);
            const Vector across(-std::sin(around), std::cos(around), 0.0);
            const double tilt = i % 2 == 0 ? 0.05 : -0.05;
            const Point axis =
                Point(0.0, 0.0, 6000.0) + shift + out * (guide.stock.radius + radius - depth);
            guide.carvers.push_back({axis, Vector(0.0, 0.0, 1.0) + across * tilt, radius});
        }

        return guide;
    }
};

}
