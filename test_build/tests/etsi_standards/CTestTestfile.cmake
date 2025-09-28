# CMake generated Testfile for 
# Source directory: /home/seksan/workspace/streamdab-analyzer/tests/etsi_standards
# Build directory: /home/seksan/workspace/streamdab-analyzer/test_build/tests/etsi_standards
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test([=[etsi_comprehensive_compliance_tests]=] "/home/seksan/workspace/streamdab-analyzer/test_build/tests/etsi_standards/eti_analyzer_etsi_comprehensive_tests")
set_tests_properties([=[etsi_comprehensive_compliance_tests]=] PROPERTIES  LABELS "ETSI;Compliance;Standards;TDD" TIMEOUT "300" _BACKTRACE_TRIPLES "/home/seksan/workspace/streamdab-analyzer/tests/etsi_standards/CMakeLists.txt;178;add_test;/home/seksan/workspace/streamdab-analyzer/tests/etsi_standards/CMakeLists.txt;0;")
