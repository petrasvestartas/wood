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
            build_up.layers.push_back({layer["name"], layer["printed"], layer["kind"], layer["thickness"], layer["width"], layer["inside"], layer["bearing"]});

        if (table == 0 || build_up.table == table)
            found.push_back(build_up);
    }

    return found;
}

std::vector<FloorLayer> FloorBuildUp::structure() const {

    std::vector<FloorLayer> found;

    for (const FloorLayer& layer : layers)
        if (layer.bearing)
            found.push_back(layer);

    return found;
}

double FloorBuildUp::top() const {

    double thickness = 0.0;

    for (const FloorLayer& layer : layers) {
        if (layer.bearing)
            break;

        thickness += layer.inside ? 0.0 : layer.thickness;
    }

    return thickness;
}

FloorBuildUp FloorBuildUp::find(size_t table, size_t number) {

    for (const FloorBuildUp& build_up : library(table))
        if (build_up.number == number)
            return build_up;

    throw std::invalid_argument("no such build-up in the tables");
}

// ═══════════════════════════════════════════════════════════════════════════
// FloorBuildUps
// ═══════════════════════════════════════════════════════════════════════════
FloorBuildUps::FloorBuildUps(const std::vector<FloorBuildUp>& build_ups, const std::string& name, size_t count)
    : WoodSession(name), build_ups(build_ups) {

    // samples: in rows, each in its group
    for (size_t k = 0; k < build_ups.size(); k++) {
        const double across = static_cast<double>(k % count) * (WIDTH + GAP) + WIDTH;
        const double along = static_cast<double>(k / count) * (LENGTH + GAP);
        add_sample(build_ups[k], Plane(Point(across, along, 0.0), Vector::y_axis(), Vector::x_axis() * -1.0));
    }
}

FloorProduct FloorBuildUps::structure(const FloorLayer& layer) const {

    FloorProduct floor;
    floor.span = LENGTH;
    floor.width = WIDTH;
    const double depth = layer.thickness;

    // joists and ribs: over the width at 600
    if (layer.kind == "joists" || layer.kind == "ribs") {
        for (size_t k = 0; k < 3; k++) {
            const double near = (WIDTH - layer.width) * 0.5 * static_cast<double>(k);
            floor.add_along(fmt::format("joist_{}", k), {{near, 0.0}, {near + layer.width, 0.0}, {near + layer.width, depth}, {near, depth}});
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
        floor.add_box("lower", {0.0, 0.0, 0.0}, {LENGTH, WIDTH, shell});

        for (size_t k = 0; k < webs; k++) {
            const double near = (WIDTH - web) * static_cast<double>(k) / static_cast<double>(webs - 1);
            floor.add_along(fmt::format("web_{}", k), {{near, shell}, {near + web, shell}, {near + web, top}, {near, top}});
        }

        if (closed)
            floor.add_box("upper", {0.0, 0.0, top}, {LENGTH, WIDTH, depth});

        return floor;
    }

    // plates
    floor.add_box("plate", {0.0, 0.0, 0.0}, {LENGTH, WIDTH, depth});

    return floor;
}

void FloorBuildUps::add_sample(const FloorBuildUp& build_up, const Plane& base) {

    const std::string sample = fmt::format("floor_{}_{}", build_up.table, build_up.number);
    const std::shared_ptr<TreeNode> group = group_named(sample);
    const std::vector<FloorLayer> bearing = build_up.structure();

    // structure: from the lowest layer up, all that lies on it as one
    double level = 0.0;

    for (size_t k = bearing.size(); k-- > 0;) {
        FloorProduct floor = structure(bearing[k]);

        if (k == 0)
            floor = floor.layered(build_up.top());

        const Plane place(base.origin() + base.z_axis() * level, base.x_axis(), base.y_axis());
        add_floor_product(*this, fmt::format("{}_{}", sample, k), floor, place, group);
        level += bearing[k].thickness;
    }

    std::vector<AssemblyModule> pieces;

    for (const std::shared_ptr<Element>& piece : get_elements_under<Element>(sample))
        pieces.push_back(AssemblyModule::part(piece));

    assemblies.push_back(AssemblyModule::module(sample, pieces));
}

}
