#pragma once
#include "src/templates/vidy/vidy_guide.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_vidy {

// ═══════════════════════════════════════════════════════════════════════════
// VidyPavilion
// ═══════════════════════════════════════════════════════════════════════════

/// Double-layer folded CLT pavilion built from a VidyGuide with non-colliding corner chamfers and Vidy DTTJ joinery.
class VidyPavilion : public WoodSession {
public:
    const VidyGuide guide; // Underlying Stage 1 base mesh and Stage 2 four-surface miter offset guide.
    const bool with_joints; // Generates stepped Double_Pins, Single_Pins, situ +Z eave tenons and mortises when true.
    const double corner_scale; // Scale multiplier on Robeller's angle-proportional corner chamfers (1.0 = exact Rhino).
    const double pin_pitch; // Centre-to-centre pitch of stepped Double_Pins and Single_Pins along creases in mm (500 mm).

    /// Builds all double-layer 45 mm CLT panels (Stage 3) and optional full-pavilion Vidy joinery (Stage 4).
    explicit VidyPavilion(
        const VidyGuide& guide = VidyGuide(11),
        bool with_joints = true,
        double corner_scale = 1.0,
        double pin_pitch = 500.0,
        const std::string& name = "vidy_pavilion"
    );

    /// Inner room-side CLT plates (h = 0 to plate_thickness) in guide facet order.
    const std::vector<std::shared_ptr<FacePlanarElement>>& inner_plates() const;

    /// Outer exterior CLT plates (h = total_offset - plate_thickness to total_offset) in guide facet order.
    const std::vector<std::shared_ptr<FacePlanarElement>>& outer_plates() const;

    /// Every CLT plate in the pavilion (inner followed by outer per facet).
    const std::vector<std::shared_ptr<FacePlanarElement>>& all_plates() const;

    /// Copies a jointed plate with a fresh GUID and translates its stock and feature loops by delta.
    static std::shared_ptr<FacePlanarElement> copy_plate(
        const FacePlanarElement& src,
        const Vector& delta = Vector(0.0, 0.0, 0.0)
    );

    /// Adds the 6-step module assembly sequence (module_1_haunch_wings .. module_6_roof_onto_walls).
    void add_module_assembly_groups(size_t bay_idx = 5, double roof_lift = 1650.0);

    /// Total number of jointed double-layer Regular crease seams (16 per bay, 176 across 11 bays).
    size_t total_jointed_seams() const;

    /// Total number of stepped cavity-bridging Double_Pins across the pavilion.
    size_t total_double_pins() const;

    /// Total number of Single_Pins and Extend_Pins across the pavilion.
    size_t total_single_pins() const;

    /// Total number of integral through-tenon pins (Double_Pins + Single_Pins + Extend_Pins).
    size_t total_pins() const;

    /// Total number of rectangular through-mortise hole loops cut across all plates in the pavilion.
    size_t total_mortise_slots() const;

private:
    std::vector<std::shared_ptr<FacePlanarElement>> _inner_plates; // Room-facing CLT layer plates.
    std::vector<std::shared_ptr<FacePlanarElement>> _outer_plates; // Exterior CLT layer plates.
    std::vector<std::shared_ptr<FacePlanarElement>> _all_plates; // Combined plate list (2 * f, 2 * f + 1).
    size_t _total_jointed_seams = 0; // Jointed double-layer Regular seam count.
    size_t _total_double_pins = 0; // Stepped Double_Pin count.
    size_t _total_single_pins = 0; // Single_Pin + Extend_Pin count.
    size_t _total_mortise_slots = 0; // Through-mortise slot count across all plates.

    void build_panels_and_joints();
    void draw_panels();
};

} // namespace wood_vidy
