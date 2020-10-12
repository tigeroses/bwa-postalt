cmake -Htest -Bbuild/test
cmake --build build/test

if [[ $? == 0 ]];
then
    echo "Compile success!"
    # CTEST_OUTPUT_ON_FAILURE=1 cmake --build build/test --target test
    ./build/test/Bwa-PostaltTests
else
    echo "Compile failed!"
fi
