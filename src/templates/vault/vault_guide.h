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

// ═══════════════════════════════════════════════════════════════════════════
// TileVaultGuide
// ═══════════════════════════════════════════════════════════════════════════

/// Inputs of a field-guided or geodesic-course free-form brick/tile shell (Du et al. IASS 2023; Vaxman Directional; Adiels et al. IASS 2017).
struct TileVaultGuide {
    static constexpr int STAGGERED = 0; // Penalizes parallel adjacent brick joints in 2x2 quad cycles (Eq. 2).
    static constexpr int RUNNING_BOND = 1; // Rewards 1-quad shifted parallel courses while penalizing stack joints (Eq. 3).
    static constexpr int HERRINGBONE = 2; // Rewards 90-degree stepped perpendicular L-pairs (Section 3.4).
    static constexpr int GEODESIC_COURSES = 3; // Constant-bed-height geodesic coordinate courses with running-bond bricks (Adiels et al. 2017).

    static constexpr int STANDARD_TILES = 0; // Constant-size rectangular tiles 2h x h x t with trimmed boundary half-tiles.
    static constexpr int MERGED_BLOCKS = 1; // Merged brick polygons extruded into watertight planar-sided blocks.

    Mesh shell; // Input free-form shell surface triangle/polygon mesh in mm.
    int pattern = RUNNING_BOND; // Brick bond / topology mode (STAGGERED, RUNNING_BOND, HERRINGBONE, GEODESIC_COURSES).
    int materialisation = MERGED_BLOCKS; // Geometric output mode (STANDARD_TILES or MERGED_BLOCKS).
    double tile_width = 180.0; // Target quad edge or geodesic bed-joint height h in mm (full brick length is 2 * tile_width).
    double thickness = 90.0; // Tile or block thickness in mm.
    double mortar_gap = 4.0; // Clearance gap around constant-size standard tiles in mm.
    double orientation_degrees = 0.0; // Rotation offset from principal curvature directions in degrees (e.g. 0 or 45).
    int shapeup_iterations = 50; // ShapeUp local-global squareness regularization iterations (0 disables ShapeUp).
    double shapeup_closeness = 0.06; // Soft closeness weight retaining the quad mesh near the input shell.
    double shapeup_smoothness = 0.02; // Laplacian fairness weight during ShapeUp regularization.
    double penalty_weight = 0.35; // BIP penalty weight w_P against parallel stack-bond pairs.
    double reward_weight = 0.30; // BIP reward weight w_R / w_H for running-bond or herringbone pairs.
    double border_weight = 0.60; // BIP matching weight (< 1) on boundary quads so cut half-tiles concentrate at borders.
    bool align_boundary = true; // Aligns the 4-RoSy orientation field to naked shell boundary curves.
    bool polar_geodesics = true; // Seeds GEODESIC_COURSES from inner oculus holes (polar geodesic coordinates) when present.
    bool planar_sides = true; // Planarizes shared side-joint quads in MERGED_BLOCKS mode.
    bool trim_base = false; // Clips springing blocks along z = base_z when true.
    double base_z = 0.0; // Horizontal springing datum plane z in mm.

    /// Loads the 3.0 m x 3.0 m x 1.22 m four-corner funicular vault shell (materialisation_cm.3dm).
    static Mesh materialisation_vault_shell();

    /// Loads the 5.12 m x 6.27 m x 2.55 m cantilevered three-support free-form shell (materialisation.3dm).
    static Mesh materialisation_tripod_shell();

    /// Running-bond planar-sided brick blocks on the 3 m funicular vault shell.
    static TileVaultGuide funicular_running_blocks();

    /// Constant-size standard rectangular tiles in running bond on the 3 m funicular vault shell.
    static TileVaultGuide funicular_standard_tiles();

    /// Herringbone brick blocks on the 3 m funicular vault shell at 45-degree field orientation.
    static TileVaultGuide funicular_herringbone_blocks();

    /// Polar geodesic-coordinate constant-bed-height running-bond brick blocks around the oculus (Adiels et al. 2017).
    static TileVaultGuide funicular_geodesic_blocks();

    /// Staggered free-form brick blocks on the 5 m cantilevered three-support tripod shell.
    static TileVaultGuide tripod_staggered_blocks();

    /// Running-bond constant-size rectangular tiles on the 5 m cantilevered three-support tripod shell.
    static TileVaultGuide tripod_running_tiles();

    /// Herringbone constant-size tiles on the 5 m cantilevered three-support tripod shell.
    static TileVaultGuide tripod_herringbone_tiles();

    /// Arch-seeded geodesic-coordinate constant-bed-height running-bond brick blocks on the 5 m tripod shell (Adiels et al. 2017).
    static TileVaultGuide tripod_geodesic_blocks();
};

// ═══════════════════════════════════════════════════════════════════════════
// CalgaryVaultGuide
// ═══════════════════════════════════════════════════════════════════════════

/// Inputs of the Calgary 2025 boundary-inclusive trivalent dual & variable-thickness stereotomic shell.
struct CalgaryVaultGuide {
    Mesh shell; // Base thrust-network shell mesh in mm (defaults to calgary_shell_mesh.json).
    double target_edge = 0.0; // Isotropic triangle remesh edge length in mm (0 uses mean input edge length).
    int remesh_iterations = 35; // Boundary-preserving isotropic remeshing passes (002_remesh_compas.py).
    double lower_pier_thickness = 300.0; // Shell thickness at lower springing supports in mm (009: 30 cm).
    double upper_pier_thickness = 200.0; // Shell thickness at upper hillside supports in mm (009: 20 cm).
    double arch_midspan_thickness = 150.0; // Shell thickness at free-arch midspans in mm (009: 15 cm).
    double crown_thickness = 100.0; // Shell thickness at the five highest crown vertices in mm (009: 10 cm).
    double chamfer_max_angle_deg = 145.0; // Maximum trivalent interior corner angle to chamfer in degrees (011).
    double chamfer_offset = 0.0; // Interior trivalent corner chamfer offset in mm (0 disables chamfering).
    double bevel_degrees = 0.0; // Optional alternating interlocking side-plane bevel in degrees.
    bool smooth_arch_borders = true; // Smooths arch boundary chains via interpolated NURBS curves (006).
    bool flatten_intrados = false; // Keeps intrados vertices flush across trivalent corners when false (010).
    bool planar_sides = true; // Planarizes shared side rulings before flat-top intersection.
    bool trim_supports = true; // Clips support pier blocks horizontally at z = 0 mm and z = 1500 mm.

    /// Loads the 9.46 m x 11.17 m x 3.00 m four-pier asymmetric Calgary 2025 thrust shell.
    static Mesh calgary_shell();

    /// Variable-thickness flat-top & flush-intrados trivalent dual blocks (000_model/010_blocks.py).
    static CalgaryVaultGuide variable_thickness_blocks();

    /// Variable-thickness trivalent dual blocks with chamfered interior corners (000_model/011_chamfer.py).
    static CalgaryVaultGuide chamfered_blocks();

    /// Variable-thickness trivalent dual blocks with alternating interlocking side-plane bevels.
    static CalgaryVaultGuide interlocking_bevel_blocks();
};

}
