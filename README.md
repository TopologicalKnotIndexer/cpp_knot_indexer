# Pure C++ Knot Indexer

This is an independent C++17 code tree for indexing knots from PD_CODE using
HOMFLY-PT and integral Khovanov invariants. It does not import Python modules
from the original `hybrid_knot_indexer` project and does not require SageMath at
runtime.

The project carries its own:

- knot indexer source files under `src/knot_indexer/`
- molecule-to-coordinate code under `src/che_to_coord/`
- 3D-coordinate-to-PD-code code under `src/link_pd_code/`
- Python build and test scripts
- vendored HOMFLY-PT and Khovanov computation code
- invariant lookup data under `data/`

## Build

Build with the Python wrapper from this directory. It finds a g++-style compiler
automatically from `--cxx`, then `CXX`, then common compiler names such as `g++`
and `clang++`.

```sh
python build.py
```

Specify a compiler:

```sh
python build.py --cxx g++
python build.py --cxx /opt/homebrew/bin/g++-15
```

The default release build enables `-O3`, `-DNDEBUG`, and probes support for
`-pipe`, `-march=native`, `-mtune=native`, `-flto`, and platform-safe link flags.
Use `--portable` when building a binary meant to run on other machines.

```sh
python build.py --portable
python build.py --no-lto
python build.py --show-command
```

The target executable is written to `build/cpp_knot_indexer` or
`build/cpp_knot_indexer.exe` on Windows. After a successful build, `build.py`
also refreshes `data/` next to the executable: an existing `build/data` folder
is removed, then the project `data/` folder is copied there.

## Usage

```sh
build/cpp_knot_indexer --pd-code "[[1,5,2,4],[3,1,4,6],[5,3,6,2]]" --timeout 60
build/cpp_knot_indexer --pd-file pd_code.txt --timeout 60 --verbose
build/cpp_knot_indexer --pd-file pd_code.txt --data-folder /path/to/my-database
```

`--timeout SEC` applies independently to the Khovanov worker and the HOMFLY-PT
worker. If one worker fails or times out, candidates are still retrieved from
the invariant that completed. Pressing `Ctrl+C` requests a clean shutdown and
terminates any active worker process.

`--verbose` writes intermediate diagnostics to stderr, including Khovanov and
HOMFLY-PT worker status, elapsed time, successful invariant strings, and captured
failure or timeout details. Final candidate knot names are always written to
stdout only, so verbose output can be redirected separately from search results.

The HOMFLY-PT path rebuilds component traversal directly from the PD graph
before calling the polynomial engine. It therefore accepts consistently
unoriented or relabeled PD codes for knots, including whole-component reversed
inputs that make SageMath's orientation-sensitive path produce the mirror
polynomial.

By default, the executable looks for a folder literally named `data` in two
places, in this order:

1. Next to the executable, for example `build/data`
2. One directory above the executable, for example `data` when running
   `build/cpp_knot_indexer`

A valid data folder must contain:

- `homfly/sorted_HOMFLY-PT.txt`
- `khovanov/sorted_khovanov.txt`
- `knotname-reg/`

Use `--data-folder PATH` to point directly to another valid data folder. A
user-specified data folder can have any name. When this option is present, the
program does not run the automatic `data` lookup; an invalid path is reported as
an error.

## Tests

Run the smoke/regression tests from this directory:

```sh
python test.py --rebuild
```

The test script builds the executable if needed, then checks unknot, R1
simplification, trefoil, stdin input, CLI timeout validation, and standalone
compilation/runtime checks for the auxiliary `che_to_coord` and `link_pd_code`
modules. The auxiliary modules are not linked into the main executable yet.

## Vendored Algorithms

- `third_party/cppkh`: MIT-licensed C++14 implementation of the JavaKh integer
  Khovanov computation path, vendored from `GGN-2015/cppkh`.
- `third_party/libhomfly`: public-domain Jenkins/Marco HOMFLY implementation.
  It is compiled as C++ with a local `gc.h` compatibility shim, so Boehm GC is
  not required. The worker-process design makes per-computation allocations
  short-lived.
- `src/link_pd_code`: MIT-licensed coordinate-to-PD-code module derived from
  `GGN-2015/link-pd-code`, refactored as a standalone C++17 library with
  deterministic projection retry, explicit genericity checks, and AABB/sweep
  intersection pruning. It is not linked into the main executable yet.

The HOMFLY-PT formatter mirrors the `L` exponent and orders terms to match the
existing Sage/KnotInfo-style database strings.

## Citation

If you use this project in academic work, please cite it as software:

```bibtex
@software{cpp_knot_indexer,
  title        = {Pure C++ Knot Indexer},
  author       = {{GGN-2015}},
  year         = {2026},
  url          = {https://github.com/GGN-2015/cpp_knot_indexer},
  note         = {C++17 software for PD-code knot lookup using HOMFLY-PT and Khovanov invariants}
}
```
