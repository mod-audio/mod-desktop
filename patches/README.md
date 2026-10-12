# Submodule patches

The three files here are applied on top of the pinned submodule commits by `utils/apply-patches.sh`:

| Patch | Submodule | What it does |
| --- | --- | --- |
| `PawPaw.patch` | `src/PawPaw` | fetch pinned git objects by SHA (jack2 is pinned to a commit no branch contains) + jack2 macOS client-thread-close patch |
| `PawPaw-macos.patch` | `src/PawPaw` | macOS-only toolchain and bootstrap fixes (applied on top of `PawPaw.patch`, only on macOS) |
| `mod-host.patch` | `src/mod-host` | fix a double free when `jack_activate` fails |
| `mod-plugin-builder.patch` | `src/mod-plugin-builder` | plugin recipe pins and fixes used by the Desktop plugin set |

Build from a clean clone: `git submodule update --init`, `utils/apply-patches.sh`, then `make`.
