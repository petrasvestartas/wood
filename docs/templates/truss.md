# Truss {#templates_truss}

[TOC]

`wood_truss::Truss` is a timber truss, a `WoodSession` built from a `TrussGuide`: chords drawn as polylines through their nodes, webs and end posts as lines. The template makes a beam per chord run, web and post and reads every butt from how the lines meet at the nodes.

```cpp
wood_truss::Truss truss(wood_truss::TrussGuide::pratt());
```

| Where the lines meet | Butt |
| --- | --- |
| a chord turns, a kink | the two runs `side_side` on the miter |
| chords end at one node, a heel | `JointBeam::butt` reads the case |
| a chord's end on another chord's run | `top_side` on it |
| a web's or post's end on a run, or where several runs end | `top_side` on each run there |
| a web's or post's end at a run's lone end | `top_side` at the end, the chord running on over it |
| a web's end on another web's middle, a K | `top_side` on it |
| webs and posts ending at one node | sorted by angle, `side_side` with each neighbour, never across the widest gap |

## Constructor

The constructor is the recipe:

```cpp
Truss::Truss(const TrussGuide& guide, const std::string& name)
    : WoodSession(name),
      guide(guide) {

    // input: the guide's lines as drawn
    add_input();

    // members: a beam per chord run and per web
    add_chords();
    add_webs();

    // chords: the runs mitered at kinks, butted at heels
    add_chord_butts();

    // webs and posts: on the chords, mitered with their neighbours at every node
    add_web_butts();
}
```

## Guide

```cpp
TrussGuide(const std::vector<Polyline>& chords, const std::vector<Line>& webs, const std::vector<Line>& posts = {}, double chord_radius = 80.0, double web_radius = 60.0);
```

## Pratt

![templates_truss_1_pratt](templates/templates_truss_1_pratt.png)

\include{lineno} templates_truss_1_pratt.cpp

## Library

Every guide of the library built by the template, `templates_truss_2_library`, each its own scene `truss_<name>.pb`.

### King post

`TrussGuide::king_post()`

![truss_king_post](templates/truss_king_post.png)

### Queen post

`TrussGuide::queen_post()`

![truss_queen_post](templates/truss_queen_post.png)

### Pratt

`TrussGuide::pratt()`

![truss_pratt](templates/truss_pratt.png)

### Howe

`TrussGuide::howe()`

![truss_howe](templates/truss_howe.png)

### Fink

`TrussGuide::fink()`

![truss_fink](templates/truss_fink.png)

### Belgian

`TrussGuide::belgian()`

![truss_belgian](templates/truss_belgian.png)

### Scissors

`TrussGuide::scissors()`

![truss_scissors](templates/truss_scissors.png)

### Warren

`TrussGuide::warren()`

![truss_warren](templates/truss_warren.png)

### Warren with verticals

`TrussGuide::warren_verticals()`

![truss_warren_verticals](templates/truss_warren_verticals.png)

### Pratt bridge

`TrussGuide::pratt_bridge()`

![truss_pratt_bridge](templates/truss_pratt_bridge.png)

### Parker

`TrussGuide::parker()`

![truss_parker](templates/truss_parker.png)

### K-truss

`TrussGuide::k_truss()`

![truss_k_truss](templates/truss_k_truss.png)

### Cambered verticals

`TrussGuide::cambered_verticals()`

![truss_cambered_verticals](templates/truss_cambered_verticals.png)

### Funicular arch

`TrussGuide::funicular_arch()`

![truss_funicular_arch](templates/truss_funicular_arch.png)

### Lens

`TrussGuide::lens()`

![truss_lens](templates/truss_lens.png)

### Triangulated arch

`TrussGuide::triangulated_arch()`

![truss_triangulated_arch](templates/truss_triangulated_arch.png)

### Sloped

`TrussGuide::sloped()`

![truss_sloped](templates/truss_sloped.png)

### Flat Pratt

`TrussGuide::pratt_flat()`

![truss_pratt_flat](templates/truss_pratt_flat.png)

### Flat peaked

`TrussGuide::flat_peaked()`

![truss_flat_peaked](templates/truss_flat_peaked.png)

### Bowstring

`TrussGuide::bowstring()`

![truss_bowstring](templates/truss_bowstring.png)

### Pitched on end posts

`TrussGuide::pitched_posts()`

![truss_pitched_posts](templates/truss_pitched_posts.png)

\include{lineno} templates_truss_2_library.cpp
