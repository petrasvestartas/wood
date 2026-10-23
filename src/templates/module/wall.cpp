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

    // openings: per opening its king studs, header and under a window its sill
    add_openings();

    // studs: the end studs, and between every two posts as few studs as keep the spacing, evenly apart, cripples over and under the openings
    add_studs();

    // sheathing: one panel on each face of the whole wall, every door a notch and every window a hole
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
        const double header_z = opening.head + half;
        const std::shared_ptr<Beam> header = member(
            Point(left, 0.0, header_z),
            Point(right, 0.0, header_z),
            guide.stud_width,
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
    }
}

void Wall::add_studs() {

    const std::shared_ptr<TreeNode> studs = group_named("studs");
    const double half = 0.5 * guide.stud_width;
    const Vector across(1.0, 0.0, 0.0);
    const double foot = guide.stud_width;
    const double top = guide.height - guide.stud_width;

    // posts: the two end studs and every king stud, by their centres along the wall
    std::vector<double> posts = {half, guide.length - half};

    for (const Opening& opening : guide.openings) {
        posts.push_back(opening.x - half);
        posts.push_back(opening.x + opening.width + half);
    }

    std::sort(posts.begin(), posts.end());
    size_t count = 0;

    for (const double x : {posts.front(), posts.back()}) {
        const std::shared_ptr<Beam> stud = member(
            Point(x, 0.0, foot),
            Point(x, 0.0, top),
            guide.stud_width,
            across,
            fmt::format("stud_{}", count)
        );
        add(stud, studs);
        count++;
    }

    // between every two posts as few studs as keep the stud spacing, evenly apart; inside an opening cripples over its header and under its sill
    for (size_t p = 0; p + 1 < posts.size(); p++) {
        const double span = posts[p + 1] - posts[p];
        const int between = static_cast<int>(std::ceil(span / guide.stud_spacing - 1e-9)) - 1;

        for (int k = 1; k <= between; k++) {
            const double x = posts[p] + span * k / (between + 1);
            std::vector<std::array<double, 2>> runs = {{foot, top}};
            std::shared_ptr<TreeNode> group = studs;
            std::string name = fmt::format("stud_{}", count);

            for (size_t o = 0; o < guide.openings.size(); o++) {
                const Opening& opening = guide.openings[o];

                if (x < opening.x || x > opening.x + opening.width)
                    continue;

                runs = {{opening.head + guide.stud_width, top}};

                if (opening.sill > 0.0)
                    runs.push_back({foot, opening.sill - guide.stud_width});

                group = group_named(fmt::format("opening_{}", o));
                name = fmt::format("cripple_{}_{}", o, count);
            }

            for (size_t r = 0; r < runs.size(); r++) {
                const std::shared_ptr<Beam> stud = member(
                    Point(x, 0.0, runs[r][0]),
                    Point(x, 0.0, runs[r][1]),
                    guide.stud_width,
                    across,
                    runs.size() == 1 ? name : fmt::format("{}_{}", name, r)
                );
                add(stud, group);
            }

            count++;
        }
    }
}

void Wall::add_sheathing() {

    const double face = 0.5 * guide.stud_depth;
    const Vector along(1.0, 0.0, 0.0);
    const Vector up(0.0, 0.0, 1.0);
    std::vector<std::pair<double, size_t>> doors;

    // doors: left to right, as the outline walks along the foot
    for (size_t o = 0; o < guide.openings.size(); o++)
        if (guide.openings[o].sill <= 0.0)
            doors.push_back({guide.openings[o].x, o});

    std::sort(doors.begin(), doors.end());

    for (size_t side = 0; side < 2; side++) {
        const std::shared_ptr<TreeNode> group = group_named(fmt::format("sheathing_{}", side));
        const double sign = side == 0 ? -1.0 : 1.0;
        const std::array<double, 2> levels = {sign * face, sign * (face + guide.sheet_thickness)};
        std::array<Polyline, 2> outlines;

        // outline: the whole wall, every door a notch up from its foot
        for (size_t k = 0; k < 2; k++) {
            std::vector<Point> corners = {Point(0.0, levels[k], 0.0)};

            for (const std::pair<double, size_t>& entry : doors) {
                const Opening& door = guide.openings[entry.second];
                corners.push_back(Point(door.x, levels[k], 0.0));
                corners.push_back(Point(door.x, levels[k], door.head));
                corners.push_back(Point(door.x + door.width, levels[k], door.head));
                corners.push_back(Point(door.x + door.width, levels[k], 0.0));
            }

            corners.push_back(Point(guide.length, levels[k], 0.0));
            corners.push_back(Point(guide.length, levels[k], guide.height));
            corners.push_back(Point(0.0, levels[k], guide.height));
            corners.push_back(corners[0]);
            outlines[k] = Polyline(corners);
        }

        const std::shared_ptr<Plate> panel = std::make_shared<Plate>(outlines[0], outlines[1], fmt::format("sheathing_{}", side));

        // windows: a hole through both faces of the panel
        Features features;
        features.bottom.push_back(panel->polylines[0]);
        features.top.push_back(panel->polylines[1]);

        for (const Opening& opening : guide.openings) {
            if (opening.sill <= 0.0)
                continue;

            const double height = opening.head - opening.sill;
            features.bottom.push_back(Polyline::rectangle(Point(opening.x, levels[0], opening.sill), along, up, opening.width, height));
            features.top.push_back(Polyline::rectangle(Point(opening.x, levels[1], opening.sill), along, up, opening.width, height));
        }

        panel->features = features;
        panel->invalidate_geometry();
        add(panel, group);
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

}
