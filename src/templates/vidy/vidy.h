#pragma once
#include "src/templates/vidy/vidy_guide.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_vidy {

// ═══════════════════════════════════════════════════════════════════════════
// VidyPavilion
// ═══════════════════════════════════════════════════════════════════════════

/// Double-layer folded CLT pavilion built from a VidyGuide with mitered panels and linked Vidy tenon joinery.
class VidyPavilion : public WoodSession {
public:
    const VidyGuide guide; // Underlying Stage 1 base mesh and Stage 2 four-surface miter offset guide.
    const bool with_joints; // Runs native Vidy linked-tenon and shadow-joint solver (types 15 and 16) when true.
    const double corner_chamfer; // Optional corner relief setback along each facet edge in mm (0 = flush miters).
    const double division_length; // Target joint division spacing along fold creases in mm (450 mm).

    /// Builds all double-layer 45 mm CLT panels (Stage 3) and optional native Vidy linked-tenon joinery (Stage 4).
    explicit VidyPavilion(
        const VidyGuide& guide = VidyGuide(1),
        bool with_joints = true,
        double corner_chamfer = 0.0,
        double division_length = 350.0,
        const std::string& name = "vidy_pavilion"
    );

    /// Inner room-side CLT plates (h = 0 to plate_thickness) in guide facet order.
    const std::vector<std::shared_ptr<FacePlanarElement>>& inner_plates() const;

    /// Outer exterior CLT plates (h = total_offset - plate_thickness to total_offset) in guide facet order.
    const std::vector<std::shared_ptr<FacePlanarElement>>& outer_plates() const;

    /// Every CLT plate in the pavilion (inner followed by outer per facet).
    const std::vector<std::shared_ptr<FacePlanarElement>>& all_plates() const;

    /// Total number of through-mortise hole loops cut across all plates by the Vidy joinery solver.
    size_t total_mortise_slots() const;

private:
    std::vector<std::shared_ptr<FacePlanarElement>> _inner_plates; // Room-facing CLT layer plates.
    std::vector<std::shared_ptr<FacePlanarElement>> _outer_plates; // Exterior CLT layer plates.
    std::vector<std::shared_ptr<FacePlanarElement>> _all_plates; // Combined plate list (2 * f, 2 * f + 1).
    size_t _total_mortise_slots = 0; // Through-mortise slot count across all plates.

    void build_panels();
    void draw_panels();
    void solve_vidy_joinery();
};

} // namespace wood_vidy
