#include "wood_assign.h"
#include "wood_session.h"

#include <cstdio>
using namespace session_cpp;
using namespace wood_session;

static int failures = 0;

static void check(bool condition, const char* name) {

    if (condition)
        return;

    std::cerr << "FAIL " << name << "\n";
    ++failures;
}

static std::vector<std::shared_ptr<Plate>> plates() {

    std::shared_ptr<Plate> element = std::make_shared<Plate>();
    element->polylines = {
        Polyline({Point(0, 0, 0), Point(10, 0, 0), Point(10, 10, 0), Point(0, 10, 0), Point(0, 0, 0)}),
        Polyline({Point(0, 0, 2), Point(10, 0, 2), Point(10, 10, 2), Point(0, 10, 2), Point(0, 0, 2)})
    };

    return {element};
}

static void insertion_tolerance() {

    globals::reset_defaults();
    const std::vector<std::shared_ptr<Plate>> elements = plates();
    std::vector<std::vector<Vector>> vectors;
    const double distances[] = {0.0, 0.19, 0.5, 0.99, 1.0, 1.5};

    for (const double distance : distances) {
        assign_insertion(elements, {Line::from_points(Point(5, -distance, 0), Point(5, -distance, 3))}, vectors);
        check(vectors.size() == 1 && vectors[0].size() == 6, "Insertion Slot Count");
        check(vectors[0][2][2] == (distance < 1.0 ? 3.0 : 0.0), "Insertion Tolerance");
        for (size_t i = 0; i < vectors[0].size(); ++i)
            if (i != 2)
                check(vectors[0][i].magnitude() == 0.0, "Unassigned Insertion Slots");
    }

    globals::DISTANCE = 0.01;
    globals::DISTANCE_SQUARED = 0.04;
    assign_insertion(elements, {Line::from_points(Point(5, -1.5, 0), Point(5, -1.5, 3))}, vectors);
    check(vectors[0][2][2] == 3.0, "Retuned Insertion Tolerance");
}

static void joint_slots() {

    globals::reset_defaults();
    const std::vector<std::shared_ptr<Plate>> elements = plates();
    std::vector<std::vector<int>> types;

    assign_joint(elements, {Point(5, -0.5, 0), Point(5, -0.5, 2), Point(10.5, 5, 0)}, {-12, -20, 30}, types);
    check(types.size() == 1 && types[0].size() == 6, "Joint Slot Count");
    check(types[0] == std::vector<int>({12, 20, -1, 30, -1, -1}), "Joint Face And Side Slots");

    assign_joint(elements, {Point(5, -1.0, 0)}, {12}, types);
    check(types[0] == std::vector<int>(6, -1), "Joint Tolerance Boundary");

    assign_joint(elements, {Point(5, 0, 0)}, {}, types);
    check(types[0] == std::vector<int>(6, -1), "Missing Joint Types");
}

static void empty_inputs() {

    std::vector<std::vector<Vector>> vectors(1, std::vector<Vector>(1, Vector(1, 2, 3)));
    std::vector<std::vector<int>> types{{42}};

    assign_insertion({}, {}, vectors);
    assign_joint({}, {}, {}, types);
    check(vectors.empty() && types.empty(), "Empty Elements Clear Outputs");

    const std::vector<std::shared_ptr<Plate>> elements = plates();
    assign_insertion(elements, {}, vectors);
    assign_joint(elements, {}, {}, types);
    check(types[0] == std::vector<int>(6, -1), "Empty Points Reset Types");
    for (const Vector& vector : vectors[0])
        check(vector.magnitude() == 0.0, "Empty Lines Reset Vectors");

    assign_joint({std::make_shared<Plate>()}, {Point(0, 0, 0)}, {12}, types);
    assign_insertion({std::make_shared<Plate>()}, {Line::from_points(Point(0, 0, 0), Point(0, 0, 1))}, vectors);
    check(types[0] == std::vector<int>(2, -1) && vectors[0].size() == 2, "Empty Element Outlines");
}

int main() {

    insertion_tolerance();
    joint_slots();
    empty_inputs();
    globals::reset_defaults();

    return failures ? 1 : 0;
}
