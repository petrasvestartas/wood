# JointBeam ts_e_p {#elements_joint_beam_ts_e_p}

[TOC]

Tee joints on a beam ending on the side of another: the plate library's top-side designs, chosen by `settings.joint_parameters[8]`, cut into both beams as solids by `JointBeam::from_contact`.

## Constructors

```cpp
static std::array<std::shared_ptr<Beam>, 2> Beam::pair_tee(double angle_degrees = 90.0, double radius = 75.0, double length = 1200.0);
static std::shared_ptr<JointBeam> from_contact(const Beam& source, const Beam& target, const InteractionContactAxis& contact, double volume_length, double cross_or_side_to_end, int flip_male = 0, const Settings& settings = Settings())
```

## ts_e_p_4: the milled wedge

![ts_e_p_4](elements/element_joint_beam_ts_e_p_4.png)

![ts_e_p_4, the unit box: male pink, female blue](joint_library/ts_ts_e_p_4_unit.png)

`Beam::pair_tee(90.0)` and `Beam::pair_tee(60.0)`: the second beam ends on the middle of the first, square and at 60 degrees, its tenon milled into a mortise through the first; the second is lifted 300.

\include{lineno} elements/element_joint_beam_ts_e_p_4.cpp

## ts_e_p_3: one tenon on a beam

![ts_e_p_3](elements/element_joint_beam_ts_e_p_3.png)

![ts_e_p_3, the unit box: male pink, female blue](joint_library/ts_ts_e_p_4_unit.png)

The plate default, two tenons, would run its mortises through the whole width of the first beam and sever it, so the tee takes the single milled tenon of `ts_e_p_4`.

\include{lineno} elements/element_joint_beam_ts_e_p_3.cpp
