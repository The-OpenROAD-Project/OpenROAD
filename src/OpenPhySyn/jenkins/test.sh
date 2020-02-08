docker run  -u $(id -u ${USER}):$(id -g ${USER}) --rm -v $(pwd):/OpenPhySyn openroad/openphysyn bash -c "cd /OpenPhySyn/build && ./unit_tests"
