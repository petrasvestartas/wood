#include "pch.h"
#include "wood_three_valence.h"
#include "wood_session.h"
using namespace session_cpp;

constexpr bool TRACE = false;

namespace wood_session {

namespace {

/// Clip a joint's volume pairs between the planes through `a` and `b` that are normal to the first volume.
void clip_joint_volumes(FeaturePlate& joint, const Point& a, const Point& b) {

    if (!joint.joint_volumes[0].has_value())
        return;

    const Polyline& volume = *joint.joint_volumes[0];
    Vector normal(0, 0, 1);
    if (volume.point_count() >= 3) {
        const Point p0 = volume.get_point(0);
        const Point p1 = volume.get_point(1);
        const Point p2 = volume.get_point(2);
        normal = (p2 - p1).cross(p0 - p1);
    }

    normal.normalize_self();
    const Plane plane_a = Plane::from_point_normal(a, normal);
    const Plane plane_b = Plane::from_point_normal(b, normal);

    for (int pair_index = 0; pair_index < 4; pair_index += 2) {

        if (!joint.joint_volumes[pair_index].has_value() || !joint.joint_volumes[pair_index + 1].has_value())
            continue;

        Polyline& volume0 = *joint.joint_volumes[pair_index];
        Polyline& volume1 = *joint.joint_volumes[pair_index + 1];
        const Line edge0 = Line::from_points(volume0.get_point(0), volume1.get_point(0));
        const Line edge1 = Line::from_points(volume0.get_point(1), volume1.get_point(1));
        const Line edge2 = Line::from_points(volume0.get_point(2), volume1.get_point(2));
        const Line edge3 = Line::from_points(volume0.get_point(3), volume1.get_point(3));

        Intersection::plane_4lines(plane_a, edge0, edge1, edge2, edge3, volume0);
        Intersection::plane_4lines(plane_b, edge0, edge1, edge2, edge3, volume1);
    }
}

} // anonymous namespace

// ═══════════════════════════════════════════════════════════════════════════
// Element pairs
// ═══════════════════════════════════════════════════════════════════════════

/// Order-independent key for an element pair.
uint64_t pair_key(int a, int b) {

    if (a > b)
        std::swap(a, b);

    return ((uint64_t)a << 32) | (uint64_t)b;
}

/// Element pair -> joint index (last joint wins); rebuilt wherever the joint list may have changed.
std::unordered_map<uint64_t, int> joints_by_element_pair(
    const std::vector<std::shared_ptr<Plate>>& elements,
    const std::vector<FeaturePlate>& joints) {

    std::unordered_map<uint64_t, int> joints_map;
    for (size_t joint_index = 0; joint_index < joints.size(); joint_index++) {
        const int element0 = index_of_plate(elements, joints[joint_index].element_a);
        const int element1 = index_of_plate(elements, joints[joint_index].element_b);

        joints_map[pair_key(element0, element1)] = (int)joint_index;
    }

    return joints_map;
}

// ═══════════════════════════════════════════════════════════════════════════
// Vidy shadow joints
// ═══════════════════════════════════════════════════════════════════════════

/// Whether two plate normals are parallel within `angle` radians, either way round.
static bool normals_parallel(const Vector& a, const Vector& b, double angle) {

    const double length_product = a.magnitude() * b.magnitude();
    if (length_product <= 0.0)
        return false;

    return std::abs(a.dot(b) / length_product) >= std::cos(angle);
}

/// The four joint volumes of a joint copied out, an empty polyline where one is missing.
static std::array<Polyline, 4> copy_joint_volumes(const FeaturePlate& joint) {

    std::array<Polyline, 4> volumes;
    for (int k = 0; k < 4; k++)
        if (joint.joint_volumes[k].has_value())
            volumes[k] = *joint.joint_volumes[k];

    return volumes;
}

/// The face of the glued plate nearest the side plate's first face, and the side plate's face farthest from it.
static std::pair<Plane, Plane> far_near_planes(const Plate& glued, const Plate& side) {

    const double glued0 = glued.planes[0].squared_distance(side.planes[0].origin());
    const double glued1 = glued.planes[1].squared_distance(side.planes[0].origin());
    const Plane far = glued0 < glued1 ? glued.planes[0] : glued.planes[1];

    const double side0 = side.planes[0].squared_distance(far.origin());
    const double side1 = side.planes[1].squared_distance(far.origin());
    const Plane near = side0 < side1 ? side.planes[1] : side.planes[0];

    return {far, near};
}

/// A linked shadow of source between side and glued, its lines and first two volumes translated onto the glued plate.
static FeaturePlate shadow_joint(const FeaturePlate& source, const Plate& side, const Plate& glued, const std::array<Line, 2>& lines, const std::array<Polyline, 4>& volumes) {

    FeaturePlate shadow;
    shadow.guid = ::guid();
    shadow.element_a = side.guid();
    shadow.element_b = glued.guid();
    shadow.contact.face_a = -1;
    shadow.contact.face_b = -1;
    shadow.cross_faces = {-1, -1};
    shadow.joint_type = source.joint_type;
    shadow.contact.polygon = source.contact.polygon;
    shadow.joint_lines = lines;
    shadow.joint_volumes = {volumes[0], volumes[1], std::nullopt, std::nullopt};
    shadow.link = true;

    return shadow;
}

/// Vidy method: shadow joints (link = true) between each side plate and the plate it is glued to, translated to that plate's far face.
void add_vidy_shadow_joints(
    const std::vector<std::vector<int>>& three_valence_groups,
    std::vector<std::shared_ptr<Plate>>& elements,
    std::vector<FeaturePlate>& joints,
    std::unordered_map<uint64_t, int>& joints_map,
    double angle) {

    if (three_valence_groups.size() < 2)
        return;

    joints.reserve(joints.size() + (three_valence_groups.size() - 1) * 2);

    for (size_t group_index = 1; group_index < three_valence_groups.size(); group_index++) {
        const std::vector<int>& group = three_valence_groups[group_index];
        if (group.size() != 4)
            continue;

        int side0 = group[0];
        int side1 = group[1];
        int glued0 = group[2];
        int glued1 = group[3];
        const int element_count = (int)elements.size();

        if (side0 < 0 || side1 < 0 || glued0 < 0 || glued1 < 0)
            continue;

        if (side0 >= element_count || side1 >= element_count)
            continue;

        if (glued0 >= element_count || glued1 >= element_count)
            return;

        if (glued0 != glued1) {
            const Vector normal_side0 = elements[side0]->planes[0].z_axis();
            const Vector normal_glued1 = elements[glued1]->planes[0].z_axis();
            const Vector normal_side1 = elements[side1]->planes[0].z_axis();
            const Vector normal_glued0 = elements[glued0]->planes[0].z_axis();

            if (!normals_parallel(normal_side0, normal_glued1, angle) || !normals_parallel(normal_side1, normal_glued0, angle))
                continue;
        }

        const auto found = joints_map.find(pair_key(side0, side1));
        if (found == joints_map.end())
            continue;

        const int joint_index = found->second;
        if (!joints[joint_index].joint_volumes[0].has_value())
            continue;

        const std::pair<Plane, Plane> planes0 = far_near_planes(*elements[glued1], *elements[side0]);
        const std::pair<Plane, Plane> planes1 = far_near_planes(*elements[glued0], *elements[side1]);
        const Plane& far_plane0 = planes0.first;
        const Plane& near_plane0 = planes0.second;
        const Plane& far_plane1 = planes1.first;
        const Plane& near_plane1 = planes1.second;

        const Polyline& joint_volume = *joints[joint_index].joint_volumes[0];
        const Line line0 = Line::from_points(joint_volume.get_point(0), joint_volume.get_point(1));
        const Line line1 = Line::from_points(joint_volume.get_point(1), joint_volume.get_point(2));

        const Point projected1 = near_plane0.project(joint_volume.get_point(1));
        const Point projected2 = near_plane0.project(joint_volume.get_point(2));
        const Vector projected_direction = projected1 - projected2;
        const bool projected_is_parallel = (projected_direction.is_parallel_to(line1.to_vector()) == 0);
        const std::array<Line, 2> ordered_lines = projected_is_parallel ? std::array<Line, 2>{line1, line0} : std::array<Line, 2>{line0, line1};

        Point far_point0;
        Point near_point0;
        Point far_point1;
        Point near_point1;

        if (!Intersection::line_plane(ordered_lines[0], far_plane0, far_point0, false))
            continue;

        if (!Intersection::line_plane(ordered_lines[0], near_plane0, near_point0, false))
            continue;

        if (glued0 == glued1) {
            far_point1 = far_point0;
            near_point1 = near_point0;
        } else {
            if (!Intersection::line_plane(ordered_lines[1], far_plane1, far_point1, false))
                continue;

            if (!Intersection::line_plane(ordered_lines[1], near_plane1, near_point1, false))
                continue;
        }

        const Vector translation0 = far_point0 - near_point0;
        const Vector translation1 = far_point1 - near_point1;

        const double translation_sum = translation0[0] + translation0[1] + translation0[2] + translation1[0] + translation1[1] + translation1[2];
        if (translation_sum < -1e8 || translation_sum > 1e8)
            continue;

        std::array<Polyline, 4> volumes0 = copy_joint_volumes(joints[joint_index]);
        std::array<Polyline, 4> volumes1 = copy_joint_volumes(joints[joint_index]);

        int shift_amount = 0;
        for (int j = 0; j < 4; j++) {
            const Vector edge = volumes1[0].get_point(j) - volumes1[0].get_point(j + 1);
            if (edge.is_parallel_to(translation1) == 1) {
                shift_amount = j;
                break;
            }
        }

        for (size_t k = 0; k < 4; k++)
            if (volumes1[k].point_count() == 5)
                volumes1[k].shift(shift_amount);

        std::array<Line, 2> joint_lines0 = joints[joint_index].joint_lines;
        std::array<Line, 2> joint_lines1 = joints[joint_index].joint_lines;

        for (int k = 0; k < 2; k++) {
            volumes0[k].translate(translation0);
            volumes1[k].translate(translation1);
            joint_lines0[k] += translation0;
            joint_lines1[k] += translation1;
        }

        if (index_of_plate(elements, joints[joint_index].element_a) == side1) {
            std::swap(glued0, glued1);
            std::swap(side0, side1);
        }

        const int shadow0_index = (int)joints.size();
        joints.push_back(shadow_joint(joints[joint_index], *elements[side0], *elements[glued0], joint_lines0, volumes0));
        joints_map[pair_key(side0, glued0)] = shadow0_index;

        int shadow1_index = -1;
        if (glued0 != glued1) {
            shadow1_index = (int)joints.size();
            joints.push_back(shadow_joint(joints[joint_index], *elements[side1], *elements[glued1], joint_lines1, volumes1));
            joints_map[pair_key(side1, glued1)] = shadow1_index;
        }

        if (glued0 != glued1)
            joints[joint_index].linked_joints = {joints[shadow0_index].guid, joints[shadow1_index].guid};
        else
            joints[joint_index].linked_joints = {joints[shadow0_index].guid};
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Annen alignment
// ═══════════════════════════════════════════════════════════════════════════

/// Annen method: shorten the two overlapping joint lines at a 3-plate corner by the plate thickness and clip the volumes to match.
void align_annen_joints(
    const std::vector<std::vector<int>>& three_valence_groups,
    const std::vector<std::shared_ptr<Plate>>& elements,
    std::vector<FeaturePlate>& joints) {

    const std::unordered_map<uint64_t, int> joints_map = joints_by_element_pair(elements, joints);

    for (size_t group_index = 1; group_index < three_valence_groups.size(); group_index++) {
        const std::vector<int>& group = three_valence_groups[group_index];
        if (group.size() != 4)
            continue;
        const int side0 = group[0];
        const int side1 = group[1];
        const int glued0 = group[2];
        const int glued1 = group[3];

        const auto found0 = joints_map.find(pair_key(side0, side1));
        const auto found1 = joints_map.find(pair_key(glued0, glued1));

        if (found0 == joints_map.end() || found1 == joints_map.end())
            continue;

        FeaturePlate& joint0 = joints[found0->second];
        FeaturePlate& joint1 = joints[found1->second];

        const Line line0 = joint0.joint_lines[0];
        const double distance_to_start = Point::distance(line0.start(), joint1.joint_lines[0].start());
        const double distance_to_end = Point::distance(line0.start(), joint1.joint_lines[0].end());
        const Line line1 = (distance_to_start <= distance_to_end) ? joint1.joint_lines[0] : -joint1.joint_lines[0];

        Line overlap;
        if (!line0.overlap_average(line1, overlap))
            continue;

        double thickness = 0;
        const int element_index = index_of_plate(elements, joint0.element_a);
        if (element_index >= 0 && element_index < (int)elements.size()) {
            const Plate& element = *elements[element_index];
            if (element.polylines.size() >= 2 && element.polylines[0].point_count() > 0 && element.polylines[1].point_count() > 0) {
                const Point p0 = element.polylines[0].get_point(0);
                const Point projected = element.planes[1].project(p0);
                thickness = Point::distance(p0, projected);
            }
        }

        thickness = std::min(thickness, overlap.length() * 0.5 - 1e-9);
        if (thickness < 0.0)
            thickness = 0.0;
        overlap.extend(-thickness, -thickness);

        joint0.joint_lines[0] = overlap;
        joint1.joint_lines[0] = overlap;

        const Point overlap_start = overlap.start();
        const Point overlap_end = overlap.end();

        clip_joint_volumes(joint0, overlap_start, overlap_end);
        clip_joint_volumes(joint1, overlap_end, overlap_start);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Stage
// ═══════════════════════════════════════════════════════════════════════════

void link_three_valence_joints(
    const std::vector<std::vector<int>>& three_valence_groups,
    std::vector<std::shared_ptr<Plate>>& elements,
    std::vector<FeaturePlate>& all_joints,
    double angle) {

    if (three_valence_groups.size() > 1) {
        std::unordered_map<uint64_t, int> joints_map = joints_by_element_pair(elements, all_joints);
        const int instruction = three_valence_groups[0].empty() ? 0 : three_valence_groups[0][0];

        if (instruction == 1) {
            const size_t before_vidy = all_joints.size();
            add_vidy_shadow_joints(three_valence_groups, elements, all_joints, joints_map, angle);
            if (TRACE)
                std::cout << fmt::format("vidy_addition: {} shadow joints created (total {})\n", all_joints.size() - before_vidy, all_joints.size());
        } else {
            align_annen_joints(three_valence_groups, elements, all_joints);
        }
    }
}

} // namespace wood_session
