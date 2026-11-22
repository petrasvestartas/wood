#include "pch.h"
#include "wood_joint_library.h"
#include "wood_config.h"
#include "yaml.hpp"
#include <sstream>
#include <fstream>
#include "wood_element_geometry.h"
#include "wood_element_joint_beam.h"

namespace wood_session {

namespace {

/// A recursive-descent reading of one expression, left to right.
struct Reader {
    const std::string& text;
    const std::map<std::string, LibraryValue>& names;
    size_t at = 0;

    /// The next character past spaces, 0 at the end.
    char peek() {

        while (at < text.size() && text[at] == ' ')
            at++;

        return at < text.size() ? text[at] : '\0';
    }

    /// Throws naming the expression.
    [[noreturn]] void fail(const std::string& what) const {
        throw std::invalid_argument(fmt::format("joint library: {} in \"{}\"", what, text));
    }

    /// A sum of terms.
    LibraryValue sum() {

        LibraryValue value = term();
        while (peek() == '+' || peek() == '-') {
            const char sign = text[at++];
            const LibraryValue right = term();
            if (value.is_vector != right.is_vector)
                fail("a number added to a vector");
            if (value.is_vector)
                value.vector = sign == '+' ? value.vector + right.vector : value.vector - right.vector;
            else
                value.number = sign == '+' ? value.number + right.number : value.number - right.number;
        }

        return value;
    }

    /// A product of factors: a vector times or over a number.
    LibraryValue term() {

        LibraryValue value = factor();
        while (peek() == '*' || peek() == '/') {
            const char sign = text[at++];
            const LibraryValue right = factor();
            if (value.is_vector && right.is_vector)
                fail("two vectors multiplied");
            const double scale = sign == '*' ? (value.is_vector ? right.number : value.number) : 1.0 / right.number;
            if (value.is_vector || right.is_vector)
                value = {0.0, (value.is_vector ? value.vector : right.vector) * scale, true};
            else
                value.number = sign == '*' ? value.number * right.number : value.number / right.number;
        }

        return value;
    }

    /// A number, a name, a call, a negation or a bracket.
    LibraryValue factor() {

        const char c = peek();
        if (c == '-') {
            at++;
            LibraryValue value = factor();
            value.number = -value.number;
            value.vector = value.vector * -1.0;
            return value;
        }

        if (c == '(') {
            at++;
            const LibraryValue value = sum();
            if (peek() != ')')
                fail("a bracket left open");
            at++;
            return value;
        }

        if (std::isdigit(static_cast<unsigned char>(c)) || c == '.') {
            size_t used = 0;
            const double number = std::stod(text.substr(at), &used);
            at += used;
            return {number, Vector(), false};
        }

        const size_t start = at;
        while (at < text.size() && (std::isalnum(static_cast<unsigned char>(text[at])) || text[at] == '_'))
            at++;
        const std::string name = text.substr(start, at - start);
        if (name.empty())
            fail(fmt::format("an unexpected '{}'", c));
        if (peek() == '(')
            return call(name);

        const std::map<std::string, LibraryValue>::const_iterator found = names.find(name);
        if (found == names.end())
            fail(fmt::format("the unknown name {}", name));

        return found->second;
    }

    /// unit, length, cross, dot or meet of its arguments.
    LibraryValue call(const std::string& name) {

        std::vector<LibraryValue> arguments;
        at++;
        while (peek() != ')') {
            arguments.push_back(sum());
            if (peek() == ',')
                at++;
        }
        at++;

        if (name == "unit" && arguments.size() == 1)
            return {0.0, arguments[0].vector.normalized(), true};
        if (name == "length" && arguments.size() == 1)
            return {arguments[0].vector.magnitude(), Vector(), false};
        if (name == "cross" && arguments.size() == 2)
            return {0.0, arguments[0].vector.cross(arguments[1].vector), true};
        if (name == "dot" && arguments.size() == 2)
            return {arguments[0].vector.dot(arguments[1].vector), Vector(), false};
        if (name == "meet" && arguments.size() == 4) {
            const double along = (arguments[2].vector - arguments[0].vector).dot(arguments[3].vector) / arguments[1].vector.dot(arguments[3].vector);
            return {0.0, arguments[0].vector + arguments[1].vector * along, true};
        }

        fail(fmt::format("{} with {} arguments", name, arguments.size()));
    }
};

/// A beam along axis, square of half side thick when wide is the same, else a board thick across up and wide across both; a
/// rolled beam keeps up even when square.
std::shared_ptr<EdgeStraightElement> stock(const Polyline& axis, const Vector& up, double thick, double wide, const std::string& name, bool rolled = false) {

    if (std::abs(thick - wide) < 1e-9 && !rolled)
        return EdgeStraightElement::square(axis, thick, name);

    const Point start = axis.get_point(0);
    const Vector along = (axis.get_point(1) - start).normalized();

    return EdgeStraightElement::from_section(Line::from_points(start, axis.get_point(1)), rectangle_section(start, along, up, thick, wide), name);
}

/// The text without the quotes and spaces around it.
std::string unquoted(std::string text) {

    text.erase(0, text.find_first_not_of(" \"'"));
    text.erase(text.find_last_not_of(" \"'") + 1);

    return text;
}

}

LibraryValue evaluate(const std::string& expression, const std::map<std::string, LibraryValue>& names) {

    Reader reader{expression, names};
    const LibraryValue value = reader.sum();
    if (reader.peek() != '\0')
        reader.fail("text left over");

    return value;
}

/// Every data/joints/*.yaml, sorted, or the one of chapter.
static std::vector<std::filesystem::path> library_files(const std::string& chapter) {

    const std::filesystem::path folder = config::session_data_dir() / "joints";
    std::vector<std::filesystem::path> files;
    for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(folder))
        if (entry.path().extension() == ".yaml" && (chapter.empty() || entry.path().stem() == chapter))
            files.push_back(entry.path());
    std::sort(files.begin(), files.end());

    return files;
}

std::vector<std::string> library_designs(const std::string& chapter) {

    std::vector<std::string> designs;
    for (const std::filesystem::path& file : library_files(chapter)) {
        std::ifstream stream(file);
        std::string line;
        while (std::getline(stream, line))
            if (!line.empty() && std::islower(static_cast<unsigned char>(line[0])) && line.back() == ':')
                designs.push_back(line.substr(0, line.size() - 1));
    }

    return designs;
}

LibraryRecord library_record(const std::string& design) {

    const std::filesystem::path folder = config::session_data_dir() / "joints";
    for (const std::filesystem::path& file : library_files("")) {
        TINY_YAML::Yaml yaml(file.string());
        if (!yaml.has(design))
            continue;

        LibraryRecord record;
        for (const std::string key : {"title", "german", "page", "category", "family", "corners"})
            record.fields[key] = unquoted(yaml[design][key].getData<std::string>());
        for (const std::string key : {"parameters", "points", "planes", "parts", "steps", "insertions"}) {
            TINY_YAML::Node& list = yaml[design][key];
            if (list.hasData())
                for (const std::string& item : list.getData<std::vector<std::string>>())
                    record.lists[key].push_back(unquoted(item));
        }

        // the keys only some records have, read from the record's own lines: filled (the members whose cuts the others fill,
        // b by default) and more (members after b)
        std::ifstream text(file);
        std::string line;
        bool inside = false;
        bool in_more = false;
        while (std::getline(text, line)) {
            if (!line.empty() && line[0] != ' ' && line[0] != '#') {
                inside = line == design + ":";
                in_more = false;
                continue;
            }
            if (!inside)
                continue;
            if (line.starts_with("  filled:"))
                record.fields["filled"] = unquoted(line.substr(9));
            if (line.starts_with("  sealed:"))
                record.fields["sealed"] = unquoted(line.substr(9));
            if (line.starts_with("  moves:"))
                record.fields["moves"] = unquoted(line.substr(8));
            if (line.starts_with("  base:"))
                record.fields["base"] = unquoted(line.substr(7));
            if (line.starts_with("  ") && !line.starts_with("    "))
                in_more = line.starts_with("  more:");
            else if (in_more && line.starts_with("    - "))
                record.lists["more"].push_back(unquoted(line.substr(6)));
        }

        return record;
    }

    throw std::invalid_argument(fmt::format("joint library: no record {} in {}", design, folder.string()));
}

std::array<std::shared_ptr<EdgeStraightElement>, 2> library_pair(const std::string& design, bool square, const Point& at) {

    const LibraryRecord record = library_record(design);
    std::map<std::string, double> numbers = {{"sit", 0.0}, {"shift", 0.0}, {"tail", 150.0}, {"pitch", 35.0}, {"reach", 150.0}, {"span", 150.0}, {"lap", 2.0}, {"stock_thick", 60.0}, {"stock_wide", 60.0}, {"beam_length", 300.0}};
    for (const std::string& line : record.lists.count("parameters") ? record.lists.at("parameters") : std::vector<std::string>()) {
        const size_t equals = line.find('=');
        numbers[unquoted(line.substr(0, equals))] = std::stod(line.substr(equals + 1));
    }

    const bool crossing = record.fields.at("category") == "cross";
    // a board keeps the section it is made with; a square beam is turned by its directions
    const bool square_stock = std::abs(numbers["stock_thick"] - numbers["stock_wide"]) < 1e-9;
    const double half = 60.0;
    const double turn = (crossing ? numbers["pitch"] : square ? 90.0 : 130.0) * Tolerance::PI / 180.0;
    const Vector along(std::cos(turn), 0.0, std::sin(turn));
    std::shared_ptr<EdgeStraightElement> carrier;
    std::shared_ptr<EdgeStraightElement> member;

    // a splice: both along x on one line, overlapping by lap section heights, the member on the +x side
    if (record.fields.at("family") == "splicing") {
        const double overlap = numbers["lap"] * 2.0 * numbers["stock_thick"];
        const double long_enough = std::max(numbers["beam_length"], overlap + 60.0);
        member = stock(Polyline({at - Vector(0.5 * overlap, 0.0, 0.0), at + Vector(long_enough - 0.5 * overlap, 0.0, 0.0)}), Vector::z_axis(), numbers["stock_thick"], numbers["stock_wide"], design + "_member");
        carrier = stock(Polyline({at - Vector(long_enough - 0.5 * overlap, 0.0, 0.0), at + Vector(0.5 * overlap, 0.0, 0.0)}), Vector::z_axis(), numbers["stock_thick"], numbers["stock_wide"], design + "_carrier");
        if (square_stock)
            member->directions = {Vector::z_axis()};
        if (square_stock)
            carrier->directions = {Vector::z_axis()};
        return {member, carrier};
    }

    // a cross: the carrier along y, the member pitched over it, its underside through the point the record sets; carrier_thick and
    // carrier_wide make the carrier a board lying flat, stock_thick and stock_wide the member a batten standing on it
    if (crossing) {
        const double carrier_thick = numbers.count("carrier_thick") ? numbers["carrier_thick"] : half;
        const double carrier_wide = numbers.count("carrier_wide") ? numbers["carrier_wide"] : half;
        const double member_thick = numbers["stock_thick"];
        // roll turns both sections about their own axes (a diagonal puzzle's sticks stand on an edge) and keeps them from being turned
        const double roll = (numbers.count("roll") ? numbers["roll"] : 0.0) * Tolerance::PI / 180.0;
        carrier = stock(Polyline({at - Vector(0.0, numbers["beam_length"] - numbers["reach"], 0.0), at + Vector(0.0, numbers["reach"], 0.0)}), Vector(std::sin(roll), 0.0, std::cos(roll)), carrier_thick, carrier_wide, design + "_carrier", roll != 0.0);
        if (std::abs(carrier_thick - carrier_wide) < 1e-9 && roll == 0.0)
            carrier->directions = {Vector::z_axis()};
        // plan turns the member about z from square across the carrier (90 degrees) to the crossing angle it names
        const double plan = (numbers.count("plan") ? numbers["plan"] : 90.0) * Tolerance::PI / 180.0;
        const Vector downhill(-std::sin(plan), -std::cos(plan), 0.0);
        const Vector up(-std::sin(turn) * std::sin(plan), -std::sin(turn) * std::cos(plan), std::cos(turn));
        const Vector slope(std::cos(turn) * std::sin(plan), std::cos(turn) * std::cos(plan), std::sin(turn));
        const Point underside = at + Vector(0.0, 0.0, carrier_thick) + downhill * (numbers["shift"] * carrier_thick) - Vector(0.0, 0.0, numbers["sit"] * 2.0 * carrier_thick);
        const Point on_axis = underside + up * member_thick;
        member = stock(Polyline({on_axis - slope * numbers["tail"], on_axis + slope * std::max(numbers["beam_length"] - numbers["tail"], 0.5 * numbers["beam_length"])}), up * std::cos(roll) + slope.cross(up) * std::sin(roll), member_thick, numbers["stock_wide"], design + "_member", roll != 0.0);
        if (square_stock && roll == 0.0)
            member->directions = {Vector(-std::cos(plan), std::sin(plan), 0.0)};
        return {member, carrier};
    }

    // an end on a side: the carrier along x, the member ending on its axis, its stock running past the end; at a corner the
    // carrier ends flush with the member's outer face
    // carrier_thick and carrier_wide give the carrier its own section (a batten); flat lays both in the plane of the joint, a board
    // ending on a batten along its end, the thickness of both across that plane
    const double carrier_thick = numbers.count("carrier_thick") ? numbers["carrier_thick"] : numbers["stock_thick"];
    const double carrier_wide = numbers.count("carrier_wide") ? numbers["carrier_wide"] : numbers["stock_wide"];
    const bool flat = numbers.count("flat") && numbers["flat"] > 0.5;
    const bool corner = record.fields.at("category") == "end_end_inplane";
    // carrier_end runs the carrier past the node by its own length (a board through a post to its far face)
    const double carrier_end = numbers.count("carrier_end") ? numbers["carrier_end"] : corner ? numbers["stock_thick"] : numbers["span"];
    // a corner carrier runs the whole length back from the corner unless the record sets its span
    bool spanned = false;
    for (const std::string& line : record.lists.count("parameters") ? record.lists.at("parameters") : std::vector<std::string>())
        spanned = spanned || unquoted(line.substr(0, line.find('='))) == "span";
    const double back = corner && !spanned ? numbers["beam_length"] - carrier_end : numbers["span"];
    const Vector across(-std::sin(turn), 0.0, std::cos(turn));
    carrier = stock(Polyline({at - Vector(back, 0.0, 0.0), at + Vector(carrier_end, 0.0, 0.0)}), flat ? Vector::y_axis() : Vector::z_axis(), carrier_thick, carrier_wide, design + "_carrier");
    // the member shows beam_length: at a corner from the carrier's outer face, on a side from the carrier's face out
    const double inside = (flat ? carrier_wide : carrier_thick) / std::max(std::abs(std::sin(turn)), 0.2);
    // the stock's extension (180 mm, so a lap can pass through the carrier) also runs past the far end: the axis is shorter by it
    const double extension = 180.0;
    const double run = (corner ? numbers["beam_length"] - inside : numbers["beam_length"] + inside) - extension;
    member = stock(Polyline({at, at + along * run}), flat ? Vector::y_axis() : across, numbers["stock_thick"], numbers["stock_wide"], design + "_member");
    if (std::abs(carrier_thick - carrier_wide) < 1e-9)
        carrier->directions = {Vector::y_axis()};
    if (square_stock)
        member->directions = {Vector::y_axis()};
    member->extension = extension;

    return {member, carrier};
}

std::vector<std::shared_ptr<EdgeStraightElement>> library_members(const std::string& design, bool square, const Point& at) {

    const std::array<std::shared_ptr<EdgeStraightElement>, 2> pair = library_pair(design, square, at);
    std::vector<std::shared_ptr<EdgeStraightElement>> members = {pair[0], pair[1]};
    const LibraryRecord record = library_record(design);
    double length = 300.0;
    bool upright = false;
    double member_turn = 0.0;
    double member_shift = 0.0;
    for (const std::string& line : record.lists.count("parameters") ? record.lists.at("parameters") : std::vector<std::string>()) {
        if (unquoted(line.substr(0, line.find('='))) == "beam_length")
            length = std::stod(line.substr(line.find('=') + 1));
        if (unquoted(line.substr(0, line.find('='))) == "upright")
            upright = std::stod(line.substr(line.find('=') + 1)) > 0.5;
        if (unquoted(line.substr(0, line.find('='))) == "member_turn")
            member_turn = std::stod(line.substr(line.find('=') + 1));
        if (unquoted(line.substr(0, line.find('='))) == "member_shift")
            member_shift = std::stod(line.substr(line.find('=') + 1));
    }

    // member_shift moves the member alone across the joint (along y, the w of a corner or T), in mm: a post wider than its beam,
    // flush with one face
    if (member_shift != 0.0)
        members[0] = members[0]->transformed(Xform::translation(0.0, member_shift, 0.0));

    // member_turn turns the member alone about its own axis, in degrees (a post set diagonally under a square beam)
    if (member_turn != 0.0) {
        const Line axis = Line::from_points(members[0]->axis.get_point(0), members[0]->axis.get_point(1));
        members[0] = members[0]->transformed(Xform::rotation_around_line(axis, member_turn, true));
    }
    for (const std::string& line : record.lists.count("more") ? record.lists.at("more") : std::vector<std::string>()) {
        const size_t equals = line.find('=');
        const std::string letter = unquoted(line.substr(0, equals));
        std::vector<double> numbers;
        std::istringstream values(line.substr(equals + 1));
        std::string value;
        while (std::getline(values, value, ','))
            for (std::istringstream words(value); words >> value;)
                numbers.push_back(std::stod(value));
        if (numbers.size() != 11)
            throw std::invalid_argument(fmt::format("{}: more {} needs start, end and up as three numbers each and two halves", design, letter));
        Point start = at + Vector(numbers[0], numbers[1], numbers[2]);
        Point end = at + Vector(numbers[3], numbers[4], numbers[5]);
        const Vector up(numbers[6], numbers[7], numbers[8]);
        // every beam as long as the record's length: one ending near the joint keeps that end and shows length past the joint's
        // section (60 mm), one passing it is centred on its middle
        const Vector along = (end - start).normalized();
        const double whole = Point::distance(start, end);
        const double node = (at - start).dot(along);
        if (whole > length && node < 0.75 * length)
            end = start + along * std::min(node + 60.0 + length, whole);
        else if (whole > length && whole - node < 0.75 * length)
            start = end - along * std::min(whole - node + 60.0 + length, whole);
        else if (whole > length) {
            const Point middle = start + along * (0.5 * whole);
            start = middle - along * (0.5 * length);
            end = middle + along * (0.5 * length);
        }
        members.push_back(EdgeStraightElement::from_section(Line::from_points(start, end), rectangle_section(start, (end - start).normalized(), up, numbers[9], numbers[10]), design + "_" + letter));
    }

    // upright stands the joint up a quarter turn about x, its more members along y then vertical (a post the beams go into)
    if (upright) {
        const Xform stand = Xform::rotation_around_line(Line::from_points(at, at + Vector::x_axis()), 90.0, true);
        for (std::shared_ptr<EdgeStraightElement>& each : members)
            each = each->transformed(stand);
    }

    return members;
}


/// Whether left turns less than right about a face's centre.
static bool by_angle(const std::pair<double, Point>& left, const std::pair<double, Point>& right) {
    return left.first < right.first;
}

/// The two faces of a convex piece across which it is a prism: its vertices on the lowest and highest plane along one of x, y, z
/// or a face normal, each face's points in turn about its centre, the shorter repeated to match the longer; empty when none.
static std::array<Polyline, 2> prism_faces(const Mesh& piece) {

    const std::pair<std::vector<Point>, std::vector<std::vector<size_t>>> data = piece.to_vertices_and_faces();
    std::vector<Vector> directions = {Vector::x_axis(), Vector::y_axis(), Vector::z_axis()};
    for (const size_t face : piece.faces())
        if (piece.face_normal(face))
            directions.push_back(piece.face_normal(face)->normalized());

    for (const Vector& direction : directions) {
        double low = 1e18;
        double high = -1e18;
        for (const Point& point : data.first) {
            low = std::min(low, Vector(point[0], point[1], point[2]).dot(direction));
            high = std::max(high, Vector(point[0], point[1], point[2]).dot(direction));
        }
        if (high - low < 1e-3)
            continue;
        std::array<std::vector<Point>, 2> faces;
        bool prism = true;
        for (const Point& point : data.first) {
            const double along = Vector(point[0], point[1], point[2]).dot(direction);
            if (std::abs(along - low) < 1e-3)
                faces[0].push_back(point);
            else if (std::abs(along - high) < 1e-3)
                faces[1].push_back(point);
            else
                prism = false;
        }
        if (!prism || faces[0].size() < 3 || faces[1].size() < 3)
            continue;

        // each face's points in turn about the face's centre, in one sense seen along direction
        const Vector side = (std::abs(direction[0]) < 0.9 ? Vector::x_axis() : Vector::y_axis()).cross(direction).normalized();
        const Vector other = direction.cross(side);
        std::array<std::vector<std::pair<double, Point>>, 2> turned;
        for (size_t k = 0; k < 2; k++) {
            Vector sum(0.0, 0.0, 0.0);
            for (const Point& point : faces[k])
                sum = sum + Vector(point[0], point[1], point[2]);
            const Vector centre = sum / static_cast<double>(faces[k].size());
            for (const Point& point : faces[k]) {
                const Vector arm = Vector(point[0], point[1], point[2]) - centre;
                turned[k].push_back({std::atan2(arm.dot(other), arm.dot(side)), point});
            }
            std::sort(turned[k].begin(), turned[k].end(), by_angle);
        }

        // the shorter face's points repeated, each where its angle is nearest, so both outlines have as many points
        const size_t longer = turned[0].size() >= turned[1].size() ? 0 : 1;
        std::array<std::vector<Point>, 2> outlines;
        for (const std::pair<double, Point>& entry : turned[longer]) {
            outlines[longer].push_back(entry.second);
            double best = 1e18;
            Point nearest = turned[1 - longer].front().second;
            for (const std::pair<double, Point>& candidate : turned[1 - longer]) {
                const double gap = std::abs(std::remainder(candidate.first - entry.first, 2.0 * Tolerance::PI));
                if (gap < best) {
                    best = gap;
                    nearest = candidate.second;
                }
            }
            outlines[1 - longer].push_back(nearest);
        }
        for (std::vector<Point>& outline : outlines)
            outline.push_back(outline.front());
        return {Polyline(outlines[0]), Polyline(outlines[1])};
    }

    return {};
}

/// How often one unit repeats in pairs: every pair the same outlines moved along axis, evenly spaced, at least three (two are
/// the cheeks either side of one tenon, a fixed joint); 1 when the pairs are no such row. axis is set to the axis of the row.
static int repeats(const std::vector<std::array<Polyline, 2>>& pairs, int& axis) {

    axis = -1;
    if (pairs.size() < 3)
        return 1;
    for (int k = 0; k < 3; k++) {
        std::vector<double> places;
        bool row = true;
        const std::vector<Point> first = pairs.front()[0].get_points();
        for (const std::array<Polyline, 2>& pair : pairs) {
            for (size_t side = 0; side < 2 && row; side++) {
                const std::vector<Point> points = pair[side].get_points();
                const std::vector<Point> reference = pairs.front()[side].get_points();
                if (points.size() != reference.size()) {
                    row = false;
                    break;
                }
                const double shift = points.front()[static_cast<size_t>(k)] - reference.front()[static_cast<size_t>(k)];
                for (size_t i = 0; i < points.size() && row; i++)
                    for (size_t j = 0; j < 3 && row; j++)
                        row = std::abs(points[i][j] - reference[i][j] - (static_cast<int>(j) == k ? shift : 0.0)) < 0.5;
            }
            if (!row)
                break;
            places.push_back(pair[0].get_points().front()[static_cast<size_t>(k)] - first.front()[static_cast<size_t>(k)]);
        }
        if (!row)
            continue;
        std::sort(places.begin(), places.end());
        bool even = places.back() - places.front() > 1.0;
        for (size_t i = 2; i < places.size() && even; i++)
            even = std::abs((places[i] - places[i - 1]) - (places[1] - places[0])) < 0.5;
        if (even) {
            axis = k;
            return static_cast<int>(pairs.size());
        }
    }

    return 1;
}

LibraryTile library_tile(const std::string& design) {

    // the joint on its members at the origin, on a square corner when the record has one
    const bool square = library_record(design).fields.at("corners").find("square") != std::string::npos;
    const std::vector<std::shared_ptr<EdgeStraightElement>> members = library_members(design, square, Point(0.0, 0.0, 0.0));
    std::vector<const EdgeStraightElement*> more;
    for (size_t m = 2; m < members.size(); m++)
        more.push_back(members[m].get());
    const std::shared_ptr<JointBeamElement> joint = JointBeamElement::from_library(design, *members[0], *members[1], {}, 2.0, more);

    // the joint's zone: the box of where the uncut members overlap, so trimming a stock's extension is no part of the tile
    Point zone_low(1e18, 1e18, 1e18);
    Point zone_high(-1e18, -1e18, -1e18);
    for (size_t i = 0; i < members.size(); i++)
        for (size_t j = i + 1; j < members.size(); j++) {
            const Mesh overlap = solid_boolean(members[i]->element_geometry_mesh(), members[j]->element_geometry_mesh(), SolidOperation::intersect);
            if (overlap.number_of_faces() == 0)
                continue;
            for (const Point& point : overlap.to_vertices_and_faces().first)
                for (size_t k = 0; k < 3; k++) {
                    zone_low[k] = std::min(zone_low[k], point[k] - 1.0);
                    zone_high[k] = std::max(zone_high[k], point[k] + 1.0);
                }
        }
    Mesh zone = Mesh::create_box(zone_high[0] - zone_low[0], zone_high[1] - zone_low[1], zone_high[2] - zone_low[2]);
    zone = zone.transformed(Xform::translation((zone_low[0] + zone_high[0]) / 2.0, (zone_low[1] + zone_high[1]) / 2.0, (zone_low[2] + zone_high[2]) / 2.0));

    // every cut: each step's wood on its member within the zone, piece by piece, as the two faces of its prism
    LibraryTile tile;
    tile.cuts.resize(members.size());
    for (const JointCutStep& step : joint->steps) {
        if (step.target >= members.size() || step.removal.number_of_faces() == 0)
            continue;
        for (const Mesh& region : solid_pieces(step.removal)) {
            const Mesh wood = solid_boolean(solid_boolean(region, members[step.target]->element_geometry_mesh(), SolidOperation::intersect), zone, SolidOperation::intersect);
            if (wood.number_of_faces() == 0)
                continue;
            for (const Mesh& piece : solid_pieces(wood)) {
                const std::array<Polyline, 2> faces = prism_faces(piece);
                if (faces[0].point_count() == 0)
                    tile.unsupported++;
                else
                    tile.cuts[step.target].push_back(faces);
            }
        }
    }
    tile.parts = joint->parts;

    // a member whose cuts are one unit in an even row repeats by divisions, measured before the unit box (in mm)
    for (const std::vector<std::array<Polyline, 2>>& member_cuts : tile.cuts) {
        int axis = -1;
        tile.divisions.push_back(repeats(member_cuts, axis));
        tile.axes.push_back(axis);
    }

    // the unit box: every outline point's box, mapped to [-0.5, 0.5]
    Point low(1e18, 1e18, 1e18);
    Point high(-1e18, -1e18, -1e18);
    std::vector<std::array<Polyline, 2>*> pairs;
    for (std::vector<std::array<Polyline, 2>>& member_cuts : tile.cuts)
        for (std::array<Polyline, 2>& pair : member_cuts)
            pairs.push_back(&pair);
    for (std::array<Polyline, 2>& pair : tile.parts)
        pairs.push_back(&pair);
    for (std::array<Polyline, 2>* pair : pairs)
        for (const Polyline& outline : *pair)
            for (const Point& point : outline.get_points())
                for (size_t k = 0; k < 3; k++) {
                    low[k] = std::min(low[k], point[k]);
                    high[k] = std::max(high[k], point[k]);
                }
    tile.centre = Point((low[0] + high[0]) / 2.0, (low[1] + high[1]) / 2.0, (low[2] + high[2]) / 2.0);
    tile.size = Vector(std::max(high[0] - low[0], 1e-6), std::max(high[1] - low[1], 1e-6), std::max(high[2] - low[2], 1e-6));
    for (std::array<Polyline, 2>* pair : pairs)
        for (Polyline& outline : *pair) {
            std::vector<Point> unit;
            for (const Point& point : outline.get_points())
                unit.emplace_back((point[0] - tile.centre[0]) / tile.size[0], (point[1] - tile.centre[1]) / tile.size[1], (point[2] - tile.centre[2]) / tile.size[2]);
            outline = Polyline(unit);
        }

    return tile;
}

}
