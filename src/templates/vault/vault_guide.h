#pragma once
#include "wood_session.h"

using namespace laika_cpp;
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

// ═══════════════════════════════════════════════════════════════════════════
// SurfaceVaultGuide
// ═══════════════════════════════════════════════════════════════════════════

/// Inputs of a NURBS / sewn-BRep NGon block vault with planarized side joints and optional shifted top/bottom facets.
struct SurfaceVaultGuide {
    static constexpr int QUADS = 0; // Quad grid (staggered when parameter > 0).
    static constexpr int DIAMONDS = 1; // Rhombic diamond grid merged across surface seams.
    static constexpr int HEXAGONS = 2; // Running-bond hexagonal NGon grid merged across surface seams.
    static constexpr int DUAL_HEXAGONS = 3; // Trivalent dual hexagonal grid across multi-patch singularities.
    static constexpr int BRICKS = 4; // Variable-count staggered courses sized by arc length.
    static constexpr int CHEBYSHEV = 5; // Equal-edge-length Chebyshev compass net.

    std::vector<NurbsSurface> surfaces; // One or multiple four-sided NURBS patches sewn along shared seams.
    int subdivision = HEXAGONS; // Subdivision pattern constant above.
    double size_u = 600.0; // Target cell step along u in mm (or edge length for Chebyshev / Bricks).
    double size_v = 600.0; // Target cell step along v in mm.
    double parameter = 0.28; // Pattern parameter: hex side ratio, quad/brick stagger, or Chebyshev angle (deg).
    double thickness = 240.0; // Block extrusion thickness in mm.
    bool planar_sides = true; // Planarizes all shared side-joint quads when true.
    double flatten_caps = 1.0; // Projects top/bottom caps onto block planes along planarized corner lines (0..1).
    Vector facet_shift = Vector(0.0, 0.0, 0.0); // Tangential shift between extrados and intrados facets in mm.
    double bevel_degrees = 0.0; // Alternating side-plane bevel angle in degrees for interlocking shifted facets.
    bool trim_base = false; // Cuts springing blocks flush along z = base_z when true.
    double base_z = 0.0; // Horizontal springing datum plane z in mm.

    /// Hexagonal planar-sided saddle-arch vault (NGon running-bond hex extrusion + planarization + ProjectPairs).
    static SurfaceVaultGuide hex_saddle_vault();

    /// Diamond planar-sided annular / toroidal arch vault.
    static SurfaceVaultGuide diamond_torus_vault();

    /// Four-patch sewn groin vault with diamonds/hexagons merged seamlessly across the four groin seams.
    static SurfaceVaultGuide sewn_groin_vault();

    /// Multi-patch sewn funnel canopy vault subdivided into isotropic dual hexagonal blocks across radial seams.
    static SurfaceVaultGuide sewn_funnel_vault();

    /// Variable-course staggered masonry funnel / catenoid vault adapting brick counts to hoop perimeter.
    static SurfaceVaultGuide courses_funnel_vault();

    /// Equal-edge Chebyshev compass-net vault over a doubly-curved pillow dome at 45 degrees.
    static SurfaceVaultGuide chebyshev_dome_vault();

    /// Hexagonal arch vault with tangentially shifted top and bottom facets and planarized side joints.
    static SurfaceVaultGuide shifted_hex_vault();

    /// Interlocking hexagonal dome with alternating bevelled side planes shifting top vs bottom facets.
    static SurfaceVaultGuide bevelled_hex_dome();

    /// Sewn groin vault with tangentially shifted diamond extrados and intrados facets.
    static SurfaceVaultGuide shifted_diamond_groin();

    /// Chebyshev equal-edge dome vault with tangentially shifted top and bottom facets.
    static SurfaceVaultGuide shifted_chebyshev_dome();
};

}
