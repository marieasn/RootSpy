#!/bin/bash

# build the tarball in a temp folder
TMPDIR=`mktemp -d` || exit 1
echo "building in " $TMPDIR "..."

# copy the base structure
mkdir $TMPDIR/RootSpy
cp -R * $TMPDIR/RootSpy/

# copy the packages
svn export https://halldsvn.jlab.org/repos/trunk/online/packages/RootSpy $TMPDIR/RootSpy/RootSpy
svn export https://halldsvn.jlab.org/repos/trunk/online/packages/DRootSpy $TMPDIR/RootSpy/DRootSpy

#cp -R ../RootSpy-standalone/RootSpy $TMPDIR/RootSpy/
#cp -R ../RootSpy-standalone/DRootSpy $TMPDIR/DRootSpy/

# clean out bad files 
rm $TMPDIR/RootSpy/make_dist_tarball.sh
rm $TMPDIR/RootSpy/make_dist_tarball.sh~

# convert build files
find $TMPDIR/RootSpy -name SConstruct -exec perl -pi -e 's/halld/rootspy/g' '{}' \;
find $TMPDIR/RootSpy -name SConscript -exec perl -pi -e 's/halld/rootspy/g' '{}' \;


# make the tarball
CWD=`pwd`
cd $TMPDIR/
tar czf RootSpy.tar.gz RootSpy/
cp RootSpy.tar.gz $CWD
cd $CWD

# clean up
rm -rf $TMPDIR
