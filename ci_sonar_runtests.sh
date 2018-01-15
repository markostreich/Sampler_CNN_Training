#!/bin/bash

mkdir log -p
mkdir log/unittests -p
# touch log/unittests/dummy
shopt -s nullglob

echo "CI: run tests"
echo "Current path for CI job execution: $CI_PROJECT_DIR"

# Programs compiled with -fprofile-args and/or --coverage create the .gcda profiling
# information by default at the (absolute!) path where the object file was located
# during the compilation process. Since the gitlab CI pipline is executed in a different
# location than for every job, we must reconfigure this behavior to use a relative path
# by stripping the path prefix
export GCOV_PREFIX_STRIP=7
export GCOV_PREFIX=./

NR_GCDA_BEFORE=`find . | grep gcda | wc -l`
NR_GCNO=`find . | grep gcno | wc -l`

echo "GCC profiling: $NR_GCNO gcno files and $NR_GCDA_BEFORE gcda files available."
echo "Performing tests in bin subfolder."
for f in ./_install/linux64/bin/*_test
do
	echo "Current Testfile - $f"

	$f --gtest_output=xml:./log/unittests/$(basename "$f").xml
done

echo "Performing tests in bin/debug subfolder."
for f in ./_install/linux64/bin/debug/*_test
do
        echo "Current Testfile - $f"

        $f --gtest_output=xml:./log/unittests/$(basename "$f").xml
done

NR_GCDA_AFTER=`find . | grep gcda | wc -l`

let NR_GCDA_GENERATED=NR_GCDA_AFTER-NR_GCDA_BEFORE

echo "GCC profiling: $NR_GCNO gcno files, $NR_GCDA_GENERATED new gcda profiling files generated"
echo "Execution of all tests completed."
