#pragma once
#include "src/templates/vault/vault.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_vault {

// ═══════════════════════════════════════════════════════════════════════════
// SurfaceVault
// ═══════════════════════════════════════════════════════════════════════════

/// A NURBS or sewn multi-surface NGon block vault with planarized side faces, optional shifted facets, and face contacts.
class SurfaceVault : public WoodSession {
public:
    const SurfaceVaultGuide guide; // Surface patches, subdivision mode, extrusion and shift settings.

    /// Builds the subdivided block vault, computes coplanar face contacts, and records the assembly module.
    explicit SurfaceVault(
        const SurfaceVaultGuide& guide = SurfaceVaultGuide::hex_saddle_vault(),
        const std::string& name = "vault_surface"
    );

    /// Returns the underlying subdivided polygonal surface mesh.
    const Mesh& mesh() const;

    /// Returns the worst corner deviation over all block side faces in mm.
    double side_planarity() const;

    /// Returns the worst corner deviation over all block top and bottom caps in mm.
    double cap_planarity() const;

private:
    Mesh _mesh; // Subdivided polygonal vault mesh.
    NgonSolidMetrics _metrics; // Side and cap planarity residuals.
};

}
