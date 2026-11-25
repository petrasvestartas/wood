#include "pch.h"
#include "wood_assembly_module.h"
#include "element.h"

namespace wood_session {

// ═══════════════════════════════════════════════════════════════════════════
// Static constructors
// ═══════════════════════════════════════════════════════════════════════════

AssemblyModule AssemblyModule::part(const std::shared_ptr<Element>& element, const std::vector<Vector>& ways) {
    return {element->name, {element->guid()}, {}, ways, false, 0.1};
}

AssemblyModule AssemblyModule::part(const std::string& name, const std::vector<std::shared_ptr<Element>>& elements, const std::vector<Vector>& ways) {

    AssemblyModule made{name, {}, {}, ways, false, 0.1};
    for (const std::shared_ptr<Element>& element : elements)
        made.elements.push_back(element->guid());

    return made;
}

AssemblyModule AssemblyModule::module(const std::string& name, const std::vector<AssemblyModule>& parts, const std::vector<Vector>& ways) {
    return {name, {}, parts, ways, false, 0.1};
}

AssemblyModule AssemblyModule::raise(const std::string& name, const std::vector<AssemblyModule>& parts, const std::vector<Vector>& ways) {
    return {name, {}, parts, ways, true, 0.1};
}

// ═══════════════════════════════════════════════════════════════════════════
// Geometry
// ═══════════════════════════════════════════════════════════════════════════

bool AssemblyModule::is_module() const {
    return !parts.empty();
}

std::vector<std::string> AssemblyModule::every_element() const {

    std::vector<std::string> every = elements;
    for (const AssemblyModule& piece : parts) {
        const std::vector<std::string> inside = piece.every_element();
        every.insert(every.end(), inside.begin(), inside.end());
    }

    return every;
}

AssemblyModule AssemblyModule::of_copies(const std::map<std::string, std::string>& copies) const {

    AssemblyModule copy = *this;
    for (std::string& guid : copy.elements)
        guid = copies.at(guid);
    for (AssemblyModule& piece : copy.parts)
        piece = piece.of_copies(copies);

    return copy;
}

}
