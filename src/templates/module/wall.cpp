#include "src/templates/module/wall.h"
#include "wood_profile.h"

namespace wood_module {

// ═══════════════════════════════════════════════════════════════════════════
// Constructor
// ═══════════════════════════════════════════════════════════════════════════

Wall::Wall(const WallGuide& guide, const std::string& name)
    : WoodSession(name),
      guide(guide) {

    // rails: the bottom rail, split at every door, and the top rail along the whole wall
    add_rails();

    // studs: a stud at every stud spacing that no opening takes
    add_studs();

    // openings: per opening its king studs, header, sill and the cripples above and below it
    add_openings();

    // sheathing: panels on both faces, split at the sheet width and at every opening edge
    add_sheathing();

    // contacts: every member end on the member it stands on, every panel on every member under it, each drawn
    add_contacts();
}

// ═══════════════════════════════════════════════════════════════════════════
// Members
// ═══════════════════════════════════════════════════════════════════════════

void Wall::add_rails() {

    const std::shared_ptr<TreeNode> rails = group_named("rails");
    const double half = 0.5 * guide.stud_width;
    const Vector up(0.0, 0.0, 1.0);
    std::vector<std::array<double, 2>> runs = {{0.0, guide.length}};

    // doors: the bottom rail stops at both sides of the clear opening
    for (const Opening& opening : guide.openings) {
        if (opening.sill > 0.0)
            continue;

        std::vector<std::array<double, 2>> split;

        for (const std::array<double, 2>& run : runs) {
            if (opening.x <= run[0] || opening.x >= run[1]) {
                split.push_back(run);
                continue;
            }

            split.push_back({run[0], opening.x});
            split.push_back({opening.x + opening.width, run[1]});
        }

        runs = split;
    }

    for (size_t i = 0; i < runs.size(); i++) {
        const std::shared_ptr<Beam> rail = member(
            Point(runs[i][0], 0.0, half),
            Point(runs[i][1], 0.0, half),
            guide.stud_width,
            up,
            fmt::format("bottom_rail_{}", i)
        );
        add(rail, rails);
    }

    const double top = guide.height - half;
    const std::shared_ptr<Beam> rail = member(
        Point(0.0, 0.0, top),
        Point(guide.length, 0.0, top),
        guide.stud_width,
        up,
        "top_rail"
    );
    add(rail, rails);
}

void Wall::add_studs() {

    const std::shared_ptr<TreeNode> studs = group_named("studs");
    const double half = 0.5 * guide.stud_width;
    const Vector across(1.0, 0.0, 0.0);
    size_t count = 0;

    for (const double x : compute_stud_positions()) {
        bool taken = false;

        // an opening takes the stud that stands in its clear width or on its king studs
        for (const Opening& opening : guide.openings)
            if (x + half > opening.x - guide.stud_width && x - half < opening.x + opening.width + guide.stud_width)
                taken = true;

        if (taken)
            continue;

        const std::shared_ptr<Beam> stud = member(
            Point(x, 0.0, guide.stud_width),
            Point(x, 0.0, guide.height - guide.stud_width),
            guide.stud_width,
            across,
            fmt::format("stud_{}", count)
        );
        add(stud, studs);
        count++;
    }
}

void Wall::add_openings() {

    const double half = 0.5 * guide.stud_width;
    const Vector up(0.0, 0.0, 1.0);
    const Vector across(1.0, 0.0, 0.0);
    const double foot = guide.stud_width;
    const double top = guide.height - guide.stud_width;

    for (size_t o = 0; o < guide.openings.size(); o++) {
        const Opening& opening = guide.openings[o];
        const std::shared_ptr<TreeNode> group = group_named(fmt::format("opening_{}", o));
        const double left = opening.x;
        const double right = opening.x + opening.width;

        // king studs: full height either side of the clear opening
        const std::array<double, 2> kings = {left - half, right + half};

        for (size_t side = 0; side < 2; side++) {
            const std::shared_ptr<Beam> king = member(
                Point(kings[side], 0.0, foot),
                Point(kings[side], 0.0, top),
                guide.stud_width,
                across,
                fmt::format("king_stud_{}_{}", o, side)
            );
            add(king, group);
        }

        // header: across the opening between the king studs, its underside the opening's head
        const double header_z = opening.head + 0.5 * guide.header_depth;
        const std::shared_ptr<Beam> header = member(
            Point(left, 0.0, header_z),
            Point(right, 0.0, header_z),
            guide.header_depth,
            up,
            fmt::format("header_{}", o)
        );
        add(header, group);

        // sill: across a window between the king studs, its top the opening's sill
        if (opening.sill > 0.0) {
            const double sill_z = opening.sill - half;
            const std::shared_ptr<Beam> sill = member(
                Point(left, 0.0, sill_z),
                Point(right, 0.0, sill_z),
                guide.stud_width,
                up,
                fmt::format("sill_{}", o)
            );
            add(sill, group);
        }

        // cripples: the studs at the stud spacing inside the opening, above the header and below the sill
        size_t count = 0;

        for (const double x : compute_stud_positions()) {
            if (x - half < left || x + half > right)
                continue;

            const std::shared_ptr<Beam> above = member(
                Point(x, 0.0, opening.head + guide.header_depth),
                Point(x, 0.0, top),
                guide.stud_width,
                across,
                fmt::format("cripple_top_{}_{}", o, count)
            );
            add(above, group);

            if (opening.sill > 0.0) {
                const std::shared_ptr<Beam> below = member(
                    Point(x, 0.0, foot),
                    Point(x, 0.0, opening.sill - guide.stud_width),
                    guide.stud_width,
                    across,
                    fmt::format("cripple_bottom_{}_{}", o, count)
                );
                add(below, group);
            }

            count++;
        }
    }
}

void Wall::add_sheathing() {

    const std::vector<std::array<double, 2>> strips = compute_strips();
    const double face = 0.5 * guide.stud_depth;
    const Vector along(1.0, 0.0, 0.0);
    const Vector up(0.0, 0.0, 1.0);

    for (size_t side = 0; side < 2; side++) {
        const std::shared_ptr<TreeNode> group = group_named(fmt::format("sheathing_{}", side));
        const double sign = side == 0 ? -1.0 : 1.0;
        const double inner = sign * face;
        const double outer = sign * (face + guide.sheet_thickness);
        size_t count = 0;

        for (const std::array<double, 2>& strip : strips) {
            const double middle = 0.5 * (strip[0] + strip[1]);
            std::vector<std::array<double, 2>> spans = {{0.0, guide.height}};

            // an opening over the strip leaves the panel below its sill and the one above its head
            for (const Opening& opening : guide.openings)
                if (middle > opening.x && middle < opening.x + opening.width) {
                    spans.clear();

                    if (opening.sill > 0.0)
                        spans.push_back({0.0, opening.sill});

                    spans.push_back({opening.head, guide.height});
                }

            for (const std::array<double, 2>& span : spans) {
                const double width = strip[1] - strip[0];
                const double height = span[1] - span[0];
                const Polyline bottom = Polyline::rectangle(Point(strip[0], inner, span[0]), along, up, width, height);
                const Polyline top = Polyline::rectangle(Point(strip[0], outer, span[0]), along, up, width, height);
                add(std::make_shared<Plate>(bottom, top, fmt::format("sheathing_{}_{}", side, count)), group);
                count++;
            }
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Contacts
// ═══════════════════════════════════════════════════════════════════════════

void Wall::add_contacts() {

    const std::shared_ptr<TreeNode> frame_group = group_named("contacts_frame");
    const std::shared_ptr<TreeNode> sheathing_group = group_named("contacts_sheathing");
    std::vector<std::shared_ptr<Beam>> members;
    std::vector<std::shared_ptr<Plate>> panels;

    for (const std::shared_ptr<Element>& element : *objects.elements) {
        if (const std::shared_ptr<Beam> beam = std::dynamic_pointer_cast<Beam>(element))
            members.push_back(beam);

        if (const std::shared_ptr<Plate> plate = std::dynamic_pointer_cast<Plate>(element))
            panels.push_back(plate);
    }

    // frame: the member that ends on the contact is its target, the one it stands on the source
    for (size_t i = 0; i < members.size(); i++)
        for (size_t j = i + 1; j < members.size(); j++) {
            const std::shared_ptr<InteractionContactFace> probe = compute_face_contact(members[i], members[j]);

            if (!probe)
                continue;

            Point origin;
            Plane plane;
            probe->polygon.get_fast_plane(origin, plane);
            const Vector normal = plane.z_axis();
            const Vector along_i = (members[i]->axis[1] - members[i]->axis[0]).normalized();
            const Vector along_j = (members[j]->axis[1] - members[j]->axis[0]).normalized();
            const double end_i = std::abs(along_i.dot(normal));
            const double end_j = std::abs(along_j.dot(normal));

            if (std::max(end_i, end_j) < 0.9)
                continue;

            const std::shared_ptr<Beam> source = end_i > end_j ? members[j] : members[i];
            const std::shared_ptr<Beam> target = end_i > end_j ? members[i] : members[j];
            const std::shared_ptr<InteractionContactFace> face = compute_face_contact(source, target);
            face->name = fmt::format("{}_on_{}", target->name, source->name);
            add_interaction(source, target, face);
            Mesh patch = Mesh::from_polylines(std::vector<Polyline>{face->polygon});
            patch.set_objectcolor(Color::red());
            add_mesh(patch, frame_group);
        }

    // sheathing: every panel on every member it covers
    for (const std::shared_ptr<Plate>& panel : panels)
        for (const std::shared_ptr<Beam>& frame : members) {
            const std::shared_ptr<InteractionContactFace> face = compute_face_contact(panel, frame);

            if (!face)
                continue;

            face->name = fmt::format("{}_on_{}", panel->name, frame->name);
            add_interaction(panel, frame, face);
            Mesh patch = Mesh::from_polylines(std::vector<Polyline>{face->polygon});
            patch.set_objectcolor(Color::red());
            add_mesh(patch, sheathing_group);
        }
}

// ═══════════════════════════════════════════════════════════════════════════
// Steps
// ═══════════════════════════════════════════════════════════════════════════

std::shared_ptr<Beam> Wall::member(
    const Point& start,
    const Point& end,
    double size,
    const Vector& up,
    const std::string& name
) const {

    const Polyline axis({start, end});
    const std::vector<Polyline> profile = profile_rectangle(guide.stud_depth, size);
    return std::make_shared<Beam>(axis, profile, std::vector<Vector>{up}, name);
}

std::vector<double> Wall::compute_stud_positions() const {

    const double half = 0.5 * guide.stud_width;
    std::vector<double> positions = {half};

    for (double x = guide.stud_spacing; x < guide.length - guide.stud_width; x += guide.stud_spacing)
        positions.push_back(x);

    positions.push_back(guide.length - half);
    return positions;
}

std::vector<std::array<double, 2>> Wall::compute_strips() const {

    std::vector<double> edges;

    for (const Opening& opening : guide.openings) {
        edges.push_back(opening.x);
        edges.push_back(opening.x + opening.width);
    }

    std::vector<double> breaks = {0.0, guide.length};

    // sheet breaks: every sheet width, unless an opening edge stands within two studs of it
    for (double x = guide.sheet_width; x < guide.length; x += guide.sheet_width) {
        bool near = false;

        for (const double edge : edges)
            if (std::abs(edge - x) < 2.0 * guide.stud_width)
                near = true;

        if (!near)
            breaks.push_back(x);
    }

    breaks.insert(breaks.end(), edges.begin(), edges.end());
    std::sort(breaks.begin(), breaks.end());
    std::vector<std::array<double, 2>> strips;

    for (size_t i = 0; i + 1 < breaks.size(); i++)
        strips.push_back({breaks[i], breaks[i + 1]});

    return strips;
}

}
