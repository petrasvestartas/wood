#!/usr/bin/env bash
# Regenerates generated/*.pb.{h,cc} from src/proto/*.proto with the protoc the kernel pins (36.2).
set -euo pipefail
cd "$(dirname "$0")/.."
PROTOC="${WOOD_PROTOC:-}"
if [ -z "$PROTOC" ]; then
    for cand in \
        ../laika/laika_cpp/.protoc/36.2/bin/protoc \
        "$HOME/.cache/laika-deps/protobuf-36.2/bin/protoc" \
        "$HOME/.cache/session-deps/protobuf-36.2/bin/protoc"; do
        if [ -x "$cand" ]; then
            PROTOC="$cand"
            break
        fi
    done
fi
"${PROTOC:?set WOOD_PROTOC to protoc 36.2}" --cpp_out=generated --proto_path=src/proto --proto_path=../laika/laika_proto src/proto/*.proto
