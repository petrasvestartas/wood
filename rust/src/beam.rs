//! Beam: a section swept along a polyline axis, mitred at every interior vertex.

use crate::element::WoodElement;
use crate::geometry::{compute_size, frame_along, profile_section, square_section, sweep_sections};
use crate::proto;
use session_rust::{Mesh, Plane, Polyline, Vector};

/// A timber beam: a radius and an up direction per axis segment, or a profile, as wood's Beam.
#[derive(Clone, Debug)]
pub struct Beam {
    pub name: String,                                        // Element name.
    pub axis: Polyline,                                      // Centreline, one segment per span.
    pub radii: Vec<f64>,                                     // Section half-width per segment.
    pub directions: Vec<Vector>, // Section up direction per segment; none takes z.
    pub allowed_type: i32, // Contacts it accepts: 0 crossing, 1 side-to-end and end-to-end, -1 any.
    pub cuts: Vec<Plane>,  // Planes the solid is cut by, kept by the C++ side.
    pub profile: Vec<Polyline>, // Section loops in the profile frame; empty takes the square of the radius.
    pub solid_features: Vec<proto::InteractionFeatureSolid>, // Features other elements put on it, carried untouched.
    pub plane_features: Vec<proto::InteractionFeaturePlane>, // Planes other elements cut it by, carried untouched.
}

impl Beam {
    /// A beam of one profile along its whole axis, the radii its half-width.
    pub fn from_profile(axis: &Polyline, profile: Vec<Polyline>, name: &str) -> Self {
        Self {
            name: name.to_string(),
            axis: axis.clone(),
            radii: vec![compute_size(&profile).0 / 2.0; axis.segment_count()],
            directions: Vec::new(),
            allowed_type: -1,
            cuts: Vec::new(),
            profile,
            solid_features: Vec::new(),
            plane_features: Vec::new(),
        }
    }

    /// The up direction of one segment, z when the beam has none for it.
    fn up(&self, segment: usize) -> Vector {
        self.directions
            .get(segment)
            .cloned()
            .unwrap_or_else(Vector::z_axis)
    }

    /// One closed outline per axis vertex, along the bisector at an interior vertex; empty without a radius.
    pub fn sections(&self) -> Vec<Polyline> {
        let segments = self.axis.segment_count();

        if segments < 1 || self.radii.is_empty() {
            return Vec::new();
        }

        let points = self.axis.get_points();
        let mut sections = Vec::new();

        for i in 0..=segments {
            let segment = i.min(segments - 1);
            let mut along = points[segment + 1].clone() - points[segment].clone();

            if i > 0 && i < segments {
                along =
                    along.normalized() + (points[i].clone() - points[i - 1].clone()).normalized();
            }

            let up = self.up(segment);
            let radius = self.radii.get(segment).copied().unwrap_or(0.0);
            sections.push(match self.profile.first() {
                Some(ring) => profile_section(&points[i], &along, &up, ring),
                None => square_section(&points[i], &along, &up, radius),
            });
        }

        sections
    }
}

impl WoodElement for Beam {
    const TYPE: &'static str = "Beam";
    type Payload = proto::Beam;

    fn from_payload(name: &str, payload: proto::Beam) -> Option<Self> {
        Some(Self {
            name: name.to_string(),
            axis: Polyline::from_proto(payload.axis?),
            radii: payload.radii,
            directions: payload
                .directions
                .into_iter()
                .map(Vector::from_proto)
                .collect(),
            allowed_type: payload.allowed_type,
            cuts: payload.cuts.into_iter().map(Plane::from_proto).collect(),
            profile: payload
                .profile
                .into_iter()
                .map(Polyline::from_proto)
                .collect(),
            solid_features: payload.solid_features,
            plane_features: payload.plane_features,
        })
    }

    fn payload(&self) -> proto::Beam {
        proto::Beam {
            solid_features: self.solid_features.clone(),
            plane_features: self.plane_features.clone(),
            axis: Some(self.axis.to_proto()),
            radii: self.radii.clone(),
            directions: self.directions.iter().map(Vector::to_proto).collect(),
            allowed_type: self.allowed_type,
            cuts: self.cuts.iter().map(Plane::to_proto).collect(),
            profile: self.profile.iter().map(Polyline::to_proto).collect(),
        }
    }

    fn name(&self) -> &str {
        &self.name
    }

    fn solid(&self) -> Mesh {
        if self.profile.len() > 1 && self.axis.segment_count() == 1 {
            let (start, end) = (
                self.axis.get_points()[0].clone(),
                self.axis.get_points()[1].clone(),
            );
            let along = end.clone() - start.clone();
            let up = self.up(0);
            let bottom: Vec<Polyline> = self
                .profile
                .iter()
                .map(|ring| profile_section(&start, &along, &up, ring))
                .collect();
            let top: Vec<Polyline> = self
                .profile
                .iter()
                .map(|ring| profile_section(&end, &along, &up, ring))
                .collect();
            return Mesh::loft(&bottom, &top, true, true);
        }

        sweep_sections(&self.sections())
    }

    fn base_plane(&self) -> Option<Plane> {
        let rings = self.sections();
        let ring = rings.first()?;

        frame_along(
            self.axis.get_point(0)?,
            ring.get_point(1)? - ring.get_point(0)?,
            self.axis.get_point(1)? - self.axis.get_point(0)?,
        )
    }
}
