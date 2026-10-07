//! BeamVariable: closed sections of one point count lofted from station to station.

use crate::element::WoodElement;
use crate::geometry::{frame_along, loft_stations, mesh_brep};
use crate::proto;
use session_rust::{BRep, Line, Mesh, Plane, Point, Polyline};

/// A beam whose section changes along a straight axis, as wood's BeamVariable.
#[derive(Clone, Debug)]
pub struct BeamVariable {
    pub name: String,                                        // Element name.
    pub axis: Line, // Straight reference line from the first section to the last.
    pub sections: Vec<Polyline>, // Closed rings with one point count, one per station in axis order.
    pub cuts: Vec<Plane>,        // Planes the solid is cut by, kept by the C++ side.
    pub solid_features: Vec<proto::InteractionFeatureSolid>, // Features other elements put on it, carried untouched.
    pub plane_features: Vec<proto::InteractionFeaturePlane>, // Planes other elements cut it by, carried untouched.
}

impl BeamVariable {
    /// A variable beam through its sections along an axis.
    pub fn new(axis: &Line, sections: Vec<Polyline>, name: &str) -> Self {
        Self {
            name: name.to_string(),
            axis: axis.clone(),
            sections,
            cuts: Vec::new(),
            solid_features: Vec::new(),
            plane_features: Vec::new(),
        }
    }

    /// A variable beam through its sections, the axis from the first section's centre to the last's.
    pub fn through(sections: Vec<Polyline>, name: &str) -> Self {
        let first: Point = sections.first().map(Polyline::center).unwrap_or_default();
        let last: Point = sections.last().map(Polyline::center).unwrap_or_default();

        Self::new(&Line::from_points(&first, &last), sections, name)
    }
}

impl WoodElement for BeamVariable {
    const TYPE: &'static str = "BeamVariable";
    type Payload = proto::BeamVariable;

    fn from_payload(name: &str, payload: proto::BeamVariable) -> Option<Self> {
        Some(Self {
            name: name.to_string(),
            axis: Line::from_proto(payload.axis?),
            sections: payload
                .sections
                .into_iter()
                .map(Polyline::from_proto)
                .collect(),
            cuts: payload.cuts.into_iter().map(Plane::from_proto).collect(),
            solid_features: payload.solid_features,
            plane_features: payload.plane_features,
        })
    }

    fn payload(&self) -> proto::BeamVariable {
        proto::BeamVariable {
            axis: Some(self.axis.to_proto()),
            sections: self.sections.iter().map(Polyline::to_proto).collect(),
            cuts: self.cuts.iter().map(Plane::to_proto).collect(),
            solid_features: self.solid_features.clone(),
            plane_features: self.plane_features.clone(),
        }
    }

    fn name(&self) -> &str {
        &self.name
    }

    fn solid(&self) -> Mesh {
        loft_stations(&self.sections)
    }

    fn brep(&self) -> BRep {
        mesh_brep(&self.solid())
    }

    fn base_plane(&self) -> Option<Plane> {
        let first = self.sections.first()?;

        frame_along(
            self.axis.start(),
            first.get_point(1)? - first.get_point(0)?,
            self.axis.to_vector(),
        )
    }
}
