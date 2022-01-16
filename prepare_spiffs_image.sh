#!/bin/sh

PROJECT_DIR=$1
BUILD_DIR=$2

SPIFFS_BUILD_DIR=${BUILD_DIR}/spiffs-image

cp -r ${PROJECT_DIR}/spiffs ${SPIFFS_BUILD_DIR}
rm ${SPIFFS_BUILD_DIR}/w/.DS_Store &

find -E ${SPIFFS_BUILD_DIR} -regex ".*(\.html|\.css|\.js|\.json|\.ico)" | xargs -I {} gzip -9 {}
