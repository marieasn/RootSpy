#!/bin/bash

# build the tarball in a temp folder
TMPDIR=`mktemp -d` || exit 1
echo "building in " $TMPDIR "..."

# copy the base structure
svn export https://halldsvn.jlab.org/repos/trunk/online/packages/RootSpy/pkg $TMPDIR/RootSpy

# copy the packages
svn export https://halldsvn.jlab.org/repos/trunk/online/packages/RootSpy $TMPDIR/RootSpy/RootSpy
svn export https://halldsvn.jlab.org/repos/trunk/online/packages/DRootSpy $TMPDIR/RootSpy/DRootSpy


# clean out bad files 
rm -f $TMPDIR/RootSpy/make_dist_tarball.sh
rm -f $TMPDIR/RootSpy/make_dist_tarball.sh~

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
