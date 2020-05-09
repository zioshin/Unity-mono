echo "iPhone simulator runtime is 32 bit only and does not build on Yamato"
echo "To build locally, run \"clone_mono_build_deps.sh\" and then \"external/buildscripts/build_runtime_iphone.sh --simulator-only\""

echo "Downloading existing artifact"

cd .yamato/scripts
mono bee.exe
cd ../..
mkdir -p incomingbuilds/iphoneruntime/embedruntimes/iphone
cp -r .yamato/scripts/artifacts/Stevedore/mono-iOS-simulator-runtime/* incomingbuilds/iphoneruntime/embedruntimes/iphone