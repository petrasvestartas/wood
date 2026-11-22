#include "pch.h"
#include "wood_joint_library.h"
#include "wood_config.h"
#include "yaml.hpp"
#include <sstream>
#include <fstream>
#include "wood_element_geometry.h"

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

}
