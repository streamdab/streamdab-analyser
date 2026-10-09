# Run from FetchContent PATCH_COMMAND inside the Qt-ADS source dir.
# Makes Versioning.cmake fall back to ADS_PINNED_VERSION when `git describe`
# finds no tag (idempotent).
set(_f "cmake/modules/Versioning.cmake")
file(READ "${_f}" _c)
if(NOT _c MATCHES "ADS_PINNED_VERSION_FALLBACK")
    set(_needle [=[string(REGEX REPLACE "^v" "" GIT_DESC "${GIT_DESC_RAW}")]=])
    set(_fallback [=[
# ADS_PINNED_VERSION_FALLBACK
if(NOT GIT_DESC)
    set(GIT_DESC "@ADS_PINNED_VERSION@")
endif()]=])
    string(CONFIGURE "${_fallback}" _fallback @ONLY)
    string(FIND "${_c}" "${_needle}" _pos)
    if(_pos EQUAL -1)
        message(FATAL_ERROR "Qt-ADS Versioning.cmake layout changed; update patch_ads_versioning.cmake")
    endif()
    string(REPLACE "${_needle}" "${_needle}\n${_fallback}" _c "${_c}")
    file(WRITE "${_f}" "${_c}")
endif()
