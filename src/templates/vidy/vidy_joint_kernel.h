#pragma once
#include "wood_session.h"

namespace wood_vidy {

/// Paired bottom and top four-corner hole loops of one rectangular through-mortise slot.
struct VidyMortiseSlot {
    session_cpp::Point bottom[4]; // Four corners on the receiving plate's bottom plane.
    session_cpp::Point top[4]; // Matching four corners on the receiving plate's top plane.
};

/// Edge frame, insertion vector and layout parameters of one tenon or rabbet band.
struct VidyPinBand {
    std::array<session_cpp::Point, 4> ends; // Miter endpoints {b0, b1, t0, t1}.
    session_cpp::Point ref_pt; // Shared seam midpoint projected onto both edge lines.
    session_cpp::Vector v_ins; // Unit insertion direction across the crease.
    double le_tab = 0.0; // Single-layer bevel step length along v_ins.
    double le_offs = 0.0; // Cavity-spanning bevel offset length along v_ins.
    bool step_bot = false; // True when the bottom face steps back by le_tab.
    int count = 0; // Pin count (or mating pin count on rabbet shelves).
    double pitch = 500.0; // Centre-to-centre pin spacing along the crease.
    double pocket_extra = 0.0; // Extra pocket half-margin beyond 0.5 * count * pitch.
    double shift = 0.0; // Longitudinal shift of the band centre from ref_pt.
    double scale = 0.0; // Tenon width scale (0 -> min(1.0, pitch / 500.0)).
};

/// Outward unit vector in a facet's plane perpendicular to directed edge (p_start -> p_end).
session_cpp::Vector vidy_inplane_outward(
    const session_cpp::Point& centroid,
    const session_cpp::Vector& normal,
    const session_cpp::Point& p_start,
    const session_cpp::Point& p_end
);

/// Directed side index on a polygon vertex loop sharing undirected vertices (u, v).
size_t vidy_find_shared_side(const std::vector<size_t>& vertices, size_t u, size_t v);

/// Half-span offset from the band midpoint to the first pin centre.
double vidy_first_pin_offset(int count, double pitch);

/// Reference point on top_start..top_end aligned with m_b0 along a plane containing v_ins.
session_cpp::Point vidy_align_top_ref(
    const session_cpp::Point& m_b0,
    const session_cpp::Point& top_start,
    const session_cpp::Point& top_end,
    const session_cpp::Vector& nf,
    const session_cpp::Vector& v_ins
);

/// Available longitudinal half-span from ref_pt/m_t0 to the nearest corner minus corner_keep.
double vidy_band_half_span(
    const std::array<session_cpp::Point, 4>& ends,
    const session_cpp::Point& ref_pt,
    const session_cpp::Vector& v_ins,
    double corner_keep
);

/// Twelve-point stepped chamfered Double_Pin contour and its base and tip arris pairs.
std::vector<session_cpp::Point> vidy_double_pin(
    const session_cpp::Point& pm,
    const session_cpp::Vector& nf,
    const session_cpp::Vector& v_ins,
    double le_tab,
    double le_offs,
    double scale,
    std::pair<session_cpp::Point, session_cpp::Point>& out_base,
    std::pair<session_cpp::Point, session_cpp::Point>& out_tip
);

/// Six-point chamfered Single_Pin contour and its shaft arris pair.
std::vector<session_cpp::Point> vidy_single_pin(
    const session_cpp::Point& pm,
    const session_cpp::Vector& nf,
    const session_cpp::Vector& v_ins,
    double le_tab,
    double scale,
    std::pair<session_cpp::Point, session_cpp::Point>& out_shaft
);

/// Projects a four-arris tenon shaft along v_ins onto (p_bot, p_top) with 0.5 mm clearance.
VidyMortiseSlot vidy_project_slot(
    const std::pair<session_cpp::Point, session_cpp::Point>& shaft_bot,
    const std::pair<session_cpp::Point, session_cpp::Point>& shaft_top,
    const session_cpp::Vector& v_ins,
    const session_cpp::Plane& p_bot,
    const session_cpp::Plane& p_top
);

/// Projects a four-arris tenon shaft onto a facet at offset heights (h_bot, h_top).
VidyMortiseSlot vidy_project_facet_slot(
    const std::pair<session_cpp::Point, session_cpp::Point>& shaft_bot,
    const std::pair<session_cpp::Point, session_cpp::Point>& shaft_top,
    const session_cpp::Vector& v_ins,
    const session_cpp::Point& centroid,
    const session_cpp::Vector& normal,
    double h_bot,
    double h_top
);

/// Paired bottom and top Multiple_Double_Pins contours plus base and tip mortise slots.
std::array<std::vector<session_cpp::Point>, 2> vidy_emit_double_pins(
    const VidyPinBand& band,
    const session_cpp::Point& fem_centroid,
    const session_cpp::Vector& fem_normal,
    const std::array<double, 4>& heights,
    size_t same_layer,
    std::vector<VidyMortiseSlot>& same_slots,
    std::vector<VidyMortiseSlot>& opp_slots
);

/// Paired bottom and top Multiple_Single_Pins contours plus mortise slots.
std::array<std::vector<session_cpp::Point>, 2> vidy_emit_single_pins(
    const VidyPinBand& band,
    const session_cpp::Point& fem_centroid,
    const session_cpp::Vector& fem_normal,
    double h_bot,
    double h_top,
    std::vector<VidyMortiseSlot>& slots
);

/// Paired bottom and top Extend_Pins cavity-bridging contours plus Single_Pin mortise slots.
std::array<std::vector<session_cpp::Point>, 2> vidy_emit_extend_pins(
    const VidyPinBand& band,
    const session_cpp::Point& fem_centroid,
    const session_cpp::Vector& fem_normal,
    double h_bot,
    double h_top,
    std::vector<VidyMortiseSlot>& slots
);

/// Paired bottom and top female Extend rabbet lip (is_shelf=false) or Extend_NoPins shelf contours.
std::array<std::vector<session_cpp::Point>, 2>
vidy_emit_extend_lip(const VidyPinBand& band, bool is_shelf);

/// True when two 4-corner quads projected onto normal's plane come closer than min_gap (2D SAT).
bool vidy_quads_closer_than_2d(
    const session_cpp::Point (&qa)[4],
    const session_cpp::Point (&qb)[4],
    const session_cpp::Vector& normal,
    double min_gap
);

/// Incoming and outgoing edge snips separating apex from limit_plane by clearance.
std::pair<double, double> vidy_plane_corner_snips(
    const session_cpp::Point& apex,
    const session_cpp::Vector& ua,
    const session_cpp::Vector& ub,
    const session_cpp::Plane& limit_plane,
    double clearance,
    double min_cos,
    double max_snip
);

/// Stitches paired bottom/top miter rings, selective corner snips and edge profiles into closed loops.
std::pair<session_cpp::Polyline, session_cpp::Polyline> vidy_stitch_layer_rings(
    const std::array<std::vector<session_cpp::Point>, 4>& miter_rings,
    const session_cpp::Vector& normal,
    size_t h_bot,
    size_t h_top,
    const std::vector<std::pair<double, double>>& snips_b,
    const std::vector<std::pair<double, double>>& snips_t,
    const std::vector<std::vector<session_cpp::Point>>& prof_bot,
    const std::vector<std::vector<session_cpp::Point>>& prof_top,
    double min_lim = 2.0,
    double prof_frac = 0.88
);

/// Attaches jointed outer outlines and consistently wound through-mortise hole loops onto plate.
void vidy_attach_plate_features(
    wood_session::FacePlanarElement& plate,
    const session_cpp::Vector& normal,
    const session_cpp::Polyline& bot_outline,
    const session_cpp::Polyline& top_outline,
    const std::vector<VidyMortiseSlot>& slots
);

}
