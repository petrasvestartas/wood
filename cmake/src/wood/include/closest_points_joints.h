#pragma once

// Benchmark: assign joint types to polyline edges by closest-point search.
// Compares three spatial structures: RTree (existing), AABBTree and BVH (session_cpp).
//
// Entry point: closest_points_joints::run_benchmark()

namespace closest_points_joints
{
void run_benchmark ();
} // namespace closest_points_joints
