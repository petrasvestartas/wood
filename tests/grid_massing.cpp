#include "wood_session.h"
#include "src/templates/grid/grid.h"

using namespace session_cpp;
using namespace wood_session;

// ═════════════════════════════
// Measurements
// ═════════════════════════════
static const double RADIUS = 15000.0;
static const double HEIGHT = 7600.0;
static const double OVERHANG = 300.0;
static const double SLIVER_DEGREES = 10.0;

/// Throws with the message when the condition fails.
static void check(bool ok, const std::string& message) {

    if (!ok)
        throw std::runtime_error(message);
}

/// The element reaching furthest from the z axis, its name and distance.
static std::pair<std::string, double> compute_furthest(wood_grid::Grid& grid) {

    std::pair<std::string, double> furthest("", 0.0);
    for (const std::shared_ptr<Element>& element : *grid.objects.elements) {
        const std::pair<std::vector<Point>, std::vector<std::vector<size_t>>> solid =
            element->model_geometry_mesh().to_vertices_and_faces();
        for (const Point& point : solid.first) {
            const double distance = std::hypot(point[0], point[1]);
            if (distance > furthest.second)
                furthest = {element->name, distance};
        }
    }

    return furthest;
}

/// The smallest angle in degrees between two plan edges leaving one vertex, over every level.
static double compute_narrowest(const wood_grid::Grid& grid) {

    double narrowest = 180.0;
    for (const wood_grid::Level& level : grid.guide.levels)
        for (const size_t vertex : level.plan.vertices()) {
            const Point origin = *level.plan.vertex_point(vertex);
            std::vector<double> angles;
            for (const size_t other :
                 level.plan.vertex_neighbors(vertex).value_or(std::vector<size_t>())) {
                const Point point = *level.plan.vertex_point(other);
                angles.push_back(std::atan2(point[1] - origin[1], point[0] - origin[0]));
            }
            std::sort(angles.begin(), angles.end());
            for (size_t i = 0; i + 1 < angles.size(); i++)
                narrowest =
                    std::min(narrowest, (angles[i + 1] - angles[i]) * Tolerance::TO_DEGREES);
        }

    return narrowest;
}

/// The frame of a massing over the radial pattern stays inside its overhang, every two edges at a node well apart.
static void check_massing(const std::string& name, const Mesh& massing) {

    const wood_grid::Building building = wood_grid::Building::from_solid(
        massing, {0.0, 3800.0, HEIGHT}, wood_grid::Pattern::radial({5000.0, 10000.0, RADIUS}, 16)
    );
    wood_grid::Grid grid(
        building, wood_grid::Framing{.system = 2, .span = 0, .spacing = 2500.0, .node = 0}, name
    );

    const std::pair<std::string, double> furthest = compute_furthest(grid);
    const double narrowest = compute_narrowest(grid);
    std::cout << fmt::format(
        "{}: {} elements, furthest {} at {:.0f} mm, narrowest edges {:.1f} degrees\n",
        name,
        grid.objects.elements->size(),
        furthest.first,
        furthest.second,
        narrowest
    );

    check(
        furthest.second <= RADIUS + OVERHANG,
        fmt::format("{}: {} reaches {:.0f} mm", name, furthest.first, furthest.second)
    );
    check(
        narrowest >= SLIVER_DEGREES,
        fmt::format("{}: two edges {:.1f} degrees apart at a node", name, narrowest)
    );
}

/// A prism of sides facets turned by degrees.
static Mesh compute_prism(int sides, double degrees) {

    const Polyline ring =
        Polyline::from_sides(sides, RADIUS, true).transformed(Xform::rotation_z(degrees, true));

    return Mesh::loft({ring}, {ring.transformed(Xform::translation(0.0, 0.0, HEIGHT))});
}

int main() {

    int failures = 0;
    const std::vector<std::pair<std::string, Mesh>> massings = {
        // the facets on the rays, the case the template was written for
        {"aligned", compute_prism(16, 0.0)},
        // the kernel's tessellated cylinder: facets at uneven angles, most of them off the rays
        {"tessellated", BRep::create_cylinder(RADIUS, HEIGHT).mesh()},
        // every facet corner between two rays
        {"turned", compute_prism(16, 5.625)},
        {"seventeen", compute_prism(17, 3.0)},
    };

    for (const std::pair<std::string, Mesh>& massing : massings)
        try {
            check_massing(massing.first, massing.second);
        } catch (const std::exception& error) {
            std::cout << "FAILED " << error.what() << std::endl;
            failures++;
        }

    if (failures == 0)
        std::cout << "grid massing checks passed" << std::endl;

    return failures;
}
