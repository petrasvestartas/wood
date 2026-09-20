#include "wood_session.h"
using namespace session_cpp;
using namespace wood_session;

static int failures = 0;

static void check(bool condition, std::string_view name) {

    if (condition)
        return;

    std::cerr << "FAIL " << name << "\n";
    ++failures;
}

/// One 10 x 10 x 2 plate in a scene.
static WoodSession scene() {

    WoodSession session("assign");
    std::shared_ptr<Plate> plate = std::make_shared<Plate>();
    plate->polylines = {
        Polyline({Point(0, 0, 0), Point(10, 0, 0), Point(10, 10, 0), Point(0, 10, 0), Point(0, 0, 0)}),
        Polyline({Point(0, 0, 2), Point(10, 0, 2), Point(10, 10, 2), Point(0, 10, 2), Point(0, 0, 2)})
    };
    session.add(plate);

    return session;
}

static void insertion_tolerance() {

    config::reset_defaults();
    WoodSession session = scene();
    const double distances[] = {0.0, 0.19, 0.5, 0.99, 1.0, 1.5};

    for (const double distance : distances) {
        session.assign_insertion_vectors({Line::from_points(Point(5, -distance, 0), Point(5, -distance, 3))});
        const std::vector<Vector>& vectors = session.plates()[0]->insertion_vectors();
        check(vectors.size() == 6, "Insertion Slot Count");
        check(vectors[2][2] == (distance < 1.0 ? 3.0 : 0.0), "Insertion Tolerance");
        for (size_t i = 0; i < vectors.size(); ++i)
            if (i != 2)
                check(vectors[i].magnitude() == 0.0, "Unassigned Insertion Slots");
    }

    session.settings.distance = 0.01;
    session.settings.distance_squared = 0.04;
    session.assign_insertion_vectors({Line::from_points(Point(5, -1.5, 0), Point(5, -1.5, 3))});
    check(session.plates()[0]->insertion_vectors()[2][2] == 3.0, "Retuned Insertion Tolerance");
}

static void joint_slots() {

    config::reset_defaults();
    WoodSession session = scene();
    const std::shared_ptr<Plate> plate = session.plates()[0];

    session.assign_joint_types({Point(5, -0.5, 0), Point(5, -0.5, 2), Point(10.5, 5, 0)}, {-12, -20, 30});
    check(plate->joint_types.size() == 6, "Joint Slot Count");
    check(plate->joint_types == std::vector<int>({12, 20, -1, 30, -1, -1}), "Joint Face And Side Slots");

    session.assign_joint_types({Point(5, -1.0, 0)}, {12});
    check(plate->joint_types == std::vector<int>(6, -1), "Joint Tolerance Boundary");

    session.assign_joint_types({Point(5, 0, 0)}, {});
    check(plate->joint_types == std::vector<int>(6, -1), "Missing Joint Types");
}

static void empty_inputs() {

    config::reset_defaults();
    WoodSession empty("empty");
    empty.assign_insertion_vectors({});
    empty.assign_joint_types({}, {});
    check(empty.plates().empty(), "Empty Scene");

    WoodSession session = scene();
    const std::shared_ptr<Plate> plate = session.plates()[0];
    plate->joint_types = {42};
    plate->insertion_vectors() = {Vector(1, 2, 3)};
    session.assign_insertion_vectors({});
    session.assign_joint_types({}, {});
    check(plate->joint_types == std::vector<int>(6, -1), "Empty Points Reset Types");
    for (const Vector& vector : plate->insertion_vectors())
        check(vector.magnitude() == 0.0, "Empty Lines Reset Vectors");

    WoodSession bare("bare");
    bare.add(std::make_shared<Plate>());
    bare.assign_joint_types({Point(0, 0, 0)}, {12});
    bare.assign_insertion_vectors({Line::from_points(Point(0, 0, 0), Point(0, 0, 1))});
    check(bare.plates()[0]->joint_types == std::vector<int>(2, -1) && bare.plates()[0]->insertion_vectors().size() == 2, "Empty Element Outlines");
}

int main() {

    insertion_tolerance();
    joint_slots();
    empty_inputs();
    config::reset_defaults();

    return failures ? 1 : 0;
}
