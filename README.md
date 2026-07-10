# Pure C++ Knot Indexer

## QuickStart

This is an independent C++17 code tree for looking up knots from PD codes using
HOMFLY-PT and integral Khovanov invariants. It does not require SageMath at
runtime.

Build from the repository root:

```sh
python build.py
```

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

Detailed documentation:

- [Command Line](docs/command-line.md)
- [Packaging](docs/packaging.md)
- [Algorithms](docs/algorithms.md)
- [Modules](docs/modules.md)
- [che_to_coord](docs/che-to-coord.md)
- [link_pd_code](docs/link-pd-code.md)
