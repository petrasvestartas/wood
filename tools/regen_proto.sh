#!/usr/bin/env bash
# Regenerates generated/*.pb.{h,cc} from src/proto/*.proto with the protoc the kernel pins (36.2).
set -euo pipefail
cd "$(dirname "$0")/.."
PROTOC="${WOOD_PROTOC:-../session/session_cpp/.protoc/36.2/bin/protoc}"
"$PROTOC" --cpp_out=generated --proto_path=src/proto --proto_path=../session/session_proto src/proto/*.proto
