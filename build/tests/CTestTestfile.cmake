# CMake generated Testfile for 
# Source directory: /Users/seksan/workspace/streamdab-analyzer/tests
# Build directory: /Users/seksan/workspace/streamdab-analyzer/build/tests
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
include("/Users/seksan/workspace/streamdab-analyzer/build/tests/eti_analyzer_unit_tests[1]_include.cmake")
add_test([=[unit_tests]=] "/Users/seksan/workspace/streamdab-analyzer/build/tests/eti_analyzer_unit_tests.app/Contents/MacOS/eti_analyzer_unit_tests")
set_tests_properties([=[unit_tests]=] PROPERTIES  _BACKTRACE_TRIPLES "/Users/seksan/workspace/streamdab-analyzer/tests/CMakeLists.txt;187;add_test;/Users/seksan/workspace/streamdab-analyzer/tests/CMakeLists.txt;0;")
add_test([=[gui_tests]=] "/Users/seksan/workspace/streamdab-analyzer/build/tests/eti_analyzer_gui_tests.app/Contents/MacOS/eti_analyzer_gui_tests")
set_tests_properties([=[gui_tests]=] PROPERTIES  _BACKTRACE_TRIPLES "/Users/seksan/workspace/streamdab-analyzer/tests/CMakeLists.txt;189;add_test;/Users/seksan/workspace/streamdab-analyzer/tests/CMakeLists.txt;0;")
