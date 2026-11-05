mod common;

use common::*;
use wood::{EdgeStraightElement, EdgeVariableElement, FacePlanarElement, NodeBlockElement, NodeSupportElement, WoodElement};

/// The Rust features of the dumped element: same types, faces and outline points as C++ within 1e-9.
fn same_features<T: WoodElement>(name: &str) {
    let element = dumped(name);
    let typed = T::from_element(&element).unwrap();
    let ours = typed.to_element();
    // C++ writes session features (joints, contacts) too; the examples have none
    let theirs = element.features();
    assert_eq!(ours.features().len(), theirs.len(), "{name}: feature count");

    for (mine, cpp) in ours.features().iter().zip(theirs) {
        assert_eq!(
            (mine.feature_type.as_str(), mine.face_index),
            (cpp.feature_type.as_str(), cpp.face_index),
            "{name}"
        );
        assert_eq!(
            mine.outlines.len(),
            cpp.outlines.len(),
            "{name}: {}",
            cpp.feature_type
        );

        for (a, b) in mine.outlines.iter().zip(&cpp.outlines) {
            let (pa, pb) = (a.get_points(), b.get_points());
            assert_eq!(pa.len(), pb.len(), "{name}: {} points", cpp.feature_type);
            assert!(
                pa.iter().zip(&pb).all(|(x, y)| x.distance(y, None) <= 1e-9),
                "{name}: {} moved",
                cpp.feature_type
            );
        }
    }
}

#[test]
fn every_element_writes_the_features_cpp_writes() {
    same_features::<FacePlanarElement>("element_face_planar");
    same_features::<EdgeStraightElement>("element_edge_straight");
    same_features::<EdgeStraightElement>("element_edge_straight_column");
    same_features::<NodeBlockElement>("element_node_block");
    same_features::<EdgeVariableElement>("element_edge_variable");
    same_features::<NodeSupportElement>("element_node_support");
}
