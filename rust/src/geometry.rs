//! The solid builders the element classes share, ported from wood_element_geometry.cpp and wood_profile.cpp.

use session_rust::{Line, Mesh, Plane, Point, Polyline, Tolerance, Vector};

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
