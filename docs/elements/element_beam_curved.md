# Curved Beam {#elements_beam_curved}

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

## Single-Curved and Double-Curved Glulam

A planar parabolic glulam arch (`140 × 240 mm`, `6` lamella layers of `40 mm` height) trimmed by plumb end cuts alongside a 3D S-curved spatial glulam beam (`2 × 6` lamella stack) with RMF twist interpolation.

![](elements/element_beam_curved.png)

\include{lineno} elements/element_beam_curved.cpp
