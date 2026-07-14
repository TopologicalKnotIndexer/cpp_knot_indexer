# Pure C++ Knot Indexer

`cpp_knot_indexer` is an independent C++17 command-line toolchain for
identifying knots from planar diagram (PD) codes. It validates and canonicalizes
the input, races invariant calculations on the original and an equivalent
simplified diagram, then searches committed HOMFLY-PT and integral Khovanov
tables for candidate knot names. SageMath is not required at build time or
runtime.

The repository also builds two reusable coordinate tools:

- `che_to_coord` converts supported molecule-style `.che` data to ordered 3D
  polygon coordinates.
- `link_pd_code` projects an ordered 3D polygon and emits a PD code.

All build inputs, runtime tables, licenses, and required third-party source
snapshots are tracked as ordinary files. The project does not use Git
submodules or download another repository during a build.

## Requirements

- Python 3.9 or newer for the build and test drivers.
- A g++-style C++17 compiler such as GCC or Clang. Pass `--cxx` to `build.py`
  or set `CXX` when the compiler is not on `PATH`.

## Quick Start

Build everything from the repository root:

```sh
python build.py
```

This creates `cpp_knot_indexer`, `che_to_coord`, and `link_pd_code` under
`build/`, and copies the committed `data/` folder next to the main executable.

Run a lookup:

```sh
build/cpp_knot_indexer --pd-code "[[1,5,2,4],[3,1,4,6],[5,3,6,2]]" --timeout 60
```

On Windows, the default executable path is:

```sh
build/cpp_knot_indexer.exe --pd-code "[[1,5,2,4],[3,1,4,6],[5,3,6,2]]" --timeout 60
```

Run the regression tests:

```sh
python test.py --rebuild
```

The regression suite covers the unknot, Reidemeister-I input, multiple trefoil
encodings, nonconsecutive labels, stdin and file input, Unicode paths, timeout
handling, invariant output, simplification control, timing output, runtime-data
validation, and both auxiliary coordinate executables.

## Inputs and Output

The main executable accepts `--pd-code`, `--pd-file`, or PD text on stdin. Both
compact `[[...], ...]` and `PD[X[...], ...]` forms are accepted. Arbitrary
positive strand labels are canonicalized after validating that every label
appears exactly twice.

Candidate knot names are written one per line to stdout. Diagnostics and
optional invariant/timing details are written to stderr, so stdout remains
suitable for scripts. Use `--data-folder` to select another compatible table
directory and `--ban-simplify` to compute only from the original diagram.

## How Lookup Works

The main process starts HOMFLY-PT, integral Khovanov, and PD-simplification
workers. If simplification returns a different equivalent diagram before an
invariant finishes, a second worker for that invariant is started on the
simplified input; the first successful result wins and the redundant worker is
terminated. Process isolation provides portable timeouts and clean `Ctrl+C`
handling without changing the mathematical lookup contract.

The committed runtime tables are:

```text
data/homfly/sorted_HOMFLY-PT.txt
data/khovanov/sorted_khovanov.txt
data/knotname-reg/
```

See [Third-party components](THIRD_PARTY.md) for source provenance and license
information.

## Manuals

- [Command Line](docs/command-line.md): user-facing options and output contract.
- [Algorithms](docs/algorithms.md): implementation details for normalization,
  simplification, invariant racing, lookup, and coordinate conversion.
- [Packaging](docs/packaging.md): Python build script, compiler flags, and
  runtime data packaging.
- [Modules](docs/modules.md): source tree and public `.hpp` entrypoints.
- [che_to_coord](docs/che-to-coord.md): molecule data to ordered coordinates.
- [link_pd_code](docs/link-pd-code.md): ordered 3D coordinates to PD code.
- [Citation](docs/modules.md#citation): BibTeX entry for academic use.
