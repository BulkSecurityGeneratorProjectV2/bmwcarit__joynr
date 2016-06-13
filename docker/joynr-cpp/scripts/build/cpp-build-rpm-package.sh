#!/bin/bash

source /data/src/docker/joynr-base/scripts/global.sh

includetests=OFF

function usage
{
    echo "usage: cpp-build-rpm-package.sh [--includetests ON|OFF]"
    echo "default is: includetests $includetests"
}

while [ "$1" != "" ]; do
    case $1 in
        -t | --includetests )   shift
                                includetests="$1"
                                ;;
        -h | --help )           usage
                                exit
                                ;;
        * )                     usage
                                exit 1
    esac
    shift
done

log "Enable core dumps"
ulimit -c unlimited

START=$(date +%s)

log "ENVIRONMENT"
env

# prepare RPM environment
mkdir /data/build/joynr/package
mkdir /data/build/joynr/package/RPM
mkdir /data/build/joynr/package/RPM/BUILD
mkdir /data/build/joynr/package/RPM/BUILDROOT
mkdir /data/build/joynr/package/RPM/RPMS
mkdir /data/build/joynr/package/RPM/SOURCES
mkdir /data/build/joynr/package/RPM/SPECS
mkdir /data/build/joynr/package/RPM/SRPMS
mkdir /data/build/joynr/package/RPM/joynr

# copy RPM spec file
cp /data/src/cpp/distribution/joynr.spec /data/build/joynr/package/RPM/SPECS

cd /data/build/joynr

SRCDIR=/data/src
DESTDIR=/data/build/joynr/package/RPM/joynr
log "BUILD RPM PACKAGE"
make -j $JOBS install DESTDIR=$DESTDIR

rpm_with_flags=""

if [ "ON" == "${includetests}" ]
then
    cd /data/build/tests
    make -j $JOBS install DESTDIR=$DESTDIR
    rpm_with_flags="--with performancetests"
fi

# Install the C++ standalone generator
JOYNR_GENERATOR_SRCDIR=$SRCDIR/tools/generator/joynr-generator-standalone
JOYNR_GENERATOR_SCRIPT_SRCDIR=$JOYNR_GENERATOR_SRCDIR/target/scripts
JOYNR_GENERATOR_SHELLSCRIPT=joynr-generator
JOYNR_GENERATOR_SHELLSCRIPT_DESTDIR=$DESTDIR/usr/bin
JOYNR_GENERATOR_CONFIG_CMAKE=JoynrGeneratorConfig.cmake
JOYNR_GENERATOR_CONFIG_CMAKE_VERSION=JoynrGeneratorConfigVersion.cmake
JOYNR_GENERATOR_CONFIG_CMAKE_SRC=$JOYNR_GENERATOR_SCRIPT_SRCDIR/$JOYNR_GENERATOR_CONFIG_CMAKE
JOYNR_GENERATOR_CONFIG_CMAKE_DESTDIR=$DESTDIR/usr/lib64/cmake/JoynrGenerator
JOYNR_GENERATOR_JAR_DESTDIR=$DESTDIR/usr/libexec/joynr

# copy the generator's JAR archive
mkdir -p $JOYNR_GENERATOR_JAR_DESTDIR
chmod 755 $JOYNR_GENERATOR_JAR_DESTDIR
cp $JOYNR_GENERATOR_SRCDIR/target/*.jar $JOYNR_GENERATOR_JAR_DESTDIR
chmod 644 $JOYNR_GENERATOR_JAR_DESTDIR/*.jar
JOYNR_GENERATOR_JAR=`basename $JOYNR_GENERATOR_SRCDIR/target/*.jar`

# copy and tweak the generator's script
sed "s/JOYNR_GENERATOR_JAR=.*/JOYNR_GENERATOR_JAR=\/usr\/libexec\/joynr\/$JOYNR_GENERATOR_JAR/" < $JOYNR_GENERATOR_SCRIPT_SRCDIR/$JOYNR_GENERATOR_SHELLSCRIPT > $JOYNR_GENERATOR_SHELLSCRIPT_DESTDIR/$JOYNR_GENERATOR_SHELLSCRIPT
chmod 755 $JOYNR_GENERATOR_SHELLSCRIPT_DESTDIR/$JOYNR_GENERATOR_SHELLSCRIPT

# copy the generator's cmake version file
mkdir -p $JOYNR_GENERATOR_CONFIG_CMAKE_DESTDIR
chmod 755 $JOYNR_GENERATOR_CONFIG_CMAKE_DESTDIR
cp $JOYNR_GENERATOR_SCRIPT_SRCDIR/$JOYNR_GENERATOR_CONFIG_CMAKE_VERSION $JOYNR_GENERATOR_CONFIG_CMAKE_DESTDIR

# copy and tweak generator's cmake file
sed "s/set(JoynrGenerator_JAR.*)/set(JoynrGenerator_JAR \"\/usr\/libexec\/joynr\/$JOYNR_GENERATOR_JAR\")/"< $JOYNR_GENERATOR_CONFIG_CMAKE_SRC > $JOYNR_GENERATOR_CONFIG_CMAKE_DESTDIR/$JOYNR_GENERATOR_CONFIG_CMAKE

cd /data/build/joynr/package/RPM/SPECS
rpmbuild -bb ${rpm_with_flags} --buildroot $DESTDIR joynr.spec

END=$(date +%s)
DIFF=$(( $END - $START ))
log "build RPM package time: $DIFF seconds"
