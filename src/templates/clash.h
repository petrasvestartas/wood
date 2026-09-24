#pragma once
#include "wood_session.h"

namespace wood_grid {

using namespace session_cpp;

/// Volume of other inside convex, the face half-spaces of convex pulled in by tolerance so faces that touch or lie within tolerance count for nothing: repeated Mesh::cut_by_plane, then Mesh::volume().
double compute_overlap(const Mesh& convex, const Mesh& other, double tolerance);

/// Every pair of world elements overlapping by more than tolerance as (guid, guid, volume), largest first, each solid judged as the convex pieces of its uncut loft under its cuts; throws on a solid with no such pieces; empty means no clash.
std::vector<std::tuple<std::string, std::string, double>> compute_clashes(const wood_session::WoodSession& session, double tolerance = 1.0);

} // namespace wood_grid
