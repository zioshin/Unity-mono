git submodule update --init --recursive
# try again in case previous update failed
git submodule update --init --recursive

echo $UNITY_THISISABUILDMACHINE

perl external/buildscripts/build_runtime_linux.pl -build64=0
if [ $? -eq 0 ]
then
  echo "mono build script ran successfully"
else
  echo "mono build script failed" >&2
  exit 1
fi

echo "Making directory incomingbuilds/linux32"
mkdir -p incomingbuilds/linux32
ls -al incomingbuilds/linux32
echo "Copying builds to incomingbuilds"
cp -r -v builds/* incomingbuilds/linux32/
ls -al incomingbuilds/linux32