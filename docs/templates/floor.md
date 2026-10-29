# Floor {#templates_floor}

[TOC]

Vaulted timber floor bay supported on four carved columns with a central interlocking oculus ring (`src/templates/floor/`).

<div class="wood-card-grid cols-3">
  <a class="wood-card" href="templates_floor_guide.html">
    <img src="962_section_construction_quads.webp" alt="Floor Guide">
    <span class="wood-card-title">Floor Guide</span>
  </a>
  <a class="wood-card" href="templates_floor_model.html">
    <img src="floor_viewer.webp" alt="Floor Model">
    <span class="wood-card-title">Floor Model</span>
  </a>
</div>

## Examples

```cpp
#include "wood_session.h"
#include "src/templates/floor/floor.h"

int main() {
    const wood_floor::FloorGuide guide({
        session_cpp::Point(-3000.0, -3000.0, 0.0),
        session_cpp::Point( 3000.0, -3000.0, 0.0),
        session_cpp::Point( 3000.0,  3000.0, 0.0),
        session_cpp::Point(-3000.0,  3000.0, 0.0),
    });
    wood_floor::Floor floor(guide);
    floor.compute_breps();
    floor.pb_dump(wood_session::pb_path("live"));
    return 0;
}
```

<div style="display:none">

- @subpage templates_floor_guide
- @subpage templates_floor_model
</div>
