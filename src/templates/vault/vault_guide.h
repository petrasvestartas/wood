#pragma once
#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_vault {

// ═══════════════════════════════════════════════════════════════════════════
// ArchGuide
// ═══════════════════════════════════════════════════════════════════════════

/// The inputs of an arch of voussoirs over a span in mm, by default a semicircle of 25 voussoirs.
struct ArchGuide {
    double span = 6000.0; // Between the two springings along x.
    double rise = 3000.0; // Intrados crown above the springings.
    double thickness = 500.0; // Radial depth of every voussoir.
    double depth = 500.0; // Width of the arch along y.
    int voussoirs = 25; // Voussoirs from springing to springing.
};

// ═══════════════════════════════════════════════════════════════════════════
// BarrelGuide
// ═══════════════════════════════════════════════════════════════════════════

/// The inputs of a barrel vault of the arch section along y in mm, by default a segmental barrel of 9 staggered courses.
struct BarrelGuide {
    double span = 6000.0; // Between the two springings along x.
    double rise = 2000.0; // Intrados crown above the springings.
    double thickness = 250.0; // Radial depth of every voussoir.
    double length = 6000.0; // Along y from 0.
    int courses = 9; // Voussoir courses from springing to springing.
    int rings = 6; // Voussoirs along the length per course.
    double stagger = 0.5; // Fraction of a voussoir every other course is shifted by.
    bool closed = false; // A half voussoir more at both ends so every course ends flush.
};

// ═══════════════════════════════════════════════════════════════════════════
// DomeGuide
// ═══════════════════════════════════════════════════════════════════════════

/// The inputs of a spherical dome of staggered hoops in mm, by default 40 meridians and 20 hoops round a 6 degree oculus.
struct DomeGuide {
    double radius = 4000.0; // Intrados radius.
    double bottom = 400.0; // Voussoir depth at the springing.
    double top = 250.0; // Voussoir depth at the oculus.
    int meridians = 40; // Voussoirs per hoop.
    int hoops = 20; // Hoops from the oculus to the springing.
    double oculus = 6.0; // Degrees from the zenith to the oculus.
    double springing = 90.0; // Degrees from the zenith to the springing.
};

// ═══════════════════════════════════════════════════════════════════════════
// WallGuide
// ═══════════════════════════════════════════════════════════════════════════

/// The inputs of a wall of bricks in stretcher bond in mm, by default 4000 long and 2000 high.
struct WallGuide {
    double length = 4000.0; // Along x from 0.
    double height = 2000.0; // Up to.
    double thickness = 200.0; // Along y from 0.
    double brick = 400.0; // Length of a brick.
    double course = 200.0; // Height of a course.
    double stagger = 0.5; // Fraction of a brick every other course is shifted by.
};

// ═══════════════════════════════════════════════════════════════════════════
// CrossVaultGuide
// ═══════════════════════════════════════════════════════════════════════════

/// The inputs of a cross (groin) vault over a square bay in mm, by default a semicircular bay of 10 courses.
struct CrossVaultGuide {
    double span = 6000.0; // Side of the square bay.
    double rise = 3000.0; // Intrados crown above the springings.
    double thickness = 480.0; // Radial depth of every voussoir.
    int courses = 10; // Voussoir courses from the springing to the crown.
    int rings = 12; // Voussoirs along a web per course.
};

// ═══════════════════════════════════════════════════════════════════════════
// CloisterVaultGuide
// ═══════════════════════════════════════════════════════════════════════════

/// The inputs of a cloister (pavilion) vault over a square bay in mm, by default a semicircular bay of 8 courses.
struct CloisterVaultGuide {
    double span = 6000.0; // Side of the square bay.
    double rise = 3000.0; // Intrados crown above the springings.
    double thickness = 300.0; // Radial depth of every voussoir.
    int courses = 8; // Voussoir courses from the springing to the crown.
    int rings = 12; // Voussoirs along a web per course.
};

// ═══════════════════════════════════════════════════════════════════════════
// GothicRibbedVaultGuide
// ═══════════════════════════════════════════════════════════════════════════

/// The inputs of a Gothic ribbed vault bay in mm: the bay, the ribs and their historical pattern.
struct GothicRibbedVaultGuide {
    static constexpr int QUADRIPARTITE = 0; // Classic quadripartite cross vault (Bern Cathedral, Wells, Salisbury, Amiens nave).
    static constexpr int TIERCERON = 1; // Tierceron star vault (Bern Cathedral, Lincoln, Exeter nave).
    static constexpr int STAR_STELLAR = 2; // Four-lozenge stellar vault (Bern Cathedral, Sterngewoelbe, Boveda Estrellada).
    static constexpr int LIERNE_STAR = 3; // Double-star lierne vault (Bern Cathedral, Ely choir, Wells Lady Chapel).
    static constexpr int NET_RETICULATED = 4; // Late Gothic diamond-lattice net vault (Bern Cathedral choir, Parler).
    static constexpr int FAN_CONOID = 5; // Perpendicular radiating fan vault (Gloucester cloisters, King's College).

    double span_x = 10000.0; // Bay along x.
    double span_y = 10000.0; // Bay along y.
    double rise = 1000.0; // Crown boss above the springings.
    double rib_width = 120.0; // Glulam rib width.
    double rib_height = 360.0; // Glulam rib depth.
    int pattern = LIERNE_STAR; // Rib pattern, one of the constants above.
    int subdivisions = 16; // Segments per rib curve, at least 8.

    /// The default bay with the rib pattern.
    static GothicRibbedVaultGuide of_pattern(int pattern) {

        GothicRibbedVaultGuide guide;
        guide.pattern = pattern;

        return guide;
    }
};

}
