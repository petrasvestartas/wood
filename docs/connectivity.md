# Connectivity {#connectivity}

The graph says which elements are connected. A graph edge (a, b) carries no payload: its guid keys the interaction collection, `WoodSession::interactions`, a map from guid to `Interaction`. The edge is the only place the two element guids live; every record below refers to "the first element" (edge v0) and "the second element" (edge v1).

\htmlonly
<pre class="mermaid">
classDiagram
    direction LR
    class WoodSession {
        Settings settings
        map~guid, Interaction~ interactions
        vector~pair~ adjacency
        vector~vector~ three_valence
        add_interaction(a, b) Interaction
        add_contact(a, b, contact) guid
        add_feature(joint) guid
        edge_of(interaction) pair
        consistent() bool
    }
    class Interaction {
        string guid
        vector~InteractionContact~ contacts
        vector~InteractionFeature~ features
        optional~InteractionStructure~ structure
        add_contact(contact) int
        add_feature(feature) int
    }
    class InteractionContact {
        string guid
        variant data
        face() ContactFace*
        axis() ContactAxis*
        cross() ContactCross*
        flipped()
        coincides(other) bool
    }
    class InteractionFeature {
        string guid
        int contact
        variant data
        plate() FeaturePlate*
        beam() FeatureBeam*
    }
    class InteractionStructure
    WoodSession "1" o-- "*" Interaction : keyed by edge guid
    Interaction "1" o-- "*" InteractionContact
    Interaction "1" o-- "*" InteractionFeature
    Interaction "1" o-- "0..1" InteractionStructure
    InteractionFeature ..&gt; InteractionContact : contact index
</pre>
\endhtmlonly

`Interaction` is composition, not a base class: three attributes, each a list or an optional. The envelopes `InteractionContact` and `InteractionFeature` hold exactly one kind at a time, a `std::variant` in C++ and a `oneof` on the wire.

## Contacts

\htmlonly
<pre class="mermaid">
classDiagram
    direction LR
    class InteractionContact {
        string guid
        variant~ContactFace, ContactAxis, ContactCross~ data
    }
    class ContactFace {
        int face_a
        int face_b
        ContactType type
        Polyline polygon
    }
    class ContactAxis {
        Line segment
        double t_a
        double t_b
        int polyline_a
        int segment_a
        int polyline_b
        int segment_b
    }
    class ContactCross {
        array~int,2~ faces_a
        array~int,2~ faces_b
        Polyline polygon
        array~Polyline,2~ lines
        array~Polyline,2~ volumes
    }
    class ContactType {
        &lt;&lt;enumeration&gt;&gt;
        unknown
        side_side
        side_top
        top_top
    }
    InteractionContact --&gt; ContactFace : one of
    InteractionContact --&gt; ContactAxis : one of
    InteractionContact --&gt; ContactCross : one of
    ContactFace --&gt; ContactType
</pre>
\endhtmlonly

- `ContactFace`: two face indices, the class, and `polygon`, the Clipper boolean intersection of the two face outlines, closed, in the first face's plane.
- `ContactAxis`: the closest segment between two polylines (beam axes or plate outlines) and where its ends sit: the parameter and the polyline and segment index on each side.
- `ContactCross`: what cross detection computed: the two side faces of each element, the mid-plane polygon, its two centrelines and the two bounding quads.

## Features

\htmlonly
<pre class="mermaid">
classDiagram
    direction LR
    class InteractionFeature {
        string guid
        int contact
        variant~FeaturePlate, FeatureBeam, FeaturePlateBeam~ data
    }
    class FeaturePlate {
        string guid
        string element_a
        string element_b
        ContactFace contact
        int joint_type
        string name
        array~Line,2~ joint_lines
        array~optional Polyline,4~ joint_volumes
        array~vector Polyline,2~ male_outlines
        array~vector Polyline,2~ female_outlines
        array~vector int,2~ male_fabrication_types
        array~vector int,2~ female_fabrication_types
        int divisions
        double shift
        array~double,3~ scale
        vector~string~ linked_joints
        array~ElementFeature,2~ element_features
    }
    class FeatureBeam {
        int end_type
        array~Polyline,4~ volumes
    }
    class FeaturePlateBeam
    class FabricationType {
        &lt;&lt;enumeration&gt;&gt;
        hole
        edge_insertion
        mill
        drill
        conic
        ...
    }
    InteractionFeature --&gt; FeaturePlate : one of
    InteractionFeature --&gt; FeatureBeam : one of
    InteractionFeature --&gt; FeaturePlateBeam : one of
    FeaturePlate --&gt; FabricationType : one per outline
</pre>
\endhtmlonly

`FeaturePlate` is the joint: the pair (male `element_a`, female `element_b`), the `ContactFace` it was solved from, the variant the joint library built, its parameters, the cut outlines per element per face with a `FabricationType` each, and the two `session_cpp::ElementFeature` handed to the host elements. The solver builds it in place and the interaction stores it whole. The joint library stays one function per variant, one header each; the variants differ by algorithm, not by data, so there is no subclass per joint.

## Where inheritance is used

Only where the kernel forces it: `Plate`, `Beam`, `Column` and `Block` derive from `session_cpp::Element`, because `Session::pb_load` rebuilds them through the kernel's `element_type` registry. Everything on the edge side is data: no virtual method, no base class.

See the `Interaction`, `InteractionContact`, `InteractionFeature` and `WoodSession` classes in the API.
