//! The solid builders the element classes share, ported from wood_element_geometry.cpp and wood_profile.cpp.

use session_rust::{BRep, Line, Mesh, Plane, Point, Polyline, Tolerance, Vector, Xform};

/// A side strip whose points lie within this of one plane becomes one face; closer points are one.
const COPLANAR: f64 = 1e-6;

/// The point as a vector from the origin.
fn from_origin(point: &Point) -> Vector {
    Vector::new(point[0], point[1], point[2])
}

/// The Newell normal of a loop, its closing point ignored; not normalized.
pub fn compute_newell(points: &[Point]) -> Vector {
    let count = if points.len() > 1
        && points[0].distance(&points[points.len() - 1], None) < Tolerance::APPROXIMATION
    {
        points.len() - 1
    } else {
        points.len()
    };
    let mut normal = Vector::new(0.0, 0.0, 0.0);

    for i in 0..count {
        normal += from_origin(&points[i]).cross(&from_origin(&points[(i + 1) % count]));
    }

    normal
}

/// The frame at origin with z along z and x the part of along across it; none when the two are parallel.
pub fn frame_along(origin: Point, along: Vector, z: Vector) -> Option<Plane> {
    let up = z.normalized();
    let across = along.clone() - up.clone() * along.dot(&up);

    if across.magnitude() < Tolerance::ZERO_TOLERANCE || z.magnitude() < Tolerance::ZERO_TOLERANCE {
        return None;
    }

    let x = across.normalized();

    Some(Plane::from_frame(origin, x.clone(), up.cross(&x), up))
}

/// The side and rise of a section frame along direction, rise as close to up as the direction allows.
fn section_frame(direction: &Vector, up: &Vector) -> (Vector, Vector) {
    let along = direction.normalized();
    let mut rise = if up.is_parallel_to(&along) == 0 {
        up.clone()
    } else {
        Vector::z_axis()
    };

    if rise.is_parallel_to(&along) != 0 {
        rise = Vector::x_axis();
    }

    let side = rise.cross(&along).normalized();

    (side.clone(), along.cross(&side).normalized())
}

/// The closed square of half-width radius at a point, normal to direction, its rise toward up.
pub fn square_section(at: &Point, direction: &Vector, up: &Vector, radius: f64) -> Polyline {
    let (side, rise) = section_frame(direction, up);
    let s = side * radius;
    let u = rise * radius;
    let corner = |a: f64, b: f64| at.clone() + u.clone() * a + s.clone() * b;

    Polyline::new(vec![
        corner(-1.0, -1.0),
        corner(-1.0, 1.0),
        corner(1.0, 1.0),
        corner(1.0, -1.0),
        corner(-1.0, -1.0),
    ])
}

/// A profile loop placed at a point, normal to direction: its x along the side, its y along the rise.
pub fn profile_section(at: &Point, direction: &Vector, up: &Vector, ring: &Polyline) -> Polyline {
    let (side, rise) = section_frame(direction, up);
    let mut points = Vec::new();

    for point in ring.get_points() {
        points.push(at.clone() + side.clone() * point[0] + rise.clone() * point[1]);
    }

    Polyline::new(points)
}

/// The width and depth of a profile's bounding rectangle, the origin always inside.
pub fn compute_size(profile: &[Polyline]) -> (f64, f64) {
    let (mut x0, mut x1, mut y0, mut y1) = (0.0_f64, 0.0_f64, 0.0_f64, 0.0_f64);

    for ring in profile {
        for point in ring.get_points() {
            x0 = x0.min(point[0]);
            x1 = x1.max(point[0]);
            y0 = y0.min(point[1]);
            y1 = y1.max(point[1]);
        }
    }

    (x1 - x0, y1 - y0)
}

/// A counter-clockwise rectangle of width by depth centred on the origin, as wood's profile_rectangle.
pub fn profile_rectangle(width: f64, depth: f64) -> Vec<Polyline> {
    let (x, y) = (width / 2.0, depth / 2.0);
    let corners = [(-x, -y), (x, -y), (x, y), (-x, y), (-x, -y)];

    vec![Polyline::new(
        corners
            .iter()
            .map(|&(a, b)| Point::new(a, b, 0.0))
            .collect(),
    )]
}

/// The points of a section without its closing point.
fn open_points(section: &Polyline) -> Vec<Point> {
    let mut points = section.get_points();

    if section.is_closed() {
        points.pop();
    }

    points
}

/// The closed tube through matching sections, one quad per side per station pair, capped at both ends.
pub fn sweep_sections(sections: &[Polyline]) -> Mesh {
    if sections.len() < 2 {
        return Mesh::new();
    }

    let mut vertices = Vec::new();
    let mut ring = 0;

    for section in sections {
        let points = open_points(section);

        if ring == 0 {
            ring = points.len();
        }

        if points.len() != ring || ring < 3 {
            return Mesh::new();
        }

        vertices.extend(points);
    }

    let mut faces = Vec::new();

    for i in 0..sections.len() - 1 {
        for j in 0..ring {
            let a = i * ring + j;
            let b = i * ring + (j + 1) % ring;
            faces.push(vec![a, b, b + ring, a + ring]);
        }
    }

    let last = (sections.len() - 1) * ring;
    faces.push((0..ring).map(|j| ring - 1 - j).collect());
    faces.push((0..ring).map(|j| last + j).collect());

    Mesh::from_vertices_and_faces(vertices, faces)
}

/// True when every point lies within COPLANAR of the Newell plane through them.
fn is_coplanar(points: &[Point]) -> bool {
    let normal = compute_newell(points);
    let origin = Point::centroid(points);

    points
        .iter()
        .all(|point| ((point.clone() - origin.clone()).dot(&normal)).abs() <= COPLANAR)
}

/// Six times the signed volume enclosed by the faces, positive when they face outwards.
fn compute_signed_volume(vertices: &[Point], faces: &[Vec<usize>]) -> f64 {
    let mut total = 0.0;

    for face in faces {
        for i in 1..face.len().saturating_sub(1) {
            let a = from_origin(&vertices[face[0]]);
            let b = from_origin(&vertices[face[i]]);
            let c = from_origin(&vertices[face[i + 1]]);
            total += a.dot(&b.cross(&c));
        }
    }

    total
}

/// The side faces of the strip under ring side j: one polygon when the strip is planar, else one quad per station pair.
fn add_strip(
    vertices: &[Point],
    stations: usize,
    count: usize,
    j: usize,
    faces: &mut Vec<Vec<usize>>,
) {
    let k = (j + 1) % count;
    let mut strip = vec![j];
    strip.extend((0..stations).map(|i| i * count + k));
    strip.extend((1..stations).rev().map(|i| i * count + j));
    let points: Vec<Point> = strip.iter().map(|&index| vertices[index].clone()).collect();

    if stations == 2 || is_coplanar(&points) {
        faces.push(strip);
        return;
    }

    for i in 0..stations - 1 {
        faces.push(vec![
            i * count + j,
            i * count + k,
            (i + 1) * count + k,
            (i + 1) * count + j,
        ]);
    }
}

/// The face's points with repeats dropped; empty when fewer than three are left.
fn compute_face_points(vertices: &[Point], face: &[usize]) -> Vec<Point> {
    let mut points: Vec<Point> = Vec::new();

    for &index in face {
        if points
            .last()
            .is_none_or(|last| last.distance(&vertices[index], None) > COPLANAR)
        {
            points.push(vertices[index].clone());
        }
    }

    while points.len() > 1 && points[points.len() - 1].distance(&points[0], None) <= COPLANAR {
        points.pop();
    }

    if points.len() < 3 {
        return Vec::new();
    }

    points
}

/// The closed solid lofted through sections of one point count, every planar side strip one face.
pub fn loft_stations(sections: &[Polyline]) -> Mesh {
    let mut rings: Vec<Vec<Point>> = Vec::new();

    for section in sections {
        let points = open_points(section);

        if points.len() < 3
            || rings
                .first()
                .is_some_and(|first| first.len() != points.len())
        {
            return Mesh::new();
        }

        rings.push(points);
    }

    if rings.len() < 2 {
        return Mesh::new();
    }

    let stations = rings.len();
    let count = rings[0].len();
    let vertices: Vec<Point> = rings.into_iter().flatten().collect();
    let mut faces = Vec::new();

    for j in 0..count {
        add_strip(&vertices, stations, count, j, &mut faces);
    }

    faces.push((0..count).map(|j| count - 1 - j).collect());
    faces.push((0..count).map(|j| (stations - 1) * count + j).collect());

    let inward = compute_signed_volume(&vertices, &faces) < 0.0;
    let mut polygons = Vec::new();

    for mut face in faces {
        if inward {
            face.reverse();
        }

        let polygon = compute_face_points(&vertices, &face);

        if !polygon.is_empty() {
            polygons.push(polygon);
        }
    }

    Mesh::from_polylines(polygons, None)
}

/// Facets a circle of radius needs so no chord strays more than tolerance, at least 8.
pub fn circle_segments(radius: f64, tolerance: f64) -> usize {
    let angle = 2.0 * (tolerance / (2.0 * radius)).min(1.0).sqrt().asin();
    let count = (std::f64::consts::PI / angle).ceil();

    (count as usize).max(8)
}

/// A drilled hole as a closed prism of circle_segments sides along the axis.
pub fn drill_mesh(axis: &Line, radius: f64, tolerance: f64) -> Mesh {
    let n = circle_segments(radius, tolerance);
    let plane = Plane::from_point_normal(axis.start(), axis.to_vector(), None);
    let mut ring = Vec::new();

    for k in 0..n {
        let angle = 2.0 * std::f64::consts::PI * k as f64 / n as f64;
        ring.push(
            axis.start()
                + plane.x_axis() * (radius * angle.cos())
                + plane.y_axis() * (radius * angle.sin()),
        );
    }

    ring.push(ring[0].clone());
    let bottom = Polyline::new(ring);
    let top = bottom.translated(&axis.to_vector());

    Mesh::loft(&[bottom], &[top], true, true)
}

/// Every vertex and face of source added to target, in source key order.
pub fn append_mesh(target: &mut Mesh, source: &Mesh) {
    let mut keys: Vec<&usize> = source.vertex.keys().collect();
    keys.sort();
    let mut moved = std::collections::HashMap::new();

    for key in keys {
        moved.insert(*key, target.add_vertex(source.vertex[key].position(), None));
    }

    let mut faces: Vec<&usize> = source.face.keys().collect();
    faces.sort();

    for key in faces {
        let indices: Vec<usize> = source.face[key]
            .iter()
            .map(|vertex| moved[vertex])
            .collect();
        let added = target.add_face(indices, None);

        if let (Some(added), Some(holes)) = (added, source.face_holes.get(key)) {
            let rings = holes
                .iter()
                .map(|ring| ring.iter().map(|vertex| moved[vertex]).collect())
                .collect();
            target.set_face_holes(added, rings);
        }
    }
}

/// A closed quad from segment i of lower to segment i of upper.
fn side_quad(lower: &Polyline, upper: &Polyline, segment: usize) -> Option<Polyline> {
    Some(Polyline::new(vec![
        lower.get_point(segment)?,
        lower.get_point(segment + 1)?,
        upper.get_point(segment + 1)?,
        upper.get_point(segment)?,
        lower.get_point(segment)?,
    ]))
}

/// The solid between bottom and top loops as a BRep, loop 0 the outline, then holes: one planar face per cap and per side quad, as C++ brep_between_loops.
pub fn brep_between_loops(bottom: &[Polyline], top: &[Polyline]) -> BRep {
    if bottom.is_empty() || bottom.len() != top.len() {
        return BRep::new();
    }

    let mut faces = vec![bottom[0].clone(), top[0].clone()];
    let mut holes: Vec<Vec<Polyline>> = vec![bottom[1..].to_vec(), top[1..].to_vec()];

    for (lower, upper) in bottom.iter().zip(top) {
        for segment in 0..lower.point_count().saturating_sub(1) {
            faces.extend(side_quad(lower, upper, segment));
            holes.push(Vec::new());
        }
    }

    BRep::from_polylines(&faces, &holes)
}

/// The sweep through sections as a BRep: the end sections and one quad per segment pair, as C++ brep_sections.
pub fn brep_sections(sections: &[Polyline]) -> BRep {
    if sections.len() < 2 {
        return BRep::new();
    }

    let mut faces = vec![sections[0].clone(), sections[sections.len() - 1].clone()];

    for pair in sections.windows(2) {
        if pair[0].point_count() != pair[1].point_count() {
            return BRep::new();
        }

        for segment in 0..pair[0].point_count().saturating_sub(1) {
            faces.extend(side_quad(&pair[0], &pair[1], segment));
        }
    }

    BRep::from_polylines(&faces, &vec![Vec::new(); faces.len()])
}

/// A face ring of the mesh as a closed polyline.
fn face_ring(mesh: &Mesh, indices: &[usize]) -> Polyline {
    let mut points: Vec<Point> = indices
        .iter()
        .map(|vertex| mesh.vertex[vertex].position())
        .collect();
    points.extend(points.first().cloned());

    Polyline::new(points)
}

/// One planar face per mesh face, its holes kept, in face key order, as C++ mesh_brep.
pub fn mesh_brep(mesh: &Mesh) -> BRep {
    let mut keys: Vec<&usize> = mesh.face.keys().collect();
    keys.sort();
    let mut faces = Vec::new();
    let mut holes = Vec::new();

    for key in keys {
        faces.push(face_ring(mesh, &mesh.face[key]));
        holes.push(
            mesh.face_holes
                .get(key)
                .map(|rings| rings.iter().map(|ring| face_ring(mesh, ring)).collect())
                .unwrap_or_default(),
        );
    }

    BRep::from_polylines(&faces, &holes)
}

/// The index moved past the target's own entries; -1 stays -1.
fn offset(index: &mut i32, by: usize) {
    if *index >= 0 {
        *index += by as i32;
    }
}

/// Every table of source added to target, its indices moved past target's, as C++ append_brep.
pub fn append_brep(target: &mut BRep, mut source: BRep) {
    for edge in &mut source.m_edges {
        offset(&mut edge.curve_3d_index, target.m_curves_3d.len());
        offset(&mut edge.start_vertex, target.m_vertices.len());
        offset(&mut edge.end_vertex, target.m_vertices.len());

        for curve in &mut edge.pcurves {
            offset(&mut curve.surface_index, target.m_surfaces.len());
            offset(&mut curve.curve_2d_index, target.m_curves_2d.len());
            offset(&mut curve.curve_2d_index_2, target.m_curves_2d.len());
        }
    }

    for wire in &mut source.m_wires {
        for reference in &mut wire.edges {
            offset(&mut reference.index, target.m_edges.len());
        }
    }

    for face in &mut source.m_faces {
        offset(&mut face.surface_index, target.m_surfaces.len());

        for reference in &mut face.wires {
            offset(&mut reference.index, target.m_wires.len());
        }
    }

    for shell in &mut source.m_shells {
        for reference in &mut shell.faces {
            offset(&mut reference.index, target.m_faces.len());
        }
    }

    for solid in &mut source.m_solids {
        for reference in &mut solid.shells {
            offset(&mut reference.index, target.m_shells.len());
        }
    }

    target.m_surfaces.append(&mut source.m_surfaces);
    target.m_curves_3d.append(&mut source.m_curves_3d);
    target.m_curves_2d.append(&mut source.m_curves_2d);
    target.m_vertices.append(&mut source.m_vertices);
    target.m_edges.append(&mut source.m_edges);
    target.m_wires.append(&mut source.m_wires);
    target.m_faces.append(&mut source.m_faces);
    target.m_shells.append(&mut source.m_shells);
    target.m_solids.append(&mut source.m_solids);
}

/// A drilled hole as an exact cylinder along the axis, as C++ drill_brep.
pub fn drill_brep(axis: &Line, radius: f64) -> BRep {
    let frame = Plane::from_point_normal(axis.start(), axis.to_vector(), None);
    let place = Xform::frame_to_world(
        &axis.start(),
        &frame.x_axis(),
        &frame.y_axis(),
        &frame.z_axis(),
    );

    BRep::create_cylinder(radius, axis.length()).transformed(&place)
}
