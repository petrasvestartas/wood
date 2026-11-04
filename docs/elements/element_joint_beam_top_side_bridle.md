# Beam Top-to-Side Bridle {#elements_joint_beam_top_side_bridle}

[TOC]

A skew T-bridle: the second beam ends in a fork of two cheeks around a tongue left in the first, both beams flush, the fork sliding in square to the first beam from its near face, selected via `settings.joint_parameters[8]` and cut by `JointBeam::from_contact`.

## Constructors

```cpp
static std::shared_ptr<JointPlate> JointPlate::top_side_bridle(double shoulder = 0.3, double tongue = 1.0 / 3.0);
static std::shared_ptr<JointBeam> JointBeam::from_contact(const EdgeStraightElement& source, const EdgeStraightElement& target, const InteractionContactAxis& contact, double volume_length, double cross_or_side_to_end, int flip_male = 0, const Settings& settings = Settings());
```

- `tongue`: the thickness left in the first beam, as a share of the height. The two cheeks share the rest.
- `shoulder`: the length of the shoulder at the acute corner, as a share of the first beam's width; on beams it is `settings.joint_parameters[7]`.

The walls of the pocket are square to the first beam at any angle, read from the second beam's lean. The second beam must reach the first beam's far face: `extension` lengthens a beam at both ends, so its stock has to allow for that.

## Skew Bridle
A pair at `57°`, 100 x 100, rebuilt from photographs of a made joint and drawn apart: the tongue with its sawn near face, the pocket's square wall and shoulder and the fork with the notch for the shoulder's tooth.

![](elements/element_joint_beam_top_side_bridle.png)

\include{lineno} elements/element_joint_beam_top_side_bridle.cpp
