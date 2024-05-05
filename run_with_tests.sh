#/usr/bin/env bash

# ctest --test-dir build/tests --output-on-failure) && ./build/bin/ty tests/src/basic.ty

export GTEST_COLOR=1

FILE="tests/src/just_add.ty"

CWD=$(pwd)
RES="$(ctest --test-dir build/tests --output-on-failure -j)\n"
if [ $? -eq 0 ]; then
    echo "$($CWD/build/bin/ty "$FILE")"
else
    echo "$RES"
fi
