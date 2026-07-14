# Third-Party Components

This repository is self-contained and uses no Git submodules. Required source
is committed as ordinary files so builds work without cloning nested projects.

- `third_party/cppkh` contains the MIT-licensed integer Khovanov backend derived
  from [`TopologicalKnotIndexer/cppkh`](https://github.com/TopologicalKnotIndexer/cppkh),
  including the local C ABI used by the worker process.
- `third_party/cpp-pd-code-simplify` contains the MIT-licensed header-only
  reducer from
  [`TopologicalKnotIndexer/cpp-pd-code-simplify`](https://github.com/TopologicalKnotIndexer/cpp-pd-code-simplify).
- `third_party/libhomfly` contains the public-domain Jenkins/Marco HOMFLY
  implementation and its upstream README. The local build uses a small `gc.h`
  compatibility shim and process isolation instead of requiring Boehm GC.
- `src/link_pd_code` is derived from the MIT-licensed
  [`TopologicalKnotIndexer/link-pd-code`](https://github.com/TopologicalKnotIndexer/link-pd-code)
  project and refactored into a reusable C++17 library.

Each component retains its license or provenance documentation in its tracked
directory. Review and test source updates in their independent repositories
before refreshing these ordinary-file snapshots.
