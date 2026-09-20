#include "wood_session.h"
#include "wood_assignment.h"
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
        assign_insertion_vectors(session.plates(), session.settings, {Line::from_points(Point(5, -distance, 0), Point(5, -distance, 3))});
        const std::vector<Vector>& vectors = session.plates()[0]->insertion_vectors();
        check(vectors.size() == 6, "Insertion Slot Count");
        check(vectors[2][2] == (distance < 1.0 ? 3.0 : 0.0), "Insertion Tolerance");
        for (size_t i = 0; i < vectors.size(); ++i)
            if (i != 2)
                check(vectors[i].magnitude() == 0.0, "Unassigned Insertion Slots");
    }

    session.settings.distance = 0.01;
    session.settings.distance_squared = 0.04;
    assign_insertion_vectors(session.plates(), session.settings, {Line::from_points(Point(5, -1.5, 0), Point(5, -1.5, 3))});
    check(session.plates()[0]->insertion_vectors()[2][2] == 3.0, "Retuned Insertion Tolerance");
}

static void joint_slots() {

    config::reset_defaults();
    WoodSession session = scene();
    const std::shared_ptr<Plate> plate = session.plates()[0];

    assign_feature_types(session.plates(), session.settings, {Point(5, -0.5, 0), Point(5, -0.5, 2), Point(10.5, 5, 0)}, {-12, -20, 30});
    check(plate->feature_types.size() == 6, "Joint Slot Count");
    check(plate->feature_types == std::vector<int>({12, 20, -1, 30, -1, -1}), "Joint Face And Side Slots");

    assign_feature_types(session.plates(), session.settings, {Point(5, -1.0, 0)}, {12});
    check(plate->feature_types == std::vector<int>(6, -1), "Joint Tolerance Boundary");

    assign_feature_types(session.plates(), session.settings, {Point(5, 0, 0)}, {});
    check(plate->feature_types == std::vector<int>(6, -1), "Missing Joint Types");
}

static void empty_inputs() {

    config::reset_defaults();
    WoodSession empty("empty");
    assign_insertion_vectors(empty.plates(), empty.settings, {});
    assign_feature_types(empty.plates(), empty.settings, {}, {});
    check(empty.plates().empty(), "Empty Scene");

    WoodSession session = scene();
    const std::shared_ptr<Plate> plate = session.plates()[0];
    plate->feature_types = {42};
    plate->insertion_vectors() = {Vector(1, 2, 3)};
    assign_insertion_vectors(session.plates(), session.settings, {});
    assign_feature_types(session.plates(), session.settings, {}, {});
    check(plate->feature_types == std::vector<int>(6, -1), "Empty Points Reset Types");
    for (const Vector& vector : plate->insertion_vectors())
        check(vector.magnitude() == 0.0, "Empty Lines Reset Vectors");

    WoodSession bare("bare");
    bare.add(std::make_shared<Plate>());
    assign_feature_types(bare.plates(), bare.settings, {Point(0, 0, 0)}, {12});
    assign_insertion_vectors(bare.plates(), bare.settings, {Line::from_points(Point(0, 0, 0), Point(0, 0, 1))});
    check(bare.plates()[0]->feature_types == std::vector<int>(2, -1) && bare.plates()[0]->insertion_vectors().size() == 2, "Empty Element Outlines");
}

int main() {

    insertion_tolerance();
    joint_slots();
    empty_inputs();
    config::reset_defaults();

    return failures ? 1 : 0;
}
