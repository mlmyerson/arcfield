# Arcfield simulation core

`engine.c` is the authoritative deterministic C99 simulation used for evolution
and winner replay. `dist/compact.js` is the small JavaScript ABI adapter; the GA
and journal remain JavaScript. The compiled engine currently executes in browser
workers, not on a provisioned external server.

## Model and limits

- Three tile routings, two quarter-circle pairings and a straight crossing.
- 60 fixed simulation steps/second, no wall-clock timers or sleeping.
- Seeded edge injection schedule shared by every candidate.
- Particle speed follows arc length (quarter-circle length = pi/4 tiles).
- Collisions at center distance < 0.21 tiles consume both particles and rewire
  the impacted tile, using a separate deterministic random stream.
- Stable pair ordering resolves simultaneous collision candidates. The previous
  JavaScript implementation used a different bucket order above 40 particles;
  high-density traces from that older version are not guaranteed identical.
- Max 24x18 tiles, 120 simulation seconds, 3,600 injected particles; a fixed
  1 MiB WebAssembly linear memory, including stack and all simulation buffers.
  This does not include JavaScript caches/UI memory.
- The C core is single-context. Use separate instances per parallel worker.

## Build and validation

Use Clang with wasm-ld:

    sh scripts/build-engine.sh

Or install Zig 0.14.1 and set its path:

    ZIG=/path/to/zig sh scripts/build-engine.sh

The prebuilt `dist/compact-engine.wasm` is checked in so deployment needs no compiler.
Native C verification uses GCC:

    gcc -O3 -ffp-contract=off -fno-builtin tests/native.test.c -lm -o /tmp/arcfield-native-test
    /tmp/arcfield-native-test
    node tests/engine.test.cjs
    node tests/worker.test.cjs
    node tests/ui.test.cjs

The engine test compares 80 seeded scenarios with the previous JS implementation,
including collisions, exits, bins and final tiles; verifies a known head-on
collision and conservation; checks maximum supported trial settings; and compares
compiled evaluation with step-by-step replay. Benchmark details are written to
`tests/benchmark.json`; those timings are local measurements, not guarantees for
phones or other hardware. Worker tests cover deterministic GA runs, cache totals
and live messages; the UI test executes compiled winner replay with a mocked DOM.
Actual browser visual QA was unavailable in this environment.

## C ABI

Copy tile bytes into `tiles_ptr()` and packed doubles `[time,x,y,entry]` into
`schedule_ptr()`. Call `configure(width,height,seconds,speed,count,impact_seed)`
(check success), then `step(n)`. Read `collision_count()`, `exit_count()`,
`active_count()`, `time_value()` and `bins_ptr()`. Call `snapshot()` before reading
`particles_ptr()` (seven doubles per particle: x,y,entry,exit,u,world_x,world_y).
The impact seed is the FNV-1a hash of UTF-16 code units of `seed + ':impacts'`.

There is no rendering, GA selection, network, allocation, or OS dependency in the
C engine. It can be linked into a native server or reused as a WebAssembly module.
A future native backend can evaluate independent candidates across CPU cores;
parallelism and hosting still need implementation and measurements.

## Worker startup and liveness

The UI dispatches `{type:'start', config}` after installing handlers. The worker
emits `ready`, then acknowledges receipt with `ack`. It reports named phases,
overall completion, current generation/candidate, trial completion, and breeding
counts. A five-second worker heartbeat and foreground watchdog/ping distinguish
an acknowledged run from silent startup. Engine fetch aborts after 15 seconds.
Async evaluation yields after roughly 16 ms of work in eight-step batches;
physical simulation steps and seeded outcomes do not depend on these yields.
Browser/OS suspension can delay timers when the page is backgrounded.

Regression checks:

    node tests/startup.test.cjs
    node tests/heartbeat.test.cjs

The startup test clicks the actual app handler and dispatches to a real Node
worker thread running the browser worker script, then verifies the winner UI.
The heartbeat test delays engine download by 6.2 seconds and verifies that the
worker reports the download phase at five seconds before completing the trial.
These cover the actual dispatch boundary missed by direct worker unit calls.

## Fitness landscape descriptors

`dist/topology.js` traces every boundary-connected route on the starting genome.
It measures full geometric route length (one for straight tile traversal, pi/4
for a quarter-circle), averages over all boundary entry ports, and counts each
undirected short boundary-to-boundary route once. The visual threshold can be
changed from one to eight tile visits without recomputing fitness. Internal
closed loops are not included because an edge-injected particle cannot enter
them in the starting layout. Collision scores still come from the dynamic trial;
the topology descriptors deliberately describe the initial genome, not its
collision-mutated final state. They do not affect GA selection.

The 3D scatter uses a deterministic reservoir of at most 1,600 observations, plus
the best observed layout. Latest-generation view keeps the complete current
population. Color indicates generation; collision height is fitness. These are
sampled measurements, not an interpolated surface or a claim that two topology
metrics uniquely determine collision fitness. Geometry and sampling use no
simulation RNG draws. Tests:

    node tests/topology.test.cjs
    node tests/landscape.test.cjs

## Experiment export

The header offers a single CSV of all candidate observations, or a standard ZIP
with settings/seed, complete candidate metrics, generation summaries, and a schema
README. Completed experiments also include the winner's initial genome (JSON),
coordinate-based tiles (CSV), and binned/cumulative collisions (CSV). Candidate
capture is independent of landscape sampling and the journal's display cap.
All eight short-route thresholds are exported. Cache hits remain observations
and are explicitly marked. Intermediate genomes and per-candidate collision time
series are not stored. Running/canceled/failed runs are exportable partial
snapshots, labeled in settings. Refreshing or starting another run clears the
in-memory capture. No experiment data is sent to a third-party exporter.

    node tests/export.test.cjs

The export test exercises 1,700 candidate rows, partial states, winner data and
ZIP construction; archives were also validated with Python's standard `zipfile`
and `csv` readers. The startup test asserts the complete real worker run appears
in the export capture.

## Multi-run cache and fitted comparisons

Completed experiments are saved locally in IndexedDB (`arcfield-experiments`),
with full export data in `outputs` and plot/fit summaries in `summaries`. There is
no server/account synchronization. The latest completed run is restored after a
refresh; Load result restores an older winner, replay, settings and export data.
Writes are atomic across both stores. Storage denial/quota failure is reported
without invalidating the completed experiment; downloads remain available.
Partial running/canceled data is not persisted automatically.

The plot can show points, fitted lines, or both for selected saved runs. Cached
comparisons use each run's initial topology metrics and dynamic collision score.
The straight line is an orthogonal least-squares fit constrained to pass through
the winning observation. A 3x3 second-moment matrix around the winner is computed
from ALL candidate observations, including cache hits; the dominant eigenvector
minimizes squared perpendicular distance. Coordinates use common plot-axis
ranges so unlike units do not dominate purely by magnitude. The displayed
segment ends at the winner and extends toward higher collision values, clipped
to the common plotting box. It is a cloud summary, not a reconstructed
chronological evolution trajectory. The legend reports the fraction of anchored
spread along the fitted line (not a regression R-squared). A zero-spread cloud
has only its winner point. Generation slicing applies to point-only mode; fitted
comparisons always use all observations. Mixed field sizes/trial horizons remain
in raw units and are labeled in the run list.

    node tests/fit.test.cjs

Cache tests require the development-only `fake-indexeddb` package (not shipped
with the website):

    npm install --prefix /tmp/arcfield-cache-test fake-indexeddb --no-audit --no-fund
    NODE_PATH=/tmp/arcfield-cache-test/node_modules node tests/cache.test.cjs

They cover two completed runs, atomic persistence, simulated page reload, full
winner/export restoration, multiple fitted overlays and run selection. Fitting
checks cover known collinear clouds, exact anchored endpoints and zero spread.
Browser visual QA remains unavailable; actual production-browser IndexedDB UI
behavior has not been manually observed.

## In-app guide

The header's circular question-mark button opens a native modal dialog with the
full site guide. It has 12 searchable sections, desktop contents links, a mobile
jump menu, scroll navigation, back-to-top, Escape/native close behavior, reduced-
motion-aware navigation, background-scroll restoration and focus restoration.
It is independent of worker/replay state. The question-mark shortcut ignores
inputs, textareas, selects and editable content. Content is stored in index.html;
scoped styles and behavior are in guide.css/guide.js. Search filters complete
sections, rather than changing the underlying experiment or page URL.

    node tests/guide.test.cjs

Handler checks cover navigation, search and no-match states, close/open, focus
and scroll restoration, backdrop clicks, and the typing-safe shortcut. HTML
structure checks verify unique IDs and all script assets. Native-browser visual
layout and focus trapping were not manually verified in this environment.

## Installable PWA and offline shell

The site includes a web app manifest (standalone display, root scope, themed
launch), deterministic 192/512px icons, a maskable icon, Apple touch icon and
install UI. Supporting browsers provide the native installation prompt; the
button falls back to Android/iOS menu instructions. Access audience remains
unchanged. Installation/authentication must be completed while online.

The service worker precaches a content-hashed, coherent copy of all app assets,
including help, workers and Wasm. It only intercepts this shell's allowlisted
same-origin GET requests and its root navigation. Query variants map to the
same canonical asset inside one version. Redirects, opaque responses and invalid
asset MIME types are rejected during atomic install, avoiding cached sign-in
pages masquerading as code. Non-shell/auth routes are not intercepted. Browser
runs are still in IndexedDB, independently of shell caches.

Updates wait; the page offers an explicit Update app button that refuses to
reload an active search. Initial activation claims clients without forcing a
reload. User-requested updates reload once after controller change. Old shell
caches are removed only on activation; IndexedDB and unrelated caches are not
deleted. Standard browser lifecycle may also activate a waiting update when all
old clients have closed. Mobile OS suspension still affects active simulations.

IMPORTANT: After editing ANY deployed asset, regenerate the shell version:

    python scripts/build-pwa.py

For reproducible icon generation (Pillow is a development dependency only):

    python scripts/create-icons.py
    python scripts/build-pwa.py

    node tests/service-worker.test.cjs
    node tests/pwa.test.cjs

The tests exercise shell precache, offline HTML/worker/Wasm fetches, query
normalization, auth-route bypass, update acknowledgement/cleanup, installation
prompt handling and active-run update prevention. Manifest/icon dimensions were
validated, and the app icon was inspected. Phone installation, production browser
service-worker registration, and browser visual QA were not observed directly.

## Run labels and history deletion

The optional Run name control is a descriptive label (up to 80 characters), with
seed fallback for unnamed and older records. It travels in configuration and is
shown in history, result details, comparison legends and download filenames.
Names do not enter any simulation/evolution RNG. Rename updates the summary,
full saved output and winner configuration atomically; loading restores the name.
The input and labels use text values/textContent, not interpreted HTML.

Saved rows have Rename/Delete controls, and Clear saved runs removes all history.
Deletion controls ask for browser confirmation. Only the IndexedDB `outputs` and
`summaries` stores are affected; PWA shell caches, installed app assets and prior
downloads remain untouched. Currently displayed results remain in memory and can
still be exported; an active search continues and can save a new completed run.
The cache test covers persistent rename, unchanged genome/seed, canceled deletion,
individual deletion, full clearing across both stores, and restoration behavior.
