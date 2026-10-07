//! Wood's element classes over the kernel `Element`: each class reads its own `wood_proto` payload from
//! `element_data` under its `element_type` (downcast) and writes them back with its plain solid (upcast).

/// The wood_proto messages, the kernel's own messages taken from session_rust.
pub mod proto {
    include!("proto/wood_proto.rs");
}

pub mod beam;
pub mod beam_variable;
pub mod block;
pub mod column;
pub mod element;
pub mod geometry;
pub mod plate;
pub mod support;

pub use beam::Beam;
pub use beam_variable::BeamVariable;
pub use block::Block;
pub use column::Column;
pub use element::WoodElement;
pub use plate::Plate;
pub use support::Support;
