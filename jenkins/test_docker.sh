set -e	
TARGET_OS="$1"
TARGET_COMPILER="$2"
docker run "openroad/openroad_${TARGET_OS}_${TARGET_COMPILER}" bash -c "/OpenROAD/test/regression"
