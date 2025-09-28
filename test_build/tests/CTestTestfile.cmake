# CMake generated Testfile for 
# Source directory: /home/seksan/workspace/streamdab-analyzer/tests
# Build directory: /home/seksan/workspace/streamdab-analyzer/test_build/tests
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test([=[unit_tests]=] "/home/seksan/workspace/streamdab-analyzer/test_build/tests/eti_analyzer_unit_tests")
set_tests_properties([=[unit_tests]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/seksan/workspace/streamdab-analyzer/tests/CMakeLists.txt;335;add_test;/home/seksan/workspace/streamdab-analyzer/tests/CMakeLists.txt;0;")
add_test([=[gui_tests]=] "/home/seksan/workspace/streamdab-analyzer/test_build/tests/eti_analyzer_gui_tests")
set_tests_properties([=[gui_tests]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/seksan/workspace/streamdab-analyzer/tests/CMakeLists.txt;337;add_test;/home/seksan/workspace/streamdab-analyzer/tests/CMakeLists.txt;0;")
add_test([=[e2e_tests]=] "/home/seksan/workspace/streamdab-analyzer/test_build/tests/eti_analyzer_e2e_tests")
set_tests_properties([=[e2e_tests]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/seksan/workspace/streamdab-analyzer/tests/CMakeLists.txt;339;add_test;/home/seksan/workspace/streamdab-analyzer/tests/CMakeLists.txt;0;")
add_test([=[eti_analyzer_unit_tests]=] "/home/seksan/workspace/streamdab-analyzer/test_build/tests/eti_analyzer_unit_tests")
set_tests_properties([=[eti_analyzer_unit_tests]=] PROPERTIES  LABELS "UNIT" PRIORITY "1" TIMEOUT "30" _BACKTRACE_TRIPLES "/home/seksan/workspace/streamdab-analyzer/tests/tdd_workflow.cmake;56;add_test;/home/seksan/workspace/streamdab-analyzer/tests/CMakeLists.txt;343;tdd_register_test;/home/seksan/workspace/streamdab-analyzer/tests/CMakeLists.txt;0;")
add_test([=[eti_analyzer_gui_tests]=] "/home/seksan/workspace/streamdab-analyzer/test_build/tests/eti_analyzer_gui_tests")
set_tests_properties([=[eti_analyzer_gui_tests]=] PROPERTIES  LABELS "GUI" PRIORITY "2" TIMEOUT "90" _BACKTRACE_TRIPLES "/home/seksan/workspace/streamdab-analyzer/tests/tdd_workflow.cmake;56;add_test;/home/seksan/workspace/streamdab-analyzer/tests/CMakeLists.txt;344;tdd_register_test;/home/seksan/workspace/streamdab-analyzer/tests/CMakeLists.txt;0;")
add_test([=[eti_analyzer_e2e_tests]=] "/home/seksan/workspace/streamdab-analyzer/test_build/tests/eti_analyzer_e2e_tests")
set_tests_properties([=[eti_analyzer_e2e_tests]=] PROPERTIES  LABELS "E2E" PRIORITY "1" TIMEOUT "120" _BACKTRACE_TRIPLES "/home/seksan/workspace/streamdab-analyzer/tests/tdd_workflow.cmake;56;add_test;/home/seksan/workspace/streamdab-analyzer/tests/CMakeLists.txt;345;tdd_register_test;/home/seksan/workspace/streamdab-analyzer/tests/CMakeLists.txt;0;")
add_test([=[performance_tests]=] "/home/seksan/workspace/streamdab-analyzer/test_build/tests/eti_analyzer_performance_tests")
set_tests_properties([=[performance_tests]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/seksan/workspace/streamdab-analyzer/tests/CMakeLists.txt;546;add_test;/home/seksan/workspace/streamdab-analyzer/tests/CMakeLists.txt;0;")
subdirs("gui")
subdirs("e2e_gui")
subdirs("etsi_standards")
subdirs("network")
subdirs("network_streaming")
