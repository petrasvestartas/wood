#pragma once
#include "src/templates/vault/vault_guide.h"
#include "wood_profile.h"

using namespace session_cpp;

/// Masonry vault templates.
namespace wood_vault {

// ═══════════════════════════════════════════════════════════════════════════
// Arches and barrels
// ═══════════════════════════════════════════════════════════════════════════

/// The voussoirs of the arch of the guide.
std::vector<std::shared_ptr<Element>> arch(const ArchGuide& guide);

/// The voussoirs of the barrel vault of the guide.
std::vector<std::shared_ptr<Element>> barrel(const BarrelGuide& guide);

// ═══════════════════════════════════════════════════════════════════════════
// Domes and walls
// ═══════════════════════════════════════════════════════════════════════════

/// The voussoirs of the spherical dome of the guide.
std::vector<std::shared_ptr<Element>> dome(const DomeGuide& guide);

/// The bricks of the wall of the guide.
std::vector<std::shared_ptr<Element>> wall(const WallGuide& guide);

// ═══════════════════════════════════════════════════════════════════════════
// Vaults over a square bay
// ═══════════════════════════════════════════════════════════════════════════

/// The voussoirs of the cross (groin) vault of the guide.
std::vector<std::shared_ptr<Element>> cross_vault(const CrossVaultGuide& guide);

/// The voussoirs of the cloister (pavilion) vault of the guide.
std::vector<std::shared_ptr<Element>> cloister_vault(const CloisterVaultGuide& guide);

// ═══════════════════════════════════════════════════════════════════════════
// Gothic ribbed vaults (LiDAR cathedral patterns — Tracing the Past / Willis / Bern Cathedral)
// ═══════════════════════════════════════════════════════════════════════════

/// A Gothic ribbed vault bay of mitered EdgeCurvedElement glulam ribs, 4 corner support blocks, and cylindrical web panels.
std::vector<std::shared_ptr<Element>> gothic_ribbed_vault(const GothicRibbedVaultGuide& guide);

/// How a Gothic bay goes together: the support blocks stand, the ribs raised on them, the web panels laid on the ribs.
AssemblyModule gothic_assembly(const std::string& name, const std::vector<std::shared_ptr<Element>>& elements);

}
