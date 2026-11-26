#include "src/templates/vault/gothic_ribbed_vault.h"

namespace wood_vault {

// ═══════════════════════════════════════════════════════════════════════════
// Constructor
// ═══════════════════════════════════════════════════════════════════════════
GothicRibbedVault::GothicRibbedVault(const GothicRibbedVaultGuide& guide, const std::string& name)
    : WoodSession(name), guide(guide) {

    // members: the support blocks, the ribs and the web panels, each in its group
    const std::shared_ptr<TreeNode> supports_group = add_group("supports");
    const std::shared_ptr<TreeNode> ribs_group = add_group("ribs");
    const std::shared_ptr<TreeNode> panels_group = add_group("panels");
    for (const std::shared_ptr<Element>& elem : gothic_ribbed_vault(guide)) {
        if (elem->name == "support_block")
            add(elem, supports_group);
        else if (elem->name == "vault_panel")
            add(elem, panels_group);
        else
            add(elem, ribs_group);
    }

    // assembly: blocks, ribs, panels
    assemblies = {gothic_assembly(name, get_elements<Element>())};
}

}
