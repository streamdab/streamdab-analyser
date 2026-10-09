# Runs every listed QtTest function of one test executable in its OWN process.
#   cmake -DEXE=<test exe> -DFUNCS=<fn1|fn2|...> -P run_each_test_function.cmake
# Used on Windows for test_dock_interactions: building several top-level
# DABAnalyserWindow objects one after another in a single process crashes
# inside Qt's accessibility layer (see the note in tests/CMakeLists.txt).
# Every function still runs; the script fails if any of them fails.
string(REPLACE "|" ";" _funcs "${FUNCS}")
set(_failed "")
foreach(_f IN LISTS _funcs)
    message(STATUS "=== ${_f}")
    execute_process(COMMAND "${EXE}" "${_f}" RESULT_VARIABLE _rc)
    if(NOT _rc EQUAL 0)
        list(APPEND _failed "${_f} (exit ${_rc})")
    endif()
endforeach()
if(_failed)
    string(REPLACE ";" "\n  " _list "${_failed}")
    message(FATAL_ERROR "Failed test functions:\n  ${_list}")
endif()
