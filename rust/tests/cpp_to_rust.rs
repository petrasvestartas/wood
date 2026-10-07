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

    // the dump holds C++'s exact BRep: the Rust one has polygonal anchor holes, so only the volume is close
    let cpp = brep_of(&dumped("element_support"));
    let rust = support().brep();
    assert!(cpp.face_count() > 0 && rust.face_count() > 0);
    assert!(
        (rust.volume() - cpp.volume()).abs() <= 2e-3 * cpp.volume().abs(),
        "support volume {} against C++ {}",
        rust.volume(),
        cpp.volume()
    );
}

#[test]
fn every_element_is_written_as_a_brep() {
    assert!(matches!(
        plate().to_element().geometry(),
        session_rust::element::ElementGeometry::BRep(_)
    ));
    assert!(support()
        .brep()
        .m_surfaces
        .iter()
        .any(|surface| surface.is_rational()));
}

#[test]
fn another_type_does_not_downcast() {
    assert!(Plate::from_element(&dumped("element_beam")).is_none());
    assert!(Beam::from_element(&dumped("element_plate")).is_none());
}
