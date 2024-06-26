#/usr/bin/env bash

# ctest --test-dir build/tests --output-on-failure) && ./build/bin/ty tests/src/basic.ty

export GTEST_COLOR=1

CWD=$(pwd)
# FILE="tests/basic_tests/just_add.ty"
# FILE="tests/basic_tests/add_with_output.ty"
# FILE="tests/basic_tests/just_multiply.ty"
# FILE="tests/basic_tests/check_inbuilt.ty"
# FILE="tests/basic_tests/triple_add.ty"
# FILE="tests/intermediate_tests/if.ty"
# FILE="tests/intermediate_tests/if2.ty"
# FILE="tests/intermediate_tests/if3.ty"
# FILE="tests/intermediate_tests/phi_propagate.ty"
# FILE="tests/intermediate_tests/prof_basic.ty"
# FILE="tests/complex_tests/while.ty"
# FILE="tests/complex_tests/while_if.ty"
# FILE="tests/complex_tests/while_2x.ty"
# FILE="tests/complex_tests/complex.ty"
# FILE="tests/complex_tests/phi_nesting.ty"
# FILE="tests/complex_tests/simple_func.ty"
# FILE="tests/complex_tests/simple_func2.ty"
# FILE="tests/complex_tests/add_func.ty"
# FILE="tests/complex_tests/fibonacci.ty"
# FILE="tests/complex_tests/nested_while.tiny"
# FILE="tests/complex_tests/func_2x.ty"
# FILE="tests/complex_tests/gcd.tiny"
# FILE="tests/complex_tests/nested_if_while.tny"
# FILE="tests/complex_tests/testComplexVoidCall.txt"
FILE="tests/complex_tests/mandelbrot.tiny"

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
# echo "$($CWD/build/bin/ty "$FILE")"
