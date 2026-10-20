# Arcfield

A tile-and-particle evolution laboratory. A genetic algorithm searches complete
curved-tile fields for layouts that minimize particle collisions under a shared,
seeded traffic trial.

The application includes a compact C simulation engine compiled to WebAssembly,
live evolutionary progress, a 3D fitness landscape, per-run fitted comparisons,
named browser-local experiment history, CSV/ZIP export, a searchable guide, and
an installable offline PWA.

## Run locally

The compiled engine is checked in; no package installation or compiler is needed
to run the application:

```sh
python3 -m http.server 8000 --directory dist
```

Open http://localhost:8000. Localhost supports service workers. After changing
any deployed asset, rebuild the service worker's offline cache version:

```sh
python3 scripts/build-pwa.py
```

Use **Update app** when a new cached version is available. Keep the app open
while experiments run; installing it does not move computation to a server.

## Project layout

| Path | Purpose |
| --- | --- |
| `native/engine.c` | Deterministic allocation-free simulation core |
| `native/README.md` | Detailed physics, engine ABI, build, testing and architecture notes |
| `dist/compact-engine.wasm` | Prebuilt engine used by evolution and replay |
| `dist/worker.js` | Genetic algorithm and worker progress protocol |
| `dist/topology.js`, `dist/fit.js` | Boundary-route metrics and winner-anchored fits |
| `dist/app.js`, `dist/landscape.js` | Main interface and interactive plots |
| `dist/run-cache.js`, `dist/export.js` | Browser history, naming/deletion and data export |
| `dist/guide.js`, `dist/guide.css` | Searchable in-app documentation |
| `dist/pwa.js`, `dist/sw.js` | Installation, offline shell and updates |
| `scripts/` | Engine/icon builds and offline-cache generation |
| `tests/` | Simulation, startup, replay, data, cache, guide and PWA checks |

## Build the engine

Use Clang with wasm-ld, or Zig 0.14.1:

```sh
sh scripts/build-engine.sh
# or
ZIG=/path/to/zig sh scripts/build-engine.sh
python3 scripts/build-pwa.py
```

See `native/README.md` for native GCC checks and detailed validation commands.
The dependency-free Node tests run from the repository root. The IndexedDB test
also needs the development-only `fake-indexeddb` package:

```sh
node tests/engine.test.cjs
node tests/startup.test.cjs
node tests/service-worker.test.cjs
npm install --prefix /tmp/arcfield-cache-test fake-indexeddb --no-audit --no-fund
NODE_PATH=/tmp/arcfield-cache-test/node_modules node tests/cache.test.cjs
```

Python 3 is needed for the offline build helper; Pillow is only needed when
regenerating icons. Development dependencies are not loaded by the website.

## Data and hosting

Saved experiments live in the current browser's IndexedDB, separately from Git.
They are not synchronized to GitHub or another device. Export ZIP/CSV files for
portable results. Never commit private experiment data or credentials by accident.

`.openai/hosting.json` identifies the existing Sites deployment. Source can be
versioned on GitHub while the deployed application continues to be hosted by
Sites. A GitHub mirror does not by itself configure automatic deployment.
