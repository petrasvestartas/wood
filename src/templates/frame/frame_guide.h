#pragma once

#include "wood_session.h"
#include "wood_profile.h"

namespace wood_frame {

using session_cpp::Line;
using session_cpp::Mesh;
using session_cpp::Plane;
using session_cpp::Point;
using session_cpp::Polyline;
using session_cpp::Vector;
using session_cpp::Xform;

// ═══════════════════════════════════════════════════════════════════════════
// Guide Primitives
// ═══════════════════════════════════════════════════════════════════════════

/// Pinned steel-and-rod base shoe under a frame leg.
struct SupportSpec {
    Plane plane = Plane::xy_plane(); // Base plane on the foundation.
    double base_size = 180.0; // Base plate width.
    double head_diameter = 130.0; // Head plate diameter.
    double height = 110.0; // Total shoe height from foundation to timber foot.
};

/// Straight or variable-section timber bar of a frame (leg, rafter, chord, post, purlin).
struct MemberSpec {
    Line axis; // Reference axis from start section centroid to end section centroid.
    std::vector<Polyline> sections; // Cross-section loops along axis (1 for prismatic, >=2 for variable).
    Vector up = Vector(0.0, 0.0, 1.0); // Section up vector when prismatic.
    std::vector<Plane> cuts; // End miter and cheek trim planes.
    std::vector<Mesh> subtractions; // Complementary half-lap cutters at articulated hinges.
    std::string family = "beam"; // Element name and group family: column, rafter, chord, post, purlin.
    int stage = 0; // Assembly erection stage from bottom up.
};

/// Planar CLT floor, ceiling, or shear-wall plate of a cellular box frame (`Plattenrahmen`).
struct PlateSpec {
    Polyline bottom; // Outer bottom loop.
    Polyline top; // Outer top loop.
    std::vector<Polyline> holes_bottom; // Optional interior cutout loops on bottom face.
    std::vector<Polyline> holes_top; // Optional interior cutout loops on top face.
    std::string family = "slab"; // Element name: slab or wall.
    int stage = 0; // Assembly erection stage from bottom up.
};

// ═══════════════════════════════════════════════════════════════════════════
// Portal, Gable, Articulated & Tiered Frame Designs (Engel §3.2, pp. 186-193)
// ═══════════════════════════════════════════════════════════════════════════

/// Multi-bay two-hinged portal hall (`Zweigelenkrahmen`) with base shoes, tapered legs, 45-degree knee miters, and haunched rafters.
struct TwoHingedPortal {
    double span = 14400.0;
    double height = 5200.0;
    int bents = 4;
    double bay_spacing = 4800.0;
    double width = 280.0;
    double foot_depth = 260.0;
    double knee_depth = 640.0;
    double mid_depth = 380.0;
    double purlin_spacing = 2400.0;
};

/// Multi-bay pitched three-hinged gable hall (`Dreigelenk-Giebelrahmen`) tapering to base pins and ridge crown hinge.
struct ThreeHingedGable {
    double span = 15600.0;
    double eaves_height = 4400.0;
    double ridge_height = 7200.0;
    int bents = 4;
    double bay_spacing = 4800.0;
    double width = 280.0;
    double foot_depth = 260.0;
    double knee_depth = 660.0;
    double crown_depth = 260.0;
    double purlin_spacing = 2200.0;
};

/// Two-bay articulated portal hall coupling outer L-frames to a central T-frame (`T-Rahmen`) via sloped Gerber hinges (Engel p174-15).
struct ArticulatedTFrame {
    double bay_span = 9000.0;
    double height = 5000.0;
    double hinge_offset = 3000.0;
    int bents = 4;
    double bay_spacing = 4600.0;
    double width = 280.0;
    double foot_depth = 260.0;
    double knee_depth = 620.0;
    double hinge_depth = 320.0;
    double lap_length = 260.0;
};

/// Portal hall on doubled interior columns carrying wide symmetric tapered cantilevers (Engel p191-04/05).
struct DoubledColumnCantilever {
    double spine_span = 2800.0;
    double cantilever = 7600.0;
    double height = 5000.0;
    int bents = 4;
    double bay_spacing = 4600.0;
    double width = 280.0;
    double foot_depth = 280.0;
    double knee_depth = 680.0;
    double tip_depth = 260.0;
    double purlin_spacing = 2400.0;
};

/// Multi-storey coupled cantilevered two-hinged portal frames meeting at central corridor hinges (Engel p188-01 top).
struct CoupledTwoHinged {
    double hall_span = 8000.0;
    double corridor_cantilever = 1800.0;
    double storey_height = 4000.0;
    int storeys = 3;
    int bents = 3;
    double bay_spacing = 5200.0;
    double width = 280.0;
    double foot_depth = 260.0;
    double knee_depth = 560.0;
    double tip_depth = 260.0;
};

/// Multi-storey stacked symmetrical three-hinged T-frames (`Dreigelenk-Stockwerkrahmen`, Engel p188-01 bottom).
struct CoupledThreeHinged {
    double bay_span = 6800.0;
    int columns_per_bent = 3;
    double storey_height = 4000.0;
    int storeys = 3;
    int bents = 3;
    double bay_spacing = 5200.0;
    double width = 280.0;
    double foot_depth = 260.0;
    double head_depth = 580.0;
    double tip_depth = 240.0;
};

/// Tiered clerestory basilica hall where an upper two-hinged frame stands on inward cantilevers of two lower portals (Engel p193-03).
struct TieredClerestoryHall {
    double side_span = 6400.0;
    double inward_cantilever = 2400.0;
    double upper_span = 9600.0;
    double lower_height = 4400.0;
    double upper_height = 3800.0;
    int bents = 4;
    double bay_spacing = 4800.0;
    double width = 280.0;
};

/// Tiered hall where an upper wide portal stands on the outward cantilevers of a lower three-hinged portal (Engel p193-05/06).
struct TieredOutwardHall {
    double lower_span = 11200.0;
    double outward_cantilever = 3400.0;
    double upper_span = 15200.0;
    double lower_height = 4400.0;
    double upper_height = 3800.0;
    int bents = 4;
    double bay_spacing = 4800.0;
    double width = 280.0;
};

// ═══════════════════════════════════════════════════════════════════════════
// Multi-Panel Vierendeel Frame Designs (Engel §3.2, pp. 194-199)
// ═══════════════════════════════════════════════════════════════════════════

/// Multi-bay Vierendeel hall whose vertical post widths widen from midspan toward the end supports to resist shear (Engel p195-07/08).
struct ColumnGradatedVierendeel {
    double span = 21000.0;
    int panels = 7;
    double pier_height = 3800.0;
    double frame_depth = 3200.0;
    int bents = 3;
    double bay_spacing = 5400.0;
    double thickness = 280.0;
    double chord_depth = 380.0;
    double min_post_width = 220.0;
    double max_post_width = 560.0;
};

/// Double-cantilevered Vierendeel bridge on central tapered piers with shear-gradated panel widths (Engel p195-09/10).
struct BayGradatedVierendeel {
    double span = 24000.0;
    int panels = 8;
    double end_bay_ratio = 2.3;
    double pier_height = 4000.0;
    double frame_depth = 3200.0;
    double hall_depth = 5200.0;
    double thickness = 280.0;
    double chord_depth = 380.0;
    double post_width = 280.0;
};

/// Open biaxial concentric `#`-grid of story-deep multi-panel Vierendeel frames on eight perimeter columns (Engel p197-01 top).
struct ConcentricVierendeelGrid {
    double size = 24000.0;
    double pier_height = 4600.0;
    double frame_depth = 3200.0;
    double thickness = 300.0;
    double chord_depth = 380.0;
    double post_width = 300.0;
};

/// Multi-storey open multi-panel Vierendeel frames stacked crosswise with two-way cantilevers (Engel p197-01 bottom).
struct CrosswiseStackedVierendeel {
    double length = 21600.0;
    int panels = 6;
    double pier_height = 4000.0;
    double frame_depth = 2800.0;
    int storeys = 3;
    double thickness = 280.0;
    double chord_depth = 340.0;
    double post_width = 280.0;
};

// ═══════════════════════════════════════════════════════════════════════════
// Cellular Slab Box Frame Designs (`Plattenrahmen`, Engel §3.4, p. 209)
// ═══════════════════════════════════════════════════════════════════════════

/// Open cellular CLT box-frame portals with a raised central cross-box (Engel p209-01 #1, `Offene Plattenrahmen`).
struct OpenBoxFrames {
    double bay_width = 5400.0;
    double depth = 7200.0;
    double height = 3400.0;
    double thickness = 200.0;
};

/// Closed cellular CLT box-frame grid elevated on pilotis columns with courtyard skylights and side openings (Engel p209-01 #2).
struct ClosedPilotisBox {
    double grid_size = 18000.0;
    double tube_width = 4800.0;
    double pier_height = 3600.0;
    double box_height = 3200.0;
    double thickness = 200.0;
};

/// Multi-storey staggered and cantilevered CLT box frames over ground portal piers (Engel p209-01 #3, `Mehrgeschoss-Plattenrahmen`).
struct CantileveredMultiBox {
    double tube_width = 5200.0;
    double box_height = 3200.0;
    double pier_height = 3400.0;
    double thickness = 200.0;
};

/// Four-level crosswise-stacked tubular CLT box frames with alternating X/Y cantilevers (Engel p209-01 #4, `Kreuzweise gestapelte Plattenrahmen`).
struct CrosswiseStackedBoxes {
    double length = 20000.0;
    double tube_width = 4600.0;
    double pier_height = 3400.0;
    double box_height = 3000.0;
    int levels = 3;
    double thickness = 200.0;
};

// ═══════════════════════════════════════════════════════════════════════════
// FrameGuide
// ═══════════════════════════════════════════════════════════════════════════

/// Geometric guide of a section-active timber frame or cellular box-frame structure.
class FrameGuide : public wood_session::WoodSession {
public:
    /// A guide from explicit support, member, and plate tables, translated along x by offset_x.
    FrameGuide(
        const std::vector<SupportSpec>& supports,
        const std::vector<MemberSpec>& members,
        const std::vector<PlateSpec>& plates,
        double offset_x = 0.0,
        const std::string& name = "frame_guide"
    );

    /// Portal, gable, articulated, and tiered hinged frames (Engel §3.2).
    explicit FrameGuide(const TwoHingedPortal& design, double offset_x = 0.0);
    explicit FrameGuide(const ThreeHingedGable& design, double offset_x = 0.0);
    explicit FrameGuide(const ArticulatedTFrame& design, double offset_x = 0.0);
    explicit FrameGuide(const DoubledColumnCantilever& design, double offset_x = 0.0);
    explicit FrameGuide(const CoupledTwoHinged& design, double offset_x = 0.0);
    explicit FrameGuide(const CoupledThreeHinged& design, double offset_x = 0.0);
    explicit FrameGuide(const TieredClerestoryHall& design, double offset_x = 0.0);
    explicit FrameGuide(const TieredOutwardHall& design, double offset_x = 0.0);

    /// Shear-gradated, concentric, and crosswise-stacked Vierendeel frames (Engel §3.2).
    explicit FrameGuide(const ColumnGradatedVierendeel& design, double offset_x = 0.0);
    explicit FrameGuide(const BayGradatedVierendeel& design, double offset_x = 0.0);
    explicit FrameGuide(const ConcentricVierendeelGrid& design, double offset_x = 0.0);
    explicit FrameGuide(const CrosswiseStackedVierendeel& design, double offset_x = 0.0);

    /// Open, pilotis, multi-storey, and crosswise-stacked CLT box frames (`Plattenrahmen`, Engel §3.4).
    explicit FrameGuide(const OpenBoxFrames& design, double offset_x = 0.0);
    explicit FrameGuide(const ClosedPilotisBox& design, double offset_x = 0.0);
    explicit FrameGuide(const CantileveredMultiBox& design, double offset_x = 0.0);
    explicit FrameGuide(const CrosswiseStackedBoxes& design, double offset_x = 0.0);

    /// Pinned base shoes of the frame.
    const std::vector<SupportSpec>& supports() const {
        return _supports;
    }

    /// Straight and variable-depth bar members of the frame.
    const std::vector<MemberSpec>& members() const {
        return _members;
    }

    /// Planar CLT floor, wall, and ceiling plates of the frame.
    const std::vector<PlateSpec>& plates() const {
        return _plates;
    }

    /// Bounding width along world X for laying out a row of frames.
    double span_x() const;

private:
    std::vector<SupportSpec> _supports;
    std::vector<MemberSpec> _members;
    std::vector<PlateSpec> _plates;

    void shift_x(double dx);
    void draw();
};

// ═══════════════════════════════════════════════════════════════════════════
// FrameDesign
// ═══════════════════════════════════════════════════════════════════════════

/// Named factory building a `FrameGuide` at a given x-offset along a row.
struct FrameDesign {
    std::string name; // Group and frame name.
    std::function<FrameGuide(double)> make; // Guide builder at x.

    FrameDesign(const std::string& name, const std::function<FrameGuide(double)>& make);
};

} // namespace wood_frame
