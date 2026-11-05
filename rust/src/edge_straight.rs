use crate::element::{polyline_feature, WoodElement};
use crate::geometry::{
    brep_between_loops, brep_sections, compute_size, compute_up, drill_brep, frame_along,
    is_square_to, profile_section, square_section, sweep_sections,
};
use crate::proto;
use prost::Message;
use session_rust::element::ElementFeature;
use session_rust::{BRep, Element, Line, Mesh, Plane, Point, Polyline, Vector};

/// A straight timber member, a beam or a column: a radius and an up direction per axis segment, or a profile, as wood's EdgeStraightElement.
#[derive(Clone, Debug)]
pub struct EdgeStraightElement {
    pub name: String,                                        // Element name.
    pub axis: Polyline,                                      // Centreline, one segment per span.
    pub radii: Vec<f64>,                                     // Section half-width per segment.
    pub directions: Vec<Vector>, // Section up direction per segment; none takes z.
    pub allowed_type: i32, // Contacts it accepts: 0 crossing, 1 side-to-end and end-to-end, -1 any.
    pub cuts: Vec<Plane>,  // Planes the solid is cut by, kept by the C++ side.
    pub extension: f64,    // Length the solid runs past each axis end: the stock an end joint at the axis end cuts back.
    pub profile: Vec<Polyline>, // Section loops in the profile frame; empty takes the square of the radius.
    pub round_radius: f64, // A round edge's radius, its BRep the exact cylinder; zero for any other section.
    pub solid_features: Vec<proto::InteractionFeatureSolid>, // Features other elements put on it, carried untouched.
    pub plane_features: Vec<proto::InteractionFeaturePlane>, // Planes other elements cut it by, carried untouched.
}

impl EdgeStraightElement {

    /// One profile along the whole axis, the radii its half-width; a direction per segment when given.
    pub fn from_profile(
        axis: &Polyline,
        profile: Vec<Polyline>,
        directions: Vec<Vector>,
        name: &str,
    ) -> Self {
        Self {
            name: name.to_string(),
            axis: axis.clone(),
            radii: vec![compute_size(&profile).0 / 2.0; axis.segment_count()],
            directions,
            allowed_type: -1,
            cuts: Vec::new(),
            extension: 0.0,
            profile,
            round_radius: 0.0,
            solid_features: Vec::new(),
            plane_features: Vec::new(),
        }
    }

    /// A world loop at the axis base as its section, x along the loop's first edge; the loop must be square to the axis.
    pub fn from_section(axis: &Line, section: &Polyline, name: &str) -> Self {
        let along = axis.to_vector().normalized();
        let first = section.get_point(0).unwrap_or_else(|| axis.start());
        let second = section.get_point(1).unwrap_or_else(|| axis.start());
        let x = (second - first).normalized();
        let y = along.cross(&x);
        let ring = section
            .get_points()
            .iter()
            .map(|point| {
                let offset = point.clone() - axis.start();
                Point::new(offset.dot(&x), offset.dot(&y), 0.0)
            })
            .collect();

        Self::from_profile(
            &Polyline::new(vec![axis.start(), axis.end()]),
            vec![Polyline::new(ring)],
            vec![y],
            name,
        )
    }

    /// What a scene saved under the tag Column holds; None for a level section leaning off its axis, an EdgeVariableElement in C++.
    fn from_column(name: &str, column: proto::Column) -> Option<Self> {
        let axis = Line::from_proto(column.axis?);
        let profile: Vec<Polyline> = column
            .profile
            .into_iter()
            .map(Polyline::from_proto)
            .collect();

        let mut edge = if profile.is_empty() {
            let section = Polyline::from_proto(column.section?);
            if !is_square_to(&section, &axis.to_vector()) {
                return None;
            }
            Self::from_section(&axis, &section, name)
        } else {
            Self::from_profile(
                &Polyline::new(vec![axis.start(), axis.end()]),
                profile,
                vec![compute_up(&axis, column.rotation)],
                name,
            )
        };
        edge.cuts = column.cuts.into_iter().map(Plane::from_proto).collect();
        edge.solid_features = column.solid_features;
        edge.plane_features = column.plane_features;

        Some(edge)
    }

    /// The line from the first axis point to the last.
    fn chord(&self) -> Option<Line> {
        let points = self.axis.get_points();

        Some(Line::from_points(points.first()?, points.last()?))
    }

    /// The up direction of one segment, z when the beam has none for it.
    fn up(&self, segment: usize) -> Vector {
        self.directions
            .get(segment)
            .cloned()
            .unwrap_or_else(Vector::z_axis)
    }

    /// The profile's loops at both ends of a one-segment beam with holes, as C++ profile_ends; None otherwise.
    fn profile_ends(&self) -> Option<(Vec<Polyline>, Vec<Polyline>)> {
        if self.profile.is_empty() || self.axis.segment_count() != 1 {
            return None;
        }

        let points = self.axis.get_points();
        let along = points[1].clone() - points[0].clone();
        let up = self.up(0);
        let reach = along.normalized() * self.extension;
        let at = |point: &session_rust::Point| -> Vec<Polyline> {
            self.profile
                .iter()
                .map(|ring| profile_section(point, &along, &up, ring))
                .collect()
        };

        Some((at(&(points[0].clone() - reach.clone())), at(&(points[1].clone() + reach))))
    }

    /// One closed outline per axis vertex, along the bisector at an interior vertex.
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
            let mut station = points[i].clone();

            if i == 0 {
                station = station - along.normalized() * self.extension;
            }

            if i == segments {
                station = station + along.normalized() * self.extension;
            }

            sections.push(match self.profile.first() {
                Some(ring) => profile_section(&station, &along, &up, ring),
                None => square_section(&station, &along, &up, radius),
            });
        }

        sections
    }
}

impl WoodElement for EdgeStraightElement {
    const TYPE: &'static str = "EdgeStraightElement";
    const LEGACY_TYPES: &'static [&'static str] = &["Beam"];
    type Payload = proto::EdgeStraightElement;

    fn from_payload(name: &str, payload: proto::EdgeStraightElement) -> Option<Self> {
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
            extension: payload.extension,
            cuts: payload.cuts.into_iter().map(Plane::from_proto).collect(),
            profile: payload
                .profile
                .into_iter()
                .map(Polyline::from_proto)
                .collect(),
            round_radius: payload.round_radius,
            solid_features: payload.solid_features,
            plane_features: payload.plane_features,
        })
    }

    fn payload(&self) -> proto::EdgeStraightElement {
        proto::EdgeStraightElement {
            solid_features: self.solid_features.clone(),
            plane_features: self.plane_features.clone(),
            axis: Some(self.axis.to_proto()),
            radii: self.radii.clone(),
            directions: self.directions.iter().map(Vector::to_proto).collect(),
            allowed_type: self.allowed_type,
            extension: self.extension,
            round_radius: self.round_radius,
            cuts: self.cuts.iter().map(Plane::to_proto).collect(),
            profile: self.profile.iter().map(Polyline::to_proto).collect(),
        }
    }

    /// Downcast: a Column is read from its own message, field 1 a Line where this class has a Polyline.
    fn from_element(element: &Element) -> Option<Self> {
        let tag = element.element_type_name();

        if tag == "Column" {
            return Self::from_column(
                &element.name,
                proto::Column::decode(element.element_data_dumps()).ok()?,
            );
        }

        if tag != Self::TYPE && !Self::LEGACY_TYPES.contains(&tag) {
            return None;
        }

        Self::from_payload(
            &element.name,
            Self::Payload::decode(element.element_data_dumps()).ok()?,
        )
    }

    fn name(&self) -> &str {
        &self.name
    }

    fn solid(&self) -> Mesh {
        match self.profile_ends() {
            Some((bottom, top)) => Mesh::loft(&bottom, &top, true, true),
            None => sweep_sections(&self.sections()),
        }
    }

    fn brep(&self) -> BRep {
        if let (true, Some(line)) = (self.round_radius > 0.0, self.chord()) {
            return drill_brep(&line, self.round_radius);
        }

        match self.profile_ends() {
            Some((bottom, top)) => brep_between_loops(&bottom, &top),
            None => brep_sections(&self.sections()),
        }
    }

    /// Its axis and a section at every axis vertex.
    fn features(&self) -> Vec<ElementFeature> {
        let mut features = vec![polyline_feature("axis", &self.axis, -1)];

        for ring in self.sections() {
            if ring.point_count() > 0 {
                features.push(polyline_feature("section", &ring, -1));
            }
        }

        features
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
