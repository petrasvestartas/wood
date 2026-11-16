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

    /// Unit assembly insertion vector of facet_idx (0 = inner layer, 1 = outer layer), pointing along insertion travel.
    Vector plate_insertion_vector(size_t facet_idx, size_t layer) const;

    /// Populates four progressive erection groups (walls, roof bottom, roof top, roof onto walls) for bay_idx.
    void add_erection_sequence_groups(size_t bay_idx = 0);

    /// Total number of rectangular through-mortise hole loops cut across all plates in the pavilion.
    size_t total_mortise_slots() const;

private:
    std::vector<std::shared_ptr<FacePlanarElement>> _inner_plates; // Room-facing CLT layer plates.
    std::vector<std::shared_ptr<FacePlanarElement>> _outer_plates; // Exterior CLT layer plates.
    std::vector<std::shared_ptr<FacePlanarElement>> _all_plates; // Combined plate list (2 * f, 2 * f + 1).
    size_t _total_mortise_slots = 0; // Through-mortise slot count across all plates.

    void build_panels_and_joints();
    void draw_panels();
};

} // namespace wood_vidy
