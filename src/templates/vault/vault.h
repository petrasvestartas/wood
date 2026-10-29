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

/// A star (stellar) vault over a square bay of span on intersecting circular barrel cylinders, crown at rise.
std::vector<std::shared_ptr<Element>>
star_vault(double span, double rise, double thickness, double star, double rib, int subdivisions);

// ═══════════════════════════════════════════════════════════════════════════
// Gothic ribbed vaults (LiDAR cathedral patterns — Tracing the Past / Willis / Bern Cathedral)
// ═══════════════════════════════════════════════════════════════════════════

/// Historical Gothic cathedral rib patterns studied via LiDAR in Tracing the Past and Bern Cathedral (Berner Muenster).
enum class GothicPattern {
    quadripartite,   // Classic quadripartite cross vault (Bern Cathedral / Wells / Salisbury / Amiens Nave)
    tierceron,       // Starcut tierceron star vault (Bern Cathedral / Lincoln / Exeter Nave)
    star_stellar,    // 4-lozenge stellar star vault (Bern Cathedral / Sterngewoelbe / Boveda Estrellada)
    lierne_star,     // Double-star lierne vault (Bern Cathedral / Ely Choir / Wells Lady Chapel)
    net_reticulated, // Late Gothic reticulated diamond-lattice net vault (Bern Cathedral Choir / Parler / Netzgewoelbe)
    fan_conoid,      // Perpendicular radiating fan / conoid vault (Gloucester Cloisters / King's College)
    crazy_vault = star_stellar // Alias kept for compatibility
};

/// Analytical top vault surface that the extrados (top face) of every BeamCurved rib follows flush.
enum class ArchMethod {
    groin_cylinders,    // Two orthogonal circular barrel cylinders intersecting along the bay diagonals
    spherical_sail,     // Alias mapped to cylindrical groin barrels (no doubly-curved patches)
    cloister_cylinders, // Alias mapped to cylindrical groin barrels (no doubly-curved patches)
    fixed_radius = groin_cylinders,
    chord_impost = groin_cylinders
};

/// A Gothic ribbed vault bay of mitered BeamCurved glulam ribs, 4 corner support blocks, and cylindrical web panels.
std::vector<std::shared_ptr<Element>> gothic_ribbed_vault(
    double span_x = 10000.0,
    double span_y = 10000.0,
    double rise = 1000.0,
    double rib_width = 120.0,
    double rib_height = 360.0,
    GothicPattern pattern = GothicPattern::lierne_star,
    ArchMethod surface_method = ArchMethod::groin_cylinders,
    int subdivisions = 16
);

/// A WoodSession template wrapping a Gothic ribbed vault bay of mitered BeamCurved ribs, corner support blocks, and cylindrical web panels.
class GothicRibbedVault : public wood_session::WoodSession {
public:
    double span_x = 10000.0;
    double span_y = 10000.0;
    double rise = 1000.0;
    double rib_width = 120.0;
    double rib_height = 360.0;
    GothicPattern pattern = GothicPattern::lierne_star;
    ArchMethod arch_method = ArchMethod::groin_cylinders;
    int subdivisions = 16;

    GothicRibbedVault(
        double span_x = 10000.0,
        double span_y = 10000.0,
        double rise = 1000.0,
        double rib_width = 120.0,
        double rib_height = 360.0,
        GothicPattern pattern = GothicPattern::lierne_star,
        ArchMethod arch_method = ArchMethod::groin_cylinders,
        int subdivisions = 16,
        const std::string& name = "gothic_ribbed_vault"
    );
};

}
