#include "pch.h"
#include "src/templates/floor/floor_geometry.h"

using namespace session_cpp;

namespace wood_floor {

// ═══════════════════════════════════════════════════════════════════════════
// Verification
// ═══════════════════════════════════════════════════════════════════════════

/// The top edge of a polygon: its highest edge, the longest among those at the top.
static Line top_edge(const std::vector<Point>& points) {

    Line best;
    double best_height = -1e300;
    double best_length = -1.0;

    for (size_t i = 0; i < points.size(); i++) {
        const Point& a = points[i];
        const Point& b = points[(i + 1) % points.size()];
        const double height = 0.5 * (a[2] + b[2]);
        const double length = (b - a).magnitude();

        if (height > best_height + 1e-6 || (std::abs(height - best_height) <= 1e-6 && length > best_length)) {
            best = Line::from_points(a, b);
            best_height = height;
            best_length = length;
        }
    }

    return best;
}

/// The name of a contact type.
static std::string type_name(wood_session::ContactType type) {

    if (type == wood_session::ContactType::side_side)
        return "side_side";

    if (type == wood_session::ContactType::end_end)
        return "end_end";

    return fmt::format("type {}", static_cast<int>(type));
}

std::shared_ptr<wood_session::InteractionContactFace> require_contact(wood_session::WoodSession& session, const std::shared_ptr<Element>& a, const std::shared_ptr<Element>& b, wood_session::ContactType expected, const std::string& relation) {

    const std::shared_ptr<wood_session::InteractionContactFace> contact = session.compute_face_contact(uncut(*a), uncut(*b));

    if (!contact)
        throw std::runtime_error(fmt::format("no contact for {} between {} and {}", relation, a->name, b->name));

    if (expected != wood_session::ContactType::unknown && contact->type != expected)
        throw std::runtime_error(fmt::format("the contact for {} between {} and {} is {}, not {}", relation, a->name, b->name, type_name(contact->type), type_name(expected)));

    return contact;
}

/// How the searched contact disagrees with the constructed one: the plane normal, the top edge midpoint and length, and the area, beyond the tolerance; empty when it agrees.
static std::string disagreement(const Relationship& row, const wood_session::InteractionContactFace& found, double tolerance) {

    const std::vector<Point> mine = geometry::open_points(row.contact);
    const std::vector<Point> theirs = geometry::open_points(found.polygon);
    const Vector normal = wood_session::compute_newell(theirs).normalized();
    const double angle = std::acos(std::clamp(std::abs(normal.dot(row.plane.z_axis())), 0.0, 1.0));
    const Line edge_mine = top_edge(mine);
    const Line edge_theirs = top_edge(theirs);
    const double midpoint = (edge_mine.center() - edge_theirs.center()).magnitude();
    const double length = std::abs(edge_mine.length() - edge_theirs.length());
    const double area_mine = row.area();
    const double area_theirs = geometry::polygon_area(found.polygon);
    const double area = std::abs(area_theirs - area_mine) / std::max(area_mine, 1e-300);

    if (angle > tolerance)
        return fmt::format("plane {:.3e} rad off", angle);

    if (midpoint > tolerance || length > tolerance)
        return fmt::format("top edge {:.3e} mm off, {:.3e} mm longer", midpoint, edge_theirs.length() - edge_mine.length());

    if (area > tolerance)
        return fmt::format("area {:.6f} against {:.6f}", area_theirs, area_mine);

    return "";
}

ContactCheck verify_contacts(wood_session::WoodSession& session, const FloorGuide& guide, const FloorMembers& members, double tolerance, const std::vector<Relation>& kinds) {

    ContactCheck check;
    std::vector<ContactMismatch>& mismatches = check.mismatches;

    for (const Relationship& row : relationships(guide)) {
        if (row.contact.point_count() == 0 || std::find(kinds.begin(), kinds.end(), row.kind) == kinds.end())
            continue;

        check.count++;

        const std::array<std::shared_ptr<Element>, 2> pair = members.pair(row);
        const std::shared_ptr<wood_session::InteractionContactFace> found = session.compute_face_contact(uncut(*pair[0]), uncut(*pair[1]));

        if (!found) {
            mismatches.push_back({row.text(), "missing"});
            continue;
        }

        if (row.type != wood_session::ContactType::unknown && found->type != row.type) {
            mismatches.push_back({row.text(), type_name(found->type) + " instead of " + type_name(row.type)});
            continue;
        }

        const std::string what = disagreement(row, *found, tolerance);

        if (!what.empty())
            mismatches.push_back({row.text(), what});
    }

    return check;
}

bool ContactCheck::ok() const {
    return mismatches.empty();
}

std::string ContactCheck::str() const {

    std::string text = fmt::format("{} of {} contacts verified by the kernel's search", count - mismatches.size(), count);

    for (const ContactMismatch& mismatch : mismatches)
        text += fmt::format("\n  mismatch: {}: {}", mismatch.relation, mismatch.what);

    return text;
}

}
