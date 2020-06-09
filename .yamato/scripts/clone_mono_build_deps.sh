#!/bin/bash

#mono-build-deps has to be present in the mono parent directory
cd ../..
hg clone --uncompressed http://hg-mirror-slo.hq.unity3d.com/unity-extra/mono-build-deps/ mono-build-deps/build/
cd mono-build-deps/build
hg pull -r default
hg up -C default