"""Write the compas_tf floor model to wood/data/floor_model.pb.

    .venv/bin/python model_to_pb.py                    # the shipped baked model
    .venv/bin/python model_to_pb.py <model.json> <out.pb>

Straight from the COMPAS model: every element keeps its identity, its group, and
- for plates and columns - the structure wood needs to solve joinery. This
replaced step_to_pb.py, which went through STEP and could only ever emit meshes.

Needs the .venv sitting next to this file:
    uv venv && uv pip install --python .venv/bin/python \
        session_py compas compas_model compas_manifold -e ../../../compas_tf
"""

import sys
from pathlib import Path

from compas.data import json_load

from compas_tf.session import write_session

HERE = Path(__file__).resolve().parent
DEFAULT_MODEL = HERE.parents[2] / "compas_tf" / "data" / "cantilevers_baked_model.json"
DEFAULT_OUT = HERE.parents[1] / "data" / "floor_model.pb"


def main(argv):
    model_path = Path(argv[1]) if len(argv) > 1 else DEFAULT_MODEL
    out = Path(argv[2]) if len(argv) > 2 else DEFAULT_OUT

    if not model_path.exists():
        sys.exit("no model at {}".format(model_path))

    print("loading {}".format(model_path.name))
    write_session(json_load(str(model_path)), out, name=model_path.stem)
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
