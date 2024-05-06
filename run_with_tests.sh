#/usr/bin/env bash

# ctest --test-dir build/tests --output-on-failure) && ./build/bin/ty tests/src/basic.ty

export GTEST_COLOR=1

CWD=$(pwd)
FILE="tests/basic_tests/check_inbuilt.ty"

if [ ! -f "$CWD/$FILE" ]; then
    echo "failed to find file: $FILE"
    exit 1
fi

RES="$(ctest --test-dir build/tests --output-on-failure -j)\n"
if [ $? -eq 0 ]; then
    echo "$($CWD/build/bin/ty "$FILE")"
else
    echo "$RES"
fi
