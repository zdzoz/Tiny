#/usr/bin/env bash

# ctest --test-dir build/tests --output-on-failure) && ./build/bin/ty tests/src/basic.ty

export GTEST_COLOR=1

CWD=$(pwd)
# FILE="tests/basic_tests/just_add.ty"
# FILE="tests/basic_tests/just_multiply.ty"
# FILE="tests/basic_tests/check_inbuilt.ty"
# FILE="tests/basic_tests/triple_add.ty"
FILE="tests/intermediate_tests/if.ty"
# FILE="tests/complex_tests/if.ty"

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
