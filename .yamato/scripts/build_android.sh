git submodule update --init --recursive

echo $UNITY_THISISABUILDMACHINE

perl external/buildscripts/build_runtime_android.pl
if [ $? -eq 0 ]
then
  echo "mono build script ran successfully"
else
  echo "mono build script failed" >&2
  exit 1
fi

mkdir -p incomingbuilds/android
cp -r builds/* incomingbuilds/android/