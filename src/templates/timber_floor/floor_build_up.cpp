#include "src/templates/timber_floor/floor_build_up.h"
#include "wood_config.h"

namespace wood_timber_floor {

// ═══════════════════════════════════════════════════════════════════════════
// FloorBuildUp
// ═══════════════════════════════════════════════════════════════════════════
std::vector<FloorBuildUp> FloorBuildUp::library(size_t table) {

    const std::filesystem::path path = config::session_data_dir() / "timber_floors" / "build_ups.json";
    std::ifstream file(path);

    if (!file)
        throw std::runtime_error("no build-ups in " + path.string());

    std::vector<FloorBuildUp> found;

    for (const nlohmann::json& item : nlohmann::json::parse(file)) {
        FloorBuildUp build_up;
        build_up.table = item["table"];
        build_up.number = item["number"];
        build_up.title = item["title"];
        build_up.name = item["name"];
        build_up.weight = item["weight"];
        build_up.height = item["height"];
        build_up.airborne = item["airborne"];
        build_up.impact = item["impact"];
        build_up.energy = item["energy"];
        build_up.ghg = item["ghg"];

        for (const nlohmann::json& layer : item["layers"])
            build_up.layers.push_back({layer["name"], layer["printed"], layer["kind"], layer["thickness"], layer["width"], layer["inside"]});

        if (table == 0 || build_up.table == table)
            found.push_back(build_up);
    }

    return found;
}

// ═══════════════════════════════════════════════════════════════════════════
// FloorBuildUps
// ═══════════════════════════════════════════════════════════════════════════
FloorBuildUps::FloorBuildUps(const std::vector<FloorBuildUp>& build_ups, const std::string& name, size_t count)
    : WoodSession(name), build_ups(build_ups) {

    // samples: in rows, a table in its own, each in its group
    size_t row = 0;
    size_t place = 0;

    for (size_t k = 0; k < build_ups.size(); k++) {
        if (k > 0 && (place == count || build_ups[k].table != build_ups[k - 1].table)) {
            row++;
            place = 0;
        }

        add_sample(
            build_ups[k],
            Plane(
                Point(static_cast<double>(place) * (WIDTH + GAP) + WIDTH, static_cast<double>(row) * (LENGTH + GAP), 0.0),
                Vector::y_axis(),
                Vector::x_axis() * -1.0
            )
        );
        place++;
    }
}

FloorProduct FloorBuildUps::structure(const FloorLayer& layer, double length) const {

    FloorProduct floor;
    floor.span = length;
    floor.width = WIDTH;
    const double depth = layer.thickness;

    // joists, ribs and battens: over the width at 600, battens at 500 across them
    if (layer.kind == "joists" || layer.kind == "ribs") {
        for (size_t k = 0; k < 3; k++) {
            const double near = (WIDTH - layer.width) * 0.5 * static_cast<double>(k);
            floor.add_along(fmt::format("joist_{}", k), {{near, 0.0}, {near + layer.width, 0.0}, {near + layer.width, depth}, {near, depth}});
        }

        return floor;
    }

    if (layer.kind == "battens") {
        const double width = layer.width > 0.0 ? layer.width : 50.0;

        for (size_t k = 0; k < 3; k++) {
            const double start = (length - width) * 0.5 * static_cast<double>(k);
            floor.parts.push_back({fmt::format("batten_{}", k), {{start, 0.0}, {start + width, 0.0}, {start + width, depth}, {start, depth}}, 0.0, WIDTH, 1, {}});
        }

        return floor;
    }

    // boards on edge and planks: side by side
    if (layer.kind == "boards" || layer.kind == "planks") {
        const size_t count = layer.kind == "boards" ? 33 : 7;
        const double width = WIDTH / static_cast<double>(count);

        for (size_t k = 0; k < count; k++) {
            const double near = width * static_cast<double>(k);
            floor.add_along(fmt::format("board_{}", k), {{near, 0.0}, {near + width, 0.0}, {near + width, depth}, {near, depth}});
        }

        return floor;
    }

    // hollow ones: a shell below and above, webs between; cross ribs in a grid, no upper shell on spaced ribs
    if (layer.kind == "box" || layer.kind == "surface" || layer.kind == "grid" || layer.kind == "spaced") {
        const double shell = layer.kind == "grid" ? 27.0 : 31.0;
        const double web = layer.kind == "spaced" ? 80.0 : layer.kind == "grid" ? 30.0 : 31.0;
        const size_t webs = layer.kind == "box" ? 8 : 5;
        const bool closed = layer.kind != "spaced";
        const double top = closed ? depth - shell : depth;
        floor.add_box("lower", {0.0, 0.0, 0.0}, {length, WIDTH, shell});

        for (size_t k = 0; k < webs; k++) {
            const double near = (WIDTH - web) * static_cast<double>(k) / static_cast<double>(webs - 1);
            floor.add_along(fmt::format("web_{}", k), {{near, shell}, {near + web, shell}, {near + web, top}, {near, top}});
        }

        if (closed)
            floor.add_box("upper", {0.0, 0.0, top}, {length, WIDTH, depth});

        return floor;
    }

    // plates
    floor.add_box("plate", {0.0, 0.0, 0.0}, {length, WIDTH, depth});

    return floor;
}

void FloorBuildUps::add_sample(const FloorBuildUp& build_up, const Plane& base) {

    const std::string sample = fmt::format("floor_{}_{}", build_up.table, build_up.number);
    const std::shared_ptr<TreeNode> group = group_named(sample);
    const auto at = [&](double x, double z) {
        return Plane(base.origin() + base.x_axis() * x + base.z_axis() * z, base.x_axis(), base.y_axis());
    };

    // from the lowest layer up, each on the one under it and ending before it
    double level = 0.0;
    size_t placed = 0;
    std::vector<AssemblyModule> pieces;

    for (size_t k = build_up.layers.size(); k-- > 0;) {
        const FloorLayer& layer = build_up.layers[k];

        if (layer.kind == "membrane" || layer.inside)
            continue;

        if (layer.kind == "cavity") {
            level += layer.thickness;
            continue;
        }

        const double start = STEP * static_cast<double>(placed);
        const double length = LENGTH - start;
        const std::string name = fmt::format("{}_{}", sample, k);

        // skins: one under the ribs, one on them
        const bool ribs = layer.kind == "ribs" && k > 0 && build_up.layers[k - 1].kind == "skins";
        FloorProduct floor = structure(layer, length);

        if (ribs) {
            const double skin = build_up.layers[k - 1].thickness;
            FloorProduct lower;
            lower.span = length;
            lower.width = WIDTH;
            lower.add_box("lower", {0.0, 0.0, 0.0}, {length, WIDTH, skin});
            add_floor_product(*this, name, lower, at(start, level), group);
            level += skin;
        }

        add_floor_product(*this, name, floor, at(start, level), group);

        // inside: what lies between the joists or the battens of this layer, listed after it, the last the lowest
        size_t end = k + 1;

        while (end < build_up.layers.size() && build_up.layers[end].inside)
            end++;

        // a lining between joists is a third up them
        double between = level;

        for (size_t j = k + 1; j < end; j++)
            if (layer.kind == "joists" && build_up.layers[j].name == "Particleboard")
                between = level + 0.3 * layer.thickness;

        for (size_t j = end; j-- > k + 1;) {
            const FloorLayer& fill = build_up.layers[j];
            const bool joists = layer.kind == "joists" || layer.kind == "ribs";

            if (!joists && layer.kind != "battens")
                continue;

            FloorProduct filling;
            filling.span = length - STEP;
            filling.width = WIDTH;
            const double low = between - level;
            const double high = low + fill.thickness;

            for (size_t b = 0; b < 2; b++) {
                const double field = joists ? WIDTH : length;
                const double near = (field - layer.width) * 0.5 * static_cast<double>(b) + layer.width;
                const double far = (field - layer.width) * 0.5 * static_cast<double>(b + 1);
                const FloorSection section = {{near, low}, {far, low}, {far, high}, {near, high}};
                const std::string part = fmt::format("fill_{}_{}", j, b);

                if (joists)
                    filling.add_along(part, section);
                else
                    filling.parts.push_back({part, section, STEP, WIDTH, 1, {}});
            }

            add_floor_product(*this, name, filling, at(start + (joists ? STEP : 0.0), level), group);
            between += fill.thickness;
        }

        level += layer.thickness;
        placed++;
    }

    for (const std::shared_ptr<Element>& piece : get_elements_under<Element>(sample))
        pieces.push_back(AssemblyModule::part(piece));

    assemblies.push_back(AssemblyModule::module(sample, pieces));
}

}
