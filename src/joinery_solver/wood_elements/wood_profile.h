#pragma once

#include "pch.h"

namespace wood_session {

// ═══════════════════════════════════════════════════════════════════════════
// Profiles
// ═══════════════════════════════════════════════════════════════════════════

/// A profile is loops in the section frame: loop 0 the outline counter-clockwise centred on the axis, x across, y up; loops 1.. holes clockwise.
std::vector<session_cpp::Polyline> profile_rectangle(double width, double depth);

/// A polygon of segments sides inscribed in diameter, a side centred on each axis so a member meets it on a flat.
std::vector<session_cpp::Polyline> profile_round(double diameter, int segments = 16);

/// An I: two flanges of thickness flange joined by a web of thickness web.
std::vector<session_cpp::Polyline> profile_w(double width, double depth, double flange, double web);

/// A hollow rectangle of wall thickness.
std::vector<session_cpp::Polyline> profile_hss(double width, double depth, double thickness);

/// Two outlines of width side by side, gap apart; the builder makes two members with the same cuts.
std::vector<session_cpp::Polyline> profile_double(double width, double depth, double gap);

/// A wide flat rectangle.
std::vector<session_cpp::Polyline> profile_slab_band(double width, double depth);

/// A T: a flange of thickness flange on top of a web of thickness web.
std::vector<session_cpp::Polyline> profile_t(double width, double depth, double web, double flange);

/// Width and depth of a profile from the bounding box of all its loops, so a double profile spans both members.
std::pair<double, double> compute_size(const std::vector<session_cpp::Polyline>& profile);

/// Support distance of a profile in a section direction: the largest reach of loop 0 along it, what a head bottom needs to circumscribe it.
double compute_support(const std::vector<session_cpp::Polyline>& profile, const session_cpp::Vector& direction);

/// The profile scaled to width across and depth up; a zero keeps that size.
std::vector<session_cpp::Polyline> compute_scaled(const std::vector<session_cpp::Polyline>& profile, double width, double depth);

/// A loop placed at a point of an axis: x along the side, y along the rise, both from the direction and up as square_section builds them.
session_cpp::Polyline profile_section(const session_cpp::Point& at, const session_cpp::Vector& direction, const session_cpp::Vector& up, const session_cpp::Polyline& loop);

} // namespace wood_session
