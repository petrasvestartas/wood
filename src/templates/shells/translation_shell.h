#pragma once
#include "src/templates/shells/shells_guide.h"
#include "src/templates/template_chamfer.h"

using namespace session_cpp;
using namespace wood_session;

// ═══════════════════════════════════════════════════════════════════════════
// TranslationShell
// ═══════════════════════════════════════════════════════════════════════════

/// A shell of mitered plates, a cross section swept along a profile.
class TranslationShell : public WoodSession {
public:
    const TranslationShellGuide guide;

    /// The shell of the guide's cross section swept along its profile, as the session named name.
    explicit TranslationShell(const TranslationShellGuide& guide, const std::string& name = "translation_shell");

private:
    /// The cross section moved to every profile point.
    std::vector<Polyline> compute_sections() const;

    /// One quad between every two neighbouring points of two neighbouring sections.
    static Mesh compute_mesh(const std::vector<Polyline>& sections);
};
