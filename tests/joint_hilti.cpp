#include "wood_session.h"
#include "wood_element_geometry.h"
#include "wood_brep_drill.h"
#include <numbers>

using namespace session_cpp;
using namespace wood_session;

// ═══════════════════════════════════════════════════════════════════════════
// Tolerances and the fixture
// ═══════════════════════════════════════════════════════════════════════════

static const double SAME = 1e-6; // mm and mm3 relative, a part against itself at another angle
static const double ZERO_REL = 1e-6; // relative to a part, an overlap or a gap
static const double SLAB_WIDTH = 600.0; // along the slope, the seam to the far end
static const double SLAB_DEPTH = 400.0; // along the seam
static const double SLAB_THICKNESS = 200.0; // CLT
static const std::vector<double> ANGLES = {0.0, 10.0, 20.0, 30.0, 40.0, 50.0};

/// Throws with the message when the condition fails.
static void check(bool ok, const std::string& message) {

    if (!ok)
        throw std::runtime_error(message);
}

/// A slab folded half the angle down from the ridge along y, on side -1 (x < 0) or +1, its seam face mitred on x = 0.
static std::shared_ptr<Plate> folded_slab(double angle, double side, const std::string& name) {

    const double half = 0.5 * angle * std::numbers::pi / 180.0;
    const Vector down_slope(side * std::cos(half), 0.0, -std::sin(half));
    const Vector up(side * std::sin(half), 0.0, std::cos(half));
    const Vector along(0.0, SLAB_DEPTH, 0.0);
    const Point ridge(0.0, 0.0, 0.0);
    const Point seam_foot = ridge - up * SLAB_THICKNESS + down_slope * (SLAB_THICKNESS * std::tan(half));
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

/// A part's shape as numbers: its volume, the sorted lengths of its loops' edges and the distance between its loops.
static std::vector<double> shape(const JointBeam& joint, size_t index) {

    std::vector<double> numbers = {compute_volume(joint.part_mesh(index))};
    const std::array<Polyline, 2>& loops = joint.parts[index];
    std::vector<double> edges;
    for (const Polyline& loop : loops)
        for (size_t i = 0; i + 1 < loop.point_count(); i++)
            edges.push_back(loop[i].distance(loop[i + 1]));
    std::sort(edges.begin(), edges.end());
    numbers.insert(numbers.end(), edges.begin(), edges.end());
    numbers.push_back(loops[0][0].distance(loops[1][0]));

    return numbers;
}

// ═══════════════════════════════════════════════════════════════════════════
// One fold angle
// ═══════════════════════════════════════════════════════════════════════════

/// The pair folded at the angle with its connector, checked; returns the parts' shapes for the comparison across angles.
static std::vector<std::vector<double>> check_angle(double angle) {

    WoodSession scene(fmt::format("hilti_{:g}", angle));
    const std::shared_ptr<Plate> left = folded_slab(angle, -1.0, "left");
    const std::shared_ptr<Plate> right = folded_slab(angle, 1.0, "right");
    scene.add(left);
    scene.add(right);

    const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(left, right);
    check(contact != nullptr, fmt::format("{:g} degrees: the slabs have no face contact", angle));
    const std::shared_ptr<JointBeam> hilti = JointBeam::hilti(*left, *right, *contact);
    check(hilti != nullptr, fmt::format("{:g} degrees: no connector on the seam", angle));
    scene.add(hilti);
    scene.add_interaction(hilti, left, hilti->interaction(0));
    scene.add_interaction(hilti, right, hilti->interaction(1));

    check(hilti->parts.size() == 4 && hilti->drill_lines.size() == 1, fmt::format("{:g} degrees: {} parts and {} rods, not 2 halves, 2 discs and 1 rod", angle, hilti->parts.size(), hilti->drill_lines.size()));

    // per slab: its half and disc inside its stock, clear of its cut model, and the stock covered by the model, the parts and the slot
    const std::array<std::shared_ptr<Plate>, 2> slabs = {left, right};
    for (size_t side = 0; side < 2; side++) {
        const std::shared_ptr<Plate>& slab = slabs[side];
        const Mesh& stock = slab->element_geometry_mesh();
        const Mesh& model = slab->model_geometry_mesh();
        check(model.is_closed() && compute_volume(model) < compute_volume(stock), fmt::format("{:g} degrees: {} is not cut", angle, slab->name));
        const BRep& brep = slab->model_geometry_brep();
        check(brep.is_valid() && brep.is_solid(), fmt::format("{:g} degrees: {} model BRep valid {} solid {}", angle, slab->name, brep.is_valid(), brep.is_solid()));

        std::vector<Mesh> filling = {model};
        for (size_t index : {2 * side, 2 * side + 1}) {
            const Mesh part = hilti->part_mesh(index);
            const double volume = compute_volume(part);
            const double inside = boolean_volume(part, stock, SolidOperation::intersect);
            check(std::abs(inside - volume) <= ZERO_REL * volume, fmt::format("{:g} degrees: part {} has {:.3f} of its {:.3f} mm3 outside {}", angle, index, volume - inside, volume, slab->name));
            // shrunk a thousandth about its centre, so a face it shares with its pocket's wall is no overlap and a real one stays
            const Mesh shrunk = part.transformed(Xform::scale_uniform(part.centroid(), 1.0 - 1e-3));
            const double overlap = boolean_volume(shrunk, model, SolidOperation::intersect);
            check(overlap <= ZERO_REL * volume, fmt::format("{:g} degrees: part {} overlaps the cut {} by {:.3f} mm3", angle, index, slab->name, overlap));
            filling.push_back(part);
        }
        const std::array<Polyline, 2>& slot = hilti->cutters[side][2];
        filling.push_back(Mesh::loft({slot[0]}, {slot[1]}, true));
        const Mesh gap = solid_difference(stock, filling);
        const double unfilled = gap.number_of_faces() == 0 ? 0.0 : compute_volume(gap);
        check(unfilled <= ZERO_REL * compute_volume(hilti->part_mesh(2 * side)), fmt::format("{:g} degrees: {} lost {:.3f} mm3 neither its half, its disc nor its slot fill", angle, slab->name, unfilled));
    }

    // the rod through both halves and both discs, each part bored exactly
    const Line& rod = hilti->drill_lines[0];
    for (size_t index = 0; index < 4; index++) {
        check(!inside_stretches(hilti->part_mesh(index), rod).empty(), fmt::format("{:g} degrees: the rod misses part {}", angle, index));
        check(cylinders(hilti->part_brep(index)) > 0, fmt::format("{:g} degrees: part {} has no exact bore", angle, index));
    }

    std::vector<std::vector<double>> shapes;
    for (size_t index = 0; index < 4; index++)
        shapes.push_back(shape(*hilti, index));

    return shapes;
}

// ═══════════════════════════════════════════════════════════════════════════
// The run
// ═══════════════════════════════════════════════════════════════════════════

int main() {

    const std::vector<std::vector<double>> flat = check_angle(ANGLES[0]);

    // the two halves are one shape, the two discs another
    for (size_t index : {0, 1})
        for (size_t k = 0; k < flat[index].size(); k++)
            check(std::abs(flat[index][k] - flat[index + 2][k]) <= SAME * std::max(1.0, std::abs(flat[index][k])), fmt::format("part {} and part {} differ at number {}", index, index + 2, k));

    // every part the same at every angle
    for (size_t a = 1; a < ANGLES.size(); a++) {
        const std::vector<std::vector<double>> folded = check_angle(ANGLES[a]);
        for (size_t index = 0; index < 4; index++)
            for (size_t k = 0; k < flat[index].size(); k++)
                check(std::abs(flat[index][k] - folded[index][k]) <= SAME * std::max(1.0, std::abs(flat[index][k])), fmt::format("part {} at {:g} degrees differs from the flat one at number {}: {:.9g} against {:.9g}", index, ANGLES[a], k, folded[index][k], flat[index][k]));
    }

    std::cout << fmt::format("joint_hilti: halves of {:.0f} mm3 and discs of {:.0f} mm3 identical at 0 to 50 degrees, inside their slabs, filling their pockets, bored by the rod", flat[0][0], flat[1][0]) << std::endl;
    return 0;
}
