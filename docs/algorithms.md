# Algorithms

This document describes the algorithms currently present in the code tree. The
main executable accepts PD codes and uses HOMFLY-PT and integral Khovanov
invariants for lookup. The coordinate conversion modules are implemented as
standalone C++17 library code but are not linked into the main executable yet.

## PD Code Normalization

The main indexer parses PD codes from text, validates integer labels, and
renumbers nonconsecutive labels into a compact canonical range. This lets the
lookup path accept equivalent PD codes that use arbitrary positive label names.

The HOMFLY-PT path rebuilds component traversal from the PD graph before
calling the polynomial engine. This avoids SageMath-style failures where a
consistently reversed knot orientation can produce the mirror polynomial or an
orientation-sensitive error.

## HOMFLY-PT

The HOMFLY-PT worker uses the vendored public-domain Jenkins/Marco HOMFLY
implementation under `third_party/libhomfly`.

Local integration details:

- The C sources are compiled as C++ in one compiler invocation.
- A local `gc.h` shim avoids requiring Boehm GC at runtime.
- Each computation runs in a worker process, so temporary allocations are
  process-local and released when the worker exits.
- The formatter mirrors the `L` exponent and orders terms to match the database
  strings used by this project.

The worker output is a normalized polynomial string. The main process uses that
string to query `data/homfly/sorted_HOMFLY-PT.txt`.

## Khovanov Homology

The Khovanov worker uses the vendored MIT-licensed `GGN-2015/cppkh`
implementation under `third_party/cppkh`, which follows the JavaKh-style
integer Khovanov computation path.

The worker output is a normalized integral Khovanov string. The main process
uses that string to query `data/khovanov/sorted_khovanov.txt`.

## Timeout and Failure Degradation

HOMFLY-PT and Khovanov are computed in separate worker processes. The public
`--timeout SEC` value applies independently to each worker.

If one worker fails or times out and the other succeeds, the main process still
performs lookup using the successful invariant. If both fail, the lookup fails.

This design keeps slow or pathological invariant computations from blocking the
whole command indefinitely.

## Candidate Lookup

The database layer maps invariant strings to candidate knot names. Name
normalization uses `data/knotname-reg/` to account for mirrors and naming
equivalences in the dataset.

When both invariants succeed, the final result is the intersection of the
HOMFLY-PT candidate set and the Khovanov candidate set. If only one invariant
succeeds, the final result is the unique candidate set from that invariant.

## Molecule Data to Coordinates

`src/che_to_coord` implements the molecule data parser. It reads `Atoms` and
`Bonds` sections, accepts compact `id x y z` rows and common LAMMPS atom row
layouts, then validates that the bond graph is one connected degree-2 cycle.

The output is a coordinate loop ordered along the molecular cycle. Invalid
input produces `cki::che_to_coord::ParseError` instead of assertion failures.

## Coordinates to PD Code

`src/link_pd_code` is derived from `GGN-2015/link-pd-code` under the MIT
license and refactored as standalone C++17 library code.

The algorithmic path is:

1. Project one or more ordered 3D polygon components to a candidate 2D plane.
2. Reject non-generic projections.
3. Find projected segment intersections with AABB/sweep broad-phase pruning.
4. Sort crossing occurrences along each oriented component.
5. Assign strand labels.
6. Encode each crossing from the incoming under-strand and clockwise half-edge
   order.

Local fixes compared with the upstream project:

- deterministic multi-direction projection search instead of a short random
  retry loop
- explicit exceptions instead of user-facing `assert` failures
- endpoint crossings, overlapping projected segments, tied over/under heights,
  and multiple crossings at one projected point are treated as non-generic
  projection failures
- segment parameters are computed from the 2D line equations instead of using
  distance ratios
- interval arithmetic filters determinant, range, and height calculations before
  accepting floating-point decisions
- exact scaled rational arithmetic is used as a fallback for near-degenerate
  projected segment intersections, so endpoint and overlap classification does
  not depend only on raw floating-point comparisons
- isolated no-crossing components return no PD crossing by default, with an
  option to reproduce upstream-style degenerate isolated component encoding

The exact rational fallback is intentionally not used for every segment pair.
The common path still uses the existing AABB/sweep pruning and double arithmetic
for clearly separated cases. Exact arithmetic is invoked only when the interval
filter marks the determinant or unit-interval boundary classification as
uncertain.
