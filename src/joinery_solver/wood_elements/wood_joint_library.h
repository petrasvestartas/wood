#pragma once

#include "pch.h"

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
    std::map<std::string, std::string> fields; // title, german, page, category, family, corners.
    std::map<std::string, std::vector<std::string>> lists; // parameters, points, planes, parts, steps, insertions.
};

/// The value of a library expression: numbers, names, + - * / and parentheses, unit(v), length(v), cross(a, b), dot(a, b) and
/// meet(point, direction, plane point, plane normal), where the line through point along direction meets the plane.
LibraryValue evaluate(const std::string& expression, const std::map<std::string, LibraryValue>& names);

/// The designs of data/joints/<chapter>.yaml in the order the file has them, every file's when chapter is empty.
std::vector<std::string> library_designs(const std::string& chapter = "");

/// The record of design in the first data/joints/*.yaml that has it; throws naming the design when none does.
LibraryRecord library_record(const std::string& design);

}
