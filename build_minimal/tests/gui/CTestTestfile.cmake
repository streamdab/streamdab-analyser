# CMake generated Testfile for 
# Source directory: /home/seksan/workspace/streamdab-analyzer/tests/gui
# Build directory: /home/seksan/workspace/streamdab-analyzer/build_minimal/tests/gui
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test([=[ETI_UI_Integration_TDD_Test]=] "/home/seksan/workspace/streamdab-analyzer/build_minimal/tests/test_eti_ui_integration")
set_tests_properties([=[ETI_UI_Integration_TDD_Test]=] PROPERTIES  ENVIRONMENT "QT_QPA_PLATFORM=offscreen" LABELS "GUI;TDD;Integration;Bangkok_ETI" TIMEOUT "60" WORKING_DIRECTORY "/home/seksan/workspace/streamdab-analyzer" _BACKTRACE_TRIPLES "/home/seksan/workspace/streamdab-analyzer/tests/gui/CMakeLists.txt;43;add_test;/home/seksan/workspace/streamdab-analyzer/tests/gui/CMakeLists.txt;0;")
