#include "pch.h"
#include "src/templates/plaited/plaited.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_plaited {

namespace {

/// Intersects four side planes with a cap plane; false when any corner fails half-space convexity or winding.
bool intersect_ring(
    const std::array<Plane, 4>& sides,
    const Plane& cap,
    const Vector& outward_normal,
    std::array<Point, 4>& out_corners
) {

    for (size_t k = 0; k < 4; k++) {
        if (!Intersection::plane_plane_plane(sides[(k + 3) % 4], sides[k], cap, out_corners[k]))
            return false;
    }

    for (size_t k = 0; k < 4; k++) {
        if (out_corners[k].distance(out_corners[(k + 1) % 4]) <= 1e-3)
            return false;
        for (size_t m = 0; m < 4; m++) {
            if ((out_corners[k] - sides[m].origin()).dot(sides[m].z_axis()) < -1e-3)
                return false;
        }
    }

    Vector area_vec(0.0, 0.0, 0.0);
    for (size_t k = 0; k < 4; k++)
        area_vec += (out_corners[k] - out_corners[0]).cross(out_corners[(k + 1) % 4] - out_corners[0]);

    return area_vec.dot(outward_normal) > 1e-2;
}

/// True when two half-space planes share practically the same oriented normal and offset.
bool same_half_space(const Plane& a, const Plane& b) {

    if (a.z_axis().dot(b.z_axis()) < 1.0 - 1e-7)
        return false;

    return std::abs((a.origin() - b.origin()).dot(a.z_axis())) < 1e-2;
}

/// Appends candidate to cuts only when it slices a non-trivial protruding portion off block.
void append_effective_cut(NodeBlockElement& block, const Plane& candidate) {

    for (const Plane& existing : block.cuts) {
        if (same_half_space(existing, candidate))
            return;
    }

    const Mesh& current = block.model_geometry_mesh();
    double min_dist = std::numeric_limits<double>::max();
    double max_dist = std::numeric_limits<double>::lowest();

    for (const size_t fk : current.faces()) {
        for (const size_t vk : current.face_vertices(fk).value()) {
            const Point pt = current.vertex_point(vk).value();
            const double d = (pt - candidate.origin()).dot(candidate.z_axis());
            min_dist = std::min(min_dist, d);
            max_dist = std::max(max_dist, d);
        }
    }

    if (min_dist < -1e-2 && max_dist > 1.0) {
        block.cuts.push_back(candidate);
        block.invalidate_geometry();
    }
}

/// Builds the initial uncut hexahedral or stock-box NodeBlockElement for one plaited cell.
std::shared_ptr<NodeBlockElement> make_base_block(const PlaitedGuide& guide, size_t cell_idx) {

    const PlaitedCell& cell = guide.cells()[cell_idx];
    const Plane extrados_cap = Plane::from_point_normal(cell.centre, -cell.normal);
    const Plane intrados_cap =
        Plane::from_point_normal(cell.centre - cell.normal * guide.thickness, cell.normal);
    const std::array<Plane, 4> sides = {
        guide.side_plane(cell_idx, 0),
        guide.side_plane(cell_idx, 1),
        guide.side_plane(cell_idx, 2),
        guide.side_plane(cell_idx, 3)
    };

    std::array<Point, 4> bot_pts;
    std::array<Point, 4> top_pts;
    if (intersect_ring(sides, intrados_cap, cell.normal, bot_pts) &&
        intersect_ring(sides, extrados_cap, cell.normal, top_pts)) {
        const Polyline bottom({bot_pts[0], bot_pts[1], bot_pts[2], bot_pts[3], bot_pts[0]});
        const Polyline top({top_pts[0], top_pts[1], top_pts[2], top_pts[3], top_pts[0]});
        return std::make_shared<NodeBlockElement>(
            std::vector<Polyline>{bottom, top}, fmt::format("block_{}", cell_idx)
        );
    }

    double max_radius = guide.thickness;
    for (const Point& pt : cell.corners)
        max_radius = std::max(max_radius, pt.distance(cell.centre));

    const double span = 3.0 * max_radius + 2.0 * guide.thickness;
    const Plane ref_plane = Plane::from_point_normal(cell.centre, cell.normal);
    const Vector ux = ref_plane.x_axis();
    const Vector uy = ref_plane.y_axis();
    const Point bot_origin =
        cell.centre - cell.normal * guide.thickness - ux * span - uy * span;
    const Polyline bottom =
        Polyline::rectangle(bot_origin, ux, uy, 2.0 * span, 2.0 * span);
    const Polyline top = bottom.translated(cell.normal * guide.thickness);
    std::shared_ptr<NodeBlockElement> block = std::make_shared<NodeBlockElement>(
        std::vector<Polyline>{bottom, top}, fmt::format("block_{}", cell_idx)
    );
    block->cuts = {sides[0], sides[1], sides[2], sides[3]};
    return block;
}

/// Trims subsurface bevel fangs and bar ends flush against convex neighbouring naked faces.
void trim_convex_boundary_fangs(
    const PlaitedGuide& guide,
    size_t cell_idx,
    NodeBlockElement& block
) {

    const std::vector<PlaitedCell>& cells = guide.cells();
    const PlaitedCell& cell = cells[cell_idx];

    for (size_t k = 0; k < 4; k++) {
        const int nbr = guide.neighbor_cell(cell_idx, k);
        if (nbr < 0)
            continue;

        const size_t nbr_idx = static_cast<size_t>(nbr);
        const PlaitedCell& nbr_cell = cells[nbr_idx];
        const size_t u_seam = cell.vertex_ids[k];
        const size_t v_seam = cell.vertex_ids[(k + 1) % 4];

        for (size_t m = 0; m < 4; m++) {
            if (guide.neighbor_cell(nbr_idx, m) >= 0)
                continue;
            const size_t u_naked = nbr_cell.vertex_ids[m];
            const size_t v_naked = nbr_cell.vertex_ids[(m + 1) % 4];
            if (u_naked != u_seam && u_naked != v_seam &&
                v_naked != u_seam && v_naked != v_seam)
                continue;

            const Plane& naked_plane = guide.seam_plane(nbr_idx, m);
            if ((cell.centre - naked_plane.origin()).dot(naked_plane.z_axis()) <= 1e-2)
                continue;

            bool convex = true;
            for (const Point& corner : cell.corners) {
                if ((corner - naked_plane.origin()).dot(naked_plane.z_axis()) < -1e-2)
                    convex = false;
            }
            if (convex)
                append_effective_cut(block, naked_plane);
        }
    }
}

/// True when two consecutive frame segments are collinear and share the same up normal.
bool frame_segments_collinear(const wood_reciprocal::BoundaryFrame& frame, size_t a, size_t b) {

    if (frame.directions[a].is_zero() || frame.directions[b].is_zero())
        return false;

    return frame.directions[a].dot(frame.directions[b]) > 1.0 - 1e-6 &&
           frame.ups[a].dot(frame.ups[b]) > 1.0 - 1e-6;
}

/// Inner shell-facing plane of every naked boundary frame member keyed by undirected mesh edge.
std::map<std::pair<size_t, size_t>, Plane> build_naked_inner_faces(
    const wood_reciprocal::MeshFaces& mesh_faces,
    const wood_reciprocal::BoundaryFrame& frame,
    double beam_width,
    double beam_height
) {

    std::map<std::pair<size_t, size_t>, Plane> faces;
    for (size_t k = 0; k < frame.naked.size(); k++) {
        if (frame.directions[k].is_zero())
            continue;
        const std::pair<int, int>& he = frame.naked[k];
        const size_t u = wood_reciprocal::half_edge_start(mesh_faces.faces, he);
        const size_t v = wood_reciprocal::half_edge_end(mesh_faces.faces, he);
        const Point edge_mid =
            Point::mid_point(mesh_faces.points.at(u), mesh_faces.points.at(v));

        std::vector<Point> face_pts;
        for (const size_t fv : mesh_faces.faces[static_cast<size_t>(he.first)])
            face_pts.push_back(mesh_faces.points.at(fv));
        const Point face_centre = Point::centroid(face_pts);

        Vector towards =
            mesh_faces.normals[static_cast<size_t>(he.first)].cross(frame.directions[k]);
        if ((face_centre - edge_mid).dot(towards) < 0.0)
            towards = -towards;

        const Plane inner = wood_reciprocal::beam_face_towards(
            edge_mid,
            frame.directions[k],
            frame.ups[k],
            beam_width,
            beam_height,
            towards
        );
        faces.emplace(wood_reciprocal::edge_key(u, v), inner);
    }

    return faces;
}

/// Appends a naked frame inner plane to cuts when it opposes out_dir and keeps bar_centre inside.
void append_convex_frame_cut(
    const std::vector<PlaitedCell>& cells,
    const std::map<std::pair<size_t, size_t>, Plane>& naked_inner_faces,
    size_t owner_cell,
    size_t owner_edge,
    const Point& bar_centre,
    const Vector& out_dir,
    std::vector<wood_reciprocal::CutFace>& cuts
) {

    const std::pair<size_t, size_t>& key = cells[owner_cell].mesh_edges[owner_edge];
    if (key.first == SIZE_MAX)
        return;

    const std::map<std::pair<size_t, size_t>, Plane>::const_iterator found =
        naked_inner_faces.find(key);
    if (found == naked_inner_faces.end())
        return;

    if (found->second.z_axis().dot(out_dir) < -0.15 &&
        (bar_centre - found->second.origin()).dot(found->second.z_axis()) > 1e-2)
        cuts.push_back(wood_reciprocal::CutFace{found->second, {}});
}

/// Computes the end cut planes of reciprocal bar cell_idx exiting through local_edge along out_dir.
std::vector<wood_reciprocal::CutFace> beam_end_cuts(
    const PlaitedGuide& guide,
    const std::map<std::pair<size_t, size_t>, Plane>& naked_inner_faces,
    double beam_width,
    bool add_boundary_frame,
    size_t cell_idx,
    size_t local_edge,
    const Vector& out_dir
) {

    const std::vector<PlaitedCell>& cells = guide.cells();
    const std::vector<PlaitedAxis>& axes = guide.axes();
    std::vector<wood_reciprocal::CutFace> cuts;

    const int nbr = guide.neighbor_cell(cell_idx, local_edge);
    if (nbr >= 0) {
        const size_t nbr_idx = static_cast<size_t>(nbr);
        const PlaitedAxis& nbr_axis = axes[nbr_idx];
        Vector side = nbr_axis.direction.cross(nbr_axis.up);
        side = side.is_zero() ? -out_dir : side.normalized();
        if ((axes[cell_idx].centre - nbr_axis.centre).dot(side) < 0.0)
            side = -side;

        const Plane cheek =
            Plane::from_point_normal(nbr_axis.centre + side * (0.5 * beam_width), side);
        cuts.push_back(wood_reciprocal::CutFace{cheek, {}});

        if (add_boundary_frame) {
            for (size_t m = 0; m < 4; m++) {
                append_convex_frame_cut(
                    cells, naked_inner_faces, cell_idx, m, axes[cell_idx].centre, out_dir, cuts
                );
                append_convex_frame_cut(
                    cells, naked_inner_faces, nbr_idx, m, axes[cell_idx].centre, out_dir, cuts
                );
            }
        }
        return cuts;
    }

    if (add_boundary_frame) {
        const std::pair<size_t, size_t>& edge_key = cells[cell_idx].mesh_edges[local_edge];
        if (edge_key.first != SIZE_MAX) {
            const std::map<std::pair<size_t, size_t>, Plane>::const_iterator found =
                naked_inner_faces.find(edge_key);
            if (found != naked_inner_faces.end())
                cuts.push_back(wood_reciprocal::CutFace{found->second, {}});
        }
        for (const size_t cheek_edge : {size_t{0}, size_t{2}}) {
            append_convex_frame_cut(
                cells, naked_inner_faces, cell_idx, cheek_edge, axes[cell_idx].centre, out_dir, cuts
            );
        }
    }

    if (cuts.empty())
        cuts.push_back(wood_reciprocal::CutFace{guide.side_plane(cell_idx, local_edge), {}});

    return cuts;
}

} // namespace

// ═══════════════════════════════════════════════════════════════════════════
// PlaitedStereotomy
// ═══════════════════════════════════════════════════════════════════════════

PlaitedStereotomy::PlaitedStereotomy(const PlaitedGuide& guide, bool with_contacts)
    : WoodSession("plaited_stereotomy"),
      guide(guide),
      with_contacts(with_contacts) {

    // voussoirs: one planar-cut NodeBlockElement per plaited cell
    _blocks = compute_blocks();
    draw();

    // seam contacts across all shared bevel planes
    if (with_contacts)
        compute_face_contacts(0);
}

PlaitedStereotomy::PlaitedStereotomy(
    const Mesh& mesh,
    PlaitedPattern pattern,
    double thickness,
    double bevel,
    double relief_depth,
    bool flip_chirality,
    bool bevel_naked_edges,
    bool with_contacts,
    double opening,
    bool flat_base
)
    : PlaitedStereotomy(
          PlaitedGuide(
              mesh,
              pattern,
              thickness,
              bevel,
              relief_depth,
              flip_chirality,
              bevel_naked_edges,
              opening,
              flat_base
          ),
          with_contacts
      ) {
}

const std::vector<std::shared_ptr<NodeBlockElement>>& PlaitedStereotomy::blocks() const {
    return _blocks;
}

std::vector<std::shared_ptr<NodeBlockElement>> PlaitedStereotomy::compute_blocks() const {

    const size_t count = guide.cells().size();
    std::vector<std::shared_ptr<NodeBlockElement>> blocks;
    blocks.reserve(count);

    for (size_t cell_idx = 0; cell_idx < count; cell_idx++) {
        std::shared_ptr<NodeBlockElement> block = make_base_block(guide, cell_idx);

        if (guide.relief_depth > 0.0) {
            for (const size_t k : {size_t{1}, size_t{3}}) {
                if (guide.neighbor_cell(cell_idx, k) >= 0 || guide.bevel_naked_edges)
                    append_effective_cut(*block, guide.relief_plane(cell_idx, k));
            }
        }

        if (!guide.bevel_naked_edges)
            trim_convex_boundary_fangs(guide, cell_idx, *block);

        if (guide.base_plane().has_value())
            append_effective_cut(*block, *guide.base_plane());

        blocks.push_back(block);
    }

    return blocks;
}

void PlaitedStereotomy::draw() {

    add_mesh(guide.mesh, group_named("mesh"));

    const std::shared_ptr<TreeNode> cell_group = group_named("cells");
    for (const PlaitedCell& cell : guide.cells()) {
        const Polyline outline({
            cell.corners[0],
            cell.corners[1],
            cell.corners[2],
            cell.corners[3],
            cell.corners[0]
        });
        add_polyline(outline, cell_group);
    }

    const std::shared_ptr<TreeNode> axis_group = group_named("axes");
    for (const PlaitedAxis& axis : guide.axes())
        add_line(axis.line, axis_group);

    const std::shared_ptr<TreeNode> block_group = group_named("blocks");
    for (const std::shared_ptr<NodeBlockElement>& block : _blocks)
        add(block, block_group);
}

// ═══════════════════════════════════════════════════════════════════════════
// PlaitedReciprocal
// ═══════════════════════════════════════════════════════════════════════════

PlaitedReciprocal::PlaitedReciprocal(
    const PlaitedGuide& guide,
    double beam_width,
    double beam_height,
    bool add_boundary_frame,
    double pin_diameter,
    wood_reciprocal::CornerJoint corner_joint,
    bool with_contacts
)
    : WoodSession("plaited_reciprocal"),
      guide(guide),
      beam_width(std::max(1.0, beam_width)),
      beam_height(beam_height > 0.0 ? beam_height : guide.thickness),
      add_boundary_frame(add_boundary_frame),
      pin_diameter(std::max(0.0, pin_diameter)),
      corner_joint(corner_joint),
      with_contacts(with_contacts) {

    // radial perimeter frame and reciprocal bars
    _mesh_faces = wood_reciprocal::MeshFaces::of(guide.mesh);
    if (this->add_boundary_frame)
        _frame_beams = compute_frame_beams();
    _beams = compute_beams();
    draw();

    // T-butt contacts and centred cylindrical pins
    if (this->with_contacts || this->pin_diameter > 0.0)
        compute_face_contacts(0);
    if (this->pin_diameter > 0.0)
        _pins = compute_pins();
}

const std::vector<std::shared_ptr<FacePlanarElement>>& PlaitedReciprocal::beams() const {
    return _beams;
}

const std::vector<std::shared_ptr<FacePlanarElement>>& PlaitedReciprocal::frame_beams() const {
    return _frame_beams;
}

const std::vector<std::shared_ptr<JointBeamElement>>& PlaitedReciprocal::pins() const {
    return _pins;
}

std::vector<std::shared_ptr<FacePlanarElement>> PlaitedReciprocal::compute_frame_beams() {

    const std::map<std::pair<size_t, size_t>, Vector> boundary_ups =
        wood_reciprocal::owner_normal_ups(guide.mesh);
    const std::map<std::pair<size_t, size_t>, int> priority =
        wood_reciprocal::through_side_priority(guide.mesh, 45.0);

    _frame = wood_reciprocal::boundary_frame(
        _mesh_faces.faces,
        _mesh_faces.owners,
        _mesh_faces.points,
        _mesh_faces.normals,
        beam_width,
        beam_height,
        boundary_ups,
        corner_joint,
        priority
    );

    const std::vector<std::pair<size_t, size_t>> ranges =
        wood_reciprocal::loop_ranges(_frame.naked, _mesh_faces.faces);
    std::vector<std::shared_ptr<FacePlanarElement>> elements;

    for (const std::pair<size_t, size_t>& range : ranges) {
        const size_t first = range.first;
        const size_t count = range.second - range.first;
        if (count == 0)
            continue;

        size_t start_offset = 0;
        if (wood_reciprocal::loop_closed(_frame.naked, _mesh_faces.faces, range)) {
            for (size_t s = 0; s < count; s++) {
                if (!frame_segments_collinear(_frame, first + (s + count - 1) % count, first + s)) {
                    start_offset = s;
                    break;
                }
            }
        }

        size_t pos = 0;
        while (pos < count) {
            const size_t idx_start = first + (start_offset + pos) % count;
            size_t run_len = 1;
            while (pos + run_len < count &&
                   frame_segments_collinear(
                       _frame,
                       first + (start_offset + pos + run_len - 1) % count,
                       first + (start_offset + pos + run_len) % count
                   ))
                run_len++;

            const size_t idx_end = first + (start_offset + pos + run_len - 1) % count;
            pos += run_len;
            if (_frame.directions[idx_start].is_zero())
                continue;

            const Point& pt_start = _mesh_faces.points.at(
                wood_reciprocal::half_edge_start(_mesh_faces.faces, _frame.naked[idx_start])
            );
            const Point& pt_end = _mesh_faces.points.at(
                wood_reciprocal::half_edge_end(_mesh_faces.faces, _frame.naked[idx_end])
            );
            const wood_reciprocal::ReciprocalBeam beam = wood_reciprocal::cut_beam(
                pt_start,
                pt_end,
                _frame.directions[idx_start],
                _frame.ups[idx_start],
                beam_width,
                beam_height,
                wood_reciprocal::unbounded(_frame.cut_from[idx_start]),
                wood_reciprocal::unbounded(_frame.cut_to[idx_end])
            );
            elements.push_back(std::make_shared<FacePlanarElement>(
                beam.bottom, beam.top, fmt::format("frame_{}", elements.size())
            ));
        }
    }

    return elements;
}

std::vector<std::shared_ptr<FacePlanarElement>> PlaitedReciprocal::compute_beams() const {

    const std::map<std::pair<size_t, size_t>, Plane> naked_inner_faces =
        add_boundary_frame
            ? build_naked_inner_faces(_mesh_faces, _frame, beam_width, beam_height)
            : std::map<std::pair<size_t, size_t>, Plane>{};

    const std::vector<PlaitedAxis>& axes = guide.axes();
    std::vector<std::shared_ptr<FacePlanarElement>> beams;
    beams.reserve(axes.size());

    for (size_t cell_idx = 0; cell_idx < axes.size(); cell_idx++) {
        const PlaitedAxis& axis = axes[cell_idx];
        const std::vector<wood_reciprocal::CutFace> cuts_from = beam_end_cuts(
            guide, naked_inner_faces, beam_width, add_boundary_frame, cell_idx, 3, -axis.direction
        );
        const std::vector<wood_reciprocal::CutFace> cuts_to = beam_end_cuts(
            guide, naked_inner_faces, beam_width, add_boundary_frame, cell_idx, 1, axis.direction
        );
        const wood_reciprocal::ReciprocalBeam beam = wood_reciprocal::cut_beam(
            axis.line.start(),
            axis.line.end(),
            axis.direction,
            axis.up,
            beam_width,
            beam_height,
            cuts_from,
            cuts_to
        );
        beams.push_back(std::make_shared<FacePlanarElement>(
            beam.bottom, beam.top, fmt::format("beam_{}", cell_idx)
        ));
    }

    return beams;
}

std::vector<std::shared_ptr<JointBeamElement>> PlaitedReciprocal::compute_pins() {

    const double radius = 0.5 * pin_diameter;
    const double pin_length = 1.4 * beam_width;
    const double inset = 0.22 * std::min(beam_width, beam_height);
    const std::shared_ptr<TreeNode> pin_group = group_named("pins");

    std::vector<std::pair<std::shared_ptr<Element>, std::shared_ptr<Element>>> pairs;
    for (size_t beam_idx = 0; beam_idx < _beams.size(); beam_idx++) {
        for (const size_t local_edge : {size_t{1}, size_t{3}}) {
            const int nbr = guide.neighbor_cell(beam_idx, local_edge);
            if (nbr >= 0) {
                pairs.emplace_back(_beams[beam_idx], _beams[static_cast<size_t>(nbr)]);
            } else {
                for (const std::shared_ptr<FacePlanarElement>& frame_member : _frame_beams)
                    pairs.emplace_back(_beams[beam_idx], frame_member);
            }
        }
    }

    std::vector<std::shared_ptr<JointBeamElement>> pins;
    for (const std::pair<std::shared_ptr<Element>, std::shared_ptr<Element>>& pair : pairs) {
        const std::shared_ptr<InteractionContactFace> contact =
            compute_face_contact(pair.first, pair.second);
        if (!contact)
            continue;
        const std::shared_ptr<JointBeamElement> pin = JointBeamElement::centred_pins(
            *pair.first,
            *pair.second,
            *contact,
            radius,
            pin_length,
            inset
        );
        if (!pin)
            continue;
        pin->name = fmt::format("pins_{}", pins.size());
        add(pin, pin_group);
        add_interaction(pin, pair.first, pin->interaction(0));
        add_interaction(pin, pair.second, pin->interaction(1));
        pins.push_back(pin);
    }

    return pins;
}

void PlaitedReciprocal::draw() {

    add_mesh(guide.mesh, group_named("mesh"));

    const std::shared_ptr<TreeNode> axis_group = group_named("axes");
    for (const PlaitedAxis& axis : guide.axes())
        add_line(axis.line, axis_group);

    if (!_frame_beams.empty()) {
        const std::shared_ptr<TreeNode> frame_group = group_named("frame");
        for (const std::shared_ptr<FacePlanarElement>& member : _frame_beams)
            add(member, frame_group);
    }

    const std::shared_ptr<TreeNode> beam_group = group_named("beams");
    for (const std::shared_ptr<FacePlanarElement>& member : _beams)
        add(member, beam_group);
}

} // namespace wood_plaited
