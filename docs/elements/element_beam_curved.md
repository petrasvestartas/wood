# BeamCurved {#elements_beam_curved}

[TOC]

A single-curved or double-curved glulam beam (inspired by [GluLamb](https://github.com/tsvilans/glulamb)) swept along a `NurbsCurve` centreline with a `(num_width × num_height)` lamella stack, Rotation Minimizing Frame (RMF) twist interpolation, and an exact 6-patch NURBS `BRep`.

## Constructors

```cpp
BeamCurved(const NurbsCurve& centreline, double width, double height, const std::string& name = "beam_curved");
BeamCurved(const NurbsCurve& centreline, double width, double height, const Vector& up, int num_width = 1, int num_height = 4, int samples = 32, const std::string& name = "beam_curved");
BeamCurved(const NurbsCurve& centreline, double width, double height, const std::vector<Vector>& directions, int num_width = 1, int num_height = 4, int samples = 32, const std::string& name = "beam_curved");

static std::shared_ptr<BeamCurved> single_curved(const NurbsCurve& centreline, double width, double height, int num_height = 6, int samples = 32, const std::string& name = "beam_curved");
static std::shared_ptr<BeamCurved> double_curved(const NurbsCurve& centreline, double width, double height, const std::vector<Vector>& directions = {}, const std::vector<double>& parameters = {}, int num_width = 2, int num_height = 6, int samples = 32, const std::string& name = "beam_curved");
static std::shared_ptr<BeamCurved> from_surface(const NurbsCurve& centreline, const NurbsSurface& surface, double width, double height, int num_width = 2, int num_height = 6, int samples = 32, const std::string& name = "beam_curved");
```

## Parameters

| Parameter | Default | Role |
| --- | --- | --- |
| `centreline` | — | Curved `NurbsCurve` spine in world space (planar for single-curved, 3D space curve for double-curved) |
| `width`, `height` | — | Total glulam cross-section width (`num_width * lamella_width()`) and height (`num_height * lamella_height()`) |
| `directions` | `{}` | Cross-section orientation drivers: empty (in-plane normal for planar curve, RMF for 3D curve), 1 vector (constant reference direction), or `>= 2` guide vectors interpolated via unwrapped twist angles relative to the RMF |
| `num_width`, `num_height` | `1`, `4` | Lamella stack subdivisions across width and height (`lamella_curves()`, `lamella_meshes()`, `lamella_breps()`) |
| `samples` | `32` | Number of cross-section stations along `centreline` |
| `extension`, `cuts` | `0.0`, `{}` | Tangent extension past each end and end/miter cut planes |

## Single-Curved and Double-Curved Glulam

A planar parabolic glulam arch (`140 × 240 mm`, `6` lamella layers of `40 mm` height) trimmed by plumb end cuts alongside a 3D S-curved spatial glulam beam (`2 × 6` lamella stack) with RMF twist interpolation.

![](elements/element_beam_curved.png)

```cpp
const std::shared_ptr<BeamCurved> single_arch = BeamCurved::single_curved(
    NurbsCurve::create_interpolated(arch_pts), 140.0, 240.0, 6, 32, "single_curved_glulam"
);
const std::shared_ptr<BeamCurved> double_arch = BeamCurved::double_curved(
    NurbsCurve::create_interpolated(spatial_pts), 140.0, 240.0, guides, {}, 2, 6, 32, "double_curved_glulam"
);
scene.add(single_arch);
scene.add(double_arch);
```

<details><summary>Example code: elements/element_beam_curved.cpp</summary>

\include{lineno} elements/element_beam_curved.cpp

</details>
