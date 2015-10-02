#!/bin/sh

PACKAGEDIR=packages
PACKAGENAME=minetest-c55-win32-`date +%y%m%d%H%M%S`
PACKAGEPATH=$PACKAGEDIR/$PACKAGENAME

mkdir -p $PACKAGEPATH
mkdir -p $PACKAGEPATH/bin
mkdir -p $PACKAGEPATH/data
mkdir -p $PACKAGEPATH/doc

cp bin/minetest.exe $PACKAGEPATH/bin/
cp bin/Irrlicht.dll $PACKAGEPATH/bin/

cp -r data/stone.png $PACKAGEPATH/data/
cp -r data/grass.png $PACKAGEPATH/data/
cp -r data/fontlucida.png $PACKAGEPATH/data/
cp -r data/water.png $PACKAGEPATH/data/
cp -r data/tf.jpg $PACKAGEPATH/data/

cp -r doc/README.txt $PACKAGEPATH/doc/README.txt

cd $PACKAGEDIR
rm $PACKAGENAME.zip
zip -r $PACKAGENAME.zip $PACKAGENAME

