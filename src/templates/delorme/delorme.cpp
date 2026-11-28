#include "pch.h"
#include "src/templates/delorme/delorme.h"

namespace wood_delorme {

namespace {

Mesh oriented_box_mesh(
    const Point& c,
    const Vector& ex,
    const Vector& ey,
    const Vector& ez,
    double hx,
    double hy,
    double hz
) {
    const Point b = c - ez * hz;
    const Polyline loop({
        b - ex * hx - ey * hy,
        b + ex * hx - ey * hy,
        b + ex * hx + ey * hy,
        b - ex * hx + ey * hy,
        b - ex * hx - ey * hy,
    });
    return Mesh::loft({loop}, {loop.translated(ez * (2.0 * hz))}, true);
}

std::vector<Point> clip_polygon_z_min(const std::vector<Point>& poly, double z_min) {

    if (poly.size() < 3)
        return poly;
    std::vector<Point> out;
    const size_t n = poly.size();
    for (size_t i = 0; i < n; i++) {
        const Point& a = poly[i];
        const Point& b = poly[(i + 1) % n];
        const bool in_a = a[2] >= z_min - 1e-6;
        const bool in_b = b[2] >= z_min - 1e-6;
        if (in_a && in_b) {
            out.push_back(b);
        } else if (in_a && !in_b) {
            const double t = (z_min - a[2]) / (b[2] - a[2]);
            out.push_back(a + (b - a) * t);
        } else if (!in_a && in_b) {
            const double t = (z_min - a[2]) / (b[2] - a[2]);
            out.push_back(a + (b - a) * t);
            out.push_back(b);
        }
    }
    return out;
}

} // namespace

DelormeSystem::DelormeSystem(const DelormeGuide& g, const std::string& session_name)
    : WoodSession(session_name), guide(g) {

    const std::shared_ptr<TreeNode> axes_group = group_named("axes");
    for (const DelormeRibCurve& rib : guide.ribs())
        add_polyline(Polyline(rib.pts), axes_group);
    for (const DelormeLierneSpec& ls : guide.liernes())
        add_line(Line::from_points(ls.start, ls.end), axes_group);

    build_curbs();
    build_planks();
    build_liernes_and_clavettes();
    build_chevilles();
    build_assembly_hierarchy();
}

void DelormeSystem::build_curbs() {

    const DelormeParams& p = guide.params;
    if (p.kind != DelormeKind::HemisphericalDome && p.kind != DelormeKind::CrossedGroin)
        return;

    const std::shared_ptr<TreeNode> curb_group = group_named("curbs");
    const bool is_dome = p.kind == DelormeKind::HemisphericalDome;
    const int n_seg = is_dome ? std::max(6, p.n_ribs) : 8;
    const double r_out = (is_dome ? p.oculus_radius : 260.0) - 0.54 * p.rib_depth - 6.0;
    const double r_in = std::max(45.0, r_out - 90.0);

    Point crown_ref;
    Vector ct, cr;
    guide.ribs().back().evaluate(guide.ribs().back().total_length(), crown_ref, ct, cr);
    const double z_curb_bot = crown_ref[2] - 0.42 * p.rib_depth;
    const double curb_h = 0.84 * p.rib_depth;

    for (int i = 0; i < n_seg; i++) {
        const double a0 = Tolerance::TWO_PI * static_cast<double>(i) / n_seg + 0.004;
        const double a1 = Tolerance::TWO_PI * static_cast<double>(i + 1) / n_seg - 0.004;
        const Polyline bot({
            Point(r_in * std::cos(a0), r_in * std::sin(a0), z_curb_bot),
            Point(r_out * std::cos(a0), r_out * std::sin(a0), z_curb_bot),
            Point(r_out * std::cos(a1), r_out * std::sin(a1), z_curb_bot),
            Point(r_in * std::cos(a1), r_in * std::sin(a1), z_curb_bot),
            Point(r_in * std::cos(a0), r_in * std::sin(a0), z_curb_bot),
        });
        const std::shared_ptr<FacePlanarElement> seg = std::make_shared<FacePlanarElement>(
            bot, bot.translated(Vector(0.0, 0.0, curb_h)), fmt::format("oculus_curb_{}", i)
        );
        add(seg, curb_group);
        _curbs.push_back(seg);
    }
}

void DelormeSystem::build_planks() {

    const DelormeParams& p = guide.params;
    const std::shared_ptr<TreeNode> rib_group = group_named("ribs");
    const double half_d = 0.5 * p.rib_depth;
    const double half_t = 0.5 * p.plank_thickness;
    const double half_stub = 0.5 * p.stub_tenon_ratio * p.rib_depth;
    const int facets = std::max(2, p.facets_per_plank);

    _planks.reserve(guide.planks().size());

    for (size_t idx = 0; idx < guide.planks().size(); idx++) {
        const DelormePlankSpec& sp = guide.planks()[idx];
        const DelormeRibCurve& rib = guide.ribs()[sp.rib_index];

        const double s_start = sp.trim_start_horizontal ? sp.s0 : (sp.s0 + 0.5 * p.seam_gap);
        const double s_end = sp.trim_end_horizontal ? sp.s1 : (sp.s1 - 0.5 * p.seam_gap);

        Point p0, p1;
        Vector t0, r0, t1, r1;
        rib.evaluate(s_start, p0, t0, r0);
        rib.evaluate(s_end, p1, t1, r1);

        std::vector<Point> contour;
        for (int f = 0; f <= facets; f++) {
            const double sf = s_start + (s_end - s_start) * static_cast<double>(f) / facets;
            Point pf;
            Vector tf, rf;
            rib.evaluate(sf, pf, tf, rf);
            Point ext_pt = pf + rf * half_d;
            if (f == 0 && sp.trim_start_horizontal && ext_pt[2] > 0.0 && std::abs(t0[2]) > 0.15)
                ext_pt = ext_pt - t0 * (ext_pt[2] / t0[2]);
            if (f == facets && sp.trim_end_horizontal && ext_pt[2] > 0.0 && std::abs(t1[2]) > 0.15)
                ext_pt = ext_pt - t1 * (ext_pt[2] / t1[2]);
            contour.push_back(ext_pt);
        }

        if (sp.has_end_tenon) {
            contour.push_back(p1 + r1 * half_stub);
            contour.push_back(p1 + r1 * half_stub + t1 * p.stub_tenon_length);
            contour.push_back(p1 - r1 * half_stub + t1 * p.stub_tenon_length);
            contour.push_back(p1 - r1 * half_stub);
        }

        for (int f = facets; f >= 0; f--) {
            const double sf = s_start + (s_end - s_start) * static_cast<double>(f) / facets;
            Point pf;
            Vector tf, rf;
            rib.evaluate(sf, pf, tf, rf);
            Point int_pt = pf - rf * half_d;
            if (f == 0 && sp.trim_start_horizontal && int_pt[2] > 0.0 && std::abs(t0[2]) > 0.15)
                int_pt = int_pt - t0 * (int_pt[2] / t0[2]);
            if (f == facets && sp.trim_end_horizontal && int_pt[2] > 0.0 && std::abs(t1[2]) > 0.15)
                int_pt = int_pt - t1 * (int_pt[2] / t1[2]);
            contour.push_back(int_pt);
        }

        if (sp.has_start_notch) {
            const double notch_h = half_stub + 1.0;
            const double notch_d = p.stub_tenon_length + 1.0;
            contour.push_back(p0 - r0 * notch_h);
            contour.push_back(p0 - r0 * notch_h + t0 * notch_d);
            contour.push_back(p0 + r0 * notch_h + t0 * notch_d);
            contour.push_back(p0 + r0 * notch_h);
        }

        if (sp.trim_start_horizontal || sp.trim_end_horizontal)
            contour = clip_polygon_z_min(contour, 0.0);

        std::vector<Point> clean_ring;
        for (const Point& pt : contour) {
            if (clean_ring.empty() || Point::distance(clean_ring.back(), pt) > 0.25)
                clean_ring.push_back(pt);
        }
        if (clean_ring.size() >= 3 && Point::distance(clean_ring.front(), clean_ring.back()) < 0.25)
            clean_ring.pop_back();
        clean_ring.push_back(clean_ring.front());

        const Vector shift_lo = rib.normal * (sp.lateral_offset - half_t);
        const Vector shift_hi = rib.normal * (sp.lateral_offset + half_t);
        const Polyline poly_lo = Polyline(clean_ring).translated(shift_lo);
        const Polyline poly_hi = Polyline(clean_ring).translated(shift_hi);

        const std::shared_ptr<FacePlanarElement> plank = std::make_shared<FacePlanarElement>(
            poly_lo,
            poly_hi,
            fmt::format("plank_r{}_p{}_{}", sp.rib_index, sp.ply_index, sp.plank_index)
        );
        add(plank, rib_group);
        _planks.push_back(plank);
    }
}

void DelormeSystem::build_liernes_and_clavettes() {

    const DelormeParams& p = guide.params;
    const std::shared_ptr<TreeNode> lierne_group = group_named("liernes");
    const std::shared_ptr<TreeNode> clavette_group = group_named("clavettes");

    size_t mortise_count = 0;
    size_t clavette_count = 0;

    for (size_t li = 0; li < guide.liernes().size(); li++) {
        const DelormeLierneSpec& ls = guide.liernes()[li];
        const double len = Point::distance(ls.start, ls.end);
        const Point mid = ls.start + (ls.end - ls.start) * 0.5;
        const double hw = 0.5 * p.lierne_width;
        const double hh = 0.5 * p.lierne_height;
        const double hl = 0.5 * len;

        const Point base = mid - ls.up_dir * hh;
        const Polyline bot({
            base - ls.axis_dir * hl - ls.width_dir * hw,
            base + ls.axis_dir * hl - ls.width_dir * hw,
            base + ls.axis_dir * hl + ls.width_dir * hw,
            base - ls.axis_dir * hl + ls.width_dir * hw,
            base - ls.axis_dir * hl - ls.width_dir * hw,
        });
        const std::shared_ptr<FacePlanarElement> lierne = std::make_shared<FacePlanarElement>(
            bot, bot.translated(ls.up_dir * p.lierne_height), ls.name
        );
        add(lierne, lierne_group);
        _liernes.push_back(lierne);

        for (const DelormeLiernePiercing& pier : ls.piercings) {
            const Point p_rib = ls.start + ls.axis_dir * pier.t_center;
            const Mesh mortise_cutter = oriented_box_mesh(
                p_rib,
                ls.axis_dir,
                ls.width_dir,
                ls.up_dir,
                pier.half_span_along_lierne + 25.0,
                hw + p.mortise_clearance,
                hh + p.mortise_clearance
            );

            for (size_t pk = 0; pk < guide.planks().size(); pk++) {
                const DelormePlankSpec& ps = guide.planks()[pk];
                if (ps.rib_index != pier.rib_index)
                    continue;
                if (pier.rib_s < ps.s0 - 5.0 || pier.rib_s > ps.s1 + 5.0)
                    continue;

                const std::shared_ptr<JointElement> mortise =
                    std::make_shared<JointElement>(mortise_cutter, SolidOperation::subtract);
                mortise->name = fmt::format("lierne_mortise_{}", mortise_count++);
                mortise->is_visible = false;
                add(mortise, lierne_group);
                add_interaction(mortise, _planks[pk], mortise->interaction(0));
            }

            for (int side : {-1, 1}) {
                const double t_key =
                    pier.t_center
                    + side * (pier.half_span_along_lierne + 0.5 * p.clavette_width + 1.4);
                const Point kc = ls.start + ls.axis_dir * t_key;

                const double head_hw = 0.5 * p.clavette_width + p.clavette_taper;
                const double kt = 0.5 * p.clavette_thickness;
                const double kh = 0.5 * p.clavette_length;

                const Mesh keyhole_cutter = oriented_box_mesh(
                    kc,
                    ls.axis_dir,
                    ls.width_dir,
                    ls.up_dir,
                    head_hw + 1.6,
                    kt + 1.6,
                    hh + 15.0
                );
                const std::shared_ptr<JointElement> keyhole =
                    std::make_shared<JointElement>(keyhole_cutter, SolidOperation::subtract);
                keyhole->name = fmt::format("lierne_keyhole_{}", clavette_count);
                keyhole->is_visible = false;
                add(keyhole, clavette_group);
                add_interaction(keyhole, lierne, keyhole->interaction(0));

                const double inner_a = -side * (0.5 * p.clavette_width);
                const double outer_top = side * (0.5 * p.clavette_width + p.clavette_taper);
                const double outer_bot =
                    side * std::max(2.0, 0.5 * p.clavette_width - p.clavette_taper);

                std::vector<Point> w_profile = {
                    kc + ls.axis_dir * inner_a + ls.up_dir * kh,
                    kc + ls.axis_dir * outer_top + ls.up_dir * kh,
                    kc + ls.axis_dir * outer_bot - ls.up_dir * kh,
                    kc + ls.axis_dir * inner_a - ls.up_dir * kh,
                };
                if (side < 0)
                    std::reverse(w_profile.begin(), w_profile.end());
                w_profile.push_back(w_profile.front());

                const Polyline w_lo = Polyline(w_profile).translated(ls.width_dir * (-kt));
                const Polyline w_hi = Polyline(w_profile).translated(ls.width_dir * kt);
                const std::shared_ptr<FacePlanarElement> clavette =
                    std::make_shared<FacePlanarElement>(
                        w_lo, w_hi, fmt::format("clavette_{}", clavette_count++)
                    );
                add(clavette, clavette_group);
                _clavettes.push_back(clavette);
            }
        }
    }
}

void DelormeSystem::build_chevilles() {

    const DelormeParams& p = guide.params;
    if (guide.chevilles().empty() || p.cheville_radius <= 0.5)
        return;

    const std::shared_ptr<TreeNode> cheville_group = group_named("chevilles");
    size_t bore_idx = 0;

    for (size_t ci = 0; ci < guide.chevilles().size(); ci++) {
        const DelormeChevilleSpec& cs = guide.chevilles()[ci];
        const std::shared_ptr<JointPinElement> pin =
            std::make_shared<JointPinElement>(cs.axis, p.cheville_radius, 0.03);
        pin->name = fmt::format("cheville_{}", ci);
        add(pin, cheville_group);
        _chevilles.push_back(pin);

        Line bore_line = cs.axis;
        bore_line.extend_equally(24.0);
        for (size_t pk = 0; pk < guide.planks().size(); pk++) {
            const DelormePlankSpec& ps = guide.planks()[pk];
            if (ps.rib_index != cs.rib_index)
                continue;
            if (cs.rib_s < ps.s0 || cs.rib_s > ps.s1)
                continue;
            const std::shared_ptr<JointElement> bore =
                JointElement::drill(bore_line, p.cheville_radius + 0.35);
            bore->name = fmt::format("cheville_bore_{}", bore_idx++);
            bore->is_visible = false;
            add(bore, cheville_group);
            add_interaction(bore, _planks[pk], bore->interaction(0));
        }
    }
}

void DelormeSystem::build_assembly_hierarchy() {

    // Base sub-module "ribs": compression curb (when present) + staggered multi-ply planks assembled plank-by-plank
    std::vector<AssemblyModule> rib_parts;
    for (const std::shared_ptr<FacePlanarElement>& c : _curbs)
        rib_parts.push_back(AssemblyModule::part(c, {{0.0, 0.0, -1.0}, {0.0, 0.0, 1.0}}));

    for (size_t pk = 0; pk < _planks.size(); pk++) {
        const DelormePlankSpec& ps = guide.planks()[pk];
        const DelormeRibCurve& rib = guide.ribs()[ps.rib_index];
        Point p0, pm;
        Vector t0, r0, tm, rm;
        rib.evaluate(ps.s0, p0, t0, r0);
        rib.evaluate(0.5 * (ps.s0 + ps.s1), pm, tm, rm);
        const Vector lat_in = ps.ply_index == 0 ? rib.normal : (rib.normal * -1.0);
        rib_parts.push_back(AssemblyModule::part(
            _planks[pk], {t0 * -1.0, lat_in, lat_in * -1.0, rm * -1.0, Vector(0.0, 0.0, -1.0)}
        ));
    }

    // Whole assembly: standing "ribs" sub-module first, then each cheville, each lierne, and each clavette along its own axis
    std::vector<AssemblyModule> whole;
    if (!rib_parts.empty())
        whole.push_back(AssemblyModule::module("ribs", rib_parts));

    for (const std::shared_ptr<JointPinElement>& pin : _chevilles) {
        const Vector d = pin->axis().to_vector().normalized();
        whole.push_back(AssemblyModule::part(pin, {d, d * -1.0}));
    }

    for (size_t li = 0; li < _liernes.size(); li++) {
        const Vector d = guide.liernes()[li].axis_dir;
        whole.push_back(AssemblyModule::part(_liernes[li], {d, d * -1.0}));
    }

    size_t k_idx = 0;
    for (size_t li = 0; li < guide.liernes().size(); li++) {
        const Vector down = guide.liernes()[li].up_dir * -1.0;
        const size_t n_keys = guide.liernes()[li].piercings.size() * 2;
        for (size_t j = 0; j < n_keys && k_idx < _clavettes.size(); j++, k_idx++)
            whole.push_back(AssemblyModule::part(_clavettes[k_idx], {down, down * -1.0}));
    }

    AssemblyModule root = AssemblyModule::module(name, whole);
    root.play = 0.25;
    assemblies = {root};
}

void DelormeSystem::append_to(
    WoodSession& target, const Vector& delta, const std::string& group_name
) const {
    const std::shared_ptr<TreeNode> grp = target.group_named(group_name);
    const Xform xf = Xform::translation(delta[0], delta[1], delta[2]);
    std::map<std::string, std::string> copies;
    std::map<std::string, std::shared_ptr<Element>> elem_copies;

    for (const std::shared_ptr<Element>& elem : elements()) {
        std::shared_ptr<Element> cloned = elem->clone();
        if (!delta.is_zero())
            cloned->place(xf);
        copies[elem->guid()] = cloned->guid();
        elem_copies[elem->guid()] = cloned;
        target.add(cloned, grp);
    }

    for (const std::shared_ptr<JointElement>& joint : get_elements<JointElement>()) {
        const std::shared_ptr<JointElement> cloned_joint =
            std::dynamic_pointer_cast<JointElement>(elem_copies[joint->guid()]);
        if (!cloned_joint)
            continue;
        const std::vector<std::string> orig_targets = joint->targets;
        cloned_joint->targets.clear();
        for (size_t ti = 0; ti < orig_targets.size(); ti++) {
            const auto it = elem_copies.find(orig_targets[ti]);
            if (it != elem_copies.end())
                target.add_interaction(cloned_joint, it->second, cloned_joint->interaction(ti));
        }
    }

    for (const AssemblyModule& whole : assemblies) {
        AssemblyModule mapped = whole.of_copies(copies);
        mapped.name = group_name;
        target.assemblies.push_back(mapped);
    }
}

} // namespace wood_delorme
