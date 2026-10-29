# Bow 1873 {#templates_truss_bow}

[TOC]

The 340 roof trusses and framed bridges of R. H. Bow, *Economics of Construction in relation to Framed Structures* (London: Spon, 1873), Part I, redrawn from the drawings of its classification tables. Each truss is stored as a graph in `data/templates/truss_bow_1873.json` (nodes in spans from the left support, x along the span and z up; chords as node sequences, webs and posts as node pairs) and built by the one `Truss` template: `TrussGraph::guide` draws the graph at its span with a section per member, and `Truss` makes the members and their butts and laps.

```cpp
const std::vector<wood_truss::TrussGraph> graphs = wood_truss::TrussGraph::library(
    config::session_data_dir() / "templates" / "truss_bow_1873.json");
for (const wood_truss::TrussGraph& graph : graphs) {
    wood_truss::Truss truss(graph.guide(), "truss_" + graph.name);
    truss.pb_dump(pb_path("truss_" + graph.name));
}
```

Each design was redrawn by hand from Bow's printed drawing as an exact construction: the few dimensions that define it read off the print (rise, collar height, camber, panel count), everything else constructed — equal panel points, members meeting at exact intersections, curved ribs on true circles or parabolas, a bar Bow draws through a joint kept as one member, symmetric designs mirrored exactly — and each checked over its scanned drawing. The numbers are Bow's; the page is that of his table. The book is transcribed in the BRG library (`framed-structures-bow`), where every table row links to its scan.

## Scale and sections {#truss_bow_sizing}

Bow gives the span of only a few designs (Nos. 130, 137, 192, 195, 293, 332); his diagrams of forces are drawn to no absolute scale. Every other design is drawn at its number of sub-spans times 8 ft (2.44 m), the sub-span of his stated examples (No. 130: 86 1/4 ft over 12 sub-spans; No. 137: 97 1/2 ft over 12; No. 332: 204 ft over 24); the arched roofs whose sub-spans he calls irregular (Nos. 193, 194, 196, 197) are taken at 100 ft. Each member's section is then sized, per truss, for timber trusses 3 m apart: roofs 1.5 kN/m2 on the top chord, bridges 5 kN/m2 on the deck chord, both x 1.4; a frame analysis with stiff joints gives every member's axial force and bending moment (so the deficient Class II frames are sized for the bending they rely on, and the redundant Class III ones for the share their stiffness takes); square GL24h sections satisfy N/A + M/W within the design strength, struts with a buckling factor, at least 100 mm for chords and 60 mm for webs, one section per chord. The span, the load and the largest forces are stored with each graph; the cards show the span.

## Roof trusses, Class I {#truss_bow_roof_i}

Class I: theoretically perfect frames: with every joint hinged they still cannot distort, and no part can be lengthened or shortened so as to induce stress.

<div class="wood-card-grid cols-4">
  <div class="wood-card">
    <img src="truss_bow_1.webp" alt="Bow No. 1" loading="lazy">
    <span class="wood-card-title">No. 1 · 2.4 m · p. 17</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_2.webp" alt="Bow No. 2" loading="lazy">
    <span class="wood-card-title">No. 2 · 2.4 m · p. 17</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_3.webp" alt="Bow No. 3" loading="lazy">
    <span class="wood-card-title">No. 3 · 4.9 m · p. 17</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_4.webp" alt="Bow No. 4" loading="lazy">
    <span class="wood-card-title">No. 4 · 4.9 m · p. 17</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_5.webp" alt="Bow No. 5" loading="lazy">
    <span class="wood-card-title">No. 5 · 4.9 m · p. 17</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_6.webp" alt="Bow No. 6" loading="lazy">
    <span class="wood-card-title">No. 6 · 4.9 m · p. 17</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_7.webp" alt="Bow No. 7" loading="lazy">
    <span class="wood-card-title">No. 7 · 4.9 m · p. 17</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_8.webp" alt="Bow No. 8" loading="lazy">
    <span class="wood-card-title">No. 8 · 4.9 m · p. 17</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_9.webp" alt="Bow No. 9" loading="lazy">
    <span class="wood-card-title">No. 9 · 4.9 m · p. 17</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_10.webp" alt="Bow No. 10" loading="lazy">
    <span class="wood-card-title">No. 10 · 4.9 m · p. 17</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_11.webp" alt="Bow No. 11" loading="lazy">
    <span class="wood-card-title">No. 11 · 7.3 m · p. 17</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_12.webp" alt="Bow No. 12" loading="lazy">
    <span class="wood-card-title">No. 12 · 7.3 m · p. 17</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_13.webp" alt="Bow No. 13" loading="lazy">
    <span class="wood-card-title">No. 13 · 7.3 m · p. 17</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_14.webp" alt="Bow No. 14" loading="lazy">
    <span class="wood-card-title">No. 14 · 7.3 m · p. 17</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_15.webp" alt="Bow No. 15" loading="lazy">
    <span class="wood-card-title">No. 15 · 7.3 m · p. 17</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_16.webp" alt="Bow No. 16" loading="lazy">
    <span class="wood-card-title">No. 16 · 7.3 m · p. 17</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_17.webp" alt="Bow No. 17" loading="lazy">
    <span class="wood-card-title">No. 17 · 7.3 m · p. 18</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_18.webp" alt="Bow No. 18" loading="lazy">
    <span class="wood-card-title">No. 18 · 7.3 m · p. 18</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_19.webp" alt="Bow No. 19" loading="lazy">
    <span class="wood-card-title">No. 19 · 9.8 m · p. 18</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_20.webp" alt="Bow No. 20" loading="lazy">
    <span class="wood-card-title">No. 20 · 9.8 m · p. 18</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_21.webp" alt="Bow No. 21" loading="lazy">
    <span class="wood-card-title">No. 21 · 9.8 m · p. 18</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_22.webp" alt="Bow No. 22" loading="lazy">
    <span class="wood-card-title">No. 22 · 9.8 m · p. 18</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_23.webp" alt="Bow No. 23" loading="lazy">
    <span class="wood-card-title">No. 23 · 9.8 m · p. 18</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_24.webp" alt="Bow No. 24" loading="lazy">
    <span class="wood-card-title">No. 24 · 9.8 m · p. 18</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_25.webp" alt="Bow No. 25" loading="lazy">
    <span class="wood-card-title">No. 25 · 9.8 m · p. 18</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_26.webp" alt="Bow No. 26" loading="lazy">
    <span class="wood-card-title">No. 26 · 9.8 m · p. 18</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_27.webp" alt="Bow No. 27" loading="lazy">
    <span class="wood-card-title">No. 27 · 9.8 m · p. 18</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_28.webp" alt="Bow No. 28" loading="lazy">
    <span class="wood-card-title">No. 28 · 9.8 m · p. 18</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_29.webp" alt="Bow No. 29" loading="lazy">
    <span class="wood-card-title">No. 29 · 9.8 m · p. 18</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_30.webp" alt="Bow No. 30" loading="lazy">
    <span class="wood-card-title">No. 30 · 9.8 m · p. 18</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_31.webp" alt="Bow No. 31" loading="lazy">
    <span class="wood-card-title">No. 31 · 9.8 m · p. 18</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_32.webp" alt="Bow No. 32" loading="lazy">
    <span class="wood-card-title">No. 32 · 9.8 m · p. 18</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_33.webp" alt="Bow No. 33" loading="lazy">
    <span class="wood-card-title">No. 33 · 9.8 m · p. 19</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_34.webp" alt="Bow No. 34" loading="lazy">
    <span class="wood-card-title">No. 34 · 9.8 m · p. 19</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_35.webp" alt="Bow No. 35" loading="lazy">
    <span class="wood-card-title">No. 35 · 9.8 m · p. 19</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_36.webp" alt="Bow No. 36" loading="lazy">
    <span class="wood-card-title">No. 36 · 9.8 m · p. 19</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_37.webp" alt="Bow No. 37" loading="lazy">
    <span class="wood-card-title">No. 37 · 9.8 m · p. 19</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_38.webp" alt="Bow No. 38" loading="lazy">
    <span class="wood-card-title">No. 38 · 9.8 m · p. 19</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_39.webp" alt="Bow No. 39" loading="lazy">
    <span class="wood-card-title">No. 39 · 9.8 m · p. 19</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_40.webp" alt="Bow No. 40" loading="lazy">
    <span class="wood-card-title">No. 40 · 9.8 m · p. 19</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_41.webp" alt="Bow No. 41" loading="lazy">
    <span class="wood-card-title">No. 41 · 9.8 m · p. 19</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_42.webp" alt="Bow No. 42" loading="lazy">
    <span class="wood-card-title">No. 42 · 9.8 m · p. 19</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_43.webp" alt="Bow No. 43" loading="lazy">
    <span class="wood-card-title">No. 43 · 9.8 m · p. 19</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_44.webp" alt="Bow No. 44" loading="lazy">
    <span class="wood-card-title">No. 44 · 9.8 m · p. 19</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_45.webp" alt="Bow No. 45" loading="lazy">
    <span class="wood-card-title">No. 45 · 9.8 m · p. 19</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_46.webp" alt="Bow No. 46" loading="lazy">
    <span class="wood-card-title">No. 46 · 9.8 m · p. 19</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_47.webp" alt="Bow No. 47" loading="lazy">
    <span class="wood-card-title">No. 47 · 9.8 m · p. 19</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_48.webp" alt="Bow No. 48" loading="lazy">
    <span class="wood-card-title">No. 48 · 9.8 m · p. 20</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_49.webp" alt="Bow No. 49" loading="lazy">
    <span class="wood-card-title">No. 49 · 9.8 m · p. 20</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_50.webp" alt="Bow No. 50" loading="lazy">
    <span class="wood-card-title">No. 50 · 9.8 m · p. 20</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_51.webp" alt="Bow No. 51" loading="lazy">
    <span class="wood-card-title">No. 51 · 9.8 m · p. 20</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_52.webp" alt="Bow No. 52" loading="lazy">
    <span class="wood-card-title">No. 52 · 9.8 m · p. 20</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_53.webp" alt="Bow No. 53" loading="lazy">
    <span class="wood-card-title">No. 53 · 9.8 m · p. 20</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_54.webp" alt="Bow No. 54" loading="lazy">
    <span class="wood-card-title">No. 54 · 9.8 m · p. 20</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_55.webp" alt="Bow No. 55" loading="lazy">
    <span class="wood-card-title">No. 55 · 9.8 m · p. 20</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_56.webp" alt="Bow No. 56" loading="lazy">
    <span class="wood-card-title">No. 56 · 9.8 m · p. 20</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_57.webp" alt="Bow No. 57" loading="lazy">
    <span class="wood-card-title">No. 57 · 9.8 m · p. 20</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_58.webp" alt="Bow No. 58" loading="lazy">
    <span class="wood-card-title">No. 58 · 9.8 m · p. 20</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_59.webp" alt="Bow No. 59" loading="lazy">
    <span class="wood-card-title">No. 59 · 9.8 m · p. 20</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_60.webp" alt="Bow No. 60" loading="lazy">
    <span class="wood-card-title">No. 60 · 9.8 m · p. 20</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_61.webp" alt="Bow No. 61" loading="lazy">
    <span class="wood-card-title">No. 61 · 9.8 m · p. 20</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_62.webp" alt="Bow No. 62" loading="lazy">
    <span class="wood-card-title">No. 62 · 9.8 m · p. 20</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_63.webp" alt="Bow No. 63" loading="lazy">
    <span class="wood-card-title">No. 63 · 12.2 m · p. 21</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_64.webp" alt="Bow No. 64" loading="lazy">
    <span class="wood-card-title">No. 64 · 12.2 m · p. 21</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_65.webp" alt="Bow No. 65" loading="lazy">
    <span class="wood-card-title">No. 65 · 12.2 m · p. 21</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_66.webp" alt="Bow No. 66" loading="lazy">
    <span class="wood-card-title">No. 66 · 12.2 m · p. 21</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_67.webp" alt="Bow No. 67" loading="lazy">
    <span class="wood-card-title">No. 67 · 12.2 m · p. 21</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_68.webp" alt="Bow No. 68" loading="lazy">
    <span class="wood-card-title">No. 68 · 12.2 m · p. 21</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_69.webp" alt="Bow No. 69" loading="lazy">
    <span class="wood-card-title">No. 69 · 12.2 m · p. 21</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_70.webp" alt="Bow No. 70" loading="lazy">
    <span class="wood-card-title">No. 70 · 12.2 m · p. 21</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_71.webp" alt="Bow No. 71" loading="lazy">
    <span class="wood-card-title">No. 71 · 14.6 m · p. 21</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_72.webp" alt="Bow No. 72" loading="lazy">
    <span class="wood-card-title">No. 72 · 14.6 m · p. 21</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_73.webp" alt="Bow No. 73" loading="lazy">
    <span class="wood-card-title">No. 73 · 14.6 m · p. 21</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_74.webp" alt="Bow No. 74" loading="lazy">
    <span class="wood-card-title">No. 74 · 14.6 m · p. 21</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_75.webp" alt="Bow No. 75" loading="lazy">
    <span class="wood-card-title">No. 75 · 14.6 m · p. 21</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_76.webp" alt="Bow No. 76" loading="lazy">
    <span class="wood-card-title">No. 76 · 14.6 m · p. 21</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_77.webp" alt="Bow No. 77" loading="lazy">
    <span class="wood-card-title">No. 77 · 14.6 m · p. 21</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_78.webp" alt="Bow No. 78" loading="lazy">
    <span class="wood-card-title">No. 78 · 14.6 m · p. 21</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_79.webp" alt="Bow No. 79" loading="lazy">
    <span class="wood-card-title">No. 79 · 14.6 m · p. 21</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_80.webp" alt="Bow No. 80" loading="lazy">
    <span class="wood-card-title">No. 80 · 14.6 m · p. 22</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_81.webp" alt="Bow No. 81" loading="lazy">
    <span class="wood-card-title">No. 81 · 14.6 m · p. 22</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_82.webp" alt="Bow No. 82" loading="lazy">
    <span class="wood-card-title">No. 82 · 14.6 m · p. 22</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_83.webp" alt="Bow No. 83" loading="lazy">
    <span class="wood-card-title">No. 83 · 14.6 m · p. 22</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_84.webp" alt="Bow No. 84" loading="lazy">
    <span class="wood-card-title">No. 84 · 14.6 m · p. 22</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_85.webp" alt="Bow No. 85" loading="lazy">
    <span class="wood-card-title">No. 85 · 14.6 m · p. 22</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_86.webp" alt="Bow No. 86" loading="lazy">
    <span class="wood-card-title">No. 86 · 14.6 m · p. 22</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_87.webp" alt="Bow No. 87" loading="lazy">
    <span class="wood-card-title">No. 87 · 14.6 m · p. 22</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_88.webp" alt="Bow No. 88" loading="lazy">
    <span class="wood-card-title">No. 88 · 14.6 m · p. 22</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_89.webp" alt="Bow No. 89" loading="lazy">
    <span class="wood-card-title">No. 89 · 14.6 m · p. 22</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_90.webp" alt="Bow No. 90" loading="lazy">
    <span class="wood-card-title">No. 90 · 14.6 m · p. 22</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_91.webp" alt="Bow No. 91" loading="lazy">
    <span class="wood-card-title">No. 91 · 14.6 m · p. 22</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_92.webp" alt="Bow No. 92" loading="lazy">
    <span class="wood-card-title">No. 92 · 14.6 m · p. 22</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_93.webp" alt="Bow No. 93" loading="lazy">
    <span class="wood-card-title">No. 93 · 14.6 m · p. 22</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_94.webp" alt="Bow No. 94" loading="lazy">
    <span class="wood-card-title">No. 94 · 14.6 m · p. 22</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_95.webp" alt="Bow No. 95" loading="lazy">
    <span class="wood-card-title">No. 95 · 14.6 m · p. 23</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_96.webp" alt="Bow No. 96" loading="lazy">
    <span class="wood-card-title">No. 96 · 14.6 m · p. 23</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_97.webp" alt="Bow No. 97" loading="lazy">
    <span class="wood-card-title">No. 97 · 14.6 m · p. 23</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_98.webp" alt="Bow No. 98" loading="lazy">
    <span class="wood-card-title">No. 98 · 14.6 m · p. 23</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_99.webp" alt="Bow No. 99" loading="lazy">
    <span class="wood-card-title">No. 99 · 17.1 m · p. 23</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_100.webp" alt="Bow No. 100" loading="lazy">
    <span class="wood-card-title">No. 100 · 17.1 m · p. 23</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_101.webp" alt="Bow No. 101" loading="lazy">
    <span class="wood-card-title">No. 101 · 17.1 m · p. 23</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_102.webp" alt="Bow No. 102" loading="lazy">
    <span class="wood-card-title">No. 102 · 17.1 m · p. 23</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_103.webp" alt="Bow No. 103" loading="lazy">
    <span class="wood-card-title">No. 103 · 17.1 m · p. 23</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_104.webp" alt="Bow No. 104" loading="lazy">
    <span class="wood-card-title">No. 104 · 17.1 m · p. 23</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_105.webp" alt="Bow No. 105" loading="lazy">
    <span class="wood-card-title">No. 105 · 17.1 m · p. 23</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_106.webp" alt="Bow No. 106" loading="lazy">
    <span class="wood-card-title">No. 106 · 19.5 m · p. 23</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_107.webp" alt="Bow No. 107" loading="lazy">
    <span class="wood-card-title">No. 107 · 19.5 m · p. 23</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_108.webp" alt="Bow No. 108" loading="lazy">
    <span class="wood-card-title">No. 108 · 19.5 m · p. 23</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_109.webp" alt="Bow No. 109" loading="lazy">
    <span class="wood-card-title">No. 109 · 19.5 m · p. 23</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_110.webp" alt="Bow No. 110" loading="lazy">
    <span class="wood-card-title">No. 110 · 19.5 m · p. 24</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_111.webp" alt="Bow No. 111" loading="lazy">
    <span class="wood-card-title">No. 111 · 19.5 m · p. 24</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_112.webp" alt="Bow No. 112" loading="lazy">
    <span class="wood-card-title">No. 112 · 19.5 m · p. 24</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_113.webp" alt="Bow No. 113" loading="lazy">
    <span class="wood-card-title">No. 113 · 19.5 m · p. 24</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_114.webp" alt="Bow No. 114" loading="lazy">
    <span class="wood-card-title">No. 114 · 19.5 m · p. 24</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_115.webp" alt="Bow No. 115" loading="lazy">
    <span class="wood-card-title">No. 115 · 19.5 m · p. 24</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_116.webp" alt="Bow No. 116" loading="lazy">
    <span class="wood-card-title">No. 116 · 19.5 m · p. 24</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_117.webp" alt="Bow No. 117" loading="lazy">
    <span class="wood-card-title">No. 117 · 19.5 m · p. 24</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_118.webp" alt="Bow No. 118" loading="lazy">
    <span class="wood-card-title">No. 118 · 19.5 m · p. 24</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_119.webp" alt="Bow No. 119" loading="lazy">
    <span class="wood-card-title">No. 119 · 19.5 m · p. 24</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_120.webp" alt="Bow No. 120" loading="lazy">
    <span class="wood-card-title">No. 120 · 19.5 m · p. 24</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_121.webp" alt="Bow No. 121" loading="lazy">
    <span class="wood-card-title">No. 121 · 19.5 m · p. 24</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_122.webp" alt="Bow No. 122" loading="lazy">
    <span class="wood-card-title">No. 122 · 21.9 m · p. 24</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_123.webp" alt="Bow No. 123" loading="lazy">
    <span class="wood-card-title">No. 123 · 24.4 m · p. 24</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_124.webp" alt="Bow No. 124" loading="lazy">
    <span class="wood-card-title">No. 124 · 24.4 m · p. 24</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_125.webp" alt="Bow No. 125" loading="lazy">
    <span class="wood-card-title">No. 125 · 29.3 m · p. 25</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_126.webp" alt="Bow No. 126" loading="lazy">
    <span class="wood-card-title">No. 126 · 29.3 m · p. 25</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_127.webp" alt="Bow No. 127" loading="lazy">
    <span class="wood-card-title">No. 127 · 29.3 m · p. 25</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_128.webp" alt="Bow No. 128" loading="lazy">
    <span class="wood-card-title">No. 128 · 29.3 m · p. 25</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_129.webp" alt="Bow No. 129" loading="lazy">
    <span class="wood-card-title">No. 129 · 29.3 m · p. 25</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_130.webp" alt="Bow No. 130" loading="lazy">
    <span class="wood-card-title">No. 130 · 26.3 m · p. 25</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_131.webp" alt="Bow No. 131" loading="lazy">
    <span class="wood-card-title">No. 131 · 29.3 m · p. 25</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_132.webp" alt="Bow No. 132" loading="lazy">
    <span class="wood-card-title">No. 132 · 29.3 m · p. 25</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_133.webp" alt="Bow No. 133" loading="lazy">
    <span class="wood-card-title">No. 133 · 29.3 m · p. 25</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_134.webp" alt="Bow No. 134" loading="lazy">
    <span class="wood-card-title">No. 134 · 29.3 m · p. 25</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_135.webp" alt="Bow No. 135" loading="lazy">
    <span class="wood-card-title">No. 135 · 29.3 m · p. 25</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_136.webp" alt="Bow No. 136" loading="lazy">
    <span class="wood-card-title">No. 136 · 29.3 m · p. 25</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_137.webp" alt="Bow No. 137" loading="lazy">
    <span class="wood-card-title">No. 137 · 29.7 m · p. 25</span>
  </div>
</div>

## Roof trusses, Class II {#truss_bow_roof_ii}

Class II: a deficiency of parts: with hinged joints they hold their shape only under equilibrating loads, and owe their resistance to distortion to stiff joints and bending of the parts.

<div class="wood-card-grid cols-4">
  <div class="wood-card">
    <img src="truss_bow_138.webp" alt="Bow No. 138" loading="lazy">
    <span class="wood-card-title">No. 138 · 7.3 m · p. 26</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_139.webp" alt="Bow No. 139" loading="lazy">
    <span class="wood-card-title">No. 139 · 7.3 m · p. 26</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_140.webp" alt="Bow No. 140" loading="lazy">
    <span class="wood-card-title">No. 140 · 7.3 m · p. 26</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_141.webp" alt="Bow No. 141" loading="lazy">
    <span class="wood-card-title">No. 141 · 9.8 m · p. 26</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_142.webp" alt="Bow No. 142" loading="lazy">
    <span class="wood-card-title">No. 142 · 9.8 m · p. 26</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_143.webp" alt="Bow No. 143" loading="lazy">
    <span class="wood-card-title">No. 143 · 9.8 m · p. 26</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_144.webp" alt="Bow No. 144" loading="lazy">
    <span class="wood-card-title">No. 144 · 9.8 m · p. 26</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_145.webp" alt="Bow No. 145" loading="lazy">
    <span class="wood-card-title">No. 145 · 9.8 m · p. 26</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_146.webp" alt="Bow No. 146" loading="lazy">
    <span class="wood-card-title">No. 146 · 9.8 m · p. 26</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_147.webp" alt="Bow No. 147" loading="lazy">
    <span class="wood-card-title">No. 147 · 12.2 m · p. 26</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_148.webp" alt="Bow No. 148" loading="lazy">
    <span class="wood-card-title">No. 148 · 12.2 m · p. 26</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_149.webp" alt="Bow No. 149" loading="lazy">
    <span class="wood-card-title">No. 149 · 12.2 m · p. 26</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_150.webp" alt="Bow No. 150" loading="lazy">
    <span class="wood-card-title">No. 150 · 12.2 m · p. 26</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_151.webp" alt="Bow No. 151" loading="lazy">
    <span class="wood-card-title">No. 151 · 12.2 m · p. 26</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_152.webp" alt="Bow No. 152" loading="lazy">
    <span class="wood-card-title">No. 152 · 14.6 m · p. 26</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_153.webp" alt="Bow No. 153" loading="lazy">
    <span class="wood-card-title">No. 153 · 14.6 m · p. 26</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_154.webp" alt="Bow No. 154" loading="lazy">
    <span class="wood-card-title">No. 154 · 14.6 m · p. 27</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_155.webp" alt="Bow No. 155" loading="lazy">
    <span class="wood-card-title">No. 155 · 14.6 m · p. 27</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_156.webp" alt="Bow No. 156" loading="lazy">
    <span class="wood-card-title">No. 156 · 14.6 m · p. 27</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_157.webp" alt="Bow No. 157" loading="lazy">
    <span class="wood-card-title">No. 157 · 14.6 m · p. 27</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_158.webp" alt="Bow No. 158" loading="lazy">
    <span class="wood-card-title">No. 158 · 14.6 m · p. 27</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_159.webp" alt="Bow No. 159" loading="lazy">
    <span class="wood-card-title">No. 159 · 14.6 m · p. 27</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_160.webp" alt="Bow No. 160" loading="lazy">
    <span class="wood-card-title">No. 160 · 17.1 m · p. 27</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_161.webp" alt="Bow No. 161" loading="lazy">
    <span class="wood-card-title">No. 161 · 19.5 m · p. 27</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_162.webp" alt="Bow No. 162" loading="lazy">
    <span class="wood-card-title">No. 162 · 19.5 m · p. 27</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_163.webp" alt="Bow No. 163" loading="lazy">
    <span class="wood-card-title">No. 163 · 24.4 m · p. 27</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_164.webp" alt="Bow No. 164" loading="lazy">
    <span class="wood-card-title">No. 164 · 29.3 m · p. 27</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_165.webp" alt="Bow No. 165" loading="lazy">
    <span class="wood-card-title">No. 165 · 29.3 m · p. 27</span>
  </div>
</div>

## Roof trusses, Class III {#truss_bow_roof_iii}

Class III: a redundancy of parts: no part can be made longer or shorter without inducing stress in others, so the stresses depend on the fitting, elasticity and temperature.

<div class="wood-card-grid cols-4">
  <div class="wood-card">
    <img src="truss_bow_166.webp" alt="Bow No. 166" loading="lazy">
    <span class="wood-card-title">No. 166 · 4.9 m · p. 28</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_167.webp" alt="Bow No. 167" loading="lazy">
    <span class="wood-card-title">No. 167 · 7.3 m · p. 28</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_168.webp" alt="Bow No. 168" loading="lazy">
    <span class="wood-card-title">No. 168 · 7.3 m · p. 28</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_169.webp" alt="Bow No. 169" loading="lazy">
    <span class="wood-card-title">No. 169 · 9.8 m · p. 28</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_170.webp" alt="Bow No. 170" loading="lazy">
    <span class="wood-card-title">No. 170 · 9.8 m · p. 28</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_171.webp" alt="Bow No. 171" loading="lazy">
    <span class="wood-card-title">No. 171 · 9.8 m · p. 28</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_172.webp" alt="Bow No. 172" loading="lazy">
    <span class="wood-card-title">No. 172 · 9.8 m · p. 28</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_173.webp" alt="Bow No. 173" loading="lazy">
    <span class="wood-card-title">No. 173 · 9.8 m · p. 28</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_174.webp" alt="Bow No. 174" loading="lazy">
    <span class="wood-card-title">No. 174 · 9.8 m · p. 28</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_175.webp" alt="Bow No. 175" loading="lazy">
    <span class="wood-card-title">No. 175 · 9.8 m · p. 28</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_176.webp" alt="Bow No. 176" loading="lazy">
    <span class="wood-card-title">No. 176 · 9.8 m · p. 28</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_177.webp" alt="Bow No. 177" loading="lazy">
    <span class="wood-card-title">No. 177 · 9.8 m · p. 28</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_178.webp" alt="Bow No. 178" loading="lazy">
    <span class="wood-card-title">No. 178 · 9.8 m · p. 28</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_179.webp" alt="Bow No. 179" loading="lazy">
    <span class="wood-card-title">No. 179 · 12.2 m · p. 28</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_180.webp" alt="Bow No. 180" loading="lazy">
    <span class="wood-card-title">No. 180 · 12.2 m · p. 29</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_181.webp" alt="Bow No. 181" loading="lazy">
    <span class="wood-card-title">No. 181 · 12.2 m · p. 29</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_182.webp" alt="Bow No. 182" loading="lazy">
    <span class="wood-card-title">No. 182 · 12.2 m · p. 29</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_183.webp" alt="Bow No. 183" loading="lazy">
    <span class="wood-card-title">No. 183 · 12.2 m · p. 29</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_184.webp" alt="Bow No. 184" loading="lazy">
    <span class="wood-card-title">No. 184 · 12.2 m · p. 29</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_185.webp" alt="Bow No. 185" loading="lazy">
    <span class="wood-card-title">No. 185 · 12.2 m · p. 29</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_186.webp" alt="Bow No. 186" loading="lazy">
    <span class="wood-card-title">No. 186 · 14.6 m · p. 29</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_187.webp" alt="Bow No. 187" loading="lazy">
    <span class="wood-card-title">No. 187 · 14.6 m · p. 29</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_188.webp" alt="Bow No. 188" loading="lazy">
    <span class="wood-card-title">No. 188 · 19.5 m · p. 29</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_189.webp" alt="Bow No. 189" loading="lazy">
    <span class="wood-card-title">No. 189 · 19.5 m · p. 29</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_190.webp" alt="Bow No. 190" loading="lazy">
    <span class="wood-card-title">No. 190 · 19.5 m · p. 29</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_191.webp" alt="Bow No. 191" loading="lazy">
    <span class="wood-card-title">No. 191 · 21.9 m · p. 29</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_192.webp" alt="Bow No. 192" loading="lazy">
    <span class="wood-card-title">No. 192 · 40.5 m · p. 30</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_193.webp" alt="Bow No. 193" loading="lazy">
    <span class="wood-card-title">No. 193 · 30.5 m · p. 30</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_194.webp" alt="Bow No. 194" loading="lazy">
    <span class="wood-card-title">No. 194 · 30.5 m · p. 30</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_195.webp" alt="Bow No. 195" loading="lazy">
    <span class="wood-card-title">No. 195 · 64.3 m · p. 30</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_196.webp" alt="Bow No. 196" loading="lazy">
    <span class="wood-card-title">No. 196 · 30.5 m · p. 30</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_197.webp" alt="Bow No. 197" loading="lazy">
    <span class="wood-card-title">No. 197 · 30.5 m · p. 30</span>
  </div>
</div>

## Roof trusses, Class IV {#truss_bow_roof_iv}

Class IV: the defects of both Classes II and III: redundant in one region, deficient in another.

<div class="wood-card-grid cols-4">
  <div class="wood-card">
    <img src="truss_bow_198.webp" alt="Bow No. 198" loading="lazy">
    <span class="wood-card-title">No. 198 · 7.3 m · p. 30</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_199.webp" alt="Bow No. 199" loading="lazy">
    <span class="wood-card-title">No. 199 · 12.2 m · p. 30</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_200.webp" alt="Bow No. 200" loading="lazy">
    <span class="wood-card-title">No. 200 · 12.2 m · p. 30</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_200_half.webp" alt="Bow No. 200½" loading="lazy">
    <span class="wood-card-title">No. 200½ · 14.6 m · p. 30</span>
  </div>
</div>

## Framed bridges, Class I {#truss_bow_bridge_i}

Class I: theoretically perfect frames: with every joint hinged they still cannot distort, and no part can be lengthened or shortened so as to induce stress.

<div class="wood-card-grid cols-4">
  <div class="wood-card">
    <img src="truss_bow_201i.webp" alt="Bow No. 201i" loading="lazy">
    <span class="wood-card-title">No. 201i · 4.9 m · p. 31</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_201iii.webp" alt="Bow No. 201iii" loading="lazy">
    <span class="wood-card-title">No. 201iii · 4.9 m · p. 31</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_202a.webp" alt="Bow No. 202A" loading="lazy">
    <span class="wood-card-title">No. 202A · 4.9 m · p. 31</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_202b.webp" alt="Bow No. 202B" loading="lazy">
    <span class="wood-card-title">No. 202B · 4.9 m · p. 31</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_202c.webp" alt="Bow No. 202C" loading="lazy">
    <span class="wood-card-title">No. 202C · 4.9 m · p. 31</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_203.webp" alt="Bow No. 203" loading="lazy">
    <span class="wood-card-title">No. 203 · 4.9 m · p. 32</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_204.webp" alt="Bow No. 204" loading="lazy">
    <span class="wood-card-title">No. 204 · 7.3 m · p. 32</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_205.webp" alt="Bow No. 205" loading="lazy">
    <span class="wood-card-title">No. 205 · 7.3 m · p. 32</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_206.webp" alt="Bow No. 206" loading="lazy">
    <span class="wood-card-title">No. 206 · 7.3 m · p. 32</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_207.webp" alt="Bow No. 207" loading="lazy">
    <span class="wood-card-title">No. 207 · 7.3 m · p. 32</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_208.webp" alt="Bow No. 208" loading="lazy">
    <span class="wood-card-title">No. 208 · 7.3 m · p. 32</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_209.webp" alt="Bow No. 209" loading="lazy">
    <span class="wood-card-title">No. 209 · 7.3 m · p. 32</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_210.webp" alt="Bow No. 210" loading="lazy">
    <span class="wood-card-title">No. 210 · 7.3 m · p. 32</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_211.webp" alt="Bow No. 211" loading="lazy">
    <span class="wood-card-title">No. 211 · 7.3 m · p. 32</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_212.webp" alt="Bow No. 212" loading="lazy">
    <span class="wood-card-title">No. 212 · 7.3 m · p. 32</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_213.webp" alt="Bow No. 213" loading="lazy">
    <span class="wood-card-title">No. 213 · 9.8 m · p. 32</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_214.webp" alt="Bow No. 214" loading="lazy">
    <span class="wood-card-title">No. 214 · 9.8 m · p. 32</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_215.webp" alt="Bow No. 215" loading="lazy">
    <span class="wood-card-title">No. 215 · 9.8 m · p. 32</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_216.webp" alt="Bow No. 216" loading="lazy">
    <span class="wood-card-title">No. 216 · 9.8 m · p. 32</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_217.webp" alt="Bow No. 217" loading="lazy">
    <span class="wood-card-title">No. 217 · 9.8 m · p. 32</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_218.webp" alt="Bow No. 218" loading="lazy">
    <span class="wood-card-title">No. 218 · 9.8 m · p. 32</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_219.webp" alt="Bow No. 219" loading="lazy">
    <span class="wood-card-title">No. 219 · 9.8 m · p. 32</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_220.webp" alt="Bow No. 220" loading="lazy">
    <span class="wood-card-title">No. 220 · 9.8 m · p. 32</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_221.webp" alt="Bow No. 221" loading="lazy">
    <span class="wood-card-title">No. 221 · 9.8 m · p. 32</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_222.webp" alt="Bow No. 222" loading="lazy">
    <span class="wood-card-title">No. 222 · 9.8 m · p. 32</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_223.webp" alt="Bow No. 223" loading="lazy">
    <span class="wood-card-title">No. 223 · 12.2 m · p. 32</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_224.webp" alt="Bow No. 224" loading="lazy">
    <span class="wood-card-title">No. 224 · 12.2 m · p. 33</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_225.webp" alt="Bow No. 225" loading="lazy">
    <span class="wood-card-title">No. 225 · 12.2 m · p. 33</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_226.webp" alt="Bow No. 226" loading="lazy">
    <span class="wood-card-title">No. 226 · 12.2 m · p. 33</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_227.webp" alt="Bow No. 227" loading="lazy">
    <span class="wood-card-title">No. 227 · 12.2 m · p. 33</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_228.webp" alt="Bow No. 228" loading="lazy">
    <span class="wood-card-title">No. 228 · 12.2 m · p. 33</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_229.webp" alt="Bow No. 229" loading="lazy">
    <span class="wood-card-title">No. 229 · 14.6 m · p. 33</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_230.webp" alt="Bow No. 230" loading="lazy">
    <span class="wood-card-title">No. 230 · 14.6 m · p. 33</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_231.webp" alt="Bow No. 231" loading="lazy">
    <span class="wood-card-title">No. 231 · 14.6 m · p. 33</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_232.webp" alt="Bow No. 232" loading="lazy">
    <span class="wood-card-title">No. 232 · 14.6 m · p. 33</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_233.webp" alt="Bow No. 233" loading="lazy">
    <span class="wood-card-title">No. 233 · 14.6 m · p. 33</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_234.webp" alt="Bow No. 234" loading="lazy">
    <span class="wood-card-title">No. 234 · 14.6 m · p. 33</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_235.webp" alt="Bow No. 235" loading="lazy">
    <span class="wood-card-title">No. 235 · 14.6 m · p. 33</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_236.webp" alt="Bow No. 236" loading="lazy">
    <span class="wood-card-title">No. 236 · 14.6 m · p. 33</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_237.webp" alt="Bow No. 237" loading="lazy">
    <span class="wood-card-title">No. 237 · 14.6 m · p. 33</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_238.webp" alt="Bow No. 238" loading="lazy">
    <span class="wood-card-title">No. 238 · 14.6 m · p. 33</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_239.webp" alt="Bow No. 239" loading="lazy">
    <span class="wood-card-title">No. 239 · 14.6 m · p. 33</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_240.webp" alt="Bow No. 240" loading="lazy">
    <span class="wood-card-title">No. 240 · 14.6 m · p. 33</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_241.webp" alt="Bow No. 241" loading="lazy">
    <span class="wood-card-title">No. 241 · 14.6 m · p. 33</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_242.webp" alt="Bow No. 242" loading="lazy">
    <span class="wood-card-title">No. 242 · 14.6 m · p. 33</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_243.webp" alt="Bow No. 243" loading="lazy">
    <span class="wood-card-title">No. 243 · 14.6 m · p. 33</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_244.webp" alt="Bow No. 244" loading="lazy">
    <span class="wood-card-title">No. 244 · 14.6 m · p. 33</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_245.webp" alt="Bow No. 245" loading="lazy">
    <span class="wood-card-title">No. 245 · 17.1 m · p. 34</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_246.webp" alt="Bow No. 246" loading="lazy">
    <span class="wood-card-title">No. 246 · 17.1 m · p. 34</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_247.webp" alt="Bow No. 247" loading="lazy">
    <span class="wood-card-title">No. 247 · 17.1 m · p. 34</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_248.webp" alt="Bow No. 248" loading="lazy">
    <span class="wood-card-title">No. 248 · 17.1 m · p. 34</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_249.webp" alt="Bow No. 249" loading="lazy">
    <span class="wood-card-title">No. 249 · 19.5 m · p. 34</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_250.webp" alt="Bow No. 250" loading="lazy">
    <span class="wood-card-title">No. 250 · 19.5 m · p. 34</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_251.webp" alt="Bow No. 251" loading="lazy">
    <span class="wood-card-title">No. 251 · 19.5 m · p. 34</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_252.webp" alt="Bow No. 252" loading="lazy">
    <span class="wood-card-title">No. 252 · 19.5 m · p. 34</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_253.webp" alt="Bow No. 253" loading="lazy">
    <span class="wood-card-title">No. 253 · 19.5 m · p. 34</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_254.webp" alt="Bow No. 254" loading="lazy">
    <span class="wood-card-title">No. 254 · 19.5 m · p. 34</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_255.webp" alt="Bow No. 255" loading="lazy">
    <span class="wood-card-title">No. 255 · 19.5 m · p. 34</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_256.webp" alt="Bow No. 256" loading="lazy">
    <span class="wood-card-title">No. 256 · 19.5 m · p. 34</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_257.webp" alt="Bow No. 257" loading="lazy">
    <span class="wood-card-title">No. 257 · 19.5 m · p. 34</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_258.webp" alt="Bow No. 258" loading="lazy">
    <span class="wood-card-title">No. 258 · 19.5 m · p. 34</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_259.webp" alt="Bow No. 259" loading="lazy">
    <span class="wood-card-title">No. 259 · 19.5 m · p. 34</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_260.webp" alt="Bow No. 260" loading="lazy">
    <span class="wood-card-title">No. 260 · 19.5 m · p. 34</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_261.webp" alt="Bow No. 261" loading="lazy">
    <span class="wood-card-title">No. 261 · 19.5 m · p. 34</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_262.webp" alt="Bow No. 262" loading="lazy">
    <span class="wood-card-title">No. 262 · 19.5 m · p. 34</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_263.webp" alt="Bow No. 263" loading="lazy">
    <span class="wood-card-title">No. 263 · 19.5 m · p. 34</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_264.webp" alt="Bow No. 264" loading="lazy">
    <span class="wood-card-title">No. 264 · 21.9 m · p. 35</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_265.webp" alt="Bow No. 265" loading="lazy">
    <span class="wood-card-title">No. 265 · 21.9 m · p. 35</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_266.webp" alt="Bow No. 266" loading="lazy">
    <span class="wood-card-title">No. 266 · 21.9 m · p. 35</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_267.webp" alt="Bow No. 267" loading="lazy">
    <span class="wood-card-title">No. 267 · 21.9 m · p. 35</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_268.webp" alt="Bow No. 268" loading="lazy">
    <span class="wood-card-title">No. 268 · 21.9 m · p. 35</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_269.webp" alt="Bow No. 269" loading="lazy">
    <span class="wood-card-title">No. 269 · 24.4 m · p. 35</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_270.webp" alt="Bow No. 270" loading="lazy">
    <span class="wood-card-title">No. 270 · 24.4 m · p. 35</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_271.webp" alt="Bow No. 271" loading="lazy">
    <span class="wood-card-title">No. 271 · 24.4 m · p. 35</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_272.webp" alt="Bow No. 272" loading="lazy">
    <span class="wood-card-title">No. 272 · 24.4 m · p. 35</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_273.webp" alt="Bow No. 273" loading="lazy">
    <span class="wood-card-title">No. 273 · 24.4 m · p. 35</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_274.webp" alt="Bow No. 274" loading="lazy">
    <span class="wood-card-title">No. 274 · 29.3 m · p. 35</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_275.webp" alt="Bow No. 275" loading="lazy">
    <span class="wood-card-title">No. 275 · 29.3 m · p. 35</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_276.webp" alt="Bow No. 276" loading="lazy">
    <span class="wood-card-title">No. 276 · 29.3 m · p. 35</span>
  </div>
</div>

## Framed bridges, Class II {#truss_bow_bridge_ii}

Class II: a deficiency of parts: with hinged joints they hold their shape only under equilibrating loads, and owe their resistance to distortion to stiff joints and bending of the parts.

<div class="wood-card-grid cols-4">
  <div class="wood-card">
    <img src="truss_bow_277.webp" alt="Bow No. 277" loading="lazy">
    <span class="wood-card-title">No. 277 · 4.9 m · p. 36</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_278.webp" alt="Bow No. 278" loading="lazy">
    <span class="wood-card-title">No. 278 · 7.3 m · p. 36</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_279.webp" alt="Bow No. 279" loading="lazy">
    <span class="wood-card-title">No. 279 · 7.3 m · p. 36</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_280.webp" alt="Bow No. 280" loading="lazy">
    <span class="wood-card-title">No. 280 · 9.8 m · p. 36</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_281.webp" alt="Bow No. 281" loading="lazy">
    <span class="wood-card-title">No. 281 · 12.2 m · p. 36</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_282.webp" alt="Bow No. 282" loading="lazy">
    <span class="wood-card-title">No. 282 · 12.2 m · p. 36</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_283.webp" alt="Bow No. 283" loading="lazy">
    <span class="wood-card-title">No. 283 · 14.6 m · p. 36</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_284.webp" alt="Bow No. 284" loading="lazy">
    <span class="wood-card-title">No. 284 · 17.1 m · p. 36</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_285.webp" alt="Bow No. 285" loading="lazy">
    <span class="wood-card-title">No. 285 · 24.4 m · p. 36</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_286.webp" alt="Bow No. 286" loading="lazy">
    <span class="wood-card-title">No. 286 · 24.4 m · p. 36</span>
  </div>
</div>

## Framed bridges, Class III {#truss_bow_bridge_iii}

Class III: a redundancy of parts: no part can be made longer or shorter without inducing stress in others, so the stresses depend on the fitting, elasticity and temperature.

<div class="wood-card-grid cols-4">
  <div class="wood-card">
    <img src="truss_bow_288.webp" alt="Bow No. 288" loading="lazy">
    <span class="wood-card-title">No. 288 · 4.9 m · p. 37</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_289.webp" alt="Bow No. 289" loading="lazy">
    <span class="wood-card-title">No. 289 · 4.9 m · p. 37</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_290.webp" alt="Bow No. 290" loading="lazy">
    <span class="wood-card-title">No. 290 · 4.9 m · p. 37</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_291.webp" alt="Bow No. 291" loading="lazy">
    <span class="wood-card-title">No. 291 · 7.3 m · p. 37</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_292.webp" alt="Bow No. 292" loading="lazy">
    <span class="wood-card-title">No. 292 · 7.3 m · p. 37</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_293.webp" alt="Bow No. 293" loading="lazy">
    <span class="wood-card-title">No. 293 · 90.2 m · p. 37</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_294.webp" alt="Bow No. 294" loading="lazy">
    <span class="wood-card-title">No. 294 · 9.8 m · p. 37</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_295.webp" alt="Bow No. 295" loading="lazy">
    <span class="wood-card-title">No. 295 · 9.8 m · p. 37</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_296.webp" alt="Bow No. 296" loading="lazy">
    <span class="wood-card-title">No. 296 · 9.8 m · p. 37</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_297.webp" alt="Bow No. 297" loading="lazy">
    <span class="wood-card-title">No. 297 · 9.8 m · p. 37</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_298.webp" alt="Bow No. 298" loading="lazy">
    <span class="wood-card-title">No. 298 · 9.8 m · p. 37</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_299.webp" alt="Bow No. 299" loading="lazy">
    <span class="wood-card-title">No. 299 · 9.8 m · p. 37</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_300.webp" alt="Bow No. 300" loading="lazy">
    <span class="wood-card-title">No. 300 · 9.8 m · p. 37</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_301.webp" alt="Bow No. 301" loading="lazy">
    <span class="wood-card-title">No. 301 · 9.8 m · p. 37</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_302.webp" alt="Bow No. 302" loading="lazy">
    <span class="wood-card-title">No. 302 · 9.8 m · p. 37</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_303.webp" alt="Bow No. 303" loading="lazy">
    <span class="wood-card-title">No. 303 · 12.2 m · p. 37</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_304.webp" alt="Bow No. 304" loading="lazy">
    <span class="wood-card-title">No. 304 · 12.2 m · p. 37</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_305.webp" alt="Bow No. 305" loading="lazy">
    <span class="wood-card-title">No. 305 · 12.2 m · p. 37</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_306.webp" alt="Bow No. 306" loading="lazy">
    <span class="wood-card-title">No. 306 · 12.2 m · p. 37</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_307.webp" alt="Bow No. 307" loading="lazy">
    <span class="wood-card-title">No. 307 · 12.2 m · p. 37</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_308.webp" alt="Bow No. 308" loading="lazy">
    <span class="wood-card-title">No. 308 · 14.6 m · p. 38</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_309.webp" alt="Bow No. 309" loading="lazy">
    <span class="wood-card-title">No. 309 · 14.6 m · p. 38</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_310.webp" alt="Bow No. 310" loading="lazy">
    <span class="wood-card-title">No. 310 · 14.6 m · p. 38</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_311.webp" alt="Bow No. 311" loading="lazy">
    <span class="wood-card-title">No. 311 · 14.6 m · p. 38</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_312.webp" alt="Bow No. 312" loading="lazy">
    <span class="wood-card-title">No. 312 · 14.6 m · p. 38</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_313.webp" alt="Bow No. 313" loading="lazy">
    <span class="wood-card-title">No. 313 · 14.6 m · p. 38</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_314.webp" alt="Bow No. 314" loading="lazy">
    <span class="wood-card-title">No. 314 · 14.6 m · p. 38</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_315.webp" alt="Bow No. 315" loading="lazy">
    <span class="wood-card-title">No. 315 · 14.6 m · p. 38</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_316.webp" alt="Bow No. 316" loading="lazy">
    <span class="wood-card-title">No. 316 · 14.6 m · p. 38</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_317.webp" alt="Bow No. 317" loading="lazy">
    <span class="wood-card-title">No. 317 · 17.1 m · p. 38</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_318.webp" alt="Bow No. 318" loading="lazy">
    <span class="wood-card-title">No. 318 · 17.1 m · p. 38</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_319.webp" alt="Bow No. 319" loading="lazy">
    <span class="wood-card-title">No. 319 · 19.5 m · p. 38</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_320.webp" alt="Bow No. 320" loading="lazy">
    <span class="wood-card-title">No. 320 · 19.5 m · p. 38</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_321.webp" alt="Bow No. 321" loading="lazy">
    <span class="wood-card-title">No. 321 · 19.5 m · p. 38</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_322.webp" alt="Bow No. 322" loading="lazy">
    <span class="wood-card-title">No. 322 · 19.5 m · p. 38</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_323.webp" alt="Bow No. 323" loading="lazy">
    <span class="wood-card-title">No. 323 · 19.5 m · p. 38</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_324.webp" alt="Bow No. 324" loading="lazy">
    <span class="wood-card-title">No. 324 · 19.5 m · p. 38</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_325.webp" alt="Bow No. 325" loading="lazy">
    <span class="wood-card-title">No. 325 · 19.5 m · p. 38</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_326.webp" alt="Bow No. 326" loading="lazy">
    <span class="wood-card-title">No. 326 · 19.5 m · p. 38</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_327.webp" alt="Bow No. 327" loading="lazy">
    <span class="wood-card-title">No. 327 · 21.9 m · p. 39</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_328.webp" alt="Bow No. 328" loading="lazy">
    <span class="wood-card-title">No. 328 · 24.4 m · p. 39</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_329.webp" alt="Bow No. 329" loading="lazy">
    <span class="wood-card-title">No. 329 · 29.3 m · p. 39</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_330.webp" alt="Bow No. 330" loading="lazy">
    <span class="wood-card-title">No. 330 · 29.3 m · p. 39</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_331.webp" alt="Bow No. 331" loading="lazy">
    <span class="wood-card-title">No. 331 · 39.0 m · p. 39</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_332.webp" alt="Bow No. 332" loading="lazy">
    <span class="wood-card-title">No. 332 · 62.2 m · p. 39</span>
  </div>
</div>

## Framed bridges, Class IV {#truss_bow_bridge_iv}

Class IV: the defects of both Classes II and III: redundant in one region, deficient in another.

<div class="wood-card-grid cols-4">
  <div class="wood-card">
    <img src="truss_bow_333.webp" alt="Bow No. 333" loading="lazy">
    <span class="wood-card-title">No. 333 · 14.6 m · p. 40</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_334.webp" alt="Bow No. 334" loading="lazy">
    <span class="wood-card-title">No. 334 · 17.1 m · p. 40</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_335.webp" alt="Bow No. 335" loading="lazy">
    <span class="wood-card-title">No. 335 · 21.9 m · p. 40</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_336.webp" alt="Bow No. 336" loading="lazy">
    <span class="wood-card-title">No. 336 · 21.9 m · p. 40</span>
  </div>
  <div class="wood-card">
    <img src="truss_bow_337.webp" alt="Bow No. 337" loading="lazy">
    <span class="wood-card-title">No. 337 · 21.9 m · p. 40</span>
  </div>
</div>
