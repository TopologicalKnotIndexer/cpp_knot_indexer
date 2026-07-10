# Packaging

The project intentionally uses a Python build wrapper instead of CMake. The
wrapper detects a g++-style compiler, probes optimization flags, compiles the
pure C++ executable, and refreshes the runtime data folder next to the output
binary.

Most reusable project code is implemented in inline `.hpp` headers. The build
therefore compiles the command line entrypoint plus vendored computation
backends; compatibility `.cpp` files in `src/*` are not required by the default
executable build.

## Build Script

Run the default release build:

```sh
python build.py
```

Select a compiler:

```sh
python build.py --cxx g++
python build.py --cxx clang++
python build.py --cxx /opt/homebrew/bin/g++-15
```

The compiler is resolved in this order:

1. `--cxx`
2. `CXX`
3. common compiler names on `PATH`, such as `g++`, `clang++`, and `c++`

## Build Options

`--build-dir PATH`

Set the build output directory. The default is `build`.

`--output PATH`

Set the exact executable output path.

`--debug`

Build with `-O0 -g` instead of release optimization.

`--portable`, `--no-native`

Disable native CPU tuning flags such as `-march=native` and `-mtune=native`.
Use this for binaries intended to run on other machines.

`--no-lto`

Disable link-time optimization.

`--static-runtime`

On Windows, link libstdc++ and libgcc statically.

`--extra-cxxflag FLAG`

Append an extra compiler flag. This option can be repeated.

`--extra-ldflag FLAG`

Append an extra linker flag. This option can be repeated.

`--show-command`

Print the full compiler command before building.

`--clean`

Remove the build directory before compiling.

## Default Release Flags

The release build starts with:

- `-std=c++17`
- `-O3`
- `-DNDEBUG`
- `-DCPPKH_SHARED_LIBRARY`

The script then probes platform support before adding optional flags:

- `-pipe`
- `-march=native`
- `-mtune=native`
- `-flto`
- `-fuse-linker-plugin`
- `-fno-plt` on supported non-Windows, non-macOS platforms

On Windows, the build defines:

- `KH_THREAD_BACKEND_WIN32`
- `NOMINMAX`

On other platforms, the build defines:

- `KH_THREAD_BACKEND_STD`
- `-pthread`

The script also probes whether `std::filesystem` needs `-lstdc++fs`.

## Data Folder Packaging

After a successful build, `build.py` copies the repository `data` folder next to
the executable. If a previous `data` folder already exists beside the output
binary, it is removed first and then copied again.

This keeps the executable independent from hard-coded embedded data. Users can
replace the runtime database by passing `--data-folder PATH`, or by replacing
the copied `data` folder beside the executable.

## Test Script

Run:

```sh
python test.py --rebuild
```

The test script builds the executable if needed, runs smoke and regression
checks, verifies data folder behavior, checks timeout argument handling, and
compiles the auxiliary coordinate modules as standalone C++17 code.

Useful options:

- `--exe PATH`
- `--cxx COMPILER`
- `--rebuild`
- `--no-build`
- `--portable`
