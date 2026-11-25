#pragma once
#include "src/templates/timber_floor/floor_product.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_timber_floor {

// ═══════════════════════════════════════════════════════════════════════════
// FloorBuildUp
// ═══════════════════════════════════════════════════════════════════════════

/// One layer of a build-up.
struct FloorLayer {
    std::string name;
    std::string printed; // Its size as the book prints it.
    std::string kind; // plate, membrane, joists, ribs, skins, box, surface, spaced, grid, boards, planks, panel, battens or cavity.
    double thickness = 0.0; // mm.
    double width = 0.0; // Width of joists, ribs and battens.
    bool inside = false; // In the height of the layer over it: insulation between joists, sand between battens.
};

/// A whole floor, its layers from the top, and what it was measured or computed to do.
struct FloorBuildUp {
    size_t table = 0; // Table of the book.
    size_t number = 0; // Number in it.
    std::string title;
    std::string name;
    double weight = 0.0; // kg/m2.
    double height = 0.0; // mm, as printed.
    std::string airborne; // R'w, dB.
    std::string impact; // L'n,w, dB.
    std::string energy; // kWh/m2 and year.
    std::string ghg; // kg CO2-eq/m2 and year.
    std::vector<FloorLayer> layers;

    /// Every build-up of data/timber_floors/build_ups.json, or those of one table.
    static std::vector<FloorBuildUp> library(size_t table = 0);
};

// ═══════════════════════════════════════════════════════════════════════════
// FloorBuildUps
// ═══════════════════════════════════════════════════════════════════════════

/// Samples of build-ups in rows, every layer cut back from the one under it.
class FloorBuildUps : public WoodSession {
public:
    static constexpr double LENGTH = 1400.0; // Of a sample along its joists.
    static constexpr double WIDTH = 1340.0; // Of a sample: three joists at 600.
    static constexpr double STEP = 90.0; // A layer ends this much before the one under it.
    static constexpr double GAP = 700.0; // Between two samples.

    const std::vector<FloorBuildUp> build_ups;

    /// The samples as the session named name, in rows of at most count, a table in its own.
    explicit FloorBuildUps(const std::vector<FloorBuildUp>& build_ups, const std::string& name = "floor_build_ups", size_t count = 4);

private:
    /// Adds the sample of a build-up in its base.
    void add_sample(const FloorBuildUp& build_up, const Plane& base);

    /// The structural layer as pieces, length long.
    FloorProduct structure(const FloorLayer& layer, double length) const;
};

}
