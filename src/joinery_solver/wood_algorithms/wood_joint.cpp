#include "pch.h"
#include "wood_joint.h"
using namespace session_cpp;

namespace wood_session {

constexpr bool TRACE = false;

// ═══════════════════════════════════════════════════════════════════════════
// WoodJoint - Operators
// ═══════════════════════════════════════════════════════════════════════════

std::ostream& operator<<(std::ostream& os, const WoodJoint& j) { return os << j.str(); }

// ═══════════════════════════════════════════════════════════════════════════
// WoodJoint - Geometry
// ═══════════════════════════════════════════════════════════════════════════

// ═══════════════════════════════════════════════════════════════════════════
// WoodJoint - Geometry
// ═══════════════════════════════════════════════════════════════════════════

const std::string& WoodJoint::feature_guid(int side) const {

    std::string& id = feature_guids[side];
    if (id.empty())
        id = ::guid();

    return id;
}

void WoodJoint::sync_features() {
    for (int side = 0; side < 2; ++side) {
        ElementFeature& f = element_features[side];
        f.guid() = feature_guid(side);
        f.feature_type = "joint";
        f.name = name.empty() ? "joint_" + std::to_string(joint_type) : name;
        f.face_index = side == 0 ? contact.face_a : contact.face_b;

        const std::array<std::vector<Polyline>, 2>& outlines = side == 0 ? male_outlines : female_outlines;
        f.outlines.clear();
        f.outlines.reserve(outlines[0].size() + outlines[1].size());
        for (int face = 0; face < 2; ++face)
            f.outlines.insert(f.outlines.end(), outlines[face].begin(), outlines[face].end());
    }
}

/// Syncs a scratch copy that carries this joint's feature guids, so a const joint reads fresh and keeps its identity.
std::array<ElementFeature, 2> WoodJoint::to_features() const {

    WoodJoint scratch = *this;
    scratch.feature_guids = {feature_guid(0), feature_guid(1)};
    scratch.sync_features();

    return std::move(scratch.element_features);
}

// ═══════════════════════════════════════════════════════════════════════════
// WoodJoint - String
// ═══════════════════════════════════════════════════════════════════════════

std::string WoodJoint::str() const {
    return fmt::format("WoodJoint(type={}, elements=({},{}), faces=({},{}), name={})", joint_type, element_a, element_b, contact.face_a, contact.face_b, name.empty() ? "-" : name);
}

// ═══════════════════════════════════════════════════════════════════════════
// Joint construction
// ═══════════════════════════════════════════════════════════════════════════

std::array<double, 3> joint_volume_extension(const std::vector<double>& extension, int joint_type) {

    const size_t triples = extension.size() / 3;
    if (triples == 0)
        return {0.0, 0.0, 0.0};

    const size_t klass = joint_type == 20 ? 1 : joint_type == 40 ? 2 : joint_type == 30 ? 3 : 0;
    const size_t at = std::min(klass, triples - 1) * 3;
    return {extension[at], extension[at + 1], extension[at + 2]};
}

int index_of(const std::vector<std::shared_ptr<Plate>>& elements, const std::string& guid) {

    for (size_t i = 0; i < elements.size(); ++i)
        if (elements[i]->guid() == guid)
            return static_cast<int>(i);

    return -1;
}

namespace {

/// Slide a volume pair to its midpoint, then apart by unit_scale_distance along the joint line.
void move_pair(WoodJoint& joint, Polyline& a, Polyline& b) {

    if (joint.unit_scale_distance == 0.0) {
        const Vector edge = a.get_point(2) - a.get_point(1);
        const double raw = std::sqrt(edge.magnitude_squared());
        // +1e-6: a near-integer edge length floors to the integer it means.
        joint.unit_scale_distance = std::floor(raw + 1e-6);
    }

    const Vector seg = b.get_point(0) - a.get_point(0);
    const Vector vec = seg * 0.5;
    const double len = std::sqrt(seg.magnitude_squared());
    if (len < 1e-12)
        return;

    const double s = (joint.unit_scale_distance * 0.5) / len;
    const Vector unit = seg * s;
    a.translate(vec);
    b.translate(-vec);
    a.translate(-unit);
    b.translate(unit);
}

}  // namespace

void apply_unit_scale(WoodJoint& joint) {

    std::array<std::optional<Polyline>, 4>& vols = joint.joint_volumes;
    if (TRACE) {
        std::ofstream out("apply_unit_scale.txt", std::ios::app);
        out << "apply_unit_scale: joint v0=" << joint.element_a << " v1=" << joint.element_b
            << " unit_scale=" << joint.unit_scale << " usd=" << joint.unit_scale_distance << "\n";
        for (int i = 0; i < 4; i++) {
            out << "  BEFORE vols[" << i << "] ";
            if (!vols[i].has_value()) {
                out << "nullopt\n";
                continue;
            }

            for (size_t k = 0; k < vols[i]->point_count(); k++) {
                const Point p = vols[i]->get_point(k);
                out << "(" << p[0] << "," << p[1] << "," << p[2] << ") ";
            }
            out << "\n";
        }
    }

    if (!joint.unit_scale)
        return;
    if (!vols[0].has_value() || !vols[1].has_value())
        return;

    move_pair(joint, *vols[0], *vols[1]);
    if (vols[2].has_value() && vols[3].has_value())
        move_pair(joint, *vols[2], *vols[3]);
}

/// Rescale the volumes along the joint line, then map the unit-cube outlines onto them by change of basis.
void joint_orient_to_connection_area(WoodJoint& joint) {

    std::array<std::optional<Polyline>, 4>& vols = joint.joint_volumes;
    if (!vols[0].has_value() || !vols[1].has_value())
        return;

    apply_unit_scale(joint);

    const Xform xf0 = Xform::from_change_of_basis(*vols[0], *vols[1]);
    const Xform xf1 = (vols[2].has_value() && vols[3].has_value())
        ? Xform::from_change_of_basis(*vols[2], *vols[3])
        : xf0;
    for (int face = 0; face < 2; face++) {
        for (Polyline& pl : joint.male_outlines[face])
            pl.transform(xf0);
        for (Polyline& pl : joint.female_outlines[face])
            pl.transform(xf1);
    }
}

namespace {

bool compute_linked_outline(
    std::array<std::vector<Point>, 2>& current,
    const std::array<std::vector<Point>, 2>& linked,
    const std::array<int, 4>& sequence
) {

    if (sequence == std::array<int, 4>{0, 0, 0, 0})
        return true;
    if (sequence[0] < 0 || sequence[2] < 0 || sequence[1] <= 0 || sequence[3] <= 0)
        return false;

    const size_t start = sequence[0];
    const size_t step = sequence[1];
    const size_t offset = sequence[2];
    const size_t stride = sequence[3];
    if (start > current[0].size())
        return false;

    const size_t count = start < current[0].size() - start ? current[0].size() - 2 * start : 0;
    const size_t iterations = count / step + (count % step != 0);
    for (size_t i = 0; i < 2; ++i) {
        if (start > current[i].size() || offset > linked[i].size())
            return false;
        if (iterations > (current[i].size() - start) / step ||
            iterations > (linked[i].size() - offset) / stride)
            return false;
    }

    std::array<std::vector<Point>, 2> result;
    for (size_t i = 0; i < 2; ++i) {
        result[i].insert(result[i].end(), current[i].begin(), current[i].begin() + start);

        size_t position = start;
        size_t index = offset;
        for (size_t j = 0; j < iterations; ++j) {
            result[i].insert(result[i].end(), current[i].begin() + position, current[i].begin() + position + step / 2);
            result[i].insert(result[i].end(), linked[i].begin() + index, linked[i].begin() + index + stride);
            result[i].insert(result[i].end(), current[i].begin() + position + step / 2, current[i].begin() + position + step);
            position += step;
            index += stride;
        }

        result[i].insert(result[i].end(), current[i].end() - start, current[i].end());
    }

    current = std::move(result);
    return true;
}

}  // namespace

void merge_linked_joints(WoodJoint& joint, std::vector<WoodJoint>& all_joints) {

    if (joint.linked_joints_seq.size() != joint.linked_joints.size())
        return;

    for (size_t i = 0; i < joint.linked_joints.size(); ++i) {

        const int index = joint.linked_joints[i];
        if (index < 0 || static_cast<size_t>(index) >= all_joints.size())
            continue;

        WoodJoint& linked = all_joints[index];
        if (&linked == &joint)
            continue;

        const bool male = joint.element_a == linked.element_a;
        const bool side = i == 1 ? !male : male;
        std::array<std::vector<Polyline>, 2>& current = male ? joint.male_outlines : joint.female_outlines;
        std::array<std::vector<Polyline>, 2>& next = side ? linked.male_outlines : linked.female_outlines;
        if (current[0].empty() || current[1].empty() || next[0].empty() || next[1].empty())
            continue;
        if (current[0].size() % 2 != 0 || joint.linked_joints_seq[i].size() != current[0].size() / 2)
            continue;

        std::array<std::vector<Point>, 2> points{current[0][0].get_points(), current[1][0].get_points()};
        const std::array<std::vector<Point>, 2> addition{next[0][0].get_points(), next[1][0].get_points()};
        bool valid = true;
        for (const std::array<int, 4>& sequence : joint.linked_joints_seq[i]) {
            if (!compute_linked_outline(points, addition, sequence)) {
                valid = false;
                break;
            }
        }
        if (!valid)
            continue;

        current[0][0] = Polyline(points[0]);
        current[1][0] = Polyline(points[1]);
        next[0].clear();
        next[1].clear();
    }
}

void joint_get_divisions(WoodJoint& joint, double division_distance) {

    joint.division_length = division_distance;
    const double length = joint.joint_lines[0].squared_length();
    if (!std::isfinite(length) || length <= 1e-10)
        return;

    joint.length = std::sqrt(length);
    joint.divisions = division_distance > 0.0
        ? static_cast<int>(std::clamp(std::ceil(joint.length / division_distance), 1.0, 100.0))
        : 1;
}

} // namespace wood_session
