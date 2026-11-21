#include "pch.h"
#include "wood_joint_library.h"
#include "wood_config.h"
#include "yaml.hpp"
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

/// A beam along axis, square of half side thick when wide is the same, else a board thick across up and wide across both.
std::shared_ptr<EdgeStraightElement> stock(const Polyline& axis, const Vector& up, double thick, double wide, const std::string& name) {

    if (std::abs(thick - wide) < 1e-9)
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

        return record;
    }

    throw std::invalid_argument(fmt::format("joint library: no record {} in {}", design, folder.string()));
}

std::array<std::shared_ptr<EdgeStraightElement>, 2> library_pair(const std::string& design, bool square, const Point& at) {

    const LibraryRecord record = library_record(design);
    std::map<std::string, double> numbers = {{"sit", 0.0}, {"shift", 0.0}, {"tail", 600.0}, {"pitch", 35.0}, {"reach", 450.0}, {"lap", 2.0}, {"stock_thick", 60.0}, {"stock_wide", 60.0}};
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
        member = stock(Polyline({at - Vector(0.5 * overlap, 0.0, 0.0), at + Vector(600.0, 0.0, 0.0)}), Vector::z_axis(), numbers["stock_thick"], numbers["stock_wide"], design + "_member");
        carrier = stock(Polyline({at - Vector(600.0, 0.0, 0.0), at + Vector(0.5 * overlap, 0.0, 0.0)}), Vector::z_axis(), numbers["stock_thick"], numbers["stock_wide"], design + "_carrier");
        if (square_stock)
            member->directions = {Vector::z_axis()};
        if (square_stock)
            carrier->directions = {Vector::z_axis()};
        return {member, carrier};
    }

    // a cross: the carrier along y, the member pitched over it, its underside through the point the record sets
    if (crossing) {
        carrier = EdgeStraightElement::square(Polyline({at + Vector(0.0, -450.0, 0.0), at + Vector(0.0, numbers["reach"], 0.0)}), half, design + "_carrier");
        if (square_stock)
            carrier->directions = {Vector::z_axis()};
        const Vector downhill(-1.0, 0.0, 0.0);
        const Vector up(-std::sin(turn), 0.0, std::cos(turn));
        const Point underside = at + Vector(0.0, 0.0, half) + downhill * (numbers["shift"] * half) - Vector(0.0, 0.0, numbers["sit"] * 2.0 * half);
        const Point on_axis = underside + up * half;
        member = EdgeStraightElement::square(Polyline({on_axis - along * numbers["tail"], on_axis + along * 600.0}), half, design + "_member");
        if (square_stock)
            member->directions = {Vector::y_axis()};
        return {member, carrier};
    }

    // an end on a side: the carrier along x, the member ending on its axis, its stock running past the end; at a corner the
    // carrier ends flush with the member's outer face
    const double carrier_end = record.fields.at("category") == "end_end_inplane" ? numbers["stock_thick"] : 450.0;
    const Vector across(-std::sin(turn), 0.0, std::cos(turn));
    carrier = stock(Polyline({at + Vector(-450.0, 0.0, 0.0), at + Vector(carrier_end, 0.0, 0.0)}), Vector::z_axis(), numbers["stock_thick"], numbers["stock_wide"], design + "_carrier");
    member = stock(Polyline({at, at + along * 600.0}), across, numbers["stock_thick"], numbers["stock_wide"], design + "_member");
    if (square_stock)
        carrier->directions = {Vector::y_axis()};
    if (square_stock)
        member->directions = {Vector::y_axis()};
    member->extension = 180.0;

    return {member, carrier};
}

}
