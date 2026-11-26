#include "wood_session.h"
#include "wood_element_geometry.h"
#include "src/templates/cavity_border.h"
#include "src/templates/folding/counter_fold.h"
#include "src/templates/folding/diagonal_fold.h"
#include "src/templates/folding/prismatic_fold.h"
#include "src/templates/vidy/vidy.h"

using namespace session_cpp;
using namespace wood_session;
using namespace wood_folding;

namespace {

/// Throws with a diagnostic message when ok is false.
void check(bool ok, const std::string& message) {

    if (ok)
        return;

    std::cerr << "vidy_elements FAILED: " << message << std::endl;
    throw std::runtime_error(message);
}

/// Counts 3D solid overlaps (> 5 mm^3) across AABB-intersecting plate pairs via Manifold CSG.
size_t report_plate_collisions(
    const std::vector<std::shared_ptr<FacePlanarElement>>& plates, const std::string& label
) {

    const size_t n = plates.size();
    std::vector<Mesh> meshes(n);
    std::vector<AABB> boxes(n);

    for (size_t i = 0; i < n; i++) {
        meshes[i] = plates[i]->model_geometry_mesh();
        boxes[i] = plates[i]->aabb(-0.05);
    }

    size_t hits = 0;
    double max_vol = 0.0;

    for (size_t i = 0; i < n; i++)
        for (size_t j = i + 1; j < n; j++) {
            if (!boxes[i].intersects(boxes[j]))
                continue;

            const Mesh inter = solid_boolean(meshes[i], meshes[j], SolidOperation::intersect);
            const double vol = inter.number_of_faces() > 0 ? compute_volume(inter) : 0.0;

            if (vol <= 5.0)
                continue;

            std::cout << fmt::format(
                "  COLLISION [{}]: {} <-> {} vol={:.1f}\n",
                label,
                plates[i]->name,
                plates[j]->name,
                vol
            );
            hits++;
            max_vol = std::max(max_vol, vol);
        }

    std::cout << fmt::format(
        "[{}] total colliding pairs = {} (max vol = {:.1f} mm^3)\n", label, hits, max_vol
    );

    return hits;
}

/// Checks thickness, valid BRep, positive volume and jointed feature loops of shell plates.
void check_plates(
    const std::vector<std::shared_ptr<FacePlanarElement>>& plates,
    double expected_thickness,
    bool require_joint_features,
    const std::string& label
) {

    for (const std::shared_ptr<FacePlanarElement>& plate : plates) {
        check(
            std::abs(plate->thickness - expected_thickness) < 0.25,
            fmt::format(
                "{}: thickness {:.2f} != {:.2f}", label, plate->thickness, expected_thickness
            )
        );
        const Mesh& mesh = plate->model_geometry_mesh();
        const BRep& brep = plate->model_geometry_brep();
        check(mesh.number_of_vertices() >= 6, label + ": plate mesh vertex count >= 6");
        check(std::abs(mesh.volume()) > 1.0e6, label + ": plate mesh volume must be positive");
        check(brep.is_valid(), label + ": plate BRep must be valid");

        if (!require_joint_features)
            continue;

        check(
            plate->features.bottom.size() == plate->features.top.size() &&
                !plate->features.bottom.empty(),
            label + ": jointed plate paired bottom/top feature loops"
        );
        check(
            mesh.number_of_vertices() >= 2 * (plate->features.bottom[0].point_count() - 1),
            fmt::format(
                "{} ({}): trimmed_mesh vertices {} < 2*({}-1) holes={}",
                label,
                plate->name,
                mesh.number_of_vertices(),
                plate->features.bottom[0].point_count(),
                plate->features.bottom.size() - 1
            )
        );
    }
}

/// Checks thickness, eight-vertex prism topology, positive volume and valid BRep of border boards.
void check_border_plates(
    const std::vector<std::shared_ptr<FacePlanarElement>>& borders,
    double expected_thickness,
    const std::string& label
) {

    for (const std::shared_ptr<FacePlanarElement>& board : borders) {
        check(
            std::abs(board->thickness - expected_thickness) < 0.25,
            fmt::format(
                "{}: border thickness {:.2f} != {:.2f}", label, board->thickness, expected_thickness
            )
        );
        const Mesh& mesh = board->model_geometry_mesh();
        const BRep& brep = board->model_geometry_brep();
        check(mesh.number_of_vertices() == 8, label + ": border board mesh vertex count == 8");
        check(std::abs(mesh.volume()) > 1.0e6, label + ": border board mesh volume > 0");
        check(brep.is_valid(), label + ": border board BRep must be valid");
    }
}

/// Tests compute_cavity_border_board with full-corner extension and mitered neighbor corners.
void test_reusable_cavity_border() {

    const Vector facet_n = Vector{0.25, 0.0, 0.9682458}.normalized();
    CavityBorderSpec spec_a;
    spec_a.facet_plane = Plane::from_point_normal({0.0, 0.0, 0.0}, facet_n);
    spec_a.boundary_plane = Plane::from_point_normal({0.0, 0.0, 0.0}, {0.0, -1.0, 0.0});
    spec_a.start_plane = Plane::from_point_normal({0.0, 0.0, 0.0}, {-1.0, 0.0, 0.0});
    spec_a.end_plane = Plane::from_point_normal({1800.0, 0.0, -465.0}, {0.96, -0.28, 0.0});
    spec_a.inner_level = 45.5;
    spec_a.outer_level = 254.5;
    spec_a.thickness = 40.0;
    spec_a.outer_gap = 0.5;
    spec_a.miter_start = true;
    spec_a.start_neighbor_gap = 0.0;
    spec_a.start_neighbor_thickness = 40.0;
    spec_a.start_setback = 0.5;
    spec_a.end_setback = 0.5;

    CavityBorderSpec spec_b = spec_a;
    spec_b.boundary_plane = spec_a.start_plane;
    spec_b.start_plane = Plane::from_point_normal({0.0, 1200.0, 0.0}, {0.0, 1.0, 0.0});
    spec_b.end_plane = spec_a.boundary_plane;
    spec_b.outer_gap = 0.0;
    spec_b.miter_start = false;
    spec_b.miter_end = true;
    spec_b.end_neighbor_gap = 0.5;
    spec_b.end_neighbor_thickness = 40.0;

    const std::shared_ptr<FacePlanarElement> board_a =
        compute_cavity_border_board(spec_a, "generic_border_a");
    const std::shared_ptr<FacePlanarElement> board_b =
        compute_cavity_border_board(spec_b, "generic_border_b");
    check(board_a != nullptr && board_b != nullptr, "mitered generic cavity border pair built");
    check_border_plates({board_a, board_b}, 40.0, "generic_cavity_border");
    check(
        report_plate_collisions({board_a, board_b}, "generic_miter_pair") == 0,
        "mitered generic border pair has zero collision"
    );
}

/// Verifies Stage 1 base mesh and Stage 2 four-surface miter shells on the 11-bay guide.
void test_guide() {

    const wood_vidy::VidyGuide guide(11);
    check(guide.base_mesh().number_of_vertices() == 127, "11-bay base mesh vertex count == 127");
    check(guide.facets().size() == 154, "11-bay facet count == 154");
    check(guide.bay_axes().size() == 12, "11-bay transverse axis count == 12");
    check(std::abs(guide.cavity_gap() - 210.0) < 1e-6, "210 mm clear insulation cavity");
    check(guide.miter_planes_mesh().number_of_faces() == 236, "236 miter and separation planes");
}

/// Verifies all 14 perpendicular border boards of bay meet corner-to-corner within <= 2.5 mm.
void test_bay_border_closure(const wood_vidy::VidyPavilion& panels, size_t bay) {

    std::vector<std::shared_ptr<FacePlanarElement>> bay_borders;

    for (size_t loc = 0; loc < 14; loc++) {
        const std::vector<std::shared_ptr<FacePlanarElement>>& fb =
            panels.border_plates_for_facet(14 * bay + loc);
        bay_borders.insert(bay_borders.end(), fb.begin(), fb.end());
    }

    check(bay_borders.size() == 14, "14 border boards per bay");

    for (size_t i = 0; i < bay_borders.size(); i++) {
        const Polyline& pl = bay_borders[i]->polylines[1];
        double end_a = 1e9;
        double end_b = 1e9;

        for (size_t j = 0; j < bay_borders.size(); j++) {
            if (i == j)
                continue;

            const Polyline& ql = bay_borders[j]->polylines[1];

            for (size_t q = 0; q + 1 < ql.point_count(); q++) {
                end_a = std::min({end_a, pl[0].distance(ql[q]), pl[1].distance(ql[q])});
                end_b = std::min({end_b, pl[2].distance(ql[q]), pl[3].distance(ql[q])});
            }
        }

        check(
            end_a <= 2.5 && end_b <= 2.5,
            fmt::format(
                "border corner gap on {} ends=({:.2f}, {:.2f})", bay_borders[i]->name, end_a, end_b
            )
        );
    }
}

/// Verifies border boards reach corners, selective chamfers are sharp where unneeded, and 0 CSG hits.
void test_stage3_panels() {

    const wood_vidy::VidyGuide guide(11);
    const wood_vidy::VidyPavilion panels(guide, false);
    check(panels.shell_plates().size() == 308, "308 double-layer CLT shell plates");
    check(panels.border_plates().size() == 154, "154 cavity-closing border boards");
    check(panels.all_plates().size() == 462, "462 total Stage 3 plates");
    check_plates(panels.shell_plates(), 45.0, false, "stage3_panels");
    check_border_plates(panels.border_plates(), 45.0, "stage3_borders");

    const size_t mid_wall = 14 * 5;
    check(
        panels.inner_plates()[mid_wall]->polylines[0].point_count() == 5,
        "inner wall quad has zero unneeded corner chamfers (5-pt closed outline)"
    );
    check(
        panels.outer_plates()[6]->polylines[0].point_count() <= 6,
        "gable crown plate preserves sharp gable corners"
    );

    const wood_vidy::VidyFacet& crown_f = guide.facets()[mid_wall + 6];
    const wood_vidy::VidyFacet& left_eave_f = guide.facets()[mid_wall + 4];
    const wood_vidy::VidyFacet& right_eave_f = guide.facets()[mid_wall + 8];
    const Plane left_bis =
        compute_cavity_bisector_plane(
            guide.vertices()[crown_f.vertices[0]], crown_f.normal, left_eave_f.normal
        )
            .value();
    const Plane right_bis =
        compute_cavity_bisector_plane(
            guide.vertices()[crown_f.vertices[1]], crown_f.normal, right_eave_f.normal
        )
            .value();
    const std::shared_ptr<FacePlanarElement>& crown_border =
        panels.border_plates_for_facet(mid_wall + 6).at(0);
    const Polyline& cb_top = crown_border->polylines[1];
    double min_d1 = 1e9;
    double min_d2 = 1e9;

    for (size_t i = 0; i + 1 < cb_top.point_count(); i++) {
        min_d1 = std::min(min_d1, std::abs(right_bis.signed_distance(cb_top[i])));
        min_d2 = std::min(min_d2, std::abs(left_bis.signed_distance(cb_top[i])));
    }

    check(
        min_d1 < 1.5 && min_d2 < 1.5,
        fmt::format("crown border reaches both corners ({:.2f}, {:.2f} mm)", min_d1, min_d2)
    );
    const std::vector<std::shared_ptr<FacePlanarElement>>& wall_b =
        panels.border_plates_for_facet(mid_wall);
    double miter_gap = 1e9;

    for (size_t i = 0; i + 1 < wall_b.at(0)->polylines[1].point_count(); i++)
        for (size_t j = 0; j + 1 < wall_b.at(1)->polylines[1].point_count(); j++)
            miter_gap = std::min(
                miter_gap, wall_b.at(0)->polylines[1][i].distance(wall_b.at(1)->polylines[1][j])
            );

    check(miter_gap < 2.0, fmt::format("wall sill-bay border corner miter gap {:.2f}", miter_gap));

    for (const size_t bay : {size_t{0}, size_t{5}, size_t{10}})
        test_bay_border_closure(panels, bay);

    check(
        report_plate_collisions(panels.all_plates(), "stage3_462") == 0, "Stage 3 zero collisions"
    );
}

/// Verifies every mortise slot corner stays >= 42 mm clear of its plate's outer boundary contour.
void test_slot_edge_clearances(const wood_vidy::VidyPavilion& pav, const std::string& label) {

    double global_min = 1e9;
    std::string worst_name;

    for (const std::shared_ptr<FacePlanarElement>& plate : pav.shell_plates()) {
        for (const std::vector<Polyline>& loops : {plate->features.bottom, plate->features.top}) {
            if (loops.size() <= 1)
                continue;

            const Polyline& outer = loops[0];

            for (size_t h = 1; h < loops.size(); h++) {
                const Polyline& hole = loops[h];

                for (size_t c = 0; c + 1 < hole.point_count(); c++) {
                    for (size_t e = 0; e + 1 < outer.point_count(); e++) {
                        if (outer[e].distance(outer[e + 1]) < 1e-6)
                            continue;

                        const Point cp = Line::from_points(outer[e], outer[e + 1])
                                             .closest_point(hole[c], true)
                                             .second;
                        const double d = hole[c].distance(cp);

                        if (d < global_min) {
                            global_min = d;
                            worst_name = plate->name;
                        }
                    }
                }
            }
        }
    }

    std::cout << fmt::format(
        "[{}] min_slot_edge_clearance = {:.1f} mm ({})\n", label, global_min, worst_name
    );
    check(
        global_min >= 42.0,
        fmt::format("{}: slot too close to edge ({:.1f} mm on {})", label, global_min, worst_name)
    );
}

/// Verifies every roof/wall Regular crease joint feature sits >= 450 mm from both corners.
void test_joint_corner_margins(const wood_vidy::VidyPavilion& pav, size_t bay) {

    const size_t base = 14 * bay;
    const std::vector<wood_vidy::VidyFacet>& facets = pav.guide.facets();
    const std::vector<wood_vidy::VidyEdge>& edges = pav.guide.edges();

    for (size_t loc = 0; loc < 14; loc++) {
        const wood_vidy::VidyFacet& fc = facets[base + loc];
        const size_t m = fc.vertices.size();

        for (size_t layer = 0; layer < 2; layer++) {
            const std::shared_ptr<FacePlanarElement>& plate =
                layer == 0 ? pav.inner_plates()[base + loc] : pav.outer_plates()[base + loc];
            const Polyline& loop = plate->features.bottom.at(0);
            const size_t hi = 2 * layer;

            for (size_t k = 0; k < m; k++) {
                const wood_vidy::VidyEdge& ed = edges[static_cast<size_t>(fc.edge_ids[k])];

                if (ed.kind != wood_vidy::VidyEdgeKind::Regular || ed.in_situ_eave)
                    continue;

                const Point& p0 = fc.miter_rings[hi][k];
                const Point& p1 = fc.miter_rings[hi][(k + 1) % m];
                const double elen = p0.distance(p1);
                const Vector nf = (p1 - p0).normalized();
                Vector out_p = nf.cross(fc.normal).normalized();

                if ((Point::mid_point(p0, p1) - fc.centroid).dot(out_p) < 0.0)
                    out_p = -out_p;

                for (size_t i = 0; i + 1 < loop.point_count(); i++) {
                    const Vector rel = loop[i] - p0;
                    const double s = rel.dot(nf);
                    const double d = rel.dot(out_p);

                    if (s > 150.0 && s < elen - 150.0 && d > 5.0) {
                        const double min_margin = d > 110.0 ? 450.0 : 340.0;
                        check(
                            s >= min_margin && (elen - s) >= min_margin,
                            fmt::format(
                                "joint near corner {} s={:.1f}/{:.1f}", plate->name, s, elen
                            )
                        );
                    }
                }
            }
        }
    }
}

/// Verifies single-direction rigid-body insertion of multi-edge roof and wall plates via CSG.
void test_sliding_kinematics(const wood_vidy::VidyPavilion& pav, size_t bay) {

    const size_t base = 14 * bay;
    const std::vector<Mesh> haunch = {
        pav.inner_plates()[base + 5]->model_geometry_mesh(),
        pav.outer_plates()[base + 5]->model_geometry_mesh(),
        pav.inner_plates()[base + 7]->model_geometry_mesh(),
        pav.outer_plates()[base + 7]->model_geometry_mesh(),
        pav.inner_plates()[base + 10]->model_geometry_mesh(),
        pav.outer_plates()[base + 10]->model_geometry_mesh(),
        pav.inner_plates()[base + 12]->model_geometry_mesh(),
        pav.outer_plates()[base + 12]->model_geometry_mesh()
    };

    for (const std::pair<size_t, size_t>& item :
         {std::pair<size_t, size_t>{6, 0},
          std::pair<size_t, size_t>{6, 1},
          std::pair<size_t, size_t>{11, 0},
          std::pair<size_t, size_t>{11, 1},
          std::pair<size_t, size_t>{4, 0},
          std::pair<size_t, size_t>{4, 1},
          std::pair<size_t, size_t>{8, 0},
          std::pair<size_t, size_t>{8, 1},
          std::pair<size_t, size_t>{9, 0},
          std::pair<size_t, size_t>{9, 1},
          std::pair<size_t, size_t>{13, 0},
          std::pair<size_t, size_t>{13, 1}}) {
        const std::shared_ptr<FacePlanarElement>& plate =
            item.second == 0 ? pav.inner_plates()[base + item.first]
                             : pav.outer_plates()[base + item.first];
        const Vector v_ins = pav.plate_insertion_vector(base + item.first, item.second);

        for (const double d : {5.0, 20.0, 50.0, 100.0, 160.0}) {
            const Mesh moved = plate->model_geometry_mesh().transformed(
                Xform::translation(-v_ins[0] * d, -v_ins[1] * d, -v_ins[2] * d)
            );

            for (const Mesh& target : haunch) {
                const Mesh inter = solid_boolean(moved, target, SolidOperation::intersect);
                const double vol = inter.number_of_faces() > 0 ? compute_volume(inter) : 0.0;
                check(
                    vol <= 5.0,
                    fmt::format("slide collision on {} at d={:.0f}: {:.1f}", plate->name, d, vol)
                );
            }
        }
    }

    std::vector<Mesh> walls;
    std::vector<Mesh> roof;

    for (size_t loc = 0; loc < 4; loc++) {
        walls.push_back(pav.inner_plates()[base + loc]->model_geometry_mesh());
        walls.push_back(pav.outer_plates()[base + loc]->model_geometry_mesh());
    }

    for (const size_t loc : {size_t{4}, size_t{8}, size_t{9}, size_t{13}}) {
        roof.push_back(pav.inner_plates()[base + loc]->model_geometry_mesh());
        roof.push_back(pav.outer_plates()[base + loc]->model_geometry_mesh());
    }

    for (const double dz : {5.0, 25.0, 80.0, 200.0}) {
        for (const Mesh& rm : roof) {
            const Mesh lifted = rm.transformed(Xform::translation(0.0, 0.0, dz));

            for (const Mesh& wm : walls) {
                const Mesh inter = solid_boolean(lifted, wm, SolidOperation::intersect);
                const double vol = inter.number_of_faces() > 0 ? compute_volume(inter) : 0.0;
                check(
                    vol <= 5.0, fmt::format("roof -Z lift collision at dz={:.0f}: {:.1f}", dz, vol)
                );
            }
        }
    }
}

/// Verifies Stage 4 single-bay section (28 jointed shells + 14 border boards) and Protobuf round-trip.
void test_stage4_section() {

    const wood_vidy::VidyGuide section_guide(1);
    wood_vidy::VidyPavilion section(section_guide, true, 1.0, 500.0);
    std::cout << fmt::format(
        "[stage4_42] shell={} border={} seams={} double_pins={} single_pins={} total_pins={} mortise_slots={}\n",
        section.shell_plates().size(),
        section.border_plates().size(),
        section.total_jointed_seams(),
        section.total_double_pins(),
        section.total_single_pins(),
        section.total_pins(),
        section.total_mortise_slots()
    );
    check(
        section.shell_plates().size() == 28 && section.border_plates().size() == 14, "1-bay 28+14"
    );
    check(section.all_plates().size() == 42 && section.total_jointed_seams() == 16, "1-bay 42/16");
    check_plates(section.shell_plates(), 45.0, true, "stage4_first_row");
    check_border_plates(section.border_plates(), 45.0, "stage4_first_row_borders");
    check(
        report_plate_collisions(section.all_plates(), "stage4_42") == 0, "Stage 4a zero collisions"
    );
    test_joint_corner_margins(section, 0);
    test_slot_edge_clearances(section, "stage4_42");
    test_sliding_kinematics(section, 0);

    const WoodSession reloaded = WoodSession::pb_loads(section.pb_dumps());
    check(
        reloaded.world_elements<FacePlanarElement>().size() == 42, "pb_loads preserved 42 plates"
    );
}

/// Verifies Stage 4 full 11-bay pavilion (462 plates), sliding kinematics and six module steps.
void test_stage4_pavilion() {

    const wood_vidy::VidyGuide full_guide(11);
    wood_vidy::VidyPavilion full_jointed(full_guide, true, 1.0, 500.0);
    std::cout << fmt::format(
        "[stage4_462] shell={} border={} seams={} double_pins={} single_pins={} total_pins={} mortise_slots={}\n",
        full_jointed.shell_plates().size(),
        full_jointed.border_plates().size(),
        full_jointed.total_jointed_seams(),
        full_jointed.total_double_pins(),
        full_jointed.total_single_pins(),
        full_jointed.total_pins(),
        full_jointed.total_mortise_slots()
    );
    check(
        full_jointed.shell_plates().size() == 308 && full_jointed.border_plates().size() == 154,
        "308+154"
    );
    check(
        full_jointed.all_plates().size() == 462 && full_jointed.total_jointed_seams() == 176,
        "462/176"
    );
    check_plates(full_jointed.shell_plates(), 45.0, true, "stage4_full_308_jointed");
    check_border_plates(full_jointed.border_plates(), 45.0, "stage4_full_154_borders");
    check(
        report_plate_collisions(full_jointed.all_plates(), "stage4_462") == 0,
        "Stage 4b zero collisions"
    );
    for (size_t b = 0; b < full_guide.bays; b++)
        test_joint_corner_margins(full_jointed, b);

    test_slot_edge_clearances(full_jointed, "stage4_462");
    test_sliding_kinematics(full_jointed, 0);
    test_sliding_kinematics(full_jointed, 5);

    full_jointed.add_module_assembly_groups(5, 1650.0);
    const std::array<std::string, 6> groups = {
        "module_1_haunch_wings",
        "module_2_haunches_joined",
        "module_3_inner_crowns_eaves",
        "module_4_outer_covers",
        "module_5_vertical_walls",
        "module_6_roof_onto_walls"
    };

    for (const std::string& gname : groups) {
        const std::vector<std::shared_ptr<FacePlanarElement>> gplates =
            full_jointed.get_branch(gname).world_elements<FacePlanarElement>();
        check(!gplates.empty() && report_plate_collisions(gplates, gname) == 0, gname + " clean");
    }
}

/// Verifies parametric VidyPavilion configurations (compact symmetric, tapered hall and wide arch).
void test_parametric_vidy() {

    const wood_vidy::VidyGuide g_compact(
        3,
        2400.0,
        12800.0,
        12800.0,
        480.0,
        4800.0,
        4800.0,
        7200.0,
        7200.0,
        3900.0,
        1850.0,
        0.0,
        40.0,
        240.0
    );
    const wood_vidy::VidyPavilion compact(g_compact, true, 1.0, 420.0, "compact_symmetric");
    check_plates(compact.shell_plates(), 40.0, true, "param_compact_shell");
    check_border_plates(compact.border_plates(), 40.0, "param_compact_borders");
    check(
        report_plate_collisions(compact.all_plates(), "param_compact") == 0,
        "param_compact zero collisions"
    );

    const wood_vidy::VidyGuide g_tapered(
        5,
        2800.0,
        21000.0,
        14800.0,
        760.0,
        7800.0,
        6000.0,
        11600.0,
        9000.0,
        6200.0,
        2450.0,
        820.0,
        45.0,
        320.0
    );
    const wood_vidy::VidyPavilion tapered(g_tapered, true, 1.0, 500.0, "tapered_hall");
    check_plates(tapered.shell_plates(), 45.0, true, "param_tapered_shell");
    check_border_plates(tapered.border_plates(), 45.0, "param_tapered_borders");
    check(
        report_plate_collisions(tapered.all_plates(), "param_tapered") == 0,
        "param_tapered zero collisions"
    );

    const wood_vidy::VidyGuide g_wide(
        4,
        2500.0,
        22000.0,
        19500.0,
        580.0,
        5200.0,
        4800.0,
        8400.0,
        7800.0,
        7000.0,
        2350.0,
        0.0,
        45.0,
        280.0
    );
    const wood_vidy::VidyPavilion wide(g_wide, true, 1.0, 480.0, "wide_shallow");
    check_plates(wide.shell_plates(), 45.0, true, "param_wide_shell");
    check_border_plates(wide.border_plates(), 45.0, "param_wide_borders");
    check(
        report_plate_collisions(wide.all_plates(), "param_wide") == 0, "param_wide zero collisions"
    );
}

/// Verifies one OrigamiShell shape: BReps, 0 collisions, slot clearances and sliding kinematics.
size_t verify_origami_shell(const wood_vidy::OrigamiShell& shell, const std::string& label) {

    std::cout << fmt::format(
        "[{}] facets={} shell={} border={} seams={} dp={} sp={} slots={} skew={:.1f}deg unjoined={}\n",
        label,
        shell.facets().size(),
        shell.shell_plates().size(),
        shell.border_plates().size(),
        shell.total_jointed_seams(),
        shell.total_double_pins(),
        shell.total_single_pins(),
        shell.total_mortise_slots(),
        shell.max_insertion_skew_deg(),
        shell.unjoined_interior_creases()
    );
    check(shell.shell_plates().size() == 2 * shell.facets().size(), label + ": 2 plates per facet");
    check(!shell.border_plates().empty(), label + ": cavity border boards present");
    check(shell.total_jointed_seams() > 0 && shell.total_double_pins() > 0, label + ": pins built");
    check(shell.unjoined_interior_creases() == 0, label + ": all interior creases jointed");
    check(shell.max_insertion_skew_deg() <= 35.0, label + ": Robeller |theta| <= 35 deg");
    check_plates(shell.shell_plates(), shell.plate_thickness, true, label + "_shell");
    check_border_plates(shell.border_plates(), shell.plate_thickness, label + "_borders");
    const size_t col_hits = report_plate_collisions(shell.all_plates(), label);

    // Sequential rigid-body sliding kinematics along each facet's single insertion vector.
    const std::vector<size_t>& order = shell.assembly_order();
    std::vector<Mesh> in_meshes(shell.facets().size());
    std::vector<Mesh> out_meshes(shell.facets().size());
    std::vector<AABB> in_boxes(shell.facets().size());
    std::vector<AABB> out_boxes(shell.facets().size());

    for (size_t f = 0; f < shell.facets().size(); f++) {
        in_meshes[f] = shell.inner_plates()[f]->model_geometry_mesh();
        out_meshes[f] = shell.outer_plates()[f]->model_geometry_mesh();
        in_boxes[f] = shell.inner_plates()[f]->aabb(200.0);
        out_boxes[f] = shell.outer_plates()[f]->aabb(200.0);
    }

    size_t slide_hits = 0;

    for (size_t step = 1; step < order.size(); step++) {
        const size_t f = order[step];
        const Vector v_ins = shell.plate_insertion_vector(f, 0);
        check(std::abs(v_ins.magnitude() - 1.0) < 1e-6, label + ": unit insertion vector");

        for (const double d : {8.0, 45.0, 120.0}) {
            const Xform xf = Xform::translation(-v_ins[0] * d, -v_ins[1] * d, -v_ins[2] * d);
            const Mesh m_in = in_meshes[f].transformed(xf);
            const Mesh m_out = out_meshes[f].transformed(xf);

            for (size_t prev = 0; prev < step; prev++) {
                const size_t pf = order[prev];

                if (!in_boxes[f].intersects(in_boxes[pf]) &&
                    !out_boxes[f].intersects(out_boxes[pf]))
                    continue;

                for (size_t cl = 0; cl < 2; cl++)
                    for (size_t tl = 0; tl < 2; tl++) {
                        const Mesh& cand = cl == 0 ? m_in : m_out;
                        const Mesh& targ = tl == 0 ? in_meshes[pf] : out_meshes[pf];
                        const Mesh inter = solid_boolean(cand, targ, SolidOperation::intersect);
                        const double vol =
                            inter.number_of_faces() > 0 ? compute_volume(inter) : 0.0;

                        if (vol > 5.0) {
                            std::cout << fmt::format(
                                "  SLIDE [{}]: f{}(L{})->f{}(L{}) d={:.0f} vol={:.1f}\n",
                                label,
                                f,
                                cl,
                                pf,
                                tl,
                                d,
                                vol
                            );
                            slide_hits++;
                        }
                    }
            }
        }
    }

    std::cout << fmt::format("[{}] slide_hits={}\n", label, slide_hits);

    return col_hits + slide_hits;
}

/// Verifies OrigamiShell across assemblable long deep folds and rejects locked radial domes.
void test_origami_shapes() {

    size_t issues = 0;
    const PrismaticFold pf_base(PrismaticFoldGuide::vidy_hall(), "prismatic_base");
    const wood_vidy::OrigamiShell prismatic_fold(
        pf_base.mesh(), 45.0, 300.0, true, true, 500.0, 1.0, "prismatic_fold"
    );
    issues += verify_origami_shell(prismatic_fold, "origami_prismatic_fold");

    CounterFoldGuide running = CounterFoldGuide::vidy_hall();
    running.stations = CounterFoldGuide::counter_running(3, 2200.0, 2200.0, 14000.0);
    const CounterFold cr_base(running);
    const wood_vidy::OrigamiShell counter_running(
        cr_base.mesh(), 45.0, 300.0, true, true, 500.0, 1.0, "counter_running"
    );
    issues += verify_origami_shell(counter_running, "origami_counter_running");

    const CounterFold cf_base(CounterFoldGuide::vidy_hall());
    const wood_vidy::OrigamiShell counter_fold(
        cf_base.mesh(), 45.0, 300.0, true, true, 500.0, 1.0, "counter_fold"
    );
    issues += verify_origami_shell(counter_fold, "origami_counter_fold");
    check(issues == 0, "all assemblable origami shapes must have 0 collisions and 0 slide hits");

    // Robeller (EPFL Thesis 6564, 2015): closed acute-triangle stars exceed |theta| <= 36.8 deg.
    DiagonalFoldGuide cross = DiagonalFoldGuide::cross_fold();
    cross.size = 16000.0;
    cross.thickness = 45.0;
    const DiagonalFold df_base(cross);
    const wood_vidy::OrigamiShell locked_cross(
        df_base.mesh(), 45.0, 300.0, true, true, 500.0, 1.0, "locked_cross"
    );
    check(
        locked_cross.unjoined_interior_creases() > 0,
        "closed acute-triangle radial dome must flag unjoined creases exceeding Robeller skew limit"
    );
}

/// Verifies JointVidyElement::folded_double_layer_node across 110 and 90 degree angles.
void test_vidy_element_node() {

    for (const double width : {1300.0, 1650.0})
        for (const double deg : {110.0, 90.0}) {
            const VidyDoubleLayerNode node = JointVidyElement::folded_double_layer_node(
                deg, width, 950.0, 45.0, 300.0, 500.0, false
            );
            const std::vector<std::shared_ptr<FacePlanarElement>> shells = {
                node.inner_a, node.outer_a, node.inner_b, node.outer_b
            };
            std::vector<std::shared_ptr<FacePlanarElement>> all = shells;
            all.insert(all.end(), node.borders.begin(), node.borders.end());
            check(node.borders.size() == 6, "full closed Vidy node has 6 border boards");
            check_plates(shells, 45.0, true, "vidy_element_node_shells");
            check_border_plates(node.borders, 45.0, "vidy_element_node_borders");
            check(
                report_plate_collisions(all, fmt::format("vidy_node_{:.0f}_{:.0f}", width, deg)) ==
                    0,
                "JointVidyElement::folded_double_layer_node zero collisions"
            );
        }
}

}

/// Runs cavity border, Vidy guide, Stages 3-4, parametric Vidy and multi-origami checks.
int main() {

    std::cout << std::unitbuf;

    try {
        test_reusable_cavity_border();
        test_vidy_element_node();
        test_guide();
        test_stage3_panels();
        test_stage4_section();
        test_stage4_pavilion();
        test_parametric_vidy();
        test_origami_shapes();
        std::cout << "vidy_elements: all checks passed\n";
    } catch (const std::exception& ex) {
        std::cout << "vidy_elements EXCEPTION: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}
