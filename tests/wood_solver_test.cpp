#include "wood_joint.h"
#include "wood_session.h"
#include "wood_cut.h"

#include <cstdio>
using namespace session_cpp;
using namespace wood_session;

namespace {
#include "joints/ss_e_op_4.h"
#include "joints/ss_e_op_5.h"
}

static int failures = 0;

static void check(bool condition, const char* name) {

    if (condition)
        return;

    std::fprintf(stderr, "FAIL %s\n", name);
    ++failures;
}

static Polyline outline(double offset) {

    std::vector<Point> points;
    for (size_t i = 0; i < 5; ++i)
        points.emplace_back(offset + i, 0, 0);

    return Polyline(points);
}

static std::vector<WoodJoint> linked_joints(const std::array<int, 4>& sequence) {

    std::vector<WoodJoint> joints(2);
    joints[0].element_a = "plate";
    joints[1].element_a = "plate";
    joints[0].linked_joints = {1};
    joints[0].linked_joints_seq = {{sequence}};

    for (size_t i = 0; i < 2; ++i) {
        joints[0].male_outlines[i] = {outline(0), outline(0)};
        joints[1].male_outlines[i] = {outline(10)};
    }

    return joints;
}

static void linked_geometry() {

    std::vector<WoodJoint> joints = linked_joints({1, 1, 0, 1});
    merge_linked_joints(joints[0], joints);

    const double expected[] = {0, 10, 1, 11, 2, 12, 3, 4};
    for (size_t i = 0; i < 2; ++i) {
        const std::vector<Point> points = joints[0].male_outlines[i][0].get_points();
        check(points.size() == 8, "Linked Outline Count");
        for (size_t j = 0; j < points.size() && j < 8; ++j)
            check(points[j][0] == expected[j], "Linked Outline Order");
        check(joints[1].male_outlines[i].empty(), "Merged Shadow Cleared");
    }

    const std::array<int, 4> invalid[] = {
        {1, -1, 0, 1}, {-1, 1, 0, 1}, {1, 0, 0, 1}, {1, 1, 0, -1},
        {8, 1, 0, 1}, {1, 20, 0, 1}, {1, 1, 4, 2}, {1, 1, -1, 1}
    };
    for (const std::array<int, 4>& sequence : invalid) {
        joints = linked_joints(sequence);
        merge_linked_joints(joints[0], joints);
        check(joints[0].male_outlines[0][0].point_count() == 5, "Invalid Merge Preserves Primary");
        check(joints[1].male_outlines[0].size() == 1, "Invalid Merge Preserves Shadow");
    }

    joints = linked_joints({1, 1, 0, 1});
    joints[1].male_outlines[1].clear();
    merge_linked_joints(joints[0], joints);
    check(joints[1].male_outlines[0].size() == 1, "Missing Shadow Face Preserves Geometry");

    joints = linked_joints({1, 1, 0, 1});
    joints[0].linked_joints = {0};
    merge_linked_joints(joints[0], joints);
    check(joints[0].male_outlines[0].size() == 2, "Self Link Preserves Geometry");

    joints = linked_joints({1, 1, 0, 1});
    joints[0].male_outlines[0].resize(4);
    joints[0].linked_joints_seq[0].push_back({1, -1, 0, 1});
    merge_linked_joints(joints[0], joints);
    check(joints[0].male_outlines[0][0].point_count() == 5, "Later Invalid Sequence Rolls Back Merge");
    check(joints[1].male_outlines[0].size() == 1, "Later Invalid Sequence Preserves Shadow");
}

static void division_limits() {

    WoodJoint joint;
    joint.joint_lines[0] = Line::from_points(Point(0, 0, 0), Point(10, 0, 0));
    const double distances[] = {2, 0, -1, 1e-300, 1000, std::numeric_limits<double>::quiet_NaN()};
    const int expected[] = {5, 1, 1, 100, 1, 1};

    for (size_t i = 0; i < 6; ++i) {
        joint_get_divisions(joint, distances[i]);
        check(joint.divisions == expected[i], "Division Limits");
        check(joint.length == 10.0, "Joint Length");
    }
}

static void linked_construction() {

    std::vector<WoodJoint> joints(3);
    joints[0].element_a = "primary";
    joints[1].element_a = "primary";
    joints[2].element_a = "other";
    joints[0].linked_joints = {1, 2};
    joints[0].divisions = 3;

    ss_e_op_5(joints[0], joints, false);
    ss_e_op_5(joints[0], joints, false);
    check(joints[0].linked_joints_seq.size() == 2, "Repeated Construction Replaces Sequences");

    merge_linked_joints(joints[0], joints);
    check(joints[0].male_outlines[0][0].point_count() == 28, "Linked Finger Outline");
    check(joints[0].female_outlines[0][0].point_count() == 22, "Linked Female Outline");
    check(joints[1].male_outlines[0].empty() && joints[2].male_outlines[0].empty(), "Both Shadows Merged");

    joints[0].linked_joints = {1, 100};
    ss_e_op_5(joints[0], joints, false);
    check(joints[0].male_outlines[0][0].point_count() == 28, "Invalid Second Link Preserves Geometry");
}

static void missing_datasets(const std::filesystem::path& folder) {

    const std::filesystem::path empty = folder / "empty.obj";
    std::ofstream(empty).close();

    for (const std::filesystem::path& path : {empty, folder / "missing.obj"}) {
        bool rejected = false;
        try {
            internal::load_polylines(path.string());
        } catch (const std::runtime_error&) {
            rejected = true;
        }
        check(rejected, "Missing Or Empty Beam Dataset");
    }
}

static void dataset_tolerance(const std::filesystem::path& folder) {

    const std::filesystem::path path = folder / "axis.obj";
    {
        std::ofstream file(path);
        file << "v 0 0 0\nv 0.01 0 0\nv 1 0 0\ncurv 0 1 1 2 3\nend\n";
    }

    globals::DUPLICATE_PTS_TOL = 0.1;
    std::vector<Polyline> axes = internal::load_polylines(path.string());
    check(axes.size() == 1 && axes[0].point_count() == 2, "Configured Beam Deduplication");
    axes = internal::load_polylines(path.string(), 0.001);
    check(axes.size() == 1 && axes[0].point_count() == 3, "Explicit Beam Deduplication");

    bool rejected = false;
    try {
        internal::load_plates(path.string());
    } catch (const std::runtime_error&) {
        rejected = true;
    }
    check(rejected, "Unpaired Plate Outline");
}

static void beam_geometry(const std::filesystem::path& folder) {

    globals::DATA_SET_INPUT_FOLDER = folder.string();
    globals::DATA_SET_OUTPUT_FILE = "beams.pb";
    std::vector<Polyline> axes{
        Polyline({Point(-5, 0, 0), Point(5, 0, 0)}),
        Polyline({Point(0, -5, 0), Point(0, 5, 0)})
    };

    beam_volumes_pipeline(axes, {{1}, {1}}, {}, {-1}, 1, 10, 0.9, 1);
    const Session valid = Session::pb_load((folder / "output/beams.pb").string());
    check(valid.objects.polylines->size() == 6, "Crossing Beam Rectangles");

    beam_volumes_pipeline(axes, {{1}}, {}, {-1}, 1, 10, 0.9, 1);
    const Session missing = Session::pb_load((folder / "output/beams.pb").string());
    check(missing.objects.polylines->size() == 2, "Missing Beam Radius Skips Volumes");

    beam_volumes_pipeline(axes, {{1}, {1}}, {{Vector(1, 0, 0)}, {Vector(0, 1, 0)}}, {-1}, 1, 10, 0.9, 1);
    const Session parallel = Session::pb_load((folder / "output/beams.pb").string());
    check(parallel.objects.polylines->size() == 2, "Degenerate Beam Frames Skip Volumes");

    axes[1] = Polyline({Point(0, 0, 0), Point(0, 5, 0)});
    beam_volumes_pipeline(axes, {{1}, {1}}, {}, {-1}, 1, 10, 0.9, 1);
    const Session side = Session::pb_load((folder / "output/beams.pb").string());
    check(side.objects.polylines->size() == 6, "Side To End Beam Trimming");

    axes[0] = Polyline({Point(-5, 0, 0), Point(0, 0, 0)});
    beam_volumes_pipeline(axes, {{1}, {1}}, {}, {-1}, 1, 10, 0.9, 1);
    const Session end = Session::pb_load((folder / "output/beams.pb").string());
    check(end.objects.polylines->size() == 6, "End To End Beam Trimming");

    axes[0] = Polyline({Point(0, 0, 0), Point(0, 0, 0)});
    beam_volumes_pipeline(axes, {{1}, {1}}, {}, {-1}, 1, 10, 0.9, 1);
    const Session degenerate = Session::pb_load((folder / "output/beams.pb").string());
    check(degenerate.objects.polylines->size() == 2, "Zero Length Beam Skips Volumes");
}

int main() {

    globals::reset_defaults();
    const std::filesystem::path folder = std::filesystem::temp_directory_path() / ("wood-solver-" + ::guid());
    std::filesystem::create_directories(folder);

    try {
        linked_geometry();
        division_limits();
        linked_construction();
        missing_datasets(folder);
        dataset_tolerance(folder);
        beam_geometry(folder);
    } catch (const std::exception& error) {
        std::fprintf(stderr, "FAIL %s\n", error.what());
        ++failures;
    }

    std::filesystem::remove_all(folder);
    globals::reset_defaults();

    return failures ? 1 : 0;
}
