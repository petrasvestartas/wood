#include "src/templates/timber_floor/floor_product.h"
#include "src/templates/timber_floor/floor_sizing.h"
#include "src/templates/module/module_contact.h"

namespace wood_timber_floor {

/// Length of a sample along its span.
static constexpr double SAMPLE = 1200.0;

/// A rectangle as a section.
static FloorSection rectangle(double left, double low, double right, double high) {
    return {{left, low}, {right, low}, {right, high}, {left, high}};
}

// ═══════════════════════════════════════════════════════════════════════════
// FloorProduct
// ═══════════════════════════════════════════════════════════════════════════
double FloorProduct::depth() const {

    double low = 1e300;
    double high = -1e300;

    for (const FloorPart& part : parts) {
        if (part.run == 2) {
            low = std::min(low, part.start);
            high = std::max(high, part.end);
            continue;
        }

        for (const std::array<double, 2>& corner : part.section) {
            low = std::min(low, corner[1]);
            high = std::max(high, corner[1]);
        }
    }

    return high - low;
}

FloorProduct FloorProduct::spanning(double length) const {

    FloorProduct floor = *this;
    floor.span = length;

    for (FloorPart& part : floor.parts)
        if (part.run == 0 && part.end == span)
            part.end = length;

    return floor;
}

FloorPart& FloorProduct::add_along(const std::string& name, const FloorSection& section) {

    parts.push_back({name, section, 0.0, span, 0, {}});

    return parts.back();
}

FloorPart& FloorProduct::add_box(const std::string& name, const std::array<double, 3>& low, const std::array<double, 3>& high) {

    parts.push_back({name, rectangle(low[1], low[2], high[1], high[2]), low[0], high[0], 0, {}});

    return parts.back();
}

// ═══════════════════════════════════════════════════════════════════════════
// Joists, ribs and boxes
// ═══════════════════════════════════════════════════════════════════════════
FloorProduct FloorProduct::joists(size_t lining, double width, double depth, double spacing) {

    FloorProduct floor;
    floor.span = SAMPLE;
    floor.width = 3.0 * spacing + width;
    const double batten = 30.0;
    const double board = 16.0;
    const double foot = lining == 2 ? board + batten : 0.0;

    // lining: boards on battens, between the joists a third up or under them
    if (lining == 2) {
        floor.add_box("lining", {0.0, 0.0, 0.0}, {floor.span, floor.width, board});

        for (size_t k = 0; k < 5; k++) {
            const double x = (floor.span - 60.0) * static_cast<double>(k) / 4.0;
            floor.parts.push_back({fmt::format("batten_{}", k), rectangle(x, board, x + 60.0, foot), 0.0, floor.width, 1, {}});
        }
    }

    for (size_t k = 0; k < 4; k++) {
        const double near = spacing * static_cast<double>(k);
        floor.add_along(fmt::format("joist_{}", k), rectangle(near, foot, near + width, foot + depth));
    }

    for (size_t k = 0; lining == 1 && k < 3; k++) {
        const double near = spacing * static_cast<double>(k) + width;
        const double far = near + spacing - width;
        const double level = depth / 3.0;
        floor.add_along(fmt::format("batten_{}_0", k), rectangle(near, level, near + 40.0, level + 40.0));
        floor.add_along(fmt::format("batten_{}_1", k), rectangle(far - 40.0, level, far, level + 40.0));
        floor.add_along(fmt::format("lining_{}", k), rectangle(near, level + 40.0, far, level + 40.0 + board));
    }

    // boards: along the joists' tops
    floor.add_box("boards", {0.0, 0.0, foot + depth}, {floor.span, floor.width, foot + depth + 21.0});

    return floor;
}

FloorProduct FloorProduct::trimmed(double width, double depth, double spacing) {

    FloorProduct floor;
    floor.span = 4000.0;
    floor.width = 6.0 * spacing + width;
    const double start = 1200.0;
    const double end = 3000.0;

    // joists: whole at both sides, those of the opening stopped by the trimmers, the two beside it doubled
    for (size_t k = 0; k < 7; k++) {
        const double near = spacing * static_cast<double>(k);
        const bool stopped = k == 3;

        if (!stopped) {
            floor.add_along(fmt::format("joist_{}", k), rectangle(near, 0.0, near + width, depth));
            continue;
        }

        floor.add_box(fmt::format("joist_{}_0", k), {0.0, near, 0.0}, {start - width, near + width, depth});
        floor.add_box(fmt::format("joist_{}_1", k), {end + width, near, 0.0}, {floor.span, near + width, depth});
    }

    const double left = 2.0 * spacing + width;
    const double right = 4.0 * spacing;
    floor.parts.push_back({"trimmer_0", rectangle(start - width, 0.0, start, depth), left, right, 1, {}});
    floor.parts.push_back({"trimmer_1", rectangle(end, 0.0, end + width, depth), left, right, 1, {}});

    return floor;
}

FloorProduct FloorProduct::ribbed(bool below, double width, double depth, double spacing, double panel) {

    FloorProduct floor;
    floor.span = SAMPLE;
    floor.width = 2.0 * spacing + width + 200.0;
    const double foot = below ? panel : 0.0;

    if (below)
        floor.add_box("panel", {0.0, 0.0, 0.0}, {floor.span, floor.width, panel});

    for (size_t k = 0; k < 3; k++) {
        const double near = 100.0 + spacing * static_cast<double>(k);
        floor.add_along(fmt::format("rib_{}", k), rectangle(near, foot, near + width, foot + depth));
    }

    if (!below)
        floor.add_box("panel", {0.0, 0.0, depth}, {floor.span, floor.width, depth + panel});

    return floor;
}

FloorProduct FloorProduct::box(double width, double depth, double spacing, double panel) {

    FloorProduct floor = ribbed(true, width, depth, spacing, panel);
    floor.add_box("panel_top", {0.0, 0.0, panel + depth}, {floor.span, floor.width, 2.0 * panel + depth});

    return floor;
}

FloorProduct FloorProduct::cross_laminated_ribs(bool closed, double plate, double width, double depth, double spacing, double element) {

    FloorProduct floor;
    floor.span = SAMPLE;
    floor.width = element;
    const double foot = closed ? plate : 0.0;
    const size_t ribs = static_cast<size_t>(std::floor((element - width) / spacing)) + 1;
    const double inset = 0.5 * (element - width - spacing * static_cast<double>(ribs - 1));

    if (closed)
        floor.add_box("plate_bottom", {0.0, 0.0, 0.0}, {floor.span, element, plate});

    for (size_t k = 0; k < ribs; k++) {
        const double near = inset + spacing * static_cast<double>(k);
        floor.add_along(fmt::format("rib_{}", k), rectangle(near, foot, near + width, foot + depth));
    }

    floor.add_box("plate_top", {0.0, 0.0, foot + depth}, {floor.span, element, foot + depth + plate});

    return floor;
}

FloorProduct FloorProduct::clt_box(size_t kind, double depth, double rib) {

    FloorProduct floor;
    floor.span = SAMPLE;
    const double element = 1200.0;
    floor.width = 2.0 * element;
    const double flange = 60.0;
    const bool closed = kind == 0;
    const double low = kind == 2 ? 0.0 : flange;
    const double high = closed || kind == 2 ? depth - flange : depth;
    const double tongue = 10.0;

    for (size_t e = 0; e < 2; e++) {
        const double near = element * static_cast<double>(e);
        const double far = near + element;

        // lower flange: cross-laminated, a tongue of 10 in the next element's groove
        if (kind != 2) {
            FloorSection lower = {{near, 0.0}, {far, 0.0}};

            if (e == 0)
                lower.insert(lower.end(), {{far, 25.0}, {far + tongue, 25.0}, {far + tongue, 35.0}, {far, 35.0}});

            lower.insert(lower.end(), {{far, flange}, {near, flange}});

            if (e == 1)
                lower.insert(lower.end(), {{near, 35.0}, {near + tongue, 35.0}, {near + tongue, 25.0}, {near, 25.0}});

            floor.add_along(fmt::format("lower_{}", e), lower);
        }

        // ribs: at both edges and the middle, the roof's two a quarter in
        const std::vector<double> places = kind == 2 ? std::vector<double>{0.25 * element - 0.5 * rib, 0.75 * element - 0.5 * rib}
                                                     : std::vector<double>{0.0, 0.5 * (element - rib), element - rib};

        for (size_t k = 0; k < places.size(); k++)
            floor.add_along(fmt::format("rib_{}_{}", e, k), rectangle(near + places[k], low, near + places[k] + rib, high));

        // upper flange: a rebate over the joint for the loose board
        if (closed || kind == 2) {
            const double rebate = 51.0;
            const double cut = 23.0;
            FloorSection upper = {{near, high}, {far, high}};

            if (e == 0)
                upper.insert(upper.end(), {{far, depth - cut}, {far - rebate, depth - cut}, {far - rebate, depth}});
            else
                upper.push_back({far, depth});

            if (e == 1)
                upper.insert(upper.end(), {{near + rebate, depth}, {near + rebate, depth - cut}, {near, depth - cut}});
            else
                upper.push_back({near, depth});

            floor.add_along(fmt::format("upper_{}", e), upper);
        }
    }

    if (closed || kind == 2)
        floor.add_box("loose_board", {0.0, element - 50.0, depth - 23.0}, {floor.span, element + 50.0, depth - 1.0});

    return floor;
}

FloorProduct FloorProduct::novatop(double depth) {

    FloorProduct floor;
    floor.span = SAMPLE;
    floor.width = 2090.0;
    const double panel = 27.0;
    const double rib = 27.0;
    const double high = depth - panel;
    floor.add_box("lower", {0.0, 0.0, 0.0}, {floor.span, floor.width, panel});
    const std::vector<double> axes = {48.5, 388.5, 728.5, 1068.5, 1408.5, 1728.5, 2041.5};

    for (size_t k = 0; k < axes.size(); k++)
        floor.add_along(fmt::format("rib_{}", k), rectangle(axes[k] - 0.5 * rib, panel, axes[k] + 0.5 * rib, high));

    // cross ribs: between the long ones, closing both ends and at the middle
    for (size_t row = 0; row < 3; row++) {
        const double x = (floor.span - rib) * static_cast<double>(row) / 2.0;

        for (size_t k = 0; k + 1 < axes.size(); k++)
            floor.parts.push_back({
                fmt::format("cross_{}_{}", row, k), rectangle(x, panel, x + rib, high), axes[k] + 0.5 * rib, axes[k + 1] - 0.5 * rib, 1, {}
            });
    }

    floor.add_box("upper", {0.0, 0.0, high}, {floor.span, floor.width, depth});

    return floor;
}

FloorProduct FloorProduct::novatop_open(double rib_depth) {

    FloorProduct floor;
    floor.span = SAMPLE;
    floor.width = 2450.0;
    const double panel = 27.0;
    const double rib = 60.0;
    floor.add_box("lower", {0.0, 0.0, 0.0}, {floor.span, floor.width, panel});

    for (size_t k = 0; k < 4; k++) {
        const double axis = 287.0 + 625.0 * static_cast<double>(k);
        floor.add_along(fmt::format("rib_{}", k), rectangle(axis - 0.5 * rib, panel, axis + 0.5 * rib, panel + rib_depth));
    }

    // cross ribs: round the ends, between the long ones
    for (size_t row = 0; row < 2; row++) {
        const double x = (floor.span - rib) * static_cast<double>(row);

        for (size_t k = 0; k < 3; k++) {
            const double axis = 287.0 + 625.0 * static_cast<double>(k);
            floor.parts.push_back({
                fmt::format("cross_{}_{}", row, k), rectangle(x, panel, x + rib, panel + rib_depth), axis + 0.5 * rib, axis + 625.0 - 0.5 * rib, 1, {}
            });
        }
    }

    return floor;
}

FloorProduct FloorProduct::lignatur_box(double depth) {

    FloorProduct floor;
    floor.span = SAMPLE;
    const double cover = 200.0;
    const size_t count = 6;
    floor.width = cover * static_cast<double>(count);
    const double shell = 31.0;
    const double web = 32.0;
    const double tongue = 13.0;
    const double gap = 12.0;

    for (size_t k = 0; k < count; k++) {
        const double near = cover * static_cast<double>(k);
        const double far = near + cover;

        // shells: over the whole cover width, a tongue in the one before, 14 from the outer face and 10 high
        for (size_t side = 0; side < 2; side++) {
            const double low = side == 0 ? 0.0 : depth - shell;
            const double high = low + shell;
            const double edge = side == 0 ? low + 14.0 : high - 24.0;
            FloorSection section = {{near, low}, {far, low}};

            if (k + 1 < count)
                section.insert(section.end(), {{far, edge}, {far - tongue, edge}, {far - tongue, edge + 10.0}, {far, edge + 10.0}});

            section.insert(section.end(), {{far, high}, {near, high}});

            if (k > 0)
                section.insert(section.end(), {{near, edge + 10.0}, {near - tongue, edge + 10.0}, {near - tongue, edge}, {near, edge}});

            floor.add_along(fmt::format("shell_{}_{}", k, side), section);

            // webs: between the shells, the far one flush, the near one set back
            if (side == 0) {
                floor.add_along(fmt::format("web_{}_0", k), rectangle(near + gap, shell, near + gap + web, depth - shell));
                floor.add_along(fmt::format("web_{}_1", k), rectangle(far - web, shell, far, depth - shell));
            }
        }
    }

    return floor;
}

FloorProduct FloorProduct::lignatur_surface(double depth, size_t kind) {

    FloorProduct floor;
    floor.span = SAMPLE;
    const double cover = 1000.0;
    const double body = 990.0;
    floor.width = 2.0 * cover - 10.0;
    const double board = depth > 320.0 ? 40.0 : 31.0;
    const double web = 31.0;
    const double pitch = (body - web) / 4.0;
    const double groove = 10.0;

    for (size_t e = 0; e < 2; e++) {
        const double near = cover * static_cast<double>(e);

        // webs: over the whole depth, the outer ones at the joint grooved for the loose tongue
        for (size_t k = 0; k < 5; k++) {
            const double left = near + pitch * static_cast<double>(k);
            const double right = left + web;
            FloorSection section = rectangle(left, 0.0, right, depth);

            if (e == 0 && k == 4)
                section = {{left, 0.0}, {right, 0.0}, {right, 14.0}, {right - groove, 14.0}, {right - groove, 24.0}, {right, 24.0}, {right, depth}, {left, depth}};

            if (e == 1 && k == 0)
                section = {{left, 0.0}, {right, 0.0}, {right, depth}, {left, depth}, {left, 24.0}, {left + groove, 24.0}, {left + groove, 14.0}, {left, 14.0}};

            floor.add_along(fmt::format("web_{}_{}", e, k), section);
        }

        // boards: between the webs, below, and above unless the top is open; an absorber or blocks on the lower ones
        for (size_t k = 0; k < 4; k++) {
            const double left = near + pitch * static_cast<double>(k) + web;
            const double right = near + pitch * static_cast<double>(k + 1);
            floor.add_along(fmt::format("lower_{}_{}", e, k), rectangle(left, 0.0, right, board));

            if (kind == 1)
                for (size_t block = 0; block < 2; block++) {
                    const double x = 100.0 + 600.0 * static_cast<double>(block);
                    floor.add_box(fmt::format("block_{}_{}_{}", e, k, block), {x, left + 45.0, board + 12.0}, {x + 400.0, right - 45.0, board + 72.0});
                    floor.add_box(fmt::format("pad_{}_{}_{}", e, k, block), {x, left + 45.0, board}, {x + 400.0, right - 45.0, board + 12.0});
                }

            if (kind == 2)
                floor.add_along(fmt::format("absorber_{}_{}", e, k), rectangle(left, board, right, board + 40.0));
            else
                floor.add_along(fmt::format("upper_{}_{}", e, k), rectangle(left, depth - board, right, depth));
        }
    }

    floor.add_along("tongue", rectangle(body - groove, 14.5, cover + groove, 23.5));

    return floor;
}

FloorProduct FloorProduct::lignotrend_rib(double depth) {

    FloorProduct floor;
    floor.span = SAMPLE;
    const double cover = 625.0;
    floor.width = 2.0 * cover;
    const std::array<std::array<double, 2>, 3> webs = {{{54.0, 124.0}, {279.0, 354.0}, {509.0, 579.0}}};
    const std::array<double, 3> chords = {129.0, 134.0, 129.0};
    const std::array<double, 3> middles = {110.0, 115.0, 110.0};
    const std::array<double, 3> feet = {115.0, 134.0, 115.0};
    const double under = 32.0;
    const double cross = 115.0;
    const double chord = 25.0;
    const double crossed = 20.0;
    const double wide = 53.0;
    const double stack = depth - under - wide - crossed - chord;

    for (size_t e = 0; e < 2; e++) {
        const double near = cover * static_cast<double>(e);

        // closed layer: a visible board 12 and boards across it 20
        floor.add_box(fmt::format("visible_{}", e), {0.0, near + 8.0, 0.0}, {floor.span, near + cover, 12.0});

        for (size_t k = 0; k < 10; k++) {
            const double x = floor.span * static_cast<double>(k) / 10.0;
            floor.parts.push_back({fmt::format("closed_{}_{}", e, k), rectangle(x, 12.0, x + floor.span / 10.0, under), near + 8.0, near + cover, 1, {}});
        }

        for (size_t w = 0; w < 3; w++) {
            const double middle = near + 0.5 * (webs[w][0] + webs[w][1]);
            const double half = 0.5 * (webs[w][1] - webs[w][0]);

            // ribs: two wide boards, the stack, then the crossed layer with a board across now and then, then the chord
            for (size_t k = 0; k < 2; k++)
                floor.add_along(fmt::format("foot_{}_{}_{}", e, w, k), rectangle(middle - 0.5 * feet[w], under + 26.5 * k, middle + 0.5 * feet[w], under + 26.5 * (k + 1)));

            const size_t boards = std::max<size_t>(1, static_cast<size_t>(std::round(stack / 20.0)));

            for (size_t k = 0; k < boards; k++) {
                const double low = under + wide + stack * static_cast<double>(k) / static_cast<double>(boards);
                floor.add_along(fmt::format("web_{}_{}_{}", e, w, k), rectangle(middle - half, low, middle + half, low + stack / static_cast<double>(boards)));
            }

            const double level = depth - chord - crossed;

            for (size_t k = 0; k < 2; k++) {
                const double start = k == 0 ? 0.0 : 0.5 * (floor.span + cross);
                const double end = k == 0 ? 0.5 * (floor.span - cross) : floor.span;
                floor.add_box(
                    fmt::format("crossed_{}_{}_{}", e, w, k), {start, middle - 0.5 * middles[w], level}, {end, middle + 0.5 * middles[w], level + crossed}
                );
            }

            floor.add_along(fmt::format("chord_{}_{}", e, w), rectangle(middle - 0.5 * chords[w], depth - chord, middle + 0.5 * chords[w], depth));
        }

        floor.parts.push_back({
            fmt::format("across_{}", e), rectangle(0.5 * (floor.span - cross), depth - chord - crossed, 0.5 * (floor.span + cross), depth - chord), near + 8.0, near + cover, 1, {}
        });
    }

    return floor;
}

FloorProduct FloorProduct::lignotrend_block(double depth) {

    FloorProduct floor;
    floor.span = SAMPLE;
    const double cover = 625.0;
    floor.width = 2.0 * cover;
    const std::array<std::array<double, 2>, 3> webs = {{{8.0, 78.0}, {279.0, 354.0}, {555.0, 625.0}}};
    const double under = 32.0;
    const double over = 45.0;
    const double high = depth - over;

    for (size_t e = 0; e < 2; e++) {
        const double near = cover * static_cast<double>(e);
        floor.add_box(fmt::format("visible_{}", e), {0.0, near + 8.0, 0.0}, {floor.span, near + cover, 12.0});

        for (size_t k = 0; k < 10; k++) {
            const double x = floor.span * static_cast<double>(k) / 10.0;
            const double next = x + floor.span / 10.0;
            floor.parts.push_back({fmt::format("closed_{}_{}", e, k), rectangle(x, 12.0, next, under), near + 8.0, near + cover, 1, {}});
            floor.parts.push_back({fmt::format("cover_{}_{}", e, k), rectangle(x, high, next, high + 20.0), near + 8.0, near + cover, 1, {}});
        }

        // webs: boards lying on each other
        for (size_t w = 0; w < 3; w++) {
            const size_t boards = std::max<size_t>(1, static_cast<size_t>(std::round((high - under) / 26.5)));

            for (size_t k = 0; k < boards; k++) {
                const double step = (high - under) / static_cast<double>(boards);
                floor.add_along(
                    fmt::format("web_{}_{}_{}", e, w, k),
                    rectangle(near + webs[w][0], under + step * static_cast<double>(k), near + webs[w][1], under + step * static_cast<double>(k + 1))
                );
            }
        }

        // top: boards along the span, back from the edges for the splice board
        for (size_t k = 0; k < 5; k++) {
            const double left = near + 50.0 + 105.0 * static_cast<double>(k);
            floor.add_along(fmt::format("top_{}_{}", e, k), rectangle(left, depth - 25.0, left + 105.0, depth));
        }
    }

    floor.add_box("splice", {0.0, cover - 47.5, depth - 25.0}, {floor.span, cover + 47.5, depth + 1.5});

    return floor;
}

FloorProduct FloorProduct::kielsteg(double depth, double chord_depth, double chord_width, double web) {

    FloorProduct floor;
    floor.span = SAMPLE;
    const double pitch = chord_width + 2.0 * web;
    const size_t count = 10;
    floor.width = pitch * static_cast<double>(count);
    const double low = chord_depth;
    const double high = depth - chord_depth;
    const size_t sides = 16;

    // chords: timbers side by side, the lower row half a pitch on
    for (size_t k = 0; k < count; k++) {
        const double near = pitch * static_cast<double>(k);
        floor.add_along(fmt::format("upper_{}", k), rectangle(near + web, high, near + pitch - web, depth));

        if (k > 0)
            floor.add_along(fmt::format("lower_{}", k), rectangle(near - 0.5 * pitch + web, 0.0, near + 0.5 * pitch - web, chord_depth));
    }

    // webs: from a joint above to the joints beside it below, upright in both and an s between them
    for (size_t k = 0; k <= count; k++)
        for (const double way : {-1.0, 1.0}) {
            if ((k == 0 && way < 0.0) || (k == count && way > 0.0))
                continue;

            const double top = pitch * static_cast<double>(k) + (way < 0.0 ? -web : 0.0);
            const double shift = way * (0.5 * pitch - web);
            FloorSection left = {{top, depth}};
            FloorSection right = {{top + web, depth}};

            for (size_t s = 0; s <= sides; s++) {
                const double t = static_cast<double>(s) / static_cast<double>(sides);
                const double x = top + shift * t * t * (3.0 - 2.0 * t);
                left.push_back({x, high + (low - high) * t});
                right.push_back({x + web, high + (low - high) * t});
            }

            left.push_back({top + shift, 0.0});
            right.push_back({top + shift + web, 0.0});
            FloorSection section = left;
            section.insert(section.end(), right.rbegin(), right.rend());
            floor.add_along(fmt::format("web_{}_{}", k, way < 0.0 ? 0 : 1), section);
        }

    return floor;
}

FloorProduct FloorProduct::lvl_rib(bool strips, double depth) {

    FloorProduct floor;
    floor.span = SAMPLE;
    const double rib = 51.0;
    const double pitch = 735.0;
    const double panel = 27.0;
    const double strip = 36.0;
    const double foot = strips ? strip : 0.0;
    floor.width = 3.0 * pitch + rib;

    for (size_t k = 0; k < 4; k++) {
        const double near = pitch * static_cast<double>(k);
        const double middle = near + 0.5 * rib;

        if (strips)
            floor.add_along(
                fmt::format("strip_{}", k), rectangle(k == 0 ? 0.0 : middle - 150.0, 0.0, k == 3 ? floor.width : middle + 150.0, strip)
            );

        floor.add_along(fmt::format("rib_{}", k), rectangle(near, foot, near + rib, foot + depth));
    }

    floor.add_box("panel", {0.0, 0.0, foot + depth}, {floor.span, floor.width, foot + depth + panel});

    return floor;
}

FloorProduct FloorProduct::ribs(
    double span, size_t count, double spacing, double width, double depth, double panel, double strip, double strip_depth) {

    FloorProduct floor;
    floor.span = span;
    floor.width = spacing * static_cast<double>(count);

    // a strip under every rib, one panel under all of them when the strips would meet
    const bool closed = strip >= spacing;

    if (closed)
        floor.add_box("under", {0.0, 0.0, 0.0}, {span, floor.width, strip_depth});

    for (size_t k = 0; k < count; k++) {
        const double middle = spacing * (static_cast<double>(k) + 0.5);

        if (!closed && strip > 0.0)
            floor.add_along(fmt::format("strip_{}", k), rectangle(middle - 0.5 * strip, 0.0, middle + 0.5 * strip, strip_depth));

        floor.add_along(
            fmt::format("rib_{}", k), rectangle(middle - 0.5 * width, strip_depth, middle + 0.5 * width, strip_depth + depth)
        );
    }

    floor.add_box("panel", {0.0, 0.0, strip_depth + depth}, {span, floor.width, strip_depth + depth + panel});

    return floor;
}

FloorProduct FloorProduct::i_joists(double span, size_t count, double spacing, double width, double depth) {

    FloorProduct floor;
    floor.span = span;
    floor.width = spacing * static_cast<double>(count);
    const double flange = 39.0;
    const double web = 10.0;

    for (size_t k = 0; k < count; k++) {
        const double middle = spacing * (static_cast<double>(k) + 0.5);
        const double left = middle - 0.5 * width;
        floor.add_along(fmt::format("flange_{}_0", k), rectangle(left, 0.0, left + width, flange));
        floor.add_along(fmt::format("web_{}", k), rectangle(middle - 0.5 * web, flange, middle + 0.5 * web, depth - flange));
        floor.add_along(fmt::format("flange_{}_1", k), rectangle(left, depth - flange, left + width, depth));
    }

    floor.add_box("deck", {0.0, 0.0, depth}, {span, floor.width, depth + 22.0});

    return floor;
}

FloorProduct FloorProduct::baubuche(double span, double g, double q) {

    const SizingRule rule = SizingRule::baubuche_joist();
    SizingStock stock = SizingStock::baubuche();
    stock.widths = {120.0};
    const SizingSection joist = rule.pick(stock.sections(rule), span, 850.0, g, q);

    return ribs(span, 3, 850.0, joist.width, joist.depth, 40.0);
}

FloorProduct FloorProduct::kerto_ripa(double span, bool boxed, double g, double q) {

    const SizingRule rule = SizingRule::kerto_ripa();
    const double spacing = 612.0;
    const SizingLayer top = {spacing, 25.0, 10500.0, 5.1};
    const SizingLayer strip = boxed ? SizingLayer{300.0, 43.0, 13800.0, 5.1} : SizingLayer{};

    for (const double depth : SizingStock::kerto().depths) {
        const SizingSection rib = rule.rectangle(boxed ? 45.0 : 51.0, depth);

        if (rule.span_ribs(rib, spacing, top, strip, g, q) >= span)
            return ribs(span, 4, spacing, rib.width, depth, top.depth, strip.width, strip.depth);
    }

    throw std::invalid_argument("no rib in stock spans that far");
}

FloorProduct FloorProduct::lignatur(double span, double g, double q) {

    const SizingRule rule = SizingRule::lignatur();

    for (const double depth : {120.0, 140.0, 160.0, 180.0, 200.0, 220.0, 240.0, 280.0, 320.0, 360.0})
        if (rule.span(rule.lignatur_section(depth), 1000.0, g, q) >= span)
            return lignatur_surface(depth).spanning(span);

    throw std::invalid_argument("no element spans that far");
}

FloorProduct FloorProduct::clt_box_spanning(double span, double g, double q) {

    const SizingRule rule = SizingRule::clt_box();

    for (double depth = 220.0; depth <= 480.0; depth += 20.0)
        for (const double rib : {80.0, 100.0, 120.0})
            if (rule.span(rule.clt_box_section(depth, rib, span), 1200.0, g, q) >= span)
                return clt_box(0, depth, rib).spanning(span);

    throw std::invalid_argument("no element spans that far");
}

FloorProduct FloorProduct::kielsteg_spanning(double span, double q) {

    const SizingRule rule = SizingRule::kielsteg();

    for (const KielstegType& type : KielstegType::all())
        if (rule.span(SizingRule::kielsteg_section(type), 1000.0, 0.0, q) >= span)
            return kielsteg(type.depth, type.chord_depth, type.chord_width, type.depth < 400.0 ? 4.0 : 8.0).spanning(span);

    throw std::invalid_argument("no element spans that far");
}

FloorProduct FloorProduct::vmg_beams(double span, size_t grade, double g, double q) {

    const SizingRule rule = SizingRule::vmg_beam(grade);
    const SizingSection beam = rule.pick(SizingStock::vmg().sections(rule), span, 1000.0, g, q);

    return ribs(span, 3, 1000.0, beam.width, beam.depth, 25.0);
}

FloorProduct FloorProduct::vmg_joists(double span, double spacing, double g, double q) {

    // two checks: the joist alone bends, the deck swings with it
    SizingRule bends = SizingRule::i_joist();
    SizingRule swings = bends;
    bends.frequency = 0.0;
    swings.deflection_end = 0.0;

    for (const double depth : {200.0, 220.0, 240.0, 250.0, 300.0, 360.0, 400.0})
        for (const double width : {45.0, 60.0, 90.0})
            if (bends.span(SizingRule::i_section(width, depth), spacing, g, q) >= span
                && swings.span(SizingRule::i_section(width, depth, spacing), spacing, g, q) >= span)
                return i_joists(span, 4, spacing, width, depth);

    throw std::invalid_argument("no joist in stock spans that far");
}

FloorProduct FloorProduct::void_panel(double plate, double core) {

    FloorProduct floor;
    floor.span = SAMPLE;
    floor.width = 2700.0;
    const double strip = 186.0;
    const double gap = 372.0;
    const double inset = 0.5 * (floor.width - 5.0 * strip - 4.0 * gap);
    floor.add_box("lower", {0.0, 0.0, 0.0}, {floor.span, floor.width, plate});

    for (size_t k = 0; k < 5; k++) {
        const double near = inset + (strip + gap) * static_cast<double>(k);
        floor.add_along(fmt::format("core_{}", k), rectangle(near, plate, near + strip, plate + core));
    }

    floor.add_box("upper", {0.0, 0.0, plate + core}, {floor.span, floor.width, 2.0 * plate + core});

    return floor;
}

FloorProduct FloorProduct::dowelled_layers() {

    FloorProduct floor;
    floor.span = 2.0 * SAMPLE;
    floor.width = 1200.0;
    const std::vector<double> layers = {24.0, 40.0, 60.0, 40.0, 24.0};
    double level = 0.0;
    std::vector<std::string> names;

    // layers: the outer ones and the middle along the span, the two between across it
    for (size_t k = 0; k < layers.size(); k++) {
        const bool across = k % 2 == 1;
        const size_t count = across ? 12 : 6;
        const double size = (across ? floor.span : floor.width) / static_cast<double>(count);

        for (size_t b = 0; b < count; b++) {
            const double near = size * static_cast<double>(b);
            const std::string name = fmt::format("layer_{}_{}", k, b);
            names.push_back(name);

            if (across)
                floor.parts.push_back({name, rectangle(near, level, near + size, level + layers[k]), 0.0, floor.width, 1, {}});
            else
                floor.add_along(name, rectangle(near, level, near + size, level + layers[k]));
        }

        level += layers[k];
    }

    // dowels: upright through all layers, in the middle of every crossing of the boards
    for (size_t i = 0; i < 6; i++)
        for (size_t j = 0; j < 3; j++) {
            const double x = 200.0 + 400.0 * static_cast<double>(i) - 100.0;
            const double y = 300.0 + 400.0 * static_cast<double>(j) - 200.0;
            FloorPins dowel = {fmt::format("dowel_{}_{}", i, j), {{{{x, y, 0.0}, {x, y, level}}}}, 10.0, {}};
            for (size_t k = 0; k < layers.size(); k++) {
                const bool across = k % 2 == 1;
                const size_t count = across ? 12 : 6;
                const double size = (across ? floor.span : floor.width) / static_cast<double>(count);
                dowel.into.push_back(fmt::format("layer_{}_{}", k, static_cast<size_t>((across ? x : y) / size)));
            }

            floor.pins.push_back(dowel);
        }

    return floor;
}

// ═══════════════════════════════════════════════════════════════════════════
// Solid timber
// ═══════════════════════════════════════════════════════════════════════════
FloorProduct FloorProduct::planks(bool comb, double depth, double plank) {

    FloorProduct floor;
    floor.span = SAMPLE;
    floor.width = 6.0 * plank;
    const double tongue = 18.0;
    const double step = depth / 5.0;

    for (size_t k = 0; k < 6; k++) {
        const double near = plank * static_cast<double>(k);
        const double far = near + plank;
        FloorSection section = {{near, 0.0}, {far, 0.0}, {far, depth}, {near, depth}};

        // comb: two grooves on the near edge, two tongues on the far one
        if (comb) {
            section = {{near, 0.0}, {far, 0.0}};

            for (size_t t = 0; k < 5 && t < 2; t++) {
                const double low = step * (1.0 + 2.0 * static_cast<double>(t));
                section.push_back({far, low});
                section.push_back({far + tongue, low});
                section.push_back({far + tongue, low + step});
                section.push_back({far, low + step});
            }

            section.push_back({far, depth});
            section.push_back({near, depth});

            for (size_t t = 2; k > 0 && t > 0; t--) {
                const double low = step * (2.0 * static_cast<double>(t) - 1.0);
                section.push_back({near, low + step});
                section.push_back({near + tongue, low + step});
                section.push_back({near + tongue, low});
                section.push_back({near, low});
            }
        }

        floor.add_along(fmt::format("plank_{}", k), section);
    }

    if (!comb)
        floor.add_box("board", {0.0, 0.0, depth}, {floor.span, floor.width, depth + 22.0});

    return floor;
}

FloorProduct FloorProduct::glulam_elements(double depth, double lamination) {

    FloorProduct floor;
    floor.span = SAMPLE;
    const size_t count = 15;
    const double element = lamination * static_cast<double>(count);
    floor.width = 2.0 * element;
    const double middle = 0.5 * depth;
    const double groove = 30.0;
    const double half = 14.0;

    // laminations: on edge, the ones at the joint grooved for half the strip
    for (size_t e = 0; e < 2; e++)
        for (size_t k = 0; k < count; k++) {
            const double near = element * static_cast<double>(e) + lamination * static_cast<double>(k);
            const double far = near + lamination;
            FloorSection section = rectangle(near, 0.0, far, depth);

            if (e == 0 && k + 1 == count)
                section = {{near, 0.0}, {far, 0.0}, {far, middle - half}, {far - groove, middle - half}, {far - groove, middle + half}, {far, middle + half}, {far, depth}, {near, depth}};

            if (e == 1 && k == 0)
                section = {{near, 0.0}, {far, 0.0}, {far, depth}, {near, depth}, {near, middle + half}, {near + groove, middle + half}, {near + groove, middle - half}, {near, middle - half}};

            floor.add_along(fmt::format("lamination_{}_{}", e, k), section);

            if (e == 0 && k + 1 == count)
                floor.add_along("strip", rectangle(element - groove, middle - half, element + groove, middle + half));
        }

    return floor;
}

FloorProduct FloorProduct::laminated(bool nailed, double depth, double board) {

    FloorProduct floor;
    floor.span = SAMPLE;
    const size_t count = 18;
    floor.width = board * static_cast<double>(count);
    std::vector<std::string> names;

    for (size_t k = 0; k < count; k++) {
        const double near = board * static_cast<double>(k);
        names.push_back(fmt::format("board_{}", k));
        floor.add_along(names.back(), rectangle(near, 0.0, near + board, depth));
    }

    if (!nailed) {
        FloorPins dowels = {"dowels", {}, 10.0, names};

        for (size_t k = 0; k < 2; k++) {
            const double x = 300.0 + 600.0 * static_cast<double>(k);
            dowels.axes.push_back({{{x, 0.0, 0.5 * depth}, {x, floor.width, 0.5 * depth}}});
        }

        floor.pins.push_back(dowels);
        return floor;
    }

    // nails: through two boards and a half, in two rows, every board's shifted past the last one's
    for (size_t k = 0; k + 2 < count; k++) {
        FloorPins nails = {fmt::format("nails_{}", k), {}, 2.0, {names[k], names[k + 1], names[k + 2]}};
        const double near = board * static_cast<double>(k);

        for (size_t n = 0; n < 4; n++) {
            const double x = 60.0 + 300.0 * static_cast<double>(n) + (k % 3) * 80.0;
            const double z = depth * (n % 2 == 0 ? 0.25 : 0.75);
            nails.axes.push_back({{{x, near, z}, {x, near + 2.5 * board, z}}});
        }

        floor.pins.push_back(nails);
    }

    return floor;
}

FloorProduct FloorProduct::cross_laminated(const std::vector<double>& layers, size_t joint) {

    FloorProduct floor;
    floor.span = SAMPLE;
    floor.width = 1800.0;
    const double middle = 0.5 * floor.width;
    const double gap = joint == 2 ? 4.0 : 0.0;
    double level = 0.0;
    double total = 0.0;

    for (const double layer : layers)
        total += layer;

    // layers: whole plates, two side by side where the floor has a joint
    for (size_t k = 0; k < layers.size(); k++) {
        const double next = level + layers[k];

        if (joint == 0) {
            floor.add_box(fmt::format("layer_{}", k), {0.0, 0.0, level}, {floor.span, floor.width, next});
            level = next;
            continue;
        }

        const bool top = k + 1 == layers.size();
        const double rebate = joint == 1 && top ? 80.0 : 0.5 * gap;
        const double groove = joint == 1 && k == layers.size() / 2 ? 40.0 : rebate;
        floor.add_box(fmt::format("layer_{}_0", k), {0.0, 0.0, level}, {floor.span, middle - (k == layers.size() / 2 ? groove : rebate), next});
        floor.add_box(fmt::format("layer_{}_1", k), {0.0, middle + (k == layers.size() / 2 ? groove : rebate), level}, {floor.span, floor.width, next});
        level = next;
    }

    const double first = total - layers.back();

    // joint: a cover board and a strip, grout, wedge keys or shear blocks
    if (joint == 1) {
        double low = 0.0;

        for (size_t k = 0; k < layers.size() / 2; k++)
            low += layers[k];

        floor.add_box("strip", {0.0, middle - 40.0, low}, {floor.span, middle + 40.0, low + layers[layers.size() / 2]});
        floor.add_box("cover", {0.0, middle - 80.0, first}, {floor.span, middle + 80.0, total});
    }

    if (joint == 2)
        floor.add_box("grout", {0.0, middle - 0.5 * gap, 0.0}, {floor.span, middle + 0.5 * gap, total});

    std::vector<std::string> both;

    for (size_t k = 0; k < layers.size(); k++)
        for (size_t side = 0; side < 2; side++)
            both.push_back(fmt::format("layer_{}_{}", k, side));

    // keys: wedge keys two dovetails wide, driven from above
    for (size_t k = 0; joint == 3 && k < 1; k++) {
        const double x = 600.0 + 1200.0 * static_cast<double>(k);
        const FloorSection key = {
            {x - 65.0, middle - 45.0}, {x + 65.0, middle - 45.0}, {x + 35.0, middle}, {x + 65.0, middle + 45.0}, {x - 65.0, middle + 45.0}, {x - 35.0, middle}
        };
        floor.parts.push_back({fmt::format("key_{}", k), key, total - 90.0, total, 2, both});
    }

    // blocks: steel blocks in slots across the joint
    for (size_t k = 0; joint == 4 && k < 2; k++) {
        const double x = 300.0 + 600.0 * static_cast<double>(k);
        floor.parts.push_back({fmt::format("block_{}", k), rectangle(x - 60.0, middle - 45.0, x + 60.0, middle + 45.0), total - 40.0, total, 2, both});
    }

    return floor;
}

FloorProduct FloorProduct::veneer(double depth) {

    FloorProduct floor;
    floor.span = SAMPLE;
    floor.width = 1800.0;
    floor.add_box("panel", {0.0, 0.0, 0.0}, {floor.span, floor.width, depth});

    return floor;
}

// ═══════════════════════════════════════════════════════════════════════════
// Timber and concrete
// ═══════════════════════════════════════════════════════════════════════════
FloorProduct FloorProduct::composite_stepped(double depth, double step, double concrete) {

    FloorProduct floor;
    floor.span = 2.0 * SAMPLE;
    const double board = 40.0;
    const size_t count = 24;
    floor.width = board * static_cast<double>(count);
    FloorSection topping = {{0.0, depth + concrete}};
    FloorSection comb;

    // boards: one deep, one shallow, the concrete down to the shallow ones
    for (size_t k = 0; k < count; k++) {
        const double near = board * static_cast<double>(k);
        const double top = k % 2 == 0 ? depth : depth - step;
        floor.add_along(fmt::format("board_{}", k), rectangle(near, 0.0, near + board, top));
        comb.push_back({near, top});
        comb.push_back({near + board, top});
    }

    topping.insert(topping.end(), comb.begin(), comb.end());
    topping.push_back({floor.width, depth + concrete});
    floor.add_along("concrete", topping);

    return floor;
}

FloorProduct FloorProduct::composite_micro(double depth, double concrete) {

    FloorProduct floor;
    floor.span = 2.0 * SAMPLE;
    floor.width = 800.0;
    const double pitch = 60.0;
    const double notch = 15.0;
    FloorSection seam = {{0.0, depth}};

    // seam: a saw's teeth along the whole span
    for (double x = 0.0; x + pitch <= floor.span + 1e-6; x += pitch) {
        seam.push_back({x + 0.5 * pitch, depth});
        seam.push_back({x + 0.5 * pitch, depth - notch});
        seam.push_back({x + pitch, depth - notch});

        if (x + 1.5 * pitch <= floor.span + 1e-6)
            seam.push_back({x + pitch, depth});
    }

    FloorSection timber = seam;
    timber.push_back({floor.span, 0.0});
    timber.push_back({0.0, 0.0});
    FloorSection topping(seam.rbegin(), seam.rend());
    topping.push_back({0.0, depth + concrete});
    topping.push_back({floor.span, depth + concrete});
    floor.parts.push_back({"timber", timber, 0.0, floor.width, 1, {}});
    floor.parts.push_back({"concrete", topping, 0.0, floor.width, 1, {}});

    return floor;
}

FloorProduct FloorProduct::composite_notched(double depth, double concrete) {

    FloorProduct floor;
    floor.span = 2.0 * SAMPLE;
    floor.width = 800.0;
    FloorSection seam = {{0.0, depth}};
    FloorPins pins = {"pins", {}, 6.0, {"timber", "concrete"}};

    for (const double x : {250.0, 750.0, 1500.0, 2000.0}) {
        seam.insert(seam.end(), {{x - 75.0, depth}, {x - 65.0, depth - 30.0}, {x + 65.0, depth - 30.0}, {x + 75.0, depth}});

        for (const double y : {200.0, 600.0})
            pins.axes.push_back({{{x, y, depth - 120.0}, {x, y, depth + 0.6 * concrete}}});
    }

    seam.push_back({floor.span, depth});
    FloorSection timber = seam;
    timber.push_back({floor.span, 0.0});
    timber.push_back({0.0, 0.0});
    FloorSection topping(seam.rbegin(), seam.rend());
    topping.push_back({0.0, depth + concrete});
    topping.push_back({floor.span, depth + concrete});
    floor.parts.push_back({"timber", timber, 0.0, floor.width, 1, {}});
    floor.parts.push_back({"concrete", topping, 0.0, floor.width, 1, {}});
    floor.pins.push_back(pins);

    return floor;
}

FloorProduct FloorProduct::composite_lugs() {

    FloorProduct floor;
    floor.span = 2.0 * SAMPLE;
    floor.width = 1600.0;
    const double width = 160.0;
    const double depth = 280.0;
    const double lug = 60.0;
    const double concrete = 100.0;

    // joists: pockets cut by the lugs of the slab on them, which is one piece with them
    for (size_t k = 0; k < 2; k++)
        floor.add_along(fmt::format("joist_{}", k), rectangle(200.0 + 1040.0 * k, 0.0, 200.0 + 1040.0 * k + width, depth));

    for (size_t k = 0; k < 2; k++)
        for (size_t n = 0; n < 4; n++) {
            const double x = 300.0 + 600.0 * static_cast<double>(n);
            const double near = 200.0 + 1040.0 * static_cast<double>(k) + 20.0;
            floor.parts.push_back({
                fmt::format("lug_{}_{}", k, n), rectangle(x - 80.0, near, x + 80.0, near + width - 40.0), depth - lug, depth, 2, {fmt::format("joist_{}", k)}
            });
        }

    floor.add_box("concrete", {0.0, 0.0, depth}, {floor.span, floor.width, depth + concrete});

    return floor;
}

FloorProduct FloorProduct::composite_wave() {

    FloorProduct floor;
    floor.span = 2.0 * SAMPLE;
    floor.width = 1200.0;
    const double depth = 90.0;
    const double concrete = 100.0;
    floor.add_box("panel", {0.0, 0.0, 0.0}, {floor.span, floor.width, depth});

    // waves: recesses of one width along a curve across the floor, the concrete in them
    for (size_t w = 0; w < 4; w++) {
        FloorSection near;
        FloorSection far;

        for (size_t k = 0; k <= 24; k++) {
            const double y = floor.width * static_cast<double>(k) / 24.0;
            const double x = 300.0 + 600.0 * static_cast<double>(w) + 60.0 * std::sin(Tolerance::TWO_PI * y / 600.0);
            near.push_back({x - 40.0, y});
            far.push_back({x + 40.0, y});
        }

        FloorSection wave = near;
        wave.insert(wave.end(), far.rbegin(), far.rend());
        floor.parts.push_back({fmt::format("wave_{}", w), wave, depth - 25.0, depth, 2, {"panel"}});
    }

    floor.add_box("concrete", {0.0, 0.0, depth}, {floor.span, floor.width, depth + concrete});

    return floor;
}

FloorProduct FloorProduct::composite_crossed() {

    FloorProduct floor;
    floor.span = 2.0 * SAMPLE;
    floor.width = 1600.0;
    const double depth = 240.0;
    const double board = 27.0;
    const double concrete = 80.0;

    for (size_t k = 0; k < 2; k++)
        floor.add_along(fmt::format("joist_{}", k), rectangle(260.0 + 960.0 * k, 0.0, 380.0 + 960.0 * k, depth));

    floor.add_box("boards", {0.0, 0.0, depth}, {floor.span, floor.width, depth + board});
    floor.add_box("concrete", {0.0, 0.0, depth + board}, {floor.span, floor.width, depth + board + concrete});

    // pins: in pairs leaning against each other along the joists
    for (size_t k = 0; k < 2; k++) {
        FloorPins pins = {fmt::format("pins_{}", k), {}, 4.0, {fmt::format("joist_{}", k), "boards", "concrete"}};
        const double middle = 320.0 + 960.0 * static_cast<double>(k);

        for (size_t n = 0; n < 7; n++)
            for (const double way : {-1.0, 1.0}) {
                const double x = 150.0 + 350.0 * static_cast<double>(n);
                const double y = middle + way * 25.0;
                pins.axes.push_back({{{x - way * 75.0, y, depth - 90.0}, {x + way * 75.0, y, depth + board + 50.0}}});
            }

        floor.pins.push_back(pins);
    }

    return floor;
}

FloorProduct FloorProduct::composite_locks() {

    FloorProduct floor = laminated(false, 160.0, 40.0);
    const double depth = 160.0;
    std::vector<std::string> boards;

    for (const FloorPart& part : floor.parts)
        boards.push_back(part.name);

    // locks: flat steel on edge across the boards, half in them and half in the concrete
    for (size_t k = 0; k < 4; k++) {
        const double x = 300.0 + 600.0 * static_cast<double>(k) - 150.0;
        floor.parts.push_back({fmt::format("lock_{}", k), rectangle(x, depth - 40.0, x + 10.0, depth + 30.0), 0.0, floor.width, 1, boards});
    }

    floor.parts.push_back({"concrete", rectangle(0.0, depth, floor.span, depth + 100.0), 0.0, floor.width, 1, {}});

    for (size_t k = 0; k < 4; k++)
        floor.parts[boards.size() + k].pockets.push_back("concrete");

    return floor;
}

FloorProduct FloorProduct::composite_bars() {

    FloorProduct floor = composite_crossed();
    floor.pins.clear();

    // bars: V-shaped, glued into the joists, their tops in the concrete
    for (size_t k = 0; k < 2; k++) {
        FloorPins bars = {fmt::format("bars_{}", k), {}, 7.0, {fmt::format("joist_{}", k), "boards", "concrete"}};
        const double middle = 320.0 + 960.0 * static_cast<double>(k);

        for (size_t n = 0; n < 5; n++) {
            const double x = 250.0 + 450.0 * static_cast<double>(n);
            bars.axes.push_back({{{x - 90.0, middle, 120.0}, {x - 12.0, middle, 315.0}}});
            bars.axes.push_back({{{x + 90.0, middle, 120.0}, {x + 12.0, middle, 315.0}}});
        }

        floor.pins.push_back(bars);
    }

    return floor;
}

FloorProduct FloorProduct::composite_plates() {

    FloorProduct floor = cross_laminated({40.0, 30.0, 40.0, 30.0, 40.0});
    const double depth = 180.0;
    std::vector<std::string> layers;

    for (const FloorPart& part : floor.parts)
        layers.push_back(part.name);

    // plates: on edge along the span, half in the timber and half in the concrete
    for (size_t k = 0; k < 3; k++) {
        const double y = 300.0 + 600.0 * static_cast<double>(k);
        FloorPart& plate = floor.add_box(fmt::format("plate_{}", k), {0.0, y - 1.25, depth - 45.0}, {floor.span, y + 1.25, depth + 45.0});
        plate.pockets = {layers[3], layers[4], "concrete"};
    }

    floor.add_box("concrete", {0.0, 0.0, depth}, {floor.span, floor.width, depth + 100.0});

    return floor;
}

FloorProduct FloorProduct::composite_glued() {

    FloorProduct floor = cross_laminated({40.0, 30.0, 40.0, 30.0, 40.0});
    floor.add_box("concrete", {0.0, 0.0, 180.0}, {floor.span, floor.width, 260.0});

    return floor;
}

FloorProduct FloorProduct::composite_ribs() {

    FloorProduct floor;
    floor.span = 3000.0;
    floor.width = 2700.0;
    const double rib = 240.0;
    const double depth = 280.0;
    const double concrete = 100.0;
    const std::vector<double> places = {0.0, 0.5 * floor.width - rib, 0.5 * floor.width, floor.width - rib};

    // ribs: one at each edge and two in the middle, in the next element's edge rib their pair
    for (size_t k = 0; k < places.size(); k++)
        floor.add_along(fmt::format("rib_{}", k), rectangle(places[k], 0.0, places[k] + rib, depth));

    // notches: 250 x 200 x 40 in the ribs' tops, the concrete in them, two pins each
    for (size_t k = 0; k < places.size(); k++)
        for (size_t n = 0; n < 4; n++) {
            const double x = 300.0 + 800.0 * static_cast<double>(n);
            const double near = places[k] + 20.0;
            floor.parts.push_back({fmt::format("notch_{}_{}", k, n), rectangle(x - 125.0, near, x + 125.0, near + 200.0), depth - 40.0, depth, 2, {fmt::format("rib_{}", k)}});
            FloorPins pins = {fmt::format("pins_{}_{}", k, n), {}, 5.0, {fmt::format("rib_{}", k), fmt::format("notch_{}_{}", k, n), "concrete"}};

            for (const double way : {-1.0, 1.0})
                pins.axes.push_back({{{x + way * 60.0, near + 100.0, depth - 235.0}, {x + way * 60.0, near + 100.0, depth + 65.0}}});

            floor.pins.push_back(pins);
        }

    floor.add_box("concrete", {0.0, 0.0, depth}, {floor.span, floor.width, depth + concrete});

    return floor;
}

// ═══════════════════════════════════════════════════════════════════════════
// Session
// ═══════════════════════════════════════════════════════════════════════════
void add_floor_product(
    WoodSession& session, const std::string& name, const FloorProduct& product, const Plane& base, const std::shared_ptr<TreeNode>& group
) {

    const auto local = [&](double x, double y, double z) {
        return base.origin() + base.x_axis() * x + base.y_axis() * y + base.z_axis() * z;
    };

    // parts: each drawn at its start and swept along its run, concrete in a group of its own
    const std::shared_ptr<TreeNode> timber = session.group_named(name + "_timber", group);

    for (const FloorPart& part : product.parts) {
        const std::shared_ptr<TreeNode> place =
            part.name == "concrete" ? session.group_named(name + "_concrete", group) : timber;
        std::vector<Point> loop;

        for (const std::array<double, 2>& at : part.section)
            loop.push_back(
                part.run == 0   ? local(part.start, at[0], at[1])
                : part.run == 1 ? local(at[0], part.start, at[1])
                                : local(at[0], at[1], part.start)
            );

        const Vector way = part.run == 0 ? base.x_axis() : part.run == 1 ? base.y_axis() : base.z_axis();
        const Polyline section = Polyline(loop).closed();
        const Point foot = section.get_point(0);
        const Line axis = Line::from_points(foot, foot + way * (part.end - part.start));
        session.add(EdgeStraightElement::from_section(axis, section, fmt::format("{}_{}", name, part.name)), place);
    }

    // pockets: what is let into a part is cut out of it
    for (const FloorPart& part : product.parts) {
        if (part.pockets.empty())
            continue;

        const std::shared_ptr<EdgeStraightElement> inside = session.get_element_by_name<EdgeStraightElement>(fmt::format("{}_{}", name, part.name));

        for (const std::string& pocket : part.pockets) {
            const std::shared_ptr<EdgeStraightElement> host = session.get_element_by_name<EdgeStraightElement>(fmt::format("{}_{}", name, pocket));
            session.add_interaction(inside, host, std::make_shared<InteractionFeatureSolid>(inside->element_geometry_mesh(), SolidOperation::subtract));
        }
    }

    // pins: each bored into its parts
    for (const FloorPins& set : product.pins)
        for (size_t k = 0; k < set.axes.size(); k++) {
            const std::array<std::array<double, 3>, 2>& ends = set.axes[k];
            const Line axis = Line::from_points(
                local(ends[0][0], ends[0][1], ends[0][2]), local(ends[1][0], ends[1][1], ends[1][2])
            );
            const std::shared_ptr<JointPinElement> pin = std::make_shared<JointPinElement>(axis, set.radius, 0.2);
            pin->name = fmt::format("{}_{}_{}", name, set.name, k);
            std::vector<std::shared_ptr<EdgeStraightElement>> hosts;

            for (const std::string& into : set.into) {
                hosts.push_back(session.get_element_by_name<EdgeStraightElement>(fmt::format("{}_{}", name, into)));
                pin->targets.push_back(hosts.back()->guid());
            }

            session.add(pin, session.group_named(name + "_pins", group));

            for (const std::shared_ptr<EdgeStraightElement>& host : hosts)
                session.add_interaction(pin, host, std::make_shared<InteractionFeatureSolid>(std::vector<Line>{axis}, set.radius));
        }
}

FloorProducts::FloorProducts(
    const std::vector<std::pair<std::string, FloorProduct>>& products, const std::string& name, size_t count
)
    : WoodSession(name), products(products) {

    // products: in rows, each in its group
    double along = 0.0;
    double across = 0.0;
    double deepest = 0.0;

    for (size_t k = 0; k < products.size(); k++) {
        if (k > 0 && k % count == 0) {
            along += deepest + GAP;
            across = 0.0;
            deepest = 0.0;
        }

        across += products[k].second.width;
        const Plane base(Point(across, along, 0.0), Vector::y_axis(), Vector::x_axis() * -1.0);
        add_product(products[k].first, products[k].second, base);
        across += GAP;
        deepest = std::max(deepest, products[k].second.span);
    }

    // contacts: every two pieces that touch
    wood_module::add_module_contacts(*this);
}

void FloorProducts::add_product(const std::string& name, const FloorProduct& product, const Plane& base) {

    const std::shared_ptr<TreeNode> group = group_named(name);
    add_floor_product(*this, name, product, base, group);

    // assembly: the pieces in their order, pins along their axes last
    std::vector<AssemblyModule> pieces;

    for (const FloorPart& part : product.parts)
        pieces.push_back(AssemblyModule::part(get_element_by_name<Element>(fmt::format("{}_{}", name, part.name))));

    assemblies.push_back(AssemblyModule::module(name, pieces));
}

}
