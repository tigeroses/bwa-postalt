# Bwa-Postalt

Use C++ multi-threading parallel to speed up bwa-postalt.js for processing sam data.

## Usage

### Build and run the standalone target

Use the following command to build executable target.

```bash
# ./script/build.sh
cmake -Hstandalone -Bbuild/standalone
cmake --build build/standalone
```

To run the executable target.

```bash
cat input.sam | ./build/standalone/Bwa-Postalt input.alt > output.sam
```

The program read sam data from stdin and write the sam data to stdout using pipelines of linux platform.

### Build and run test suite

Use the following commands from the project's root directory to run the test suite.

```bash
# ./script/test.sh
cmake -Htest -Bbuild/test
cmake --build build/test

if [[ $? == 0 ]];
then
    echo "Compile success!"
    # CTEST_OUTPUT_ON_FAILURE=1 cmake --build build/test --target test
    # or simply call the executable: 
    ./build/test/Bwa-PostaltTests
else
    echo "Compile failed!"
fi
```

To collect code coverage information, run CMake with the `-DENABLE_TEST_COVERAGE=1` option.

### Build the documentation

The documentation is automatically built and [published](https://thelartians.github.io/ModernCppStarter) whenever a [GitHub Release](https://help.github.com/en/github/administering-a-repository/managing-releases-in-a-repository) is created.
To manually build documentation, call the following command.

```bash
# ./script/doc.sh
cmake -Hdocumentation -Bbuild/doc
cmake --build build/doc --target GenerateDocs
# view the docs
# open build/doc/doxygen/html/index.html
```

To build the documentation locally, you will need Doxygen, jinja2 and Pygments on installed your system.

## Performance

TODO
