# CMake generated Testfile for 
# Source directory: /home/seksan/workspace/streamdab-analyzer/tests
# Build directory: /home/seksan/workspace/streamdab-analyzer/build_test/tests
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test([=[unit_tests]=] "/home/seksan/workspace/streamdab-analyzer/build_test/tests/eti_analyzer_unit_tests")
set_tests_properties([=[unit_tests]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/seksan/workspace/streamdab-analyzer/tests/CMakeLists.txt;281;add_test;/home/seksan/workspace/streamdab-analyzer/tests/CMakeLists.txt;0;")
add_test([=[gui_tests]=] "/home/seksan/workspace/streamdab-analyzer/build_test/tests/eti_analyzer_gui_tests")
set_tests_properties([=[gui_tests]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/seksan/workspace/streamdab-analyzer/tests/CMakeLists.txt;283;add_test;/home/seksan/workspace/streamdab-analyzer/tests/CMakeLists.txt;0;")
add_test([=[performance_tests]=] "/home/seksan/workspace/streamdab-analyzer/build_test/tests/eti_analyzer_performance_tests")
set_tests_properties([=[performance_tests]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/seksan/workspace/streamdab-analyzer/tests/CMakeLists.txt;459;add_test;/home/seksan/workspace/streamdab-analyzer/tests/CMakeLists.txt;0;")
subdirs("../_deps/googletest-build")
