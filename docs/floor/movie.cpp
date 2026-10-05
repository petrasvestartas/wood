#include "docs/floor/movie.h"

namespace movie {

// ═══════════════════════════════════════════════════════════════════════════
// Frame
// ═══════════════════════════════════════════════════════════════════════════

Frame::Frame(const std::string& frame_chapter, size_t number, const std::string& slug, const std::string& frame_caption, const std::string& frame_view, const Box& frame_box)
    : chapter(frame_chapter), name(fmt::format("{:03d}_{}", number, slug)), caption(fmt::format("{}  {}", number, frame_caption)), view(frame_view), box(frame_box), scene(name) {}

void Frame::label(const std::string& text, const Point& at, bool centred) {
    labels.push_back({{"text", text}, {"at", {at[0], at[1], at[2]}}, {"centred", centred}});
}

void Frame::polyline(Polyline polyline, const Color& color, double width) {
    polyline.linecolor = color;
    polyline.width = width;
    scene.add_polyline(polyline);
}

void Frame::line(Line line, const Color& color, double width, bool dashed, bool headed) {

    line.linecolor = color;
    line.width = width;
    line.arrowhead = headed ? Arrowhead::END : Arrowhead::NONE;
    const double length = line.length();
    const double dash = std::max(box[3] - box[0], box[4] - box[1]) * 0.012;

    // the viewer draws no dash pattern, so a dashed line is a row of short lines, a dash and two thirds of one as gap
    if (!dashed || length <= dash * 2.0) {
        scene.add_line(line);
        return;
    }

    const Point start = line.start();
    const Vector direction = (line.end() - start).normalized();

    for (double at = 0.0; at < length; at += dash * 5.0 / 3.0) {
        Line piece = Line::from_points(start + direction * at, start + direction * std::min(at + dash, length));
        piece.linecolor = color;
        piece.width = width;
        piece.arrowhead = headed && at + dash * 5.0 / 3.0 >= length ? Arrowhead::END : Arrowhead::NONE;
        scene.add_line(piece);
    }
}

void Frame::point(Point point, const Color& color, double width) {
    point.pointcolor = color;
    point.width = width;
    scene.add_point(point);
}

void Frame::plane(Plane plane, const Color& color) {
    plane.linecolor = color;
    plane.width = 2.0;
    scene.add_plane(plane);
}

void Frame::element(const std::shared_ptr<Element>& element, const Color& color) {
    scene.set_node_color(scene.add(element->clone()), color);
}

void Frame::write(const std::filesystem::path& dir) {
    scene.pb_dump((dir / (name + ".pb")).string());
    const nlohmann::json notes = {
        {"chapter", chapter}, {"caption", caption}, {"labels", labels}, {"view", view}, {"bounds", box},
        {"plane_size", plane_size}, {"features", features}, {"key", key}, {"orbit", orbit}, {"distance", distance},
    };
    std::ofstream(dir / (name + ".json")) << notes.dump(2);
    std::cout << fmt::format("{}: {} labels", name, labels.size()) << std::endl;
}

// ═══════════════════════════════════════════════════════════════════════════
// Helpers
// ═══════════════════════════════════════════════════════════════════════════

Point middle(const Outline& outline) {
    return Point::centroid({area_centroid(outline.top), area_centroid(outline.bottom)});
}

std::vector<Outline> outlines(const Quarter& quarter, Family family) {

    if (family == Family::outer_ribs)
        return quarter.outer_ribs();
    if (family == Family::inner_ribs)
        return quarter.inner_ribs();
    if (family == Family::inner_beams)
        return quarter.inner_beams();
    if (family == Family::wedges)
        return quarter.wedges();
    if (family == Family::tsections)
        return quarter.tsections();

    std::vector<Outline> beds;
    for (const std::vector<Outline>& row : quarter.beds())
        beds.insert(beds.end(), row.begin(), row.end());

    return beds;
}

std::vector<Member> members(const QuarterMembers& quarter, Family family) {

    if (family == Family::outer_ribs)
        return quarter.outer_ribs;
    if (family == Family::inner_ribs)
        return quarter.inner_ribs;
    if (family == Family::inner_beams)
        return quarter.inner_beams;
    if (family == Family::wedges)
        return quarter.wedges;
    if (family == Family::tsections)
        return quarter.tsections;

    std::vector<Member> beds;
    for (const std::vector<Member>& row : quarter.beds)
        beds.insert(beds.end(), row.begin(), row.end());

    return beds;
}

std::string member_name(Family family, size_t index, size_t q) {

    MemberRef ref;
    ref.quarter = static_cast<int>(q);
    ref.family = family;
    ref.index = index;

    return ref.name();
}

void plan_context(Frame& frame, const FloorGuide& guide, bool quarters) {

    for (const BayEdge& edge : guide.edges)
        frame.line(up(edge.line), GREY, 2.0);

    if (quarters)
        for (size_t q = 0; q < 4; q++)
            frame.polyline(up(Polyline(guide.geometry[q].polygon).closed()), GREY, 1.0);
}

void quarter_members(Frame& frame, const Floor& floor, size_t q, const Color& color) {

    for (const Family family : FAMILIES)
        for (const Member& member : members(floor.members.quarters[q], family))
            frame.element(member.element, color);

    if (q < floor.members.ring.size())
        frame.element(floor.members.ring[q].element, color);

    if (q < floor.members.columns.size() && floor.members.columns[q].column)
        frame.element(floor.members.columns[q].column, color);
}

}
