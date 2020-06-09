git submodule update --init --recursive

echo $UNITY_THISISABUILDMACHINE

perl external/buildscripts/build_classlibs_osx.pl
if [ $? -eq 0 ]
then
  echo "mono build script ran successfully"
else
  echo "mono build script failed" >&2
  exit 1
fi

mkdir -p incomingbuilds/classlibs
cp -r ZippedClasslibs.tar.gz incomingbuilds/classlibs/
cd incomingbuilds/classlibs
tar -pzxf ZippedClasslibs.tar.gz
rm -f ZippedClasslibs.tar.gz
cd ../..