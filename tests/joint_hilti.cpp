#include "wood_session.h"
#include "wood_element_geometry.h"
#include "wood_brep_drill.h"
#include <numbers>

using namespace session_cpp;
using namespace wood_session;

// ═══════════════════════════════════════════════════════════════════════════
// Tolerances and the fixture
// ═══════════════════════════════════════════════════════════════════════════
static const double SAME = 1e-6;
static const double ZERO_REL = 1e-6;
static const double SLAB_WIDTH = 600.0;
static const double SLAB_DEPTH = 400.0;
static const double SLAB_THICKNESS = 200.0;
static const std::vector<double> ANGLES = {0.0, 10.0, 20.0, 30.0, 40.0, 50.0};

/// Throws with the message when the condition fails.
static void check(bool ok, const std::string& message) {

    if (!ok)
        throw std::runtime_error(message);
}

/// A slab folded half the angle down from the ridge along y, on side -1 (x < 0) or +1.
static std::shared_ptr<Plate> folded_slab(double angle, double side, const std::string& name) {

    const double half = 0.5 * angle * std::numbers::pi / 180.0;
    const Vector down_slope(side * std::cos(half), 0.0, -std::sin(half));
    const Vector up(side * std::sin(half), 0.0, std::cos(half));
    const Vector along(0.0, SLAB_DEPTH, 0.0);
    const Point ridge(0.0, 0.0, 0.0);
    const Point seam_foot =
        ridge - up * SLAB_THICKNESS + down_slope * (SLAB_THICKNESS * std::tan(half));
    const Point far_foot = ridge - up * SLAB_THICKNESS + down_slope * SLAB_WIDTH;
    const Point far_top = ridge + down_slope * SLAB_WIDTH;

    const Polyline bottom({seam_foot, far_foot, far_foot + along, seam_foot + along, seam_foot});
    const Polyline top({ridge, far_top, far_top + along, ridge + along, ridge});

    return std::make_shared<Plate>(bottom, top, name);
}

/// The volume of a boolean of two solids, zero for an empty result.
static double boolean_volume(const Mesh& a, const Mesh& b, SolidOperation operation) {

    const Mesh result = solid_boolean(a, b, operation);
    return result.number_of_faces() == 0 ? 0.0 : compute_volume(result);
}

/// The faces of a BRep on a rational surface: its cylinders.
static size_t cylinders(const BRep& brep) {

    size_t count = 0;
    for (const BRepFace& face : brep.m_faces)
        count += brep.m_surfaces[face.surface_index].is_rational();

    return count;
}

// ═══════════════════════════════════════════════════════════════════════════
// One fold angle
// ═══════════════════════════════════════════════════════════════════════════

/// The pair folded at the angle with its connector, checked.
static std::array<double, 2> check_angle(double angle) {

    WoodSession scene(fmt::format("hilti_{:g}", angle));
    const std::shared_ptr<Plate> left = folded_slab(angle, -1.0, "left");
    const std::shared_ptr<Plate> right = folded_slab(angle, 1.0, "right");
    scene.add(left);
    scene.add(right);

    const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(left, right);
    check(contact != nullptr, fmt::format("{:g} degrees: the slabs have no face contact", angle));
    const std::shared_ptr<JointPlate> hilti =
        JointPlate::side_side_inplane_hilti(*left, *right, *contact);
    check(hilti != nullptr, fmt::format("{:g} degrees: no connector on the seam", angle));
    scene.add(hilti);
    scene.add_interaction(hilti, left, hilti->interaction(0));
    scene.add_interaction(hilti, right, hilti->interaction(1));

    const Mesh key = hilti->key_mesh();
    const std::vector<Line> bolts = hilti->drill_axes();
    check(
        key.is_closed() && bolts.size() == 1,
        fmt::format(
            "{:g} degrees: key closed {} and {} bolts, not 1", angle, key.is_closed(), bolts.size()
        )
    );
    const double key_volume = compute_volume(key);

    const Mesh shrunk = key.transformed(Xform::scale_uniform(key.centroid(), 1.0 - 1e-3));

    // per slab: the key clear of its cut model, and the slab losing only its pocket
    const std::array<std::shared_ptr<Plate>, 2> slabs = {left, right};
    std::array<Plane, 2> tops;
    std::vector<Mesh> stocks;
    for (size_t side = 0; side < 2; side++) {
        const std::shared_ptr<Plate>& slab = slabs[side];
        const Mesh& stock = slab->element_geometry_mesh();
        const Mesh& model = slab->model_geometry_mesh();
        stocks.push_back(stock);
        check(
            model.is_closed() && compute_volume(model) < compute_volume(stock),
            fmt::format("{:g} degrees: {} is not cut", angle, slab->name)
        );
        const BRep& brep = slab->model_geometry_brep();
        check(
            brep.is_valid() && brep.is_solid(),
            fmt::format(
                "{:g} degrees: {} model BRep valid {} solid {}",
                angle,
                slab->name,
                brep.is_valid(),
                brep.is_solid()
            )
        );

        // the top face: the slab's plane facing up the most
        tops[side] = slab->planes[0];
        for (const Plane& plane : slab->planes)
            if (plane.z_axis()[2] > tops[side].z_axis()[2])
                tops[side] = plane;

        const double overlap = boolean_volume(shrunk, model, SolidOperation::intersect);
        check(
            overlap <= ZERO_REL * key_volume,
            fmt::format(
                "{:g} degrees: the key overlaps the cut {} by {:.3f} mm3",
                angle,
                slab->name,
                overlap
            )
        );

        const Mesh pocket =
            JointPlate::side_solids(hilti->connections[0], static_cast<int>(side)).at(0);
        const Mesh gap = solid_difference(stock, {model, pocket});
        const double unfilled = gap.number_of_faces() == 0 ? 0.0 : compute_volume(gap);
        check(
            unfilled <= ZERO_REL * key_volume,
            fmt::format(
                "{:g} degrees: {} lost {:.3f} mm3 its pocket does not account for",
                angle,
                slab->name,
                unfilled
            )
        );
    }

    const Mesh outside = solid_difference(shrunk, stocks);
    double below = 0.0;
    for (const std::pair<const size_t, VertexData>& vertex : outside.vertex)
        below = std::max(
            below,
            -std::max(
                tops[0].signed_distance(vertex.second.position()),
                tops[1].signed_distance(vertex.second.position())
            )
        );
    check(
        below <= 1e-6 * SLAB_THICKNESS,
        fmt::format("{:g} degrees: the key leaves the slabs {:.6f} below their tops", angle, below)
    );

    check(
        !inside_stretches(key, bolts[0]).empty(),
        fmt::format("{:g} degrees: the bolt misses the key", angle)
    );
    check(
        cylinders(hilti->element_geometry_brep()) > 0,
        fmt::format("{:g} degrees: the bolt is no exact cylinder", angle)
    );

    return {key_volume, bolts[0].length()};
}

// ═══════════════════════════════════════════════════════════════════════════
// The run
// ═══════════════════════════════════════════════════════════════════════════
int main() {

    const std::array<double, 2> flat = check_angle(ANGLES[0]);

    for (size_t a = 1; a < ANGLES.size(); a++) {
        const std::array<double, 2> folded = check_angle(ANGLES[a]);
        for (size_t k = 0; k < 2; k++)
            check(
                std::abs(flat[k] - folded[k]) <= SAME * std::max(1.0, flat[k]),
                fmt::format(
                    "at {:g} degrees number {} differs from the flat one: {:.9g} against {:.9g}",
                    ANGLES[a],
                    k,
                    folded[k],
                    flat[k]
                )
            );
    }

    std::cout << fmt::format(
                     "joint_hilti: a key of {:.0f} mm3 and a bolt of {:.1f} mm identical at 0 to "
                     "50 degrees, in the slabs or standing out of their tops, each slab losing "
                     "only its 2024 bow-tie pocket",
                     flat[0],
                     flat[1]
                 )
              << std::endl;
    return 0;
}
