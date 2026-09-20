# Serialization

Every class has `pb_dumps` / `pb_loads`, and `jsondump` / `jsonload` derived from the same proto message through `wood_serialization`: the proto is the one schema.

```mermaid
flowchart LR
    R[record in memory] -- pb_dumps --> M[wood_proto message bytes]
    M -- pb_loads --> R
    M -- json_of --> J[ordered JSON, proto field names, a type key]
    J -- message_from_json --> M
```

## The file

A scene file is a `wood_proto.WoodSession`. Fields 1 to 7 are `session_proto.Session` field for field, then `interactions` at field 100 and `settings` at 101. A reader that knows only the kernel Session opens the file and drops the two extra fields as unknown; `WoodSession::pb_load` reads all of it.

```mermaid
flowchart TB
    subgraph wood_proto.WoodSession
        direction TB
        F1[1 name]
        F2[2 guid]
        F3[3 objects]
        F4[4 tree]
        F5[5 graph]
        F6[6 bvh_boxes]
        F7[7 xforms]
        F100[100 interactions, in guid order]
        F101[101 settings]
    end
    V[session_viewer, session_py, session_rust] -. read 1..7 .-> F1
    W[WoodSession::pb_load] -. read all .-> F101
```

- Interactions are written as a repeated field in guid order, never a protobuf map, so the bytes are identical across languages.
- The element payload the kernel carries opaquely in `element_data` is the class's own message (`wood_proto.Plate`, `Beam`, `Column`); a payload written in the kernel's JSON by older files is still read, the one hand-written JSON reader left.
- Kernel geometry inside a message (polylines, lines, element features) is nested as the kernel's own message: `mutable_polygon()->ParseFromString(polygon.pb_dumps())`.

## Messages

One `.proto` per class under `src/proto/`, same names without the `wood_` prefix. `interaction_contact.proto` and `interaction_feature.proto` carry a `oneof` for the kinds; `wood_session.proto` is the file format above; `settings.proto` records what the scene was solved with.

Regenerate the committed C++ with:

```bash
tools/regen_proto.sh
```
