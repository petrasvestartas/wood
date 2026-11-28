#pragma once

#include "src/templates/delorme/delorme_guide.h"

namespace wood_delorme {

/// Philibert de l'Orme (1561) side-by-side overlapped flat-plank vault, pointed roof or dome assembly.
class DelormeSystem : public WoodSession {
public:
    const DelormeGuide guide;

    explicit DelormeSystem(
        const DelormeGuide& guide = DelormeGuide(), const std::string& name = "delorme"
    );

    const std::vector<std::shared_ptr<FacePlanarElement>>& planks() const { return _planks; }
    const std::vector<std::shared_ptr<FacePlanarElement>>& liernes() const { return _liernes; }
    const std::vector<std::shared_ptr<FacePlanarElement>>& clavettes() const { return _clavettes; }
    const std::vector<std::shared_ptr<JointPinElement>>& chevilles() const { return _chevilles; }
    const std::vector<std::shared_ptr<FacePlanarElement>>& curbs() const { return _curbs; }

    /// Clones every element and assembly module into `target` under `group_name` translated by `delta`.
    void append_to(
        WoodSession& target,
        const Vector& delta = Vector(0.0, 0.0, 0.0),
        const std::string& group_name = "delorme"
    ) const;

private:
    std::vector<std::shared_ptr<FacePlanarElement>> _curbs;
    std::vector<std::shared_ptr<FacePlanarElement>> _planks;
    std::vector<std::shared_ptr<FacePlanarElement>> _liernes;
    std::vector<std::shared_ptr<FacePlanarElement>> _clavettes;
    std::vector<std::shared_ptr<JointPinElement>> _chevilles;

    void build_curbs();
    void build_planks();
    void build_liernes_and_clavettes();
    void build_chevilles();
    void build_assembly_hierarchy();
};

} // namespace wood_delorme
