#pragma once

#include "pch.h"
#include "wood_session.h"

using namespace laika_cpp;
using namespace wood_session;

namespace wood_delorme {

/// Structural typology of a Philibert de l'Orme (1561) small-timber plank vault or dome.
enum class DelormeKind {
    SemicircularBarrel,
    PointedImperiale,
    HemisphericalDome,
    CrossedGroin,
    CustomRibs,
};

/// Geometric and carpentry parameters for Philibert de l'Orme's 1561 side-by-side overlapped flat-plank system.
struct DelormeParams {
    DelormeKind kind = DelormeKind::SemicircularBarrel;
    double span = 4800.0; // Clear span (or dome diameter) in mm.
    double rise = 2400.0; // Vault crown height above springing z = 0 in mm.
    double length = 3200.0; // Longitudinal vault bay depth in mm (barrel & pointed vaults).
    int n_ribs = 4; // Number of parallel arch ribs (or meridian ribs in a dome).
    int n_plies = 2; // Number of side-by-side overlapped flat-board plies per rib (2 or 3).
    double plank_length = 1200.0; // Target chord length of each short board (~4 French pieds).
    double rib_depth = 220.0; // Radial depth of each curved plank (~8-9 pouces).
    double plank_thickness = 34.0; // Thickness of each flat plank stood on edge (~1.25 pouces).
    double ply_gap = 0.8; // Assembly clearance between side-by-side plies in mm.
    double seam_gap = 0.6; // Radial butt clearance between consecutive planks in a ply in mm.
    double stub_tenon_length = 26.0; // Central edgewise stub tenon length (tenon et entaille) in mm; 0 disables.
    double stub_tenon_ratio = 0.34; // Fraction of rib_depth occupied by the central stub tenon.
    double lierne_spacing = 950.0; // Target arc spacing between transverse binding liernes in mm.
    double lierne_width = 34.0; // Width of each transverse lierne along the rib tangent in mm.
    double lierne_height = 68.0; // Radial height of each transverse lierne piercing the rib mid-depth in mm.
    double lierne_overhang = 135.0; // Projection of each lierne beyond the outer rib cheeks in mm.
    double mortise_clearance = 1.4; // Clearance around lierne through-mortises in the planks in mm.
    double clavette_width = 16.0; // Mean width of each tapered wooden wedge along the lierne axis in mm.
    double clavette_thickness = 16.0; // Thickness of each clavette across the lierne width in mm.
    double clavette_length = 148.0; // Length of each tapered clavette driven perpendicular to the lierne in mm.
    double clavette_taper = 3.0; // Half-width taper difference between clavette head and tip in mm.
    double cheville_radius = 8.0; // Radius of wooden alignment pegs through overlapped plies in mm; 0 disables.
    double oculus_radius = 580.0; // Compression oculus inner radius for hemispherical domes in mm.
    int facets_per_plank = 4; // Number of arc facets along the curved extrados/intrados of each plank.
    bool flat_springing = true; // Slices springing planks horizontally flush on z = 0 when true.
};

/// One planar arch or meridian rib centreline curve in a vertical plane (origin, u_axis, Z).
struct DelormeRibCurve {
    std::string name;
    Point origin;
    Vector u_axis; // Unit horizontal direction in the rib plane.
    Vector normal; // Unit normal to the flat plank cheeks (u_axis x Z).
    std::vector<Point> pts; // Sampled mid-depth rib curve points.
    std::vector<Vector> tangents; // Unit forward tangents along the rib curve.
    std::vector<Vector> radials; // Unit outward radials in the rib plane (normal x tangent).
    std::vector<double> arc_s; // Cumulative arc length at each sample point.
    double kink_s = -1.0; // Arc length of a pointed ridge kink when >= 0 (splits half-arches cleanly).

    double total_length() const;
    void evaluate(double s, Point& out_pt, Vector& out_tan, Vector& out_rad) const;
};

/// Specification of one flat curved-edge plank in a multi-ply Delorme rib.
struct DelormePlankSpec {
    size_t rib_index = 0;
    size_t ply_index = 0;
    size_t plank_index = 0;
    double s0 = 0.0;
    double s1 = 0.0;
    double lateral_offset = 0.0; // Signed offset of this ply's mid-plane along rib.normal.
    bool has_start_notch = false; // Central open mortise (entaille) at s0.
    bool has_end_tenon = false; // Central projecting stub tenon at s1.
    bool trim_start_horizontal = false; // Springing horizontal cut on z = 0 at start.
    bool trim_end_horizontal = false; // Springing horizontal cut on z = 0 at end.
};

/// One rib crossing along a straight transverse or chordal lierne.
struct DelormeLiernePiercing {
    size_t rib_index = 0;
    double rib_s = 0.0;
    double t_center = 0.0; // Distance from lierne.start along axis_dir to rib mid-plane.
    double half_span_along_lierne = 0.0; // Projected half-thickness of the multi-ply rib along axis_dir.
};

/// Specification of one straight binding lierne and the ribs it threads through.
struct DelormeLierneSpec {
    std::string name;
    Point start;
    Point end;
    Vector axis_dir; // Unit direction from start to end.
    Vector up_dir; // Unit outward radial direction (height of lierne).
    Vector width_dir; // Unit direction perpendicular to up_dir and axis_dir.
    std::vector<DelormeLiernePiercing> piercings;
};

/// Specification of one wooden alignment peg (cheville) bored across overlapped plies.
struct DelormeChevilleSpec {
    size_t rib_index = 0;
    double rib_s = 0.0;
    Line axis;
};

/// Geometric layout guide computing rib curves, quinconce staggered planks, liernes, clavettes and chevilles.
class DelormeGuide {
public:
    DelormeParams params;

    explicit DelormeGuide(const DelormeParams& params = DelormeParams());
    DelormeGuide(const DelormeParams& params, const std::vector<DelormeRibCurve>& custom_ribs);

    static DelormeGuide barrel(
        double span = 4800.0,
        double rise = 2400.0,
        double length = 3200.0,
        int n_ribs = 4,
        int n_plies = 2,
        double plank_length = 1200.0,
        double rib_depth = 220.0,
        double plank_thickness = 34.0
    );

    static DelormeGuide pointed_imperiale(
        double span = 4600.0,
        double rise = 3000.0,
        double length = 3200.0,
        int n_ribs = 4,
        int n_plies = 2,
        double plank_length = 1150.0,
        double rib_depth = 220.0,
        double plank_thickness = 34.0
    );

    static DelormeGuide dome(
        double diameter = 6400.0,
        double oculus_diameter = 1240.0,
        int n_ribs = 12,
        int n_plies = 2,
        double plank_length = 1100.0,
        double rib_depth = 210.0,
        double plank_thickness = 32.0
    );

    static DelormeGuide crossed_vault(
        double span = 4800.0,
        double rise = 2100.0,
        int ribs_per_axis = 2,
        int n_plies = 2,
        double plank_length = 1150.0,
        double rib_depth = 210.0,
        double plank_thickness = 32.0
    );

    double total_rib_thickness() const;

    const std::vector<DelormeRibCurve>& ribs() const { return _ribs; }
    const std::vector<DelormePlankSpec>& planks() const { return _planks; }
    const std::vector<DelormeLierneSpec>& liernes() const { return _liernes; }
    const std::vector<DelormeChevilleSpec>& chevilles() const { return _chevilles; }

private:
    std::vector<DelormeRibCurve> _ribs;
    std::vector<DelormePlankSpec> _planks;
    std::vector<DelormeLierneSpec> _liernes;
    std::vector<DelormeChevilleSpec> _chevilles;

    void build_default_ribs();
    void build_planks_and_fasteners();
};

} // namespace wood_delorme
