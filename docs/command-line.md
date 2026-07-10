# Command Line

The main executable is `cpp_knot_indexer` on Unix-like systems and
`cpp_knot_indexer.exe` on Windows.

## Input Forms

Provide a PD code directly:

```sh
build/cpp_knot_indexer --pd-code "[[1,5,2,4],[3,1,4,6],[5,3,6,2]]"
```

Read a PD code from a file:

```sh
build/cpp_knot_indexer --pd-file code.txt
```

Read a PD code from stdin:

```sh
build/cpp_knot_indexer < code.txt
```

## Runtime Options

`--pd-code TEXT`

Read the PD code from the command line.

`--pd-file PATH`

Read the PD code from a file.

`--timeout SEC`

Set the maximum number of seconds for each invariant worker. The timeout is
applied independently to the HOMFLY-PT worker and the Khovanov worker. The
default is `60`. Use `0` to disable worker timeouts. Negative values are
rejected.

`--data-folder PATH`

Use an explicit invariant database folder. A valid data folder must contain:

- `homfly/sorted_HOMFLY-PT.txt`
- `khovanov/sorted_khovanov.txt`
- `knotname-reg/`

When this option is present, the executable does not search default data folder
locations. The folder name itself does not have to be `data`.

`--print-invariants`

Print the final HOMFLY-PT and Khovanov invariant strings to stderr.

`--verbose`

Print worker status, elapsed time, failure details, timeout details, and
successful invariant strings to stderr.

`--help`, `-h`

Print command usage.

## Output Contract

Candidate knot names are written to stdout, one name per line.

Diagnostics, verbose invariant strings, timeout messages, and worker failure
details are written to stderr.

If one invariant computation fails or times out but the other succeeds, lookup
continues with the successful invariant. If both computations fail or time out,
the program exits with status `2`.

Pressing `Ctrl+C` requests a clean shutdown. Active worker processes are
terminated and the program exits with status `130`.

## Default Data Folder Search

If `--data-folder` is not provided, the executable searches for a folder named
`data` in this order:

1. Next to the executable, such as `build/data`
2. One directory above the executable, such as `data` when running
   `build/cpp_knot_indexer`

If neither location is valid, the program exits with an error.

## Internal Worker Options

The executable uses internal worker options when spawning itself:

- `--worker khovanov|homfly`
- `--input PATH`
- `--output PATH`

These options are implementation details and are not intended as the public
user interface.

## Auxiliary Tools

The coordinate conversion tools have separate executables and documentation:

- [che_to_coord](che-to-coord.md)
- [link_pd_code](link-pd-code.md)
