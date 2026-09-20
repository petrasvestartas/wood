# Architecture {#architecture}

## Layers

\htmlonly
<pre class="mermaid">
flowchart TB
    subgraph top [callers]
        EX[examples, templates, tests]
    end
    subgraph orchestration
        S[WoodSession: store + pipeline entry]
        V[wood_view]
        IO[wood_io]
    end
    subgraph algorithms [wood_algorithms - functions, every input by argument]
        CD[contact detection]
        FD[feature detection]
        FC[construction + registry]
        TV[three valence]
        MM[merge]
    end
    subgraph data [wood_interaction - records, no virtuals]
        I[Interaction]
        C[contacts]
        F[features]
    end
    subgraph elements [wood_elements - Element subclasses]
        P[Plate]
        B[Beam]
    end
    K[session kernel]
    EX --&gt; S
    EX --&gt; V
    S --&gt; algorithms
    S --&gt; data
    V --&gt; S
    IO --&gt; S
    algorithms --&gt; data
    algorithms --&gt; elements
    data --&gt; elements
    elements --&gt; K
    ST[Settings] -.-&gt; algorithms
    ST -.-&gt; S
</pre>
\endhtmlonly

- `wood_elements` knows the kernel. `wood_interaction` knows elements. `wood_algorithms` knows both and takes every input by argument, `Settings` included. `wood_session` orchestrates and stores; `wood_view` and `wood_io` sit beside it. Examples and templates sit on top.
- Composition throughout: an interaction is three lists, a contact or a feature is an envelope with a variant. Inheritance only for the element classes, because the kernel registry dispatches on `element_type`.
- One fact once on the edge side, with one deliberate exception: a `FeaturePlate` keeps its pair and its contact next to the edge and the interaction that also hold them. `WoodSession::consistent` checks they agree and the round trip asserts it.

## The review

The review of what was wrong, what was done about it and what remains is on the [main page](index.html).
