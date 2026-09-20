#pragma once

#include "pch.h"

namespace wood_session {

class WoodSession;

namespace io {

/// Adjacent element pairs from the sidecar, one "a b" pair per line; empty when there is no sidecar.
std::vector<std::pair<int, int>> load_adjacency(const std::string& adjacency_name);

/// Per element, one insertion vector per face slot from the sidecar; empty when there is no sidecar.
std::vector<std::vector<session_cpp::Vector>> load_insertion_vectors(const std::string& insertion_vectors_name, size_t count);

/// Per element, one joint id per face slot from the joints_types sidecar; empty when there is no sidecar.
std::vector<std::vector<int>> load_feature_types(const std::string& feature_types_name, size_t count);

/// Three-valence groups from the sidecar: the first row [instruction], then [s0, s1, e20, e31] rows; empty when there is no sidecar.
std::vector<std::vector<int>> load_three_valence(const std::string& three_valence_name);

/// The polylines of `data/<name>.obj` or an .obj path: plate outline pairs, or one beam axis each; duplicate_points_tolerance > 0 removes consecutive duplicate points.
std::vector<session_cpp::Polyline> load_obj(const std::string& dataset_name, double duplicate_points_tolerance = 0.0);

/// `data/output/pb/<name>.pb`, with the directory created; "live" is the file session_viewer watches.
std::filesystem::path pb_path(const std::string& name);

/// `<pb>_meta.txt` and `<pb>_coords.txt` beside a dataset's .pb: every plate's merged outlines, the parity record a refactor is diffed against.
void write_parity_dumps(const WoodSession& scene, const std::filesystem::path& pb);

}} // namespace wood_session::io
