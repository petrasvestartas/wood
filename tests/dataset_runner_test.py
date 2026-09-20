from pathlib import Path
import os
import re
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]


class DatasetRunnerTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.folder = tempfile.TemporaryDirectory(prefix="wood-dataset-test-")
        cls.addClassCleanup(cls.folder.cleanup)
        cls.root = Path(cls.folder.name)
        source = ROOT / "examples/main_all_datasets.cpp"
        names = re.findall(r"(type_\w+)\(\)", source.read_text())
        header = (
            "#include <cstdio>\n#include <cstdlib>\n"
            "static int calls = 0;\n"
            "struct Report { ~Report() { std::printf(\"%d\\n\", calls); } };\n"
            "static Report report;\n"
            "static bool dataset() {\n"
            "    ++calls;\n"
            "    return calls != std::atoi(std::getenv(\"WOOD_TEST_FAIL\"));\n"
            "}\n"
        )
        for name in names:
            header += f"inline bool {name}() {{ return dataset(); }}\n"
        (cls.root / "wood_test.h").write_text(header)
        for name in ("main_all_datasets", "main_dataset_runner"):
            subprocess.run(
                [str(ROOT / "tools/run_guarded.sh"), "-n", "wood-build", "--", os.environ.get("CXX", "c++"), "-std=c++17", "-Wall", "-Wextra", "-Wpedantic", "-I", str(cls.root), str(ROOT / "examples" / (name + ".cpp")), "-o", str(cls.root / name)],
                check=True, timeout=60
            )

    def run_dataset(self, name, failure):
        env = dict(os.environ)
        env["WOOD_TEST_FAIL"] = str(failure)
        output = self.root / "count.txt"
        result = subprocess.run(
            [str(ROOT / "tools/run_guarded.sh"), "-n", "wood-solver", "--", "bash", "-c", '"$1" > "$2"', "bash", str(self.root / name), str(output)],
            env=env, timeout=60
        )
        return result.returncode, int(output.read_text())

    def test_all_success(self):
        self.assertEqual(self.run_dataset("main_all_datasets", 0), (0, 44))

    def test_first_failure_keeps_running_and_fails(self):
        self.assertEqual(self.run_dataset("main_all_datasets", 1), (1, 44))

    def test_last_failure_fails(self):
        self.assertEqual(self.run_dataset("main_all_datasets", 44), (1, 44))

    def test_single_success_runs_one_dataset(self):
        self.assertEqual(self.run_dataset("main_dataset_runner", 0), (0, 1))

    def test_single_failure_fails(self):
        self.assertEqual(self.run_dataset("main_dataset_runner", 1), (1, 1))


if __name__ == "__main__":
    unittest.main()
