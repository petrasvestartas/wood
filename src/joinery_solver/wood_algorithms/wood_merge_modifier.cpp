#include "pch.h"
#include "wood_merge_modifier.h"
#include "wood_session.h"
using namespace session_cpp;

constexpr bool TRACE = false;

namespace wood_session {

// ═══════════════════════════════════════════════════════════════════════════
// Constructors
// ═══════════════════════════════════════════════════════════════════════════

MergeModifier::MergeModifier(const Plate& plate, int plate_index, double distance_squared)
    : plate(plate), plate_index(plate_index) {

    if (TRACE) {
        log_file.open((config::output_dir() / "merge.txt").string(), std::ios::app);
        if (log_file.is_open())
            log = &log_file;
    }

    top_points = plate.polylines[0].get_points();
    bottom_points = plate.polylines[1].get_points();
    joint_planes = plate.planes;
    top_original_front = top_points.empty() ? Point(0, 0, 0) : top_points.front();
    bottom_original_front = bottom_points.empty() ? Point(0, 0, 0) : bottom_points.front();
    this->distance_squared = distance_squared;
}

// ═══════════════════════════════════════════════════════════════════════════
// Geometry
// ═══════════════════════════════════════════════════════════════════════════

std::vector<Polyline> MergeModifier::apply(
    const Plate& plate,
    const std::vector<std::vector<std::pair<int, bool>>>& membership,
    std::vector<InteractionFeaturePlate>& joints,
    int plate_index,
    double distance_squared
) {

    MergeModifier state(plate, plate_index, distance_squared);
    state.log_plate();

    state.insert_side_joints(membership, joints);

    Polyline merged_top = build_merged_outline(state.top_points, state.top_runs, state.top_original_front);
    Polyline merged_bottom = build_merged_outline(state.bottom_points, state.bottom_runs, state.bottom_original_front);
    state.close_corner(merged_top, merged_bottom);

    std::vector<Polyline> result;
    state.cut_holes_top_bottom(membership, joints, result);
    state.cut_holes_side(membership, joints, result);
    result.push_back(merged_top);
    result.push_back(merged_bottom);

    state.log_result(merged_top, merged_bottom);
    return result;
}

double MergeModifier::perpendicular_distance_squared(const Point& point, const Point& line_a, const Point& line_b) {

    const Vector direction = line_b - line_a;
    const double length_squared = direction.magnitude_squared();

    if (length_squared < 1e-20)
        return 0.0;

    const double parameter = (point - line_a).dot(direction) / length_squared;
    const Point projected = line_a + direction * parameter;

    return (point - projected).magnitude_squared();
}

void MergeModifier::log_points(std::ofstream& log, const Polyline& polyline) {
    for (size_t k = 0; k < polyline.point_count(); k++) {
        const Point point = polyline.get_point(k);
        log << " (" << point[0] << "," << point[1] << "," << point[2] << ")";
    }
}

void MergeModifier::log_plate() const {

    if (!log || plate_index < 0)
        return;

    std::ofstream& stream = *log;
    stream << "ELEMENT " << plate_index
        << " planes0_o=(" << plate.planes[0].origin()[0] << "," << plate.planes[0].origin()[1] << "," << plate.planes[0].origin()[2]
        << ") planes0_n=(" << plate.planes[0].z_axis()[0] << "," << plate.planes[0].z_axis()[1] << "," << plate.planes[0].z_axis()[2]
        << ") planes1_o=(" << plate.planes[1].origin()[0] << "," << plate.planes[1].origin()[1] << "," << plate.planes[1].origin()[2]
        << ") planes1_n=(" << plate.planes[1].z_axis()[0] << "," << plate.planes[1].z_axis()[1] << "," << plate.planes[1].z_axis()[2]
        << ")\n";
    stream << "  pline0 pts=" << plate.polylines[0].point_count();
    log_points(stream, plate.polylines[0]);
    stream << "\n  pline1 pts=" << plate.polylines[1].point_count();
    log_points(stream, plate.polylines[1]);
    stream << "\n";
}

void MergeModifier::log_result(const Polyline& merged_top, const Polyline& merged_bottom) const {

    if (!log)
        return;

    std::ofstream& stream = *log;
    stream << "  MERGED el=" << plate_index
        << " top.n=" << merged_top.point_count()
        << " bot.n=" << merged_bottom.point_count()
        << (merged_top.point_count() != merged_bottom.point_count() ? " COUNT_MISMATCH" : "")
        << "\n";
    stream << "  merged_top:";
    log_points(stream, merged_top);
    stream << "\n  merged_bot:";
    log_points(stream, merged_bottom);
    stream << "\n";
}

std::array<std::vector<Polyline>, 2>* MergeModifier::joint_outlines(InteractionFeaturePlate& joint, size_t face, int joint_id, bool male_or_female) const {

    std::array<std::vector<Polyline>, 2>& outlines = male_or_female ? joint.male_outlines : joint.female_outlines;
    if (outlines[0].size() < 2 || outlines[1].size() < 2)
        return nullptr;

    if (outlines[0][1].point_count() < 2 || outlines[1][1].point_count() < 2)
        return nullptr;

    const Point marker_top = outlines[0][1].get_point(0);
    const Point marker_bottom = outlines[1][1].get_point(0);
    const double distance_top = Point::distance(marker_top, plate.planes[0].project(marker_top));
    const double distance_bottom = Point::distance(marker_bottom, plate.planes[0].project(marker_bottom));
    const bool is_reversed = (distance_top * distance_top) > (distance_bottom * distance_bottom);

    if (log) {
        *log << "  J el=" << plate_index << " i=" << face
             << " jid=" << joint_id << " mf=" << (male_or_female ? 'M' : 'F')
             << " jt=" << joint.joint_type
             << " ep_top0=(" << marker_top[0] << "," << marker_top[1] << "," << marker_top[2]
             << ") ep_bot0=(" << marker_bottom[0] << "," << marker_bottom[1] << "," << marker_bottom[2]
             << ") d_top=" << distance_top << " d_bot=" << distance_bottom
             << " reversed=" << (is_reversed ? 1 : 0);
    }

    if (is_reversed)
        std::swap(outlines[0], outlines[1]);

    return &outlines;
}

void MergeModifier::insert_rectangle_cut(const std::array<std::vector<Polyline>, 2>& outlines) {

    Polyline clipped_top;
    std::pair<double, double> parameters_top;
    if (!Intersection::closed_and_open_paths_2d(plate.polylines[0], outlines[0][0], plate.planes[0], clipped_top, parameters_top))
        return;

    Polyline clipped_bottom;
    std::pair<double, double> parameters_bottom;
    if (!Intersection::closed_and_open_paths_2d(plate.polylines[1], outlines[1][0], plate.planes[1], clipped_bottom, parameters_bottom))
        return;

    const size_t key_top = (size_t)(EDGE_SCALE * std::floor(parameters_top.first)) + (size_t)(FRACTION_SCALE * std::fmod(parameters_top.first, 1.0));
    const size_t key_bottom = (size_t)(EDGE_SCALE * std::floor(parameters_bottom.first)) + (size_t)(FRACTION_SCALE * std::fmod(parameters_bottom.first, 1.0));

    top_runs.insert({key_top, {parameters_top, clipped_top.get_points()}});
    bottom_runs.insert({key_bottom, {parameters_bottom, clipped_bottom.get_points()}});
}

MergeModifier::RelocatedCorners MergeModifier::corner_intersections(size_t face, int previous, int next, bool z_axis_valid) const {

    const std::vector<Plane>& planes = joint_planes;
    RelocatedCorners corners;
    corners.has_top_at_previous = z_axis_valid && Intersection::plane_plane_plane(planes[2 + previous], planes[face], planes[0], corners.top_at_previous);
    corners.has_top_at_next = z_axis_valid && Intersection::plane_plane_plane(planes[2 + next], planes[face], planes[0], corners.top_at_next);
    corners.has_bottom_at_previous = z_axis_valid && Intersection::plane_plane_plane(planes[2 + previous], planes[face], planes[1], corners.bottom_at_previous);
    corners.has_bottom_at_next = z_axis_valid && Intersection::plane_plane_plane(planes[2 + next], planes[face], planes[1], corners.bottom_at_next);

    return corners;
}

void MergeModifier::relocate_previous_corners(size_t face, const Point& top_start, const Point& bottom_start, RelocatedCorners& corners) const {

    if (last_id != (int)face - 1)
        return;

    const Point top_edge_a = plate.polylines[0].get_point(face - 2);
    const Point top_edge_b = plate.polylines[0].get_point(face - 1);
    const Point bottom_edge_a = plate.polylines[1].get_point(face - 2);
    const Point bottom_edge_b = plate.polylines[1].get_point(face - 1);
    const bool top_is_offset = perpendicular_distance_squared(top_start, top_edge_a, top_edge_b) > distance_squared;
    const bool bottom_is_offset = perpendicular_distance_squared(bottom_start, bottom_edge_a, bottom_edge_b) > distance_squared;

    if (!top_is_offset && !bottom_is_offset)
        return;

    const std::vector<Plane>& planes = joint_planes;
    Point top;
    Point bottom;
    const bool has_top = Intersection::plane_plane_plane(planes[face], planes[face - 1], planes[0], top);
    const bool has_bottom = Intersection::plane_plane_plane(planes[face], planes[face - 1], planes[1], bottom);

    if (has_top && has_bottom) {
        corners.top_at_previous = top;
        corners.bottom_at_previous = bottom;
    }
}

bool MergeModifier::relocate_edge_vertices(std::array<std::vector<Polyline>, 2>& outlines, size_t face) {

    const Point top_start = outlines[0][1].get_point(0);
    const Point top_end = outlines[0][1].get_point(1);
    const Point bottom_start = outlines[1][1].get_point(0);
    const Point bottom_end = outlines[1][1].get_point(1);
    const Vector x_axis = top_end - top_start;
    const Vector y_axis = top_start - bottom_start;
    const Vector z_axis = x_axis.cross(y_axis);
    const bool z_axis_valid = z_axis.magnitude() > 1e-12;
    if (z_axis_valid)
        joint_planes[face] = Plane::from_point_normal(top_start, z_axis);

    if (top_points.size() < 4 || joint_planes.size() != top_points.size() + 1)
        return false;

    const size_t edge_count = top_points.size() - 1;
    const int edge_index = static_cast<int>(face) - 2;
    const int previous = ((int)edge_count + edge_index - 1) % (int)edge_count;
    const int next = (edge_index + 1) % (int)edge_count;
    RelocatedCorners corners = corner_intersections(face, previous, next, z_axis_valid);
    relocate_previous_corners(face, top_start, bottom_start, corners);

    if (corners.has_top_at_previous)
        top_points[edge_index] = corners.top_at_previous;
    if (corners.has_top_at_next)
        top_points[next] = corners.top_at_next;
    if (corners.has_bottom_at_previous)
        bottom_points[edge_index] = corners.bottom_at_previous;
    if (corners.has_bottom_at_next)
        bottom_points[next] = corners.bottom_at_next;

    last_top_segment = {{top_start, top_end}};
    last_bottom_segment = {{bottom_start, bottom_end}};
    if (face == 2) {
        first_top_segment = last_top_segment;
        first_bottom_segment = last_bottom_segment;
    }

    last_id = (int)face;
    return true;
}

void MergeModifier::flip_and_insert_cut(const InteractionFeaturePlate& joint, std::array<std::vector<Polyline>, 2>& outlines, size_t face, int joint_id, bool male_or_female) {

    const int edge_index = static_cast<int>(face) - 2;
    const Polyline& reference = outlines[0][0];
    if (reference.point_count() >= 1) {
        const Point front = reference.get_point(0);
        const Point back = reference.get_point(reference.point_count() - 1);
        const Point reference_point = top_points[edge_index + 1];
        const double front_distance_squared = (front - reference_point).magnitude_squared();
        const double back_distance_squared = (back - reference_point).magnitude_squared();
        const bool flipped = front_distance_squared < back_distance_squared;

        if (log) {
            *log << " fr_front=(" << front[0] << "," << front[1] << "," << front[2]
                 << ") fr_back=(" << back[0] << "," << back[1] << "," << back[2]
                 << ") ref=(" << reference_point[0] << "," << reference_point[1] << "," << reference_point[2]
                 << ") d_f=" << front_distance_squared << " d_b=" << back_distance_squared
                 << " flipped=" << (flipped ? 1 : 0)
                 << " jm0[0].first=(" << outlines[0][0].get_point(0)[0] << "," << outlines[0][0].get_point(0)[1] << "," << outlines[0][0].get_point(0)[2] << ")"
                 << " jm1[0].first=(" << outlines[1][0].get_point(0)[0] << "," << outlines[1][0].get_point(0)[1] << "," << outlines[1][0].get_point(0)[2] << ")"
                 << "\n";
        }

        if (flipped) {
            outlines[0][0].reverse();
            outlines[1][0].reverse();
        }
    } else if (log) {
        *log << " (flip_ref empty)\n";
    }

    const std::pair<double, double> parameters(edge_index + 0.1, edge_index + 0.9);
    const size_t sort_key = (size_t)(EDGE_SCALE * std::floor(parameters.first)) + (size_t)(FRACTION_SCALE * std::fmod(parameters.first, 1.0));

    if (log) {
        *log << "  INSERT el=" << plate_index << " i=" << face
             << " jid=" << joint_id << " jt=" << joint.joint_type
             << " mf=" << (male_or_female ? 'M' : 'F')
             << " jm0[0].n=" << outlines[0][0].point_count()
             << " jm1[0].n=" << outlines[1][0].point_count()
             << (outlines[0][0].point_count() != outlines[1][0].point_count() ? " COUNT_DIFF" : "")
             << "\n";
    }

    top_runs.insert({sort_key, {parameters, outlines[0][0].get_points()}});
    bottom_runs.insert({sort_key, {parameters, outlines[1][0].get_points()}});
}

void MergeModifier::insert_side_joints(const std::vector<std::vector<std::pair<int, bool>>>& membership, std::vector<InteractionFeaturePlate>& joints) {
    for (size_t face = 2; face < membership.size() && face < plate.planes.size(); face++) {
        for (size_t j = 0; j < membership[face].size(); j++) {
            const int joint_id = membership[face][j].first;
            const bool male_or_female = membership[face][j].second;
            InteractionFeaturePlate& joint = joints[joint_id];
            std::array<std::vector<Polyline>, 2>* outlines = joint_outlines(joint, face, joint_id, male_or_female);
            if (!outlines)
                continue;

            const size_t marker = (*outlines)[0][1].point_count();
            if (marker == 5) {
                insert_rectangle_cut(*outlines);
                continue;
            }

            if (marker != 2)
                continue;

            if (!relocate_edge_vertices(*outlines, face))
                continue;

            flip_and_insert_cut(joint, *outlines, face, joint_id, male_or_female);
        }
    }
}

Polyline MergeModifier::build_merged_outline(const std::vector<Point>& points, std::multimap<size_t, std::pair<std::pair<double, double>, std::vector<Point>>>& runs, const Point& original_front) {

    std::vector<bool> point_flags(points.size(), true);
    for (const std::multimap<size_t, std::pair<std::pair<double, double>, std::vector<Point>>>::value_type& entry : runs) {
        const std::pair<double, double>& parameters = entry.second.first;
        for (size_t k = (size_t)std::ceil(parameters.first); k <= (size_t)std::floor(parameters.second) && k < point_flags.size(); k++)
            point_flags[k] = false;
    }

    if (points.size() > 1) {
        const Vector closing = points.back() - original_front;
        if (std::abs(closing[0]) < 1e-6 && std::abs(closing[1]) < 1e-6 && std::abs(closing[2]) < 1e-6)
            point_flags.back() = false;
    }

    if (!runs.empty() && !point_flags.empty()) {
        const std::pair<double, double>& last_parameters = runs.rbegin()->second.first;
        if (last_parameters.first > (double)points.size() - 2.0 && last_parameters.second < 1.0)
            point_flags[0] = false;
    }

    for (size_t k = 0; k < point_flags.size(); k++) {

        if (!point_flags[k])
            continue;

        const size_t sort_key = (size_t)(k * EDGE_SCALE);
        runs.insert({sort_key, {{(double)k, (double)k}, std::vector<Point>{points[k]}}});
    }

    std::vector<Point> merged;
    for (const std::multimap<size_t, std::pair<std::pair<double, double>, std::vector<Point>>>::value_type& entry : runs) {
        const std::vector<Point>& points_run = entry.second.second;
        merged.insert(merged.end(), points_run.begin(), points_run.end());
    }

    if (!merged.empty())
        merged.push_back(merged.front());

    return Polyline(merged);
}

void MergeModifier::close_corner(Polyline& merged_top, Polyline& merged_bottom) const {

    if (last_id != (int)top_points.size())
        return;

    if (!((first_top_segment[0] - first_top_segment[1]).magnitude_squared() > distance_squared))
        return;

    const Line first_top = Line::from_points(first_top_segment[0], first_top_segment[1]);
    const Line last_top = Line::from_points(last_top_segment[0], last_top_segment[1]);
    const Line first_bottom = Line::from_points(first_bottom_segment[0], first_bottom_segment[1]);
    const Line last_bottom = Line::from_points(last_bottom_segment[0], last_bottom_segment[1]);
    Point top_close;
    Point bottom_close;
    const bool has_top = Intersection::line_line_3d(first_top, last_top, top_close);
    const bool has_bottom = Intersection::line_line_3d(first_bottom, last_bottom, bottom_close);

    if (!has_top || !has_bottom)
        return;

    std::vector<Point> closed_top = merged_top.get_points();
    std::vector<Point> closed_bottom = merged_bottom.get_points();
    if (!closed_top.empty())
        closed_top[0] = top_close;
    if (!closed_bottom.empty())
        closed_bottom[0] = bottom_close;
    if (closed_top.size() > 1)
        closed_top.back() = closed_top.front();
    if (closed_bottom.size() > 1)
        closed_bottom.back() = closed_bottom.front();

    merged_top = Polyline(closed_top);
    merged_bottom = Polyline(closed_bottom);
}

void MergeModifier::cut_holes_top_bottom(const std::vector<std::vector<std::pair<int, bool>>>& membership, std::vector<InteractionFeaturePlate>& joints, std::vector<Polyline>& result) const {
    for (size_t face = 0; face < 2 && face < membership.size(); face++) {
        for (size_t k = 0; k < membership[face].size(); k++) {
            const int joint_id = membership[face][k].first;
            const bool male_or_female = membership[face][k].second;
            InteractionFeaturePlate& joint = joints[joint_id];
            std::array<std::vector<Polyline>, 2>& outlines = male_or_female ? joint.male_outlines : joint.female_outlines;
            std::array<std::vector<int>, 2>& cut_types = male_or_female ? joint.male_fabrication_types : joint.female_fabrication_types;

            if (outlines[0].empty() || outlines[1].empty())
                continue;

            const Point top_back = outlines[0].back().get_point(0);
            const Point bottom_back = outlines[1].back().get_point(0);
            const double distance_top = Point::distance(top_back, plate.planes[0].project(top_back));
            const double distance_bottom = Point::distance(bottom_back, plate.planes[0].project(bottom_back));
            if ((distance_top * distance_top) > (distance_bottom * distance_bottom)) {
                std::swap(outlines[0], outlines[1]);
                std::swap(cut_types[0], cut_types[1]);
            }

            const size_t outline_count = outlines[0].size() > 1 ? outlines[0].size() - 1 : 0;
            for (size_t outline_index = 0; outline_index < outline_count && outline_index < outlines[1].size(); outline_index++) {
                Polyline top = outlines[0][outline_index];
                Polyline bottom = outlines[1][outline_index];
                if (!top.is_clockwise(plate.planes[0])) {
                    top.reverse();
                    bottom.reverse();
                }

                result.push_back(top);
                result.push_back(bottom);
            }
        }
    }
}

void MergeModifier::cut_holes_side(const std::vector<std::vector<std::pair<int, bool>>>& membership, std::vector<InteractionFeaturePlate>& joints, std::vector<Polyline>& result) const {
    for (size_t face = 2; face < membership.size(); face++) {
        for (size_t k = 0; k < membership[face].size(); k++) {
            const int joint_id = membership[face][k].first;
            const bool male_or_female = membership[face][k].second;
            InteractionFeaturePlate& joint = joints[joint_id];
            std::array<std::vector<Polyline>, 2>& outlines = male_or_female ? joint.male_outlines : joint.female_outlines;
            std::array<std::vector<int>, 2>& cut_types = male_or_female ? joint.male_fabrication_types : joint.female_fabrication_types;

            if (outlines[0].empty() || outlines[1].empty())
                continue;

            if (cut_types[0].empty())
                continue;

            std::vector<int> hole_indices;
            for (int cut_index = 0; cut_index < (int)cut_types[0].size(); cut_index += 2)
                if (cut_types[0][cut_index] == FabricationType::hole)
                    hole_indices.push_back(cut_index);

            if (hole_indices.empty())
                continue;

            const Point top_back = outlines[0].back().get_point(0);
            const Point bottom_back = outlines[1].back().get_point(0);
            const double distance_top = Point::distance(top_back, plate.planes[0].project(top_back));
            const double distance_bottom = Point::distance(bottom_back, plate.planes[0].project(bottom_back));
            if ((distance_top * distance_top) > (distance_bottom * distance_bottom)) {
                std::swap(outlines[0], outlines[1]);
                std::swap(cut_types[0], cut_types[1]);
            }

            for (const int cut_index : hole_indices) {

                if (cut_index >= (int)outlines[0].size() || cut_index >= (int)outlines[1].size())
                    continue;

                Polyline top = outlines[0][cut_index];
                Polyline bottom = outlines[1][cut_index];
                if (!top.is_clockwise(plate.planes[0])) {
                    top.reverse();
                    bottom.reverse();
                }

                result.push_back(top);
                result.push_back(bottom);
            }
        }
    }
}

} // namespace wood_session
