#pragma once
#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_timber_floor {

/// A section's corners: across and up for a part along the span, along and up for one across it, along and across for an upright one.
using FloorSection = std::vector<std::array<double, 2>>;

// ═══════════════════════════════════════════════════════════════════════════
// FloorPart
// ═══════════════════════════════════════════════════════════════════════════

/// One straight piece of a floor product, in mm from its corner at its underside.
struct FloorPart {
    std::string name;
    FloorSection section;
    double start = 0.0; // Where it starts along its run.
    double end = 0.0; // Where it ends.
    size_t run = 0; // 0 along the span, 1 across it, 2 upright.
    std::vector<std::string> pockets; // The parts it is let into.
};

/// Pins through a floor product: their axes and the parts they are bored into.
struct FloorPins {
    std::string name;
    std::vector<std::array<std::array<double, 3>, 2>> axes;
    double radius = 4.0;
    std::vector<std::string> into; // The parts they are bored into.
};

// ═══════════════════════════════════════════════════════════════════════════
// FloorProduct
// ═══════════════════════════════════════════════════════════════════════════

/// A floor system as its pieces: what the timber floor chapter shows and what producers make, a sample of it.
struct FloorProduct {
    double span = 0.0; // Along the span, x.
    double width = 0.0; // Across it, y.
    std::vector<FloorPart> parts; // In the order they go together.
    std::vector<FloorPins> pins;

    /// Depth from the lowest to the highest corner.
    double depth() const;

    // ═══════════════════════════════════════════════════════════════════════════
    // Joists, ribs and boxes
    // ═══════════════════════════════════════════════════════════════════════════

    /// Joists under boards.
    static FloorProduct joists(double width = 140.0, double depth = 200.0, double spacing = 600.0);

    /// Ribs glued to one panel, over them or under them.
    static FloorProduct ribbed(bool below = false, double width = 80.0, double depth = 220.0, double spacing = 625.0, double panel = 27.0);

    /// Ribs glued between two panels.
    static FloorProduct box(double width = 60.0, double depth = 180.0, double spacing = 600.0, double panel = 27.0);

    /// A cross-laminated plate on glulam ribs, or two with the ribs between.
    static FloorProduct cross_laminated_ribs(bool closed, double plate, double width, double depth, double spacing, double element);

    /// Best wood Schneider CLT BOX: glulam ribs and cross-laminated flanges 60, kind 0 closed, 1 open above, 2 the roof's.
    static FloorProduct clt_box(size_t kind = 0, double depth = 240.0, double rib = 80.0);

    /// Novatop Element: three-layer panels 27 over and under ribs 27 both ways, every 340 along.
    static FloorProduct novatop(double depth = 240.0);

    /// Novatop Open: ribs 60 every 625 on a three-layer panel 27.
    static FloorProduct novatop_open(double rib_depth = 220.0);

    /// Lignatur box element LKE: a hollow box per 200, tongue in groove.
    static FloorProduct lignatur_box(double depth = 200.0);

    /// Lignatur surface element LFE: four cells per 1000.
    static FloorProduct lignatur_surface(double depth = 200.0);

    /// Lignotrend Rippe Q3: ribs of boards lying on each other, crossed now and then, on a closed layer.
    static FloorProduct lignotrend_rib(double depth = 190.0);

    /// Lignotrend Block Q3: a closed box of the same boards.
    static FloorProduct lignotrend_block(double depth = 190.0);

    /// Stora Enso LVL rib: veneer lumber ribs 51 every 735 under a panel 27, a strip 300 x 36 under each when semi-open.
    static FloorProduct lvl_rib(bool strips, double depth = 357.0);

    /// Ribs every spacing under a panel, count of them; a strip under each when it has a width, a panel when it reaches the next.
    static FloorProduct ribs(
        double span, size_t count, double spacing, double width, double depth, double panel, double strip = 0.0, double strip_depth = 0.0
    );

    /// VMG Lignum joists: flanges width x 39 of veneer lumber, a web 10 of particle board, under a deck 22.
    static FloorProduct i_joists(double span, size_t count, double spacing, double width, double depth);

    /// Pollmeier BauBuche: GL75 joists 120 every 850 under BauBuche Q 40, as deep as the span and the loads ask.
    static FloorProduct baubuche(double span, double g = 3.0, double q = 3.0);

    /// Metsa Kerto-Ripa: ribs every 612 under Kerto-Q 25, a strip 300 x 43 under each when boxed, as deep as asked.
    static FloorProduct kerto_ripa(double span, bool boxed, double g = 1.9, double q = 2.0);

    /// Lignatur surface elements as deep as the span and the loads ask.
    static FloorProduct lignatur(double span, double g = 1.0, double q = 2.8);

    /// Schneider CLT BOX, the least depth and then the least rib.
    static FloorProduct clt_box_spanning(double span, double g = 2.5, double q = 3.0);

    /// VMG Lignum beams of a grade every metre under boards 25, the lightest of the stock.
    static FloorProduct vmg_beams(double span, size_t grade = 48, double g = 1.0, double q = 2.0);

    /// VMG Lignum joists every spacing, the lightest of the stock.
    static FloorProduct vmg_joists(double span, double spacing = 600.0, double g = 1.0, double q = 2.0);

    /// Derix void panel: two cross-laminated plates with strips of a core between them.
    static FloorProduct void_panel(double plate = 80.0, double core = 120.0);

    /// Holz100 and the like: layers of boards and squared timber held by upright beech dowels, no glue.
    static FloorProduct dowelled_layers();

    // ═══════════════════════════════════════════════════════════════════════════
    // Solid timber
    // ═══════════════════════════════════════════════════════════════════════════

    /// Solid beams butted under a board; with a comb of two tongues instead.
    static FloorProduct planks(bool comb, double depth = 160.0, double plank = 200.0);

    /// Glulam elements on edge with a strip in their joint.
    static FloorProduct glulam_elements(double depth = 160.0, double lamination = 40.0);

    /// Boards on edge held by dowels, or by nails.
    static FloorProduct laminated(bool nailed, double depth = 160.0, double board = 40.0);

    /// Cross-laminated layers from the bottom, the odd ones along the span; joint 0 none, 1 cover board and strip, 2 grout, 3 wedge keys, 4 shear blocks.
    static FloorProduct cross_laminated(const std::vector<double>& layers, size_t joint = 0);

    /// Veneer lumber panel.
    static FloorProduct veneer(double depth = 75.0);

    // ═══════════════════════════════════════════════════════════════════════════
    // Timber and concrete
    // ═══════════════════════════════════════════════════════════════════════════

    /// Boards on edge of two depths, the concrete between the deep ones.
    static FloorProduct composite_stepped(double depth = 160.0, double step = 40.0, double concrete = 80.0);

    /// Boards on edge with small notches over their whole top.
    static FloorProduct composite_micro(double depth = 160.0, double concrete = 80.0);

    /// Boards on edge with deep notches and pins in them.
    static FloorProduct composite_notched(double depth = 160.0, double concrete = 100.0);

    /// Joists with pockets for the lugs of a precast slab.
    static FloorProduct composite_lugs();

    /// Joists and boards with crossed pins.
    static FloorProduct composite_crossed();

    /// Boards on edge with flat steel locks let in across them.
    static FloorProduct composite_locks();

    /// Joists with bars let in along them.
    static FloorProduct composite_bars();

    /// Cross-laminated timber with concrete glued on.
    static FloorProduct composite_glued();

    /// Cree, ZECH: glulam ribs 240 x 280, paired in the middle, under concrete 100 let into their notches.
    static FloorProduct composite_ribs();

    /// The floor with all the layers on it as one, set back from the front, 0 for none.
    FloorProduct layered(double layers) const;

    /// The floor with its parts along the whole span as long as a new one.
    FloorProduct spanning(double length) const;

    /// Adds a box from low to high as a part along the span.
    FloorPart& add_box(const std::string& name, const std::array<double, 3>& low, const std::array<double, 3>& high);

    /// Adds a part along the span of a section.
    FloorPart& add_along(const std::string& name, const FloorSection& section);
};

// ═══════════════════════════════════════════════════════════════════════════
// FloorProducts
// ═══════════════════════════════════════════════════════════════════════════

/// Floor products side by side, their sections to the front.
class FloorProducts : public WoodSession {
public:
    static constexpr double GAP = 900.0; // Between two products.

    const std::vector<std::pair<std::string, FloorProduct>> products; // Every product's name and pieces.
    const double layers; // All the layers on every product together, as one.

    /// Every product as the session named name, in rows of count.
    explicit FloorProducts(
        const std::vector<std::pair<std::string, FloorProduct>>& products,
        const std::string& name = "floor_products",
        size_t count = 4,
        double layers = 0.0
    );

private:
    /// Adds a product in its base, its parts and what is let into them, its pins.
    void add_product(const std::string& name, const FloorProduct& product, const Plane& base);
};

/// Adds the parts and pins of a product in base to session under group, every name prefixed; x of base is along the span.
void add_floor_product(
    WoodSession& session, const std::string& name, const FloorProduct& product, const Plane& base, const std::shared_ptr<TreeNode>& group
);

}
