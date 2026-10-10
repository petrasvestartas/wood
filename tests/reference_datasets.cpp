#include "oracle_polyline.h"
#include <chrono>
#include <iomanip>

using namespace session_cpp;
using namespace wood_session;

// ═══════════════════════════════════════════════════════════════════════════
// The scoreboard: every dataset through the new solver, its merged plate outlines and its drill lines against the 2025 CGAL
// solver's (tests/golden/reference_2025/<name>.json, written by tools/reference_2025.py from the same data inputs)
// ═══════════════════════════════════════════════════════════════════════════

static const double MATCH_TOLERANCE = 1e-3; // mm, a reference outline is matched when every point has a twin this close
static const double DRILL_TOLERANCE = 2e-2; // mm, a reference drill line is matched when both ends have a twin this close: 2024 rounded the rings its drills stand on at two decimals in a frame of their own, so a coordinate may fall a hundredth either way
static const std::string GOLDEN_DIR = std::string(WOOD_SOURCE_DIR) + "/tests/golden/reference_2025";
static const std::string MATCHED_FILE = GOLDEN_DIR + "/matched.txt"; // The datasets that matched when the goldens were last accepted: a regression of one fails the run.

/// One dataset's line of the scoreboard.
struct Score {
    std::string dataset;
    size_t plates = 0;
    size_t matched_plates = 0;
    size_t drills = 0; // The dataset's drill lines, repeats dropped.
    bool matched = false;
    std::string deviation; // The first deviation, empty when matched.
    double worst = 0.0; // The largest distance of an outline or a drill line from its nearest reference twin, over the plates whose outline counts agree.
    double ms = 0.0;
};

// ═══════════════════════════════════════════════════════════════════════════
// Reference and comparison
// ═══════════════════════════════════════════════════════════════════════════

/// The loop as a shape: without its repeated and its forward-collinear vertices, which carry no geometry. 2024 kept the repeated points its merge made and the corners a run passes straight through; the merge drops them now, so both sides are compared as shapes.
static Polyline shape(Polyline loop) {

    loop.remove_consecutive_duplicates();
    loop.merge_collinear();
    return loop;
}

/// The reference outlines per plate from the json record, without the two-point drill lines 2024's merge wrote among them, which reference_drills compares.
static std::vector<std::vector<Polyline>> reference_outlines(const nlohmann::json& record) {

    std::vector<std::vector<Polyline>> plates;

    for (const nlohmann::json& plate : record.at("plates")) {
        std::vector<Polyline> outlines;
        for (const nlohmann::json& outline : plate.at("outlines")) {
            if (outline.size() == 2)
                continue;
            std::vector<Point> points;
            for (const nlohmann::json& p : outline)
                points.emplace_back(p[0].get<double>(), p[1].get<double>(), p[2].get<double>());
            outlines.push_back(shape(Polyline(points)));
        }
        plates.push_back(std::move(outlines));
    }

    return plates;
}

/// The reference outline the loop matches: equal point count and every point within MATCH_TOLERANCE of a twin; the closest miss in `nearest`.
static int match_outline(const Polyline& loop, const std::vector<Polyline>& reference, const std::vector<bool>& used, double& nearest) {

    const size_t count = oracle::open_point_count(loop);
    nearest = std::numeric_limits<double>::infinity();

    for (size_t i = 0; i < reference.size(); i++) {
        if (used[i] || oracle::open_point_count(reference[i]) != count)
            continue;
        const double distance = oracle::point_set_distance(loop, reference[i]);
        nearest = std::min(nearest, distance);
        if (distance <= MATCH_TOLERANCE)
            return static_cast<int>(i);
    }

    return -1;
}

/// Empty when the plate's merged outlines are the reference's; else the first deviation. `worst` grows to the largest distance of an outline from its nearest reference outline.
static std::string compare_plate(const Plate& plate, const std::vector<Polyline>& reference, double& worst) {

    std::vector<Polyline> loops = oracle::merged_loops(plate);
    for (Polyline& loop : loops)
        loop = shape(loop);

    if (loops.size() != reference.size())
        return fmt::format("{} outlines against {} in the reference ({} holes against {})", loops.size(), reference.size(), (loops.size() - 2) / 2, (reference.size() - 2) / 2);

    std::vector<bool> used(reference.size(), false);
    std::string first;
    for (size_t i = 0; i < loops.size(); i++) {
        double nearest = 0.0;
        const int found = match_outline(loops[i], reference, used, nearest);
        if (!std::isinf(nearest))
            worst = std::max(worst, nearest);
        if (found >= 0) {
            used[found] = true;
            continue;
        }
        if (!first.empty())
            continue;
        if (std::isinf(nearest))
            first = fmt::format("outline {} has {} points, no reference outline left with that count", i, oracle::open_point_count(loops[i]));
        else
            first = fmt::format("outline {} deviates {:.6g} mm from the nearest reference outline", i, nearest);
    }

    return first;
}

// ═══════════════════════════════════════════════════════════════════════════
// Drill lines: the pins of the top-top family, which never enter the merged outlines
// ═══════════════════════════════════════════════════════════════════════════

/// How far two drill lines are apart: the larger end distance of the closer of the two pairings, so a line matched either way round is at zero.
static double drill_distance(const Line& a, const Line& b) {

    const double forward = std::max(a.start().distance(b.start()), a.end().distance(b.end()));
    const double backward = std::max(a.start().distance(b.end()), a.end().distance(b.start()));
    return std::min(forward, backward);
}

/// The lines without their repeats: 2024 wrote every drill twice on each face of its side, and the port keeps that layout.
static std::vector<Line> unique_drills(const std::vector<Line>& lines) {

    std::vector<Line> unique;
    for (const Line& line : lines) {
        bool seen = false;
        for (const Line& kept : unique)
            if (drill_distance(line, kept) <= 1e-9)
                seen = true;
        if (!seen)
            unique.push_back(line);
    }

    return unique;
}

/// The reference drill lines of the dataset: the two-point joint polylines of output type 3 over every plate.
static std::vector<Line> reference_drills(const nlohmann::json& record) {

    std::vector<Line> lines;
    for (const nlohmann::json& plate : record.at("joints"))
        for (const nlohmann::json& polyline : plate)
            if (polyline.size() == 2)
                lines.push_back(Line::from_points(
                    Point(polyline[0][0].get<double>(), polyline[0][1].get<double>(), polyline[0][2].get<double>()),
                    Point(polyline[1][0].get<double>(), polyline[1][1].get<double>(), polyline[1][2].get<double>())
                ));

    return unique_drills(lines);
}

/// The port's drill lines of the dataset: the drill axes of every plate joint, the drill-typed two-point outlines as the builders wrote them.
static std::vector<Line> wood_drills(const WoodSession& scene) {

    std::vector<Line> lines;
    for (const std::shared_ptr<JointPlate>& joint : scene.get_elements<JointPlate>()) {
        const std::vector<Line> axes = joint->drill_axes();
        lines.insert(lines.end(), axes.begin(), axes.end());
    }

    return unique_drills(lines);
}

/// Empty when the dataset's drill lines are the reference's, taken as one set over every plate: the port bores each plate along the line through itself where 2024 wrote each plate the line through the other plate of its pair, so the lines agree over the pair and not plate by plate. Else the first deviation; `worst` grows to the largest distance of a drill line from its nearest reference twin.
static std::string compare_drills(const std::vector<Line>& drills, const std::vector<Line>& reference, double& worst) {

    if (drills.size() != reference.size())
        return fmt::format("{} drill lines against {} in the reference", drills.size(), reference.size());

    std::vector<bool> used(reference.size(), false);
    for (size_t i = 0; i < drills.size(); i++) {
        int found = -1;
        double nearest = std::numeric_limits<double>::infinity();
        for (size_t j = 0; j < reference.size() && found < 0; j++) {
            if (used[j])
                continue;
            const double distance = drill_distance(drills[i], reference[j]);
            nearest = std::min(nearest, distance);
            if (distance <= DRILL_TOLERANCE)
                found = static_cast<int>(j);
        }
        if (found < 0)
            return fmt::format("drill line {} deviates {:.6g} mm from the nearest reference drill line", i, nearest);
        used[found] = true;
        worst = std::max(worst, nearest);
    }

    return "";
}

/// One dataset solved as main_all_datasets solves it and scored against its reference.
static Score run(const std::string& name) {

    Score score;
    score.dataset = name;
    const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

    try {
        std::ifstream file(GOLDEN_DIR + "/" + name + ".json");
        const nlohmann::json record = nlohmann::json::parse(file);
        const std::vector<std::vector<Polyline>> reference = reference_outlines(record);

        config::reset_defaults();
        WoodSession scene = WoodSession::yaml_load(name);
        scene.compute_features();
        const std::vector<std::shared_ptr<Plate>> plates = scene.plates();
        score.plates = plates.size();

        if (plates.size() != reference.size()) {
            score.deviation = fmt::format("{} plates against {} in the reference", plates.size(), reference.size());
        } else {
            for (size_t i = 0; i < plates.size(); i++) {
                const std::string deviation = compare_plate(*plates[i], reference[i], score.worst);
                if (deviation.empty())
                    score.matched_plates++;
                else if (score.deviation.empty())
                    score.deviation = fmt::format("plate {}: {}", i, deviation);
            }

            // the drill lines of the whole dataset
            const std::vector<Line> drills = wood_drills(scene);
            score.drills = drills.size();
            const std::string deviation = compare_drills(drills, reference_drills(record), score.worst);
            if (!deviation.empty() && score.deviation.empty())
                score.deviation = deviation;
            score.matched = score.matched_plates == plates.size() && deviation.empty();
        }
    } catch (const std::exception& e) {
        score.deviation = std::string("throws: ") + e.what();
    }

    score.ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    return score;
}

// ═══════════════════════════════════════════════════════════════════════════
// The sweep
// ═══════════════════════════════════════════════════════════════════════════

/// Every dataset with a reference json, sorted.
static std::vector<std::string> dataset_names() {

    std::vector<std::string> names;
    for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(GOLDEN_DIR))
        if (entry.path().extension() == ".json")
            names.push_back(entry.path().stem().string());
    std::sort(names.begin(), names.end());

    return names;
}

/// The dataset names of the accepted matched list, one per line.
static std::set<std::string> accepted_matches() {

    std::set<std::string> names;
    std::ifstream file(MATCHED_FILE);
    std::string line;
    while (std::getline(file, line))
        if (!line.empty())
            names.insert(line);

    return names;
}

int main(int argc, char** argv) {

    std::vector<std::string> names;
    for (int i = 1; i < argc; i++)
        names.emplace_back(argv[i]);
    if (names.empty())
        names = dataset_names();

    std::vector<Score> scores;
    for (const std::string& name : names)
        scores.push_back(run(name));

    // the scoreboard
    std::cout << std::left << std::setw(36) << "dataset" << std::right << std::setw(7) << "plates" << std::setw(9) << "matched" << std::setw(7) << "drills" << std::setw(11) << "worst mm" << std::setw(7) << "ms" << "  status\n";
    size_t matched = 0;
    size_t thrown = 0;
    for (const Score& score : scores) {
        matched += score.matched;
        thrown += score.deviation.starts_with("throws:");
        std::cout << std::left << std::setw(36) << score.dataset << std::right << std::setw(7) << score.plates << std::setw(9) << score.matched_plates << std::setw(7) << score.drills
                  << std::setw(11) << std::fixed << std::setprecision(5) << score.worst << std::setw(7) << std::setprecision(0) << score.ms << "  " << (score.matched ? "match" : score.deviation) << "\n";
    }
    std::cout << fmt::format("\nreference 2025: {} / {} datasets match, {} throw\n", matched, scores.size(), thrown);

    // the ratchet: a dataset that matched when the goldens were accepted must still match
    const bool update = std::getenv("WOOD_UPDATE_GOLDEN") != nullptr;
    const std::set<std::string> accepted = accepted_matches();
    size_t regressed = 0;
    for (const Score& score : scores) {
        if (accepted.count(score.dataset) && !score.matched) {
            regressed++;
            std::cerr << fmt::format("REGRESSION {}: matched the reference before, now {}\n", score.dataset, score.deviation);
        }
        if (score.matched && !accepted.count(score.dataset))
            std::cout << fmt::format("new match {}: add it with WOOD_UPDATE_GOLDEN=1\n", score.dataset);
    }
    if (update) {
        std::ofstream file(MATCHED_FILE);
        for (const Score& score : scores)
            if (score.matched)
                file << score.dataset << "\n";
        std::cout << "matched list rewritten: " << MATCHED_FILE << "\n";
    }

    return thrown > 0 || regressed > 0 ? 1 : 0;
}
