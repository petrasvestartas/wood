#include "src/templates/module/framed_panel.h"
#include "wood_profile.h"

namespace wood_module {

// ═══════════════════════════════════════════════════════════════════════════
// Constructor
// ═══════════════════════════════════════════════════════════════════════════

FramedPanel::FramedPanel(
    WoodSession& session,
    const WallGuide& guide,
    const Plane& base,
    const std::string& name,
    const std::shared_ptr<TreeNode>& parent
)
    : guide(guide),
      base(base),
      name(name),
      _session(session),
      _group(name.empty() ? parent : session.group_named(name, parent)) {

    // rails: the bottom rail, split at every door, and the top rail along the whole panel
    add_rails();

    // openings: per opening a jamb stud either side, the header between them and under a window its sill
    add_openings();

    // studs: the edge studs, and the studs on the grid of the stud spacing where no edge or jamb stud stands, cripples over and under the openings
    add_studs();

    // sheathing: one sheet on each face of the whole panel, every door a notch and every window a hole
    add_sheathing();
}

// ═══════════════════════════════════════════════════════════════════════════
// Members
// ═══════════════════════════════════════════════════════════════════════════

void FramedPanel::add_rails() {

    const std::shared_ptr<TreeNode> rails = group("rails");
    const double half = 0.5 * guide.stud_width;
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
            local(runs[i][0], 0.0, half),
            local(runs[i][1], 0.0, half),
            guide.stud_width,
            base.y_axis(),
            fmt::format("bottom_rail_{}", i)
        );
        _session.add(rail, rails);
    }

    const double top = guide.height - half;
    const std::shared_ptr<Beam> rail = member(
        local(0.0, 0.0, top),
        local(guide.length, 0.0, top),
        guide.stud_width,
        base.y_axis(),
        "top_rail"
    );
    _session.add(rail, rails);
}

void FramedPanel::add_openings() {

    const double w = guide.stud_width;
    const double half = 0.5 * w;
    const double foot = w;
    const double top = guide.height - w;

    for (size_t o = 0; o < guide.openings.size(); o++) {
        const Opening& opening = guide.openings[o];
        const std::shared_ptr<TreeNode> frame = group(fmt::format("opening_{}", o));
        const double left = opening.x;
        const double right = opening.x + opening.width;

        // jamb studs: full height either side of the clear opening
        const std::array<double, 2> jambs = {left - half, right + half};

        for (size_t side = 0; side < 2; side++) {
            const std::shared_ptr<Beam> jamb = member(
                local(jambs[side], 0.0, foot),
                local(jambs[side], 0.0, top),
                w,
                base.x_axis(),
                fmt::format("jamb_stud_{}_{}", o, side)
            );
            _session.add(jamb, frame);
        }

        // header: between the jamb studs, its underside the opening's head
        const double header_z = opening.head + half;
        const std::shared_ptr<Beam> header = member(
            local(left, 0.0, header_z),
            local(right, 0.0, header_z),
            w,
            base.y_axis(),
            fmt::format("header_{}", o)
        );
        _session.add(header, frame);

        // sill: across a window between the jamb studs, its top the opening's sill
        if (opening.sill > 0.0) {
            const double sill_z = opening.sill - half;
            const std::shared_ptr<Beam> sill = member(
                local(left, 0.0, sill_z),
                local(right, 0.0, sill_z),
                w,
                base.y_axis(),
                fmt::format("sill_{}", o)
            );
            _session.add(sill, frame);
        }
    }
}

void FramedPanel::add_studs() {

    const std::shared_ptr<TreeNode> studs = group("studs");
    const double w = guide.stud_width;
    const double half = 0.5 * w;
    const double foot = w;
    const double top = guide.height - w;
    size_t count = 0;

    // edge studs: at both ends of the panel
    for (const double x : {half, guide.length - half}) {
        const std::shared_ptr<Beam> stud = member(
            local(x, 0.0, foot),
            local(x, 0.0, top),
            w,
            base.x_axis(),
            fmt::format("stud_{}", count)
        );
        _session.add(stud, studs);
        count++;
    }

    // studs: on the grid of the stud spacing from the panel's start, where no edge or jamb stud stands, so a stud doubles a jamb only
    // where the opening is off the grid; inside an opening cripples over its header and under its sill
    for (double x = guide.stud_spacing; x + half < guide.length - w; x += guide.stud_spacing) {
        std::vector<std::array<double, 2>> runs = {{foot, top}};
        std::shared_ptr<TreeNode> place = studs;
        std::string part = fmt::format("stud_{}", count);

        for (size_t o = 0; o < guide.openings.size(); o++) {
            const Opening& opening = guide.openings[o];
            const bool inside = x - half >= opening.x && x + half <= opening.x + opening.width;
            const bool framed = x + half > opening.x - w && x - half < opening.x + opening.width + w;

            if (!framed)
                continue;

            runs.clear();

            if (!inside)
                break;

            runs.push_back({opening.head + w, top});

            if (opening.sill > 0.0)
                runs.push_back({foot, opening.sill - w});

            place = group(fmt::format("opening_{}", o));
            part = fmt::format("cripple_{}_{}", o, count);
            break;
        }

        for (size_t r = 0; r < runs.size(); r++) {
            const std::shared_ptr<Beam> stud = member(
                local(x, 0.0, runs[r][0]),
                local(x, 0.0, runs[r][1]),
                w,
                base.x_axis(),
                runs.size() == 1 ? part : fmt::format("{}_{}", part, r)
            );
            _session.add(stud, place);
        }

        if (!runs.empty())
            count++;
    }
}

void FramedPanel::add_sheathing() {

    const double face = 0.5 * guide.stud_depth;
    std::vector<std::pair<double, size_t>> doors;

    // doors: left to right, as the outline walks along the foot
    for (size_t o = 0; o < guide.openings.size(); o++)
        if (guide.openings[o].sill <= 0.0)
            doors.push_back({guide.openings[o].x, o});

    std::sort(doors.begin(), doors.end());

    for (size_t side = 0; side < 2; side++) {
        const std::shared_ptr<TreeNode> sheets = group(fmt::format("sheathing_{}", side));
        const double sign = side == 0 ? -1.0 : 1.0;
        const std::array<double, 2> levels = {sign * face, sign * (face + guide.sheet_thickness)};
        std::array<Polyline, 2> outlines;

        // outline: the whole panel, every door a notch up from its foot
        for (size_t k = 0; k < 2; k++) {
            std::vector<Point> corners = {local(0.0, levels[k], 0.0)};

            for (const std::pair<double, size_t>& entry : doors) {
                const Opening& door = guide.openings[entry.second];
                corners.push_back(local(door.x, levels[k], 0.0));
                corners.push_back(local(door.x, levels[k], door.head));
                corners.push_back(local(door.x + door.width, levels[k], door.head));
                corners.push_back(local(door.x + door.width, levels[k], 0.0));
            }

            corners.push_back(local(guide.length, levels[k], 0.0));
            corners.push_back(local(guide.length, levels[k], guide.height));
            corners.push_back(local(0.0, levels[k], guide.height));
            corners.push_back(corners[0]);
            outlines[k] = Polyline(corners);
        }

        const std::shared_ptr<Plate> sheet = std::make_shared<Plate>(outlines[0], outlines[1], named(fmt::format("sheathing_{}", side)));

        // windows: a hole through both faces of the sheet
        Features features;
        features.bottom.push_back(sheet->polylines[0]);
        features.top.push_back(sheet->polylines[1]);

        for (const Opening& opening : guide.openings) {
            if (opening.sill <= 0.0)
                continue;

            const double height = opening.head - opening.sill;
            features.bottom.push_back(Polyline::rectangle(local(opening.x, levels[0], opening.sill), base.x_axis(), base.y_axis(), opening.width, height));
            features.top.push_back(Polyline::rectangle(local(opening.x, levels[1], opening.sill), base.x_axis(), base.y_axis(), opening.width, height));
        }

        sheet->features = features;
        sheet->invalidate_geometry();
        _session.add(sheet, sheets);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Steps
// ═══════════════════════════════════════════════════════════════════════════

Point FramedPanel::local(double along, double across, double up) const {
    return base.origin() + base.x_axis() * along + base.z_axis() * across + base.y_axis() * up;
}

std::string FramedPanel::named(const std::string& part) const {
    return name.empty() ? part : fmt::format("{}_{}", name, part);
}

std::shared_ptr<TreeNode> FramedPanel::group(const std::string& part) const {
    return _session.group_named(named(part), _group);
}

std::shared_ptr<Beam> FramedPanel::member(
    const Point& start,
    const Point& end,
    double size,
    const Vector& up,
    const std::string& part
) const {

    const Polyline axis({start, end});
    const std::vector<Polyline> profile = profile_rectangle(guide.stud_depth, size);
    return std::make_shared<Beam>(axis, profile, std::vector<Vector>{up}, named(part));
}

}
