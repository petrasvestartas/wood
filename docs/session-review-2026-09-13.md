# Wood review — 2026-09-13

Applied the requested session-reviewer rules to wood's assignment helpers, dataset
loaders and runners, linked joints, beam geometry, and their verification paths. Cross-language parity and session quicktests
do not apply to this C++ consumer. This report covers the changes below; it is not
a complete audit of every solver and template function.

The three excluded examples (`1_io.cpp`, `2_contact_detection.cpp`, and
`3_joint_detection.cpp`) are byte-for-byte unchanged. All 13 files recorded as
protected or already modified at the start retain their original content. Existing
staged deletions were preserved. No session kernel or viewer source was edited.

## Assignment

parity : Public signatures and slot conventions are preserved in
`src/joinery_solver/wood_assign.h:10` and `:19`.

safety : `src/joinery_solver/wood_assign.cpp:72` now populates the R-tree by reference,
avoiding an owning raw-pointer tree copy when named return-value optimization is
disabled. Elements without points are skipped before inserting invalid bounds.
At `:150`, insertion assignment now expands its broad-phase range to cover the
same tolerance accepted by the segment-distance test. Previously, anchors 0.5 or
0.99 units outside a face were missed with the default one-unit acceptance radius.
The search callback remains because it is the R-tree API; its return controls
continuation. The unused numeric search result is a match count, not an error code.

tests : Three C++ test groups in `tests/wood_assign_test.cpp:29`, `:48`, and `:61`
cover tolerance boundaries, independently retuned tolerances, face/side assignment,
missing types, empty queries, and empty element outlines. The same tests linked
against the original assignment implementation fail three tolerance assertions.
The corrected implementation passes.

style : Removed file headers, historical narration, and obsolete comments from
`src/joinery_solver/wood_assign.cpp:1` and `wood_assign.h:1`. Kept concise API
docstrings, renamed private calculation/access helpers, made query bounds and
structured bindings const, split compound statements, and simplified single-body
conditionals and loops.

ran : `wood_assign` CTest passes. The assignment translation unit also compiles
with `-Wall -Wextra -Wpedantic -Werror -fno-elide-constructors`; its linked regression
binary passes. This warning result applies to this translation unit, not every
file in the repository.

## Dataset and shell runners

parity : `examples/main_dataset_runner.cpp:3` now runs one selected dataset, matching
its documented purpose. `examples/main_all_datasets.cpp:3` retains all 44 calls.

safety : Both runners now propagate dataset failure to their exit status. The full
runner still attempts later datasets after a failure. `bash/cpp.sh:20` guards
configure/build commands, limits builds to four jobs, and guards each executable
at `:32`. Configure, build, and solver failures stop the script. The default target
at `:16` is the single-dataset runner; the missing helper dependency and obsolete
`main_wood_*` discovery were removed.

tests : Six shell orchestration tests in `tests/cpp_runner_test.py:13` and five
dataset exit-status/count tests in `tests/dataset_runner_test.py:12` pass. The latter
compile the real runner sources against deterministic dataset stubs and verify
successful runs, first/last failures, all 44 attempts, and exactly one single-runner
attempt. `CMakeLists.txt:170` registers the tests with CTest on Unix, with Python
tests enabled when an interpreter is available and actual compiler/solver tests
marked serial.

style : The shell entry point is self-contained and uses ordinary loops and quoted
arguments. Usage instructions in both dataset examples now respect the run guard
and build limit.

ran : All three registered CTest suites pass. The real shell entry point builds
and runs `main_dataset_runner` and `main_element_mapping_check` successfully. Only
the hexbox dataset output changed; its pre-test contents were restored. The full
44-dataset solver sweep was built but not executed.

## Element mapping

parity : `examples/main_element_mapping_check.cpp:104` requests the block GUID before
serialization, matching session_cpp's current lazy identity contract. Both JSON
and protobuf assertions compare with that requested identity. No library identity
behavior was changed.

safety : No new solver behavior.

tests : The existing element-mapping and session-round-trip executables both pass.

style : The mapping example's usage command at `:121` now uses the guarded script.

ran : `bash bash/cpp.sh main_dataset_runner main_element_mapping_check` and
`tools/run_guarded.sh -n wood-solver -- build/main_session_round_trip` pass.
`git diff --check` passes for the edited tracked paths.

## Linked joints and division counts

parity : Public C++ signatures remain unchanged. Valid linked outlines retain the
original vertex interleaving, including the real two-shadow finger-joint pattern.

safety : `src/joinery_solver/wood_joint.cpp:126` validates signed sequence values,
available vertices on both faces, and every insertion range before merging.
The loop uses a finite iteration count. At `:170`, missing faces and self-links
are skipped; an invalid later sequence leaves both the primary and shadow geometry
unchanged for that link. At `:206`, division counts are clamped before conversion
to an integer, including tiny positive spacing; zero, negative, or NaN spacing
selects one division, and non-finite line lengths are ignored.

`src/joinery_solver/joints/ss_e_op_5.h:9` validates both linked indices before
accessing either joint. At `:12`, rebuilding geometry replaces old linking
sequences rather than appending another set and breaking the merge count check.

tests : `tests/wood_solver_test.cpp:50`, `:87`, and `:99` cover exact interleaving,
negative/zero/oversized steps, missing faces, self-links, rollback on a later invalid
sequence, division limits, repeated construction, real finger-joint geometry, and
an invalid second linked index.

style : The merge validation and bounded interleaving are a private helper; the
public merge function handles selecting and committing each link. Removed the
edited files' header narration and historical linking comments.

ran : `wood_solver` CTest and both existing serialization checks pass.

## Dataset loading

parity : `src/joinery_solver/wood_internal.cpp:32` and `:43` share one loading and
deduplication path. Both preserve the existing signatures and filename rules.

safety : Missing or empty beam datasets throw instead of returning an empty
successful load. Plate loading rejects an unmatched final outline instead of
silently dropping it. Beam deduplication now honors the configured tolerance,
with an explicit positive argument taking precedence, as for plate loading.

tests : Missing/empty files, an unmatched outline, configured deduplication, and
an explicit tolerance override are covered at `tests/wood_solver_test.cpp:118`
and `:132`.

style : Removed duplicate loading logic and historical header/comment blocks;
paths and tolerances use const local variables.

ran : `wood_solver` CTest passes.

## Beam geometry

parity : The pipeline signature and serialized axes/rectangle layout are retained.
No public joint output was removed: the discarded local scaling arrays were never
returned or serialized by this function.

safety : `src/joinery_solver/wood_beams.cpp:109` skips zero-length axes and checks
intersection success and finite parameters. At `:203`, missing/nonpositive radii,
invalid volume lengths, and degenerate frames skip volume generation. The shared
trimming helper at `:34` checks all four plane intersections and finite coordinates
before replacing either rectangle. Failed trimming discards the contact.

style : Removed unused joint/point/index arrays and the dead eccentricity scaling
pass, including its unchecked radius indexing. Reused cached axis points, factored
duplicate rectangle trimming into one helper, and removed historical narration.
Existing opt-in diagnostic output remains available.

tests : `tests/wood_solver_test.cpp:152` checks crossing rectangles, side-to-end and
end-to-end trimming, missing radii, degenerate frames, and zero-length axes using
the pipeline's serialized output.

ran : All four wood CTest suites pass. The three changed C++ translation units pass
syntax checks with `-Wall -Wextra -Wpedantic` without diagnostics. The dependency
rebuild emitted warnings in Abseil, Protobuf, and session_cpp; project-wide zero
warnings is not claimed. The protected examples and all recorded pre-existing
edits remain byte-for-byte unchanged.
