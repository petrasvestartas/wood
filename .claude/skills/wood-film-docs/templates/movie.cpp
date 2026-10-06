#include "docs/floor/movie.h"

namespace movie {

// ═══════════════════════════════════════════════════════════════════════════
// Frame
// ═══════════════════════════════════════════════════════════════════════════

/// The colour as the viewer must be given it to show it: the viewer reads colours as linear light and writes sRGB, so the palette's sRGB values are decoded first.
static Color shown(const Color& color) {
    const auto linear = [](float c) { return c <= 0.04045f ? c / 12.92f : std::pow((c + 0.055f) / 1.055f, 2.4f); };
    return Color(linear(color.r), linear(color.g), linear(color.b), color.a, color.name);
}

/// Whether two colours are the same role colour.
static bool same(const Color& a, const Color& b) {
    return a.r == b.r && a.g == b.g && a.b == b.b;
}

/// Whether the colour is the context grey.
static bool context(const Color& color) {
    return same(color, GREY);
}

/// Whether surfaces in the colour are drawn solid, light grey, rather than see-through: context, and what a step reads from earlier ones.
static bool solid_grey(const Color& color) {
    return context(color) || same(color, INPUT);
}

/// The colour mixed with white by the amount, for a face under its edge.
static Color tint(const Color& color, float amount) {
    return Color(color.r + (1.0f - color.r) * amount, color.g + (1.0f - color.g) * amount, color.b + (1.0f - color.b) * amount, color.a, color.name);
}

Frame::Frame(const std::string& frame_chapter, size_t number, const std::string& slug, const std::string& frame_caption, const std::string& frame_view, const Box& frame_box)
    : chapter(frame_chapter), name(fmt::format("{:03d}_{}", number, slug)), caption(frame_caption), view(frame_view), box(frame_box), scene(name), bare(name) {}

void Frame::label(const std::string& text, const Point& at, bool centred) {
    labels.push_back({{"text", text}, {"at", {at[0], at[1], at[2]}}, {"centred", centred}});
}

/// The rank of a line colour: context under input under every role colour.
static int rank(const Color& color) {
    return context(color) || same(color, EDGE_GREY) ? 0 : same(color, INPUT) ? 1 : 2;
}

void Frame::polyline(Polyline polyline, const Color& color, double) {

    const std::vector<Point> points = polyline.get_points();

    // a lower rank goes segment by segment, so the parts a higher line covers can be cut away
    if (rank(color) < 2) {
        for (size_t i = 0; i + 1 < points.size(); i++)
            line(Line::from_points(points[i], points[i + 1]), color);

        return;
    }

    for (size_t i = 0; i + 1 < points.size(); i++)
        covers.push_back({Line::from_points(points[i], points[i + 1]), 2});

    polyline.linecolor = shown(color);
    polyline.width = PEN;
    scene.add_polyline(polyline);
    bare.add_polyline(polyline);
}

/// One finished segment into both scenes, or held back for write when its rank is below the top.
void Frame::stroke(Line line, const Color& color, bool headed) {

    line.linecolor = shown(color);
    line.width = PEN;
    line.arrowhead = headed ? Arrowhead::END : Arrowhead::NONE;

    if (rank(color) < 2) {
        pending.push_back({line, rank(color)});
        return;
    }

    covers.push_back({line, 2});
    scene.add_line(line);
    bare.add_line(line);
}

void Frame::line(Line line, const Color& color, double, bool dashed, bool headed) {

    const double length = line.length();
    // mm per screen pixel, about: the camera fits the box's largest side into the frame's height
    const double px = std::max({box[3] - box[0], box[4] - box[1], box[5] - box[2]}) / 760.0;
    // the viewer draws no dash pattern, so a dashed line is a row of short lines, one pattern for every dashed line since every line has one pen
    const double dash = 5.0 * PEN * px;
    const double gap = (2.0 * PEN + 4.0) * px;
    const size_t count = static_cast<size_t>(std::floor((length + gap) / (dash + gap)));

    if (!dashed || count < 2) {
        stroke(line, color, headed);
        return;
    }

    // the gaps stretched so the line starts and ends on a full dash
    const double step = dash + (length - static_cast<double>(count) * dash) / static_cast<double>(count - 1);
    const Point start = line.start();
    const Vector direction = (line.end() - start).normalized();

    for (size_t i = 0; i < count; i++)
        stroke(Line::from_points(start + direction * (static_cast<double>(i) * step), start + direction * (static_cast<double>(i) * step + dash)), color, headed && i + 1 == count);
}

void Frame::point(Point point, const Color& color, double width) {
    point.pointcolor = shown(color);
    point.width = width;
    scene.add_point(point);
    bare.add_point(point);
}

/// The direction a plan or elevation looks along, zero for a 3D view.
static Vector view_direction(const std::string& view) {

    if (view == "top")
        return Vector(0.0, 0.0, 1.0);
    if (view == "front")
        return Vector(0.0, 1.0, 0.0);
    if (view == "right")
        return Vector(1.0, 0.0, 0.0);

    return Vector(0.0, 0.0, 0.0);
}

void Frame::plane(Plane plane, const Color& color) {

    // a plane seen edge-on is a line over others: only its normal shows which way it faces
    if (std::abs(plane.z_axis().dot(view_direction(view))) < 0.02 && view_direction(view).magnitude() > 0.0) {
        line(Line::from_points(plane.origin(), plane.origin() + plane.z_axis() * plane_size), color, 2.5, false, true);
        return;
    }

    // the viewer's plane: its square, x and y axes and headed normal, unfilled so the view's outlines leave it alone
    plane.linecolor = shown(color);
    plane.width = PEN;
    scene.add_plane(plane);
    bare.add_plane(plane);
}

void Frame::face(const Polyline& loop, const Color& color, double width) {

    mesh(Mesh::from_polylines({loop}), context(color) ? color : tint(color, 0.62f));
    polyline(loop, context(color) ? EDGE_GREY : color, width);
}

void Frame::fill(const Polyline& loop, const Color& color) {
    mesh(Mesh::from_polylines({loop}), context(color) ? color : tint(color, 0.62f));
}

void Frame::dimension(const Point& a, const Point& b, const Vector& offset, const std::string& text) {

    const double px = std::max({box[3] - box[0], box[4] - box[1], box[5] - box[2]}) / 760.0;
    const Vector along = (b - a).normalized();
    Vector across = offset.magnitude() > 0.0 ? offset.normalized() : along.cross(view_direction(view).magnitude() > 0.0 ? view_direction(view) : Vector(0.0, 0.0, 1.0)).normalized();
    const Vector tick = (along + across).normalized() * (6.0 * px);
    const Point a1 = a + offset;
    const Point b1 = b + offset;

    if (offset.magnitude() > 0.0) {
        line(Line::from_points(a + across * (4.0 * px), a1 + across * (6.0 * px)), INK, 1.5);
        line(Line::from_points(b + across * (4.0 * px), b1 + across * (6.0 * px)), INK, 1.5);
    }

    line(Line::from_points(a1, b1), INK, 1.5);
    line(Line::from_points(a1 - tick, a1 + tick), INK, 1.5);
    line(Line::from_points(b1 - tick, b1 + tick), INK, 1.5);
    label(text, Line::from_points(a1, b1).center());
}

void Frame::solid(const Outline& outline, const Color& color) {
    element(to_plate({up(outline.top), up(outline.bottom)}, "outline"), color);
}

void Frame::element(const std::shared_ptr<Element>& element, const Color& color) {

    // a mesh in the colour with black edges; the element itself only where its drill and cutter features must show
    if (!features) {
        mesh(element->model_geometry_mesh(), color);
        return;
    }

    scene.set_node_color(scene.add(element->clone()), shown(color));
    translucent = true;
}

void Frame::mesh(Mesh mesh, const Color& color) {

    mesh.set_objectcolor(shown(solid_grey(color) ? GREY : color));
    mesh.set_linecolors(std::vector<Color>(mesh.number_of_edges(), shown(INK)), std::vector<double>(mesh.number_of_edges(), PEN));
    scene.add_mesh(mesh);
    bare.add_mesh(mesh);
}

void Frame::brep(const BRep& brep, const Color& color) {
    scene.set_node_color(scene.add_brep(brep), shown(color));
    translucent = true;
}

/// The point as the view sees it: a plan or elevation drops the depth along its view, a 3D view keeps it.
static Point seen(const Point& point, const Vector& view) {
    return point - view * (point - Point(0.0, 0.0, 0.0)).dot(view);
}

/// The parts of a held-back segment no higher-ranked segment covers on screen, within tolerance mm of it.
static std::vector<std::array<double, 2>> uncovered(const Line& line, int line_rank, const std::vector<std::pair<Line, int>>& covers, const Vector& view, double tolerance) {

    const Point a = seen(line.start(), view);
    const Point b = seen(line.end(), view);
    const double length = (b - a).magnitude();

    if (length < 1e-9)
        return {{0.0, 1.0}};

    const Vector d = (b - a) * (1.0 / length);
    std::vector<std::array<double, 2>> covered;

    for (const auto& [cover, cover_rank] : covers) {
        if (cover_rank <= line_rank)
            continue;

        const Point p = seen(cover.start(), view);
        const Point q = seen(cover.end(), view);
        const auto off = [&](const Point& x) { const Vector v = x - a; return (v - d * v.dot(d)).magnitude(); };

        if (off(p) > tolerance || off(q) > tolerance)
            continue;

        const double tp = (p - a).dot(d);
        const double tq = (q - a).dot(d);
        covered.push_back({std::max(std::min(tp, tq) - tolerance, 0.0) / length, std::min(std::max(tp, tq) + tolerance, length) / length});
    }

    std::sort(covered.begin(), covered.end());
    std::vector<std::array<double, 2>> left;
    double at = 0.0;

    for (const std::array<double, 2>& c : covered) {
        if (c[0] > at)
            left.push_back({at, c[0]});
        at = std::max(at, c[1]);
    }

    if (at < 1.0)
        left.push_back({at, 1.0});

    return left;
}

void Frame::write(const std::filesystem::path& dir) {

    const double px = std::max({box[3] - box[0], box[4] - box[1], box[5] - box[2]}) / 760.0;
    // input lines cover context lines too, so they are cut and then count as covers
    std::stable_sort(pending.begin(), pending.end(), [](const auto& x, const auto& y) { return x.second > y.second; });

    for (const auto& [held, held_rank] : pending) {
        const Vector along = held.end() - held.start();

        for (const std::array<double, 2>& part : uncovered(held, held_rank, covers, view_direction(view), 2.5 * px)) {
            if ((part[1] - part[0]) * along.magnitude() < 2.0 * px)
                continue;

            Line piece = Line::from_points(held.start() + along * part[0], held.start() + along * part[1]);
            piece.linecolor = held.linecolor;
            piece.width = held.width;
            piece.arrowhead = part[1] >= 1.0 ? held.arrowhead : Arrowhead::NONE;
            scene.add_line(piece);
            bare.add_line(piece);
            covers.push_back({piece, held_rank});
        }
    }

    scene.pb_dump((dir / (name + ".pb")).string());

    if (translucent)
        bare.pb_dump((dir / (name + ".bare")).string());

    const nlohmann::json notes = {
        {"chapter", chapter}, {"caption", caption}, {"labels", labels}, {"view", view}, {"bounds", box},
        {"plane_size", plane_size}, {"features", features}, {"key", key}, {"translucent", translucent}, {"orbit", orbit}, {"distance", distance},
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
