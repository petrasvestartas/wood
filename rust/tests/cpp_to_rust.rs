//! C++ -> Rust: every element a wood example dumped reads back as its class, field for field, with the same solid.

mod common;

use common::*;
use prost::Message;
use wood::{Beam, BeamVariable, Block, Column, Plate, Support, WoodElement};

/// The dumped element downcasts, its payload survives the class unchanged, and the Rust solid equals the C++ one.
fn reads_back<T: WoodElement>(name: &str, built: &T, solid: bool) {
    let element = dumped(name);
    let typed = T::from_element(&element)
        .unwrap_or_else(|| panic!("{name} does not downcast to {}", T::TYPE));
    let stored = T::Payload::decode(element.element_data_dumps()).unwrap();

    assert_eq!(
        typed.payload(),
        stored,
        "{name}: the class changes the payload"
    );
    assert_eq!(typed.name(), element.name);
    assert_eq!(
        built.payload(),
        stored,
        "{name}: the Rust constructor writes another payload than C++"
    );

    if solid {
        let cpp = mesh_of(&element);
        assert!(
            same_solid(&typed.solid(), &cpp, 1e-9),
            "{name}: the Rust solid differs from the C++ one"
        );

        // the BRep is the planar faces of that solid, one per mesh face
        let brep = typed.brep();
        assert_eq!(
            brep.face_count(),
            cpp.number_of_faces(),
            "{name}: BRep faces"
        );
        assert!(
            (brep.volume() - cpp.volume()).abs() <= 1e-9 * cpp.volume().abs().max(1.0),
            "{name}: the Rust BRep encloses another volume than the C++ solid"
        );
    }
}

#[test]
fn plate_reads_back() {
    reads_back::<Plate>("element_plate", &plate(), true);
}

#[test]
fn beam_reads_back() {
    reads_back::<Beam>("element_beam", &beam(), true);
}

#[test]
fn column_reads_back() {
    reads_back::<Column>("element_column", &column(), true);
}

#[test]
fn block_reads_back() {
    reads_back::<Block>("element_block", &block(), true);
}

#[test]
fn beam_variable_reads_back() {
    reads_back::<BeamVariable>("element_beam_variable", &beam_variable(), true);
}

#[test]
fn support_reads_back() {
    reads_back::<Support>("element_support", &support(), false);

    // both exact: as many faces, six of them cylinders (four anchor holes, the rod, the head plate)
    let cpp = brep_of(&dumped("element_support"));
    let rust = support().brep();
    assert_eq!(rust.face_count(), cpp.face_count(), "support faces");
    assert_eq!(cylinders(&rust), 6, "support cylinders");
    assert!(
        (rust.volume() - cpp.volume()).abs() <= 1e-4 * cpp.volume().abs(),
        "support volume {} against C++ {}",
        rust.volume(),
        cpp.volume()
    );
}

/// The faces on a rational surface: the cylinders of a wood element.
fn cylinders(brep: &session_rust::BRep) -> usize {
    brep.m_faces
        .iter()
        .filter(|face| brep.m_surfaces[face.surface_index as usize].is_rational())
        .count()
}

#[test]
fn a_drilled_plate_is_exact() {
    let (half, thickness, radius) = (100.0, 10.0, 7.0);
    let centres = [[-60.0, -60.0], [60.0, -60.0], [60.0, 60.0], [-60.0, 60.0]];
    let plate = wood::geometry::drilled_plate_brep(half, thickness, &centres, radius);
    let volume = (4.0 * half * half - 4.0 * std::f64::consts::PI * radius * radius) * thickness;

    assert_eq!(plate.face_count(), 10, "two caps, four sides, four holes");
    assert_eq!(cylinders(&plate), 4, "four exact holes");
    assert!(plate.is_solid());
    assert!(
        (plate.volume() - volume).abs() <= 1e-3 * volume,
        "plate volume {} against {volume}",
        plate.volume()
    );
}

#[test]
fn every_element_is_written_as_a_brep() {
    assert!(matches!(
        plate().to_element().geometry(),
        session_rust::element::ElementGeometry::BRep(_)
    ));
    assert!(matches!(
        support().to_element().geometry(),
        session_rust::element::ElementGeometry::BRep(_)
    ));
}

#[test]
fn another_type_does_not_downcast() {
    assert!(Plate::from_element(&dumped("element_beam")).is_none());
    assert!(Beam::from_element(&dumped("element_plate")).is_none());
}
