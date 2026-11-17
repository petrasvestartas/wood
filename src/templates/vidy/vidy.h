#pragma once
#include "src/templates/vidy/vidy_guide.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_vidy {

// ═══════════════════════════════════════════════════════════════════════════
// VidyPavilion
// ═══════════════════════════════════════════════════════════════════════════

/// Double-layer folded CLT pavilion built from a VidyGuide with corner chamfers, cavity border boards and Vidy joinery.
class VidyPavilion : public WoodSession {
public:
    const VidyGuide guide; // Underlying base mesh and four-surface miter offset guide.
    const bool with_joints; // Generates stepped tenons, situ eave joints and mortises when true.
    const double corner_scale; // Scale multiplier on multi-valent corner chamfers (1.0 = Rhino).
    const double pin_pitch; // Centre-to-centre pitch of stepped tenon pins along creases in mm.

    /// Builds the double-layer 45 mm CLT panels, perpendicular cavity border boards and optional Vidy joinery.
    explicit VidyPavilion(
        const VidyGuide& guide = VidyGuide(11),
        bool with_joints = true,
        double corner_scale = 1.0,
        double pin_pitch = 500.0,
        const std::string& name = "vidy_pavilion"
    );

    /// Not copied, as a session is not.
    VidyPavilion(const VidyPavilion&) = delete;

    /// Not assigned, as it is not copied.
    VidyPavilion& operator=(const VidyPavilion&) = delete;

    /// Inner room-side CLT plates (h = 0 to plate_thickness) in guide facet order.
    const std::vector<std::shared_ptr<FacePlanarElement>>& inner_plates() const;

    /// Outer exterior CLT plates (h = total_offset - plate_thickness to total_offset) in guide facet order.
    const std::vector<std::shared_ptr<FacePlanarElement>>& outer_plates() const;

    /// All main folded inner and outer CLT shell plates (inner followed by outer per facet).
    const std::vector<std::shared_ptr<FacePlanarElement>>& shell_plates() const;

    /// Perpendicular cavity-closing border boards along BaySeparation, GableEnd and Ground edges.
    const std::vector<std::shared_ptr<FacePlanarElement>>& border_plates() const;

    /// Perpendicular cavity-closing border boards belonging to facet_idx.
    const std::vector<std::shared_ptr<FacePlanarElement>>&
    border_plates_for_facet(size_t facet_idx) const;

    /// Every CLT plate in the pavilion (main shell plates plus cavity-closing border boards).
    const std::vector<std::shared_ptr<FacePlanarElement>>& all_plates() const;

    /// Adds the six-step module assembly sequence (module_1_haunch_wings to module_6_roof_onto_walls).
    void add_module_assembly_groups(size_t bay_idx = 5, double roof_lift = 1650.0);

    /// Total number of jointed double-layer Regular crease seams across the pavilion.
    size_t total_jointed_seams() const;

    /// Total number of stepped cavity-bridging Double_Pins across the pavilion.
    size_t total_double_pins() const;

    /// Total number of Single_Pins and Extend_Pins across the pavilion.
    size_t total_single_pins() const;

    /// Total number of integral through-tenon pins (Double_Pins + Single_Pins + Extend_Pins).
    size_t total_pins() const;

    /// Total number of rectangular through-mortise hole loops cut across all plates.
    size_t total_mortise_slots() const;

private:
    std::vector<std::shared_ptr<FacePlanarElement>> _inner_plates; // Room-facing CLT layer plates.
    std::vector<std::shared_ptr<FacePlanarElement>> _outer_plates; // Exterior CLT layer plates.
    std::vector<std::shared_ptr<FacePlanarElement>> _shell_plates; // Main folded shell plates.
    std::vector<std::shared_ptr<FacePlanarElement>> _border_plates; // Cavity-closing border boards.
    std::vector<std::vector<std::shared_ptr<FacePlanarElement>>>
        _facet_border_plates; // Border boards indexed by facet f.
    std::vector<std::shared_ptr<FacePlanarElement>> _all_plates; // Shell and border plates.
    size_t _total_jointed_seams = 0; // Jointed double-layer Regular seam count.
    size_t _total_double_pins = 0; // Stepped Double_Pin count.
    size_t _total_single_pins = 0; // Single_Pin and Extend_Pin count.
    size_t _total_mortise_slots = 0; // Through-mortise slot count across all plates.

    /// Builds the inner and outer chamfered CLT shell plates with optional tenon and mortise joinery.
    void compute_shell_plates();

    /// Builds the perpendicular cavity-closing border boards on all non-Regular facet edges.
    void compute_border_plates();

    /// Adds the roof haunch and inner crown assembly steps 1..3 of bay b.
    void add_haunch_assembly_steps(size_t b);

    /// Adds the outer roof cover and border board step 4 of bay b.
    void add_cover_assembly_step(size_t b);

    /// Adds the vertical wall and roof-lowering steps 5..6 of bay b to the session tree.
    void add_wall_assembly_steps(size_t b, double roof_lift);

    /// Adds all inner and outer shell plates and cavity border boards to their session groups.
    void add_plates();
};

}
