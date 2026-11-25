#pragma once

#include "pch.h"

using namespace session_cpp;

namespace wood_session {

/// A piece of an assembly: a part of one or more elements that move as one, or a module of pieces that goes together first and
/// then moves as one. The pieces of a module stand in the order they are meant to go in, or go in lowest first (raised).
struct AssemblyModule {
    std::string name;
    std::vector<std::string> elements; // The guids of a part's elements; empty for a module.
    std::vector<AssemblyModule> parts; // A module's pieces; empty for a part.
    std::vector<Vector> ways; // Unit directions the piece is tried to go in along before any other.
    bool raised = false; // The module's pieces go in lowest first, whatever their order.
    double play = 0.1; // mm, of a whole: the mean thickness of shared wood under which two solids only touch; more where a design is known to bind.

    // ═══════════════════════════════════════════════════════════════════════════
    // Static constructors
    // ═══════════════════════════════════════════════════════════════════════════

    /// The part of one element, named as it.
    static AssemblyModule part(const std::shared_ptr<Element>& element, const std::vector<Vector>& ways = {});

    /// The part of several elements that move as one.
    static AssemblyModule part(const std::string& name, const std::vector<std::shared_ptr<Element>>& elements, const std::vector<Vector>& ways = {});

    /// The module of the pieces, in the order they go in.
    static AssemblyModule module(const std::string& name, const std::vector<AssemblyModule>& parts, const std::vector<Vector>& ways = {});

    /// The module of the pieces, lowest first.
    static AssemblyModule raise(const std::string& name, const std::vector<AssemblyModule>& parts, const std::vector<Vector>& ways = {});

    /// One part per element.
    template <class T>
    static std::vector<AssemblyModule> parts_of(const std::vector<std::shared_ptr<T>>& elements, const std::vector<Vector>& ways = {}) {

        std::vector<AssemblyModule> made;
        for (const std::shared_ptr<T>& element : elements)
            made.push_back(part(element, ways));

        return made;
    }

    /// One module `<name>_<key>` per key, in key order, of the elements with that key in their order.
    template <class T>
    static std::vector<AssemblyModule> modules_of(const std::string& name, const std::vector<std::shared_ptr<T>>& elements, const std::vector<size_t>& keys) {

        std::map<size_t, std::vector<AssemblyModule>> parts;
        for (size_t i = 0; i < elements.size(); i++)
            parts[keys[i]].push_back(part(elements[i]));
        std::vector<AssemblyModule> made;
        for (const std::pair<const size_t, std::vector<AssemblyModule>>& group : parts)
            made.push_back(module(name + "_" + std::to_string(group.first), group.second));

        return made;
    }

    // ═══════════════════════════════════════════════════════════════════════════
    // Geometry
    // ═══════════════════════════════════════════════════════════════════════════

    /// Whether the piece is a module.
    bool is_module() const;

    /// The guids of every element in it.
    std::vector<std::string> every_element() const;

    /// The same piece of copies: every element the one copies gives for it.
    AssemblyModule of_copies(const std::map<std::string, std::string>& copies) const;
};

}
