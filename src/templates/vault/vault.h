#pragma once
#include "wood_session.h"
#include "wood_profile.h"

using namespace session_cpp;

/// Masonry vault templates.
namespace wood_vault {

// ═══════════════════════════════════════════════════════════════════════════
// Arches and barrels
// ═══════════════════════════════════════════════════════════════════════════

/// An arch of voussoirs over span.
std::vector<std::shared_ptr<Element>>
arch(double span, double rise, double thickness, double depth, int voussoirs);

/// A barrel vault of the arch section along y from 0 to length.
std::vector<std::shared_ptr<Element>> barrel(
    double span,
    double rise,
    double thickness,
    double length,
    int courses,
    int rings,
    double stagger = 0.5,
    bool closed = false
);

// ═══════════════════════════════════════════════════════════════════════════
// Domes and walls
// ═══════════════════════════════════════════════════════════════════════════

/// A spherical dome of intrados radius.
std::vector<std::shared_ptr<Element>> dome(
    double radius,
    double bottom,
    double top,
    int meridians,
    int hoops,
    double oculus,
    double springing
);

/// A wall of bricks along x from 0 to length, up to height, thickness along y from 0.
std::vector<std::shared_ptr<Element>> wall(
    double length,
    double height,
    double thickness,
    double brick,
    double course,
    double stagger = 0.5
);

// ═══════════════════════════════════════════════════════════════════════════
// Vaults over a square bay
// ═══════════════════════════════════════════════════════════════════════════

/// A cross (groin) vault over a square bay.
std::vector<std::shared_ptr<Element>>
cross_vault(double span, double rise, double thickness, int courses, int rings);

/// A cloister (pavilion) vault over a square bay of span.
std::vector<std::shared_ptr<Element>>
cloister_vault(double span, double rise, double thickness, int courses, int rings);

/// A star (stellar) vault over a square bay of span on the sail surface through its corners, crown at rise.
std::vector<std::shared_ptr<Element>>
star_vault(double span, double rise, double thickness, double star, double rib, int subdivisions);

}
