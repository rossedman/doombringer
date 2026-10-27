# VCV Rack

The VCV Rack 2 plugin mirrors the patch.Init. The cutoff and the three [character macros](panel.md#the-macros) sit on top, and the settings the hardware keeps on its shift layer get knobs of their own. Knobs and CVs go through the same `doom::panelToParams` as the firmware, so a patch translates one-to-one.

See [Reading the panel](panel.md) for what each sigil means, and [Getting started](getting-started.md#vcv-rack) to install it.

## Differences from the patch.Init


- No shift layer: morph, FM depth, HUNGER and fine tune each have a knob.
- Ping is a button with a trigger input next to it, and the undertones' restart has its own trigger input.
- Bass compensation is a switch (the trident). Up is compensated.
- The envelope drives a red light and an output jack.
- The FM input has its own jack, scaled by the FM depth knob.

## Building

```sh
scripts/fetch-rack-sdk.sh ../Rack-SDK lin-x64   # or win-x64, mac-x64, mac-arm64
make vcv RACK_DIR=../Rack-SDK                   # vcv/dist/Doombringer-<version>-<platform>.vcvplugin
```

On Windows, build in an MSYS2 MinGW 64-bit shell with `mingw-w64-x86_64-gcc`, `jq`, `make`, `unzip` and `zstd` installed. On an Apple silicon Mac, `make -C vcv RACK_DIR=<mac-x64 SDK> CROSS_COMPILE=x86_64-apple-darwin dist` builds the Intel package.

Every release carries signed packages for all four: Linux x64, Windows x64, macOS x64 and macOS arm64.
