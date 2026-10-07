//! The example inputs both directions of the parity tests build from, and mesh comparison.
#![allow(dead_code)] // each test file uses part of it

use session_rust::element::ElementGeometry;
use session_rust::{BRep, Element, Line, Mesh, Plane, Point, Polyline, Session};
use wood::geometry::profile_rectangle;
use wood::{Beam, BeamVariable, Block, Column, Plate, Support};

/// A point from three numbers.
pub fn p(x: f64, y: f64, z: f64) -> Point {
    Point::new(x, y, z)
}

/// A rectangle at a corner, as Polyline::rectangle along x and y.
pub fn rectangle(corner: Point, width: f64, height: f64) -> Polyline {
    Polyline::new(vec![
        corner.clone(),
        corner.clone() + session_rust::Vector::new(width, 0.0, 0.0),
        corner.clone() + session_rust::Vector::new(width, height, 0.0),
        corner.clone() + session_rust::Vector::new(0.0, height, 0.0),
        corner,
    ])
}

/// The plate of examples/elements/element_plate.cpp.
pub fn plate() -> Plate {
    let bottom = Polyline::new(vec![
        p(0.0, 0.0, 0.0),
        p(600.0, 0.0, 0.0),
        p(500.0, 400.0, 0.0),
        p(0.0, 300.0, 0.0),
        p(0.0, 0.0, 0.0),
    ]);
    Plate::new(
        &bottom,
        &bottom.translated(&session_rust::Vector::new(0.0, 0.0, 40.0)),
        "plate",
    )
}

/// The beam of examples/elements/element_beam.cpp.
pub fn beam() -> Beam {
    let axis = Polyline::new(vec![
        p(0.0, 0.0, 0.0),
        p(600.0, 0.0, 0.0),
        p(1000.0, 300.0, 0.0),
    ]);
    Beam::from_profile(
        &axis,
        vec![rectangle(p(-60.0, -100.0, 0.0), 120.0, 200.0)],
        "beam",
    )
}

/// The column of examples/elements/element_column.cpp.
pub fn column() -> Column {
    Column::new(
        &Line::from_points(&p(0.0, 0.0, 0.0), &p(0.0, 0.0, 3500.0)),
        &rectangle(p(-100.0, -150.0, 0.0), 200.0, 300.0),
        "column",
    )
}

/// The block of examples/elements/element_block.cpp.
pub fn block() -> Block {
    Block::new(
        vec![
            rectangle(p(50.0, 50.0, 0.0), 200.0, 200.0),
            rectangle(p(0.0, 0.0, 250.0), 300.0, 300.0),
        ],
        "block",
    )
}

/// The variable beam of examples/elements/element_beam_variable.cpp.
pub fn beam_variable() -> BeamVariable {
    let length = 3000.0;
    let sections = (0..7)
        .map(|i| {
            let x = length * i as f64 / 6.0;
            let depth = 300.0 + 430.0 * (1.0 - x / length) * (1.0 - x / length);
            Polyline::new(vec![
                p(x, -60.0, 0.0),
                p(x, -60.0, -depth),
                p(x, 60.0, -depth),
                p(x, 60.0, 0.0),
                p(x, -60.0, 0.0),
            ])
        })
        .collect();
    BeamVariable::new(
        &Line::from_points(&p(0.0, 0.0, 0.0), &p(length, 0.0, 0.0)),
        sections,
        "rib",
    )
}

/// The support of examples/elements/element_support.cpp.
pub fn support() -> Support {
    Support::new(&Plane::xy_plane(), "support")
}

/// A column from a profile, the Element Column command's path.
pub fn column_from_profile() -> Column {
    Column::from_profile(
        &Line::from_points(&p(0.0, 0.0, 0.0), &p(0.0, 0.0, 3000.0)),
        profile_rectangle(200.0, 300.0),
        0.0,
        "column",
    )
}

/// The only element of a dump written by a wood example.
pub fn dumped(name: &str) -> Element {
    let session =
        Session::pb_loads(&std::fs::read(format!("tests/data/{name}.pb")).unwrap()).unwrap();
    let element = session.objects.elements.iter().next().unwrap();
    (**element).clone()
}

/// The element's mesh; empty for a BRep or none.
pub fn mesh_of(element: &Element) -> Mesh {
    match element.geometry() {
        ElementGeometry::Mesh(mesh) => mesh.clone(),
        _ => Mesh::new(),
    }
}

/// The element's BRep; empty for a mesh or none.
pub fn brep_of(element: &Element) -> BRep {
    match element.geometry() {
        ElementGeometry::BRep(brep) => brep.clone(),
        _ => BRep::new(),
    }
}

/// True when the two BReps hold the same faces and vertices and enclose the same volume within tolerance.
pub fn same_brep(a: &BRep, b: &BRep, tolerance: f64) -> bool {
    let positions = |brep: &BRep| {
        let mut points: Vec<[f64; 3]> = brep
            .m_vertices
            .iter()
            .map(|v| [v.point[0], v.point[1], v.point[2]])
            .collect();
        points.sort_by(|x, y| x.partial_cmp(y).unwrap());
        points
    };
    let (pa, pb) = (positions(a), positions(b));
    let scale = a.volume().abs().max(1.0);

    a.face_count() == b.face_count()
        && pa.len() == pb.len()
        && pa
            .iter()
            .zip(&pb)
            .all(|(x, y)| (0..3).all(|k| (x[k] - y[k]).abs() <= tolerance))
        && (a.volume() - b.volume()).abs() <= tolerance * scale
}

/// The mesh's vertex positions, sorted.
fn sorted_vertices(mesh: &Mesh) -> Vec<[f64; 3]> {
    let mut points: Vec<[f64; 3]> = mesh.vertex.values().map(|v| [v.x, v.y, v.z]).collect();
    points.sort_by(|a, b| a.partial_cmp(b).unwrap());
    points
}

/// True when the two meshes share their vertices and their volume within tolerance.
pub fn same_solid(a: &Mesh, b: &Mesh, tolerance: f64) -> bool {
    let (va, vb) = (sorted_vertices(a), sorted_vertices(b));
    let scale = a.volume().abs().max(1.0);

    va.len() == vb.len()
        && va
            .iter()
            .zip(&vb)
            .all(|(x, y)| (0..3).all(|k| (x[k] - y[k]).abs() <= tolerance))
        && (a.volume() - b.volume()).abs() <= tolerance * scale
        && a.number_of_faces() == b.number_of_faces()
}
