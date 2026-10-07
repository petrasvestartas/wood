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
        assert!(
            same_solid(&typed.solid(), &mesh_of(&element), 1e-9),
            "{name}: the Rust solid differs from the C++ one"
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
    // its dump holds the exact BRep, so its mesh is compared in wood/tests/rust_elements.cpp
    reads_back::<Support>("element_support", &support(), false);
}

#[test]
fn another_type_does_not_downcast() {
    assert!(Plate::from_element(&dumped("element_beam")).is_none());
    assert!(Beam::from_element(&dumped("element_plate")).is_none());
}
