#pragma once

#include "pch.h"
#include "wood_element_edge_straight.h"

using namespace session_cpp;

namespace wood_session {

/// A number, or a vector when is_vector is set: what an expression of the joint library gives.
struct LibraryValue {
    double number = 0.0;
    Vector vector;
    bool is_vector = false;
};

/// One record of the joint library, data/joints/*.yaml: its fields and its lists in order, quotes stripped.
struct LibraryRecord {
    std::map<std::string, std::string> fields; // title, german, page, category, family, corners, and filled and sealed when given.
    std::map<std::string, std::vector<std::string>> lists; // parameters, points, planes, parts, steps, insertions, more.
};

/// The value of a library expression: numbers, names, + - * / and parentheses, unit(v), length(v), cross(a, b), dot(a, b) and
/// meet(point, direction, plane point, plane normal), where the line through point along direction meets the plane.
LibraryValue evaluate(const std::string& expression, const std::map<std::string, LibraryValue>& names);

/// A record's repeat ("k = count", count an expression of its parameters and frame, k the index): every line of its points,
/// planes, parts and insertions with a {expression} written once for k = 0 ... count - 1 with each {expression} its whole
/// number, and in a step every such region, but for one naming a unit past the count; names gets count. A record with no repeat is left as it is.
void library_repeat(LibraryRecord& record, std::map<std::string, LibraryValue>& names);

/// The record of a plate design of data/joints/plate/*.yaml: its fields ("unit_scale", "fabrication") and its lists
/// ("parameters", "outlines", "types"), quotes stripped; empty when no file names the design.
LibraryRecord plate_record(const std::string& design);

/// The designs of data/joints/<chapter>.yaml in the order the file has them, every file's when chapter is empty.
std::vector<std::string> library_designs(const std::string& chapter = "");

/// The record of design in the first data/joints/*.yaml that has it; throws naming the design when none does.
LibraryRecord library_record(const std::string& design);

/// The member and the carrier a record is drawn on, 120 x 120, standing in the xz plane at `at`, on a square corner or an inclined
/// one: a side_end member ending on a carrier along x, square or leaning 50 degrees from it, its stock 180 mm past its end (an
/// end_end_inplane corner's carrier ending flush with the member's outer face); a cross
/// member pitched over a carrier along y, its underside through the carrier's top centre moved shift of the half width downhill and sit of the depth
/// down, running tail mm past the crossing (the record's parameters pitch, sit, shift and tail, 35 degrees, 0, 0 and 150 when it has none), the carrier running reach mm past
/// it (150; 60 for a corner, flush with the member's side); a splice's two along x on one line, overlapping by lap section heights (2).
/// A record with stock_thick and stock_wide (half sizes, 60 each) draws boards: thick across the plane of the joint, wide out of it.
/// carrier_thick and carrier_wide give a side or corner carrier its own section and span its half length (150); flat lays a board
/// and its batten in the plane of the joint, their thickness across it. roll turns a crossing's two sections about their axes (degrees). Every beam is beam_length mm long (300), extra members trimmed to it.
std::array<std::shared_ptr<EdgeStraightElement>, 2> library_pair(const std::string& design, bool square, const Point& at);

/// The members a record is drawn on: library_pair's member and carrier, then one more per line of its list more, "c = start,
/// end, up, half_up, half_side", three numbers each in mm from `at` for start, end and the up of its section, halves in mm.
std::vector<std::shared_ptr<EdgeStraightElement>> library_members(const std::string& design, bool square, const Point& at);

/// A record as its joinery tile: per member in targets order (a the male, b the female, then more members) the wood each cut
/// takes away as pairs of outlines, the two parallel faces of a prism the solver lofts, and the record's loose parts likewise;
/// all in the tile's unit box, the box of every cut and part mapped to [-0.5, 0.5] along x, y and z. A cut no prism between
/// two parallel faces is counted, not drawn.
struct LibraryTile {
    std::vector<std::vector<std::array<Polyline, 2>>> cuts; // Per member, its cuts' outline pairs.
    std::vector<std::array<Polyline, 2>> parts; // The pegs, keys and wedges, each its two end outlines.
    int unsupported = 0; // Cuts that are no prism between two parallel faces.
    std::vector<int> divisions; // Per member, how often one unit repeats along the joint (fingers, tenons, keys); 1 for a fixed tile.
    std::vector<int> axes; // Per member, the unit box axis it repeats along (0 x, 1 y, 2 z); -1 for a fixed tile.
    Point centre; // The unit box's centre in the record's drawing (library_members at the origin, square corner when it has one).
    Vector size; // The unit box's sizes along x, y and z in mm.
};

/// The joinery tile of design: its joint built on library_members at the origin, every cut step's wood clipped to its member.
LibraryTile library_tile(const std::string& design);

}
