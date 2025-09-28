# CMake generated Testfile for 
# Source directory: /home/seksan/workspace/streamdab-analyzer/tests/network
# Build directory: /home/seksan/workspace/streamdab-analyzer/test_build/tests/network
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test([=[NetworkTDD_EtiOverIpReceiver]=] "/home/seksan/workspace/streamdab-analyzer/test_build/tests/network/test_eti_over_ip_receiver_tdd")
set_tests_properties([=[NetworkTDD_EtiOverIpReceiver]=] PROPERTIES  LABELS "Network;TDD;Integration;Equipment" TIMEOUT "420" WORKING_DIRECTORY "/home/seksan/workspace/streamdab-analyzer/test_build/tests/network" _BACKTRACE_TRIPLES "/home/seksan/workspace/streamdab-analyzer/tests/network/CMakeLists.txt;371;add_test;/home/seksan/workspace/streamdab-analyzer/tests/network/CMakeLists.txt;0;")
add_test([=[NetworkTDD_StreamingProcessor]=] "/home/seksan/workspace/streamdab-analyzer/test_build/tests/network/test_streaming_processor_tdd")
set_tests_properties([=[NetworkTDD_StreamingProcessor]=] PROPERTIES  LABELS "Network;TDD;Integration" TIMEOUT "300" WORKING_DIRECTORY "/home/seksan/workspace/streamdab-analyzer/test_build/tests/network" _BACKTRACE_TRIPLES "/home/seksan/workspace/streamdab-analyzer/tests/network/CMakeLists.txt;377;add_test;/home/seksan/workspace/streamdab-analyzer/tests/network/CMakeLists.txt;0;")
add_test([=[NetworkTDD_BroadcastEquipmentIntegration]=] "/home/seksan/workspace/streamdab-analyzer/test_build/tests/network/test_broadcast_equipment_integration_tdd")
set_tests_properties([=[NetworkTDD_BroadcastEquipmentIntegration]=] PROPERTIES  LABELS "Network;TDD;Integration;Equipment" TIMEOUT "420" WORKING_DIRECTORY "/home/seksan/workspace/streamdab-analyzer/test_build/tests/network" _BACKTRACE_TRIPLES "/home/seksan/workspace/streamdab-analyzer/tests/network/CMakeLists.txt;383;add_test;/home/seksan/workspace/streamdab-analyzer/tests/network/CMakeLists.txt;0;")
add_test([=[NetworkTDD_PerformanceValidationFramework]=] "/home/seksan/workspace/streamdab-analyzer/test_build/tests/network/test_performance_validation_framework_tdd")
set_tests_properties([=[NetworkTDD_PerformanceValidationFramework]=] PROPERTIES  LABELS "Network;TDD;Performance;Extended" TIMEOUT "600" WORKING_DIRECTORY "/home/seksan/workspace/streamdab-analyzer/test_build/tests/network" _BACKTRACE_TRIPLES "/home/seksan/workspace/streamdab-analyzer/tests/network/CMakeLists.txt;389;add_test;/home/seksan/workspace/streamdab-analyzer/tests/network/CMakeLists.txt;0;")
add_test([=[NetworkTDD_ProfessionalWorkflowIntegration]=] "/home/seksan/workspace/streamdab-analyzer/test_build/tests/network/test_professional_workflow_integration_tdd")
set_tests_properties([=[NetworkTDD_ProfessionalWorkflowIntegration]=] PROPERTIES  LABELS "Network;TDD;Integration" TIMEOUT "300" WORKING_DIRECTORY "/home/seksan/workspace/streamdab-analyzer/test_build/tests/network" _BACKTRACE_TRIPLES "/home/seksan/workspace/streamdab-analyzer/tests/network/CMakeLists.txt;395;add_test;/home/seksan/workspace/streamdab-analyzer/tests/network/CMakeLists.txt;0;")
