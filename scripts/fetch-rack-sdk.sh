#!/usr/bin/env bash
# Download and verify the VCV Rack 2 SDK for one platform into the given directory.
#   scripts/fetch-rack-sdk.sh [dest] [platform]
# dest defaults to ../Rack-SDK; platform is lin-x64 (default), win-x64, mac-x64 or mac-arm64.
set -euo pipefail

version=2.6.4
dest=${1:-../Rack-SDK}
platform=${2:-lin-x64}

case "$platform" in
lin-x64) sha256=3eb7cdf86524a7296e445c86d1d13d1935f5b7e68ef76a369ef1d3a240b49068 ;;
win-x64) sha256=aa57b3d225c11b213faa2f0ce405971876473588ca4eb7de65e41e996e715aad ;;
mac-x64) sha256=99b83ba63716287a6fea53e936b353b9c2fa37e2a1dd573f5f36a266d1af15a4 ;;
mac-arm64) sha256=01aebe38122e2077359ed1a18c2c2742d4f8ed42fbb6bae0c04db703a75110f4 ;;
*)
  echo "unknown platform: $platform (lin-x64, win-x64, mac-x64, mac-arm64)" >&2
  exit 2
  ;;
esac

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
curl -fsSLo "$tmp/sdk.zip" "https://vcvrack.com/downloads/Rack-SDK-${version}-${platform}.zip"
# macOS has shasum rather than sha256sum.
if command -v sha256sum >/dev/null; then
  echo "${sha256}  $tmp/sdk.zip" | sha256sum -c -
else
  echo "${sha256}  $tmp/sdk.zip" | shasum -a 256 -c -
fi
unzip -q "$tmp/sdk.zip" -d "$tmp"
rm -rf "$dest"
mv "$tmp/Rack-SDK" "$dest"
echo "Rack SDK ${version} (${platform}) in ${dest}"
