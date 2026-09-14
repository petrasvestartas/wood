from pathlib import Path
import json
import os
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]


class RunnerTest(unittest.TestCase):
    def setUp(self):
        self.folder = tempfile.TemporaryDirectory(prefix="wood runner ")
        self.addCleanup(self.folder.cleanup)
        self.root = Path(self.folder.name)
        for name in ("bash", "tools", "build", "bin"):
            (self.root / name).mkdir()
        shutil.copyfile(ROOT / "bash/cpp.sh", self.root / "bash/cpp.sh")
        (self.root / "build/CMakeCache.txt").touch()
        guard = self.root / "tools/run_guarded.sh"
        guard.write_text(
            "#!/usr/bin/env python3\n"
            "import json, os, subprocess, sys\n"
            "with open(os.environ['WOOD_TEST_LOG'], 'a') as output:\n"
            "    output.write(json.dumps(sys.argv[1:]) + '\\n')\n"
            "sys.exit(subprocess.call(sys.argv[sys.argv.index('--') + 1:]))\n"
        )
        guard.chmod(0o755)
        cmake = self.root / "bin/cmake"
        cmake.write_text("#!/bin/sh\nexit \"${WOOD_TEST_BUILD_EXIT:-0}\"\n")
        cmake.chmod(0o755)
        self.log = self.root / "calls.jsonl"
        self.env = dict(os.environ)
        self.env["PATH"] = str(self.root / "bin") + os.pathsep + self.env["PATH"]
        self.env["WOOD_TEST_LOG"] = str(self.log)

    def executable(self, name, status=0):
        path = self.root / "build" / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(f"#!/bin/sh\nexit {status}\n")
        path.chmod(0o755)

    def run_script(self, *args):
        result = subprocess.run(
            ["bash", str(self.root / "bash/cpp.sh"), *args],
            env=self.env, capture_output=True, text=True, timeout=10
        )
        calls = []
        for line in self.log.read_text().splitlines():
            calls.append(json.loads(line))
        return result.returncode, calls

    def test_default_runs_only_one_dataset_with_guards(self):
        self.executable("main_dataset_runner")
        status, calls = self.run_script()
        self.assertEqual(status, 0)
        self.assertEqual(calls, [
            ["-m", "6", "-n", "wood-build", "--", "cmake", "--build", "build", "--config", "Release", "--parallel", "4", "--target", "main_dataset_runner"],
            ["-n", "wood-solver", "--", "build/main_dataset_runner"],
        ])

    def test_configure_failure_stops_build_and_run(self):
        self.env["WOOD_TEST_BUILD_EXIT"] = "7"
        status, calls = self.run_script("--clean")
        self.assertEqual(status, 7)
        self.assertEqual(len(calls), 1)
        self.assertIn("-S", calls[0])

    def test_build_failure_stops_run(self):
        self.env["WOOD_TEST_BUILD_EXIT"] = "8"
        status, calls = self.run_script()
        self.assertEqual(status, 8)
        self.assertEqual(len(calls), 1)

    def test_run_failure_stops_later_targets(self):
        self.executable("first", 9)
        self.executable("second")
        status, calls = self.run_script("first", "second")
        self.assertEqual(status, 9)
        self.assertEqual(len(calls), 2)
        self.assertEqual(calls[-1][-1], "build/first")

    def test_explicit_targets_run_sequentially(self):
        self.executable("first")
        self.executable("second")
        status, calls = self.run_script("first", "second")
        self.assertEqual(status, 0)
        self.assertEqual(calls[0][-2:], ["first", "second"])
        self.assertEqual(calls[1][-1], "build/first")
        self.assertEqual(calls[2][-1], "build/second")

    def test_release_executable(self):
        self.executable("Release/main_dataset_runner.exe")
        status, calls = self.run_script()
        self.assertEqual(status, 0)
        self.assertEqual(calls[-1][-1], "build/Release/main_dataset_runner.exe")


if __name__ == "__main__":
    unittest.main()
