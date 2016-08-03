#!/bin/bash

source /data/src/docker/joynr-base/scripts/global.sh

START=$(date +%s)
ADDITIONAL_CMAKE_ARGS=''
DLT='ON'

function usage
{
    echo "usage: cpp-radio-app.sh [--jobs X  --additionalcmakeargs <args>]"
    echo "default jobs is $JOBS and additionalcmakeargs is $ADDITIONAL_CMAKE_ARGS"
}

while [ "$1" != "" ]; do
    case $1 in
        --dlt )                 shift
                                DLT=$1
                                ;;
        --jobs )                shift
                                JOBS=$1
                                ;;
        --additionalcmakeargs ) shift
                                ADDITIONAL_CMAKE_ARGS=$1
                                ;;
        * )                     usage
                                exit 1
    esac
    shift
done

log "CPP RADIO APP JOBS: $JOBS"

log "ENVIRONMENT"
env
echo "ADDITIONAL_CMAKE_ARGS is $ADDITIONAL_CMAKE_ARGS"

# fail on first error
set -e

# radio currently also generating for Java, so install the java-generator too
# until we can exclude the Java build of radio using @executionId (see below)
cd /data/src/
mvn clean install -P no-license-and-notice,no-java-formatter,no-checkstyle -DskipTests \
--projects \
io.joynr.tools.generator:generator-framework,\
io.joynr.tools.generator:joynr-generator-maven-plugin,\
io.joynr.tools.generator:java-generator,\
io.joynr.tools.generator:cpp-generator,\
io.joynr.examples:radio-app

rm -rf /data/build/radio
mkdir /data/build/radio
cd /data/build/radio

cmake -DJOYNR_ENABLE_DLT_LOGGING=$DLT \
      -DCMAKE_PREFIX_PATH=$JOYNR_INSTALL_DIR \
      -DJOYNR_SERVER=localhost:8080 \
      $ADDITIONAL_CMAKE_ARGS \
      /data/src/examples/radio-app

time make -j $JOBS

END=$(date +%s)
DIFF=$(( $END - $START ))
log "Radio App build time: $DIFF seconds"
