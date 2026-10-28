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

// ═══════════════════════════════════════════════════════════════════════════
// Gothic ribbed vaults (LiDAR cathedral patterns — Tracing the Past / Willis)
// ═══════════════════════════════════════════════════════════════════════════

/// Historical Gothic cathedral rib patterns studied via LiDAR in Tracing the Past (Buchanan, Webb, Peterson, Hillson).
enum class GothicPattern {
    quadripartite,   // Early English quadripartite bay (Wells / Salisbury / Amiens Nave)
    tierceron,       // English Decorated Starcut tierceron vault (Lincoln / Exeter Nave)
    star_stellar,    // Stellar 8-pointed star vault (Sterngewoelbe / Boveda Estrellada — Gdansk / Burgos)
    lierne_star,     // English Decorated double-star lierne vault (Ely Choir / Wells Lady Chapel)
    net_reticulated, // Late Gothic diamond-lattice net vault (Netzgewoelbe — Prague / Vienna / Canterbury)
    fan_conoid,      // English Perpendicular fan / conoid vault (Gloucester Cloisters / King's College)
    crazy_vault = star_stellar // Alias kept for compatibility
};

/// Analytical top vault surface that the extrados (top face) of every BeamCurved rib follows flush.
enum class ArchMethod {
    spherical_sail,     // Spherical sail surface through the 4 corners with crown at rise
    groin_cylinders,    // Two orthogonal circular barrel cylinders intersecting along the bay diagonals
    cloister_cylinders, // Four cylindrical webs rising inward from the four perimeter walls
    fixed_radius = spherical_sail,
    chord_impost = groin_cylinders
};

/// A Gothic ribbed vault bay of mitered BeamCurved glulam ribs and 4 corner support blocks, with rib top faces flush on the vault surface.
std::vector<std::shared_ptr<Element>> gothic_ribbed_vault(
    double span_x = 10000.0,
    double span_y = 10000.0,
    double rise = 1000.0,
    double rib_width = 120.0,
    double rib_height = 180.0,
    GothicPattern pattern = GothicPattern::lierne_star,
    ArchMethod surface_method = ArchMethod::spherical_sail,
    int subdivisions = 16
);

/// A WoodSession template wrapping a Gothic ribbed vault bay of mitered BeamCurved ribs and corner support blocks.
class GothicRibbedVault : public wood_session::WoodSession {
public:
    double span_x = 10000.0;
    double span_y = 10000.0;
    double rise = 1000.0;
    double rib_width = 120.0;
    double rib_height = 180.0;
    GothicPattern pattern = GothicPattern::lierne_star;
    ArchMethod arch_method = ArchMethod::spherical_sail;
    int subdivisions = 16;

    GothicRibbedVault(
        double span_x = 10000.0,
        double span_y = 10000.0,
        double rise = 1000.0,
        double rib_width = 120.0,
        double rib_height = 180.0,
        GothicPattern pattern = GothicPattern::lierne_star,
        ArchMethod arch_method = ArchMethod::spherical_sail,
        int subdivisions = 16,
        const std::string& name = "gothic_ribbed_vault"
    );
};

}
