//! Rust -> C++: a session of one element per class, written here, which wood/tests/rust_elements.cpp loads.
//! WOOD_WRITE_GOLDEN=1 rewrites tests/data/rust_elements.pb; otherwise the committed file must still match.

mod common;

use common::*;
use session_rust::{Element, Session};
use wood::{Beam, BeamVariable, Block, Column, Plate, Support, WoodElement};

const GOLDEN: &str = "tests/data/rust_elements.pb";

/// One element per class, the variable beam and the profile column included.
fn elements() -> Vec<Element> {
    vec![
        plate().to_element(),
        beam().to_element(),
        column().to_element(),
        column_from_profile().to_element(),
        block().to_element(),
        beam_variable().to_element(),
        support().to_element(),
    ]
}

#[test]
fn the_golden_session_holds_one_element_per_class() {
    if std::env::var("WOOD_WRITE_GOLDEN").as_deref() == Ok("1") {
        let mut session = Session::new("rust_elements");

        for element in elements() {
            session.add_element(element, None);
        }

        std::fs::write(GOLDEN, session.pb_dumps()).unwrap();
    }

    let session = Session::pb_loads(&std::fs::read(GOLDEN).unwrap()).unwrap();
    let stored: Vec<Element> = session
        .objects
        .elements
        .iter()
        .map(|element| (**element).clone())
        .collect();
    let built = elements();
    assert_eq!(stored.len(), built.len());

    for (stored, built) in stored.iter().zip(&built) {
        assert_eq!(stored.element_type, built.element_type);
        assert_eq!(
            stored.element_data, built.element_data,
            "{} payload",
            stored.name
        );
        assert!(
            same_solid(&mesh_of(stored), &mesh_of(built), 1e-9),
            "{} solid",
            stored.name
        );
    }

    assert!(Plate::from_element(&stored[0]).is_some());
    assert!(Beam::from_element(&stored[1]).is_some());
    assert!(
        Column::from_element(&stored[2]).is_some() && Column::from_element(&stored[3]).is_some()
    );
    assert!(Block::from_element(&stored[4]).is_some());
    assert!(BeamVariable::from_element(&stored[5]).is_some());
    assert!(Support::from_element(&stored[6]).is_some());
}
