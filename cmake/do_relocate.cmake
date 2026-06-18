# Build-time helper – invoked by relocate_to_ram() in relocate_to_ram.cmake.
# Variables passed via -D flags: OBJCOPY  BUILD_DIR  OBJ_NAME  LABEL
#
# OBJ_NAME is the source filename (e.g. "resample.cpp").
# We search for *<OBJ_NAME>.obj and *<OBJ_NAME>.o to handle both Windows and
# Linux CMake build trees.
file(GLOB_RECURSE _objs LIST_DIRECTORIES false
    "${BUILD_DIR}/*${OBJ_NAME}.obj"
    "${BUILD_DIR}/*${OBJ_NAME}.o"
)
if(NOT _objs)
    message(WARNING "relocate_to_ram: no compiled object for '${OBJ_NAME}' found under ${BUILD_DIR} — skipping")
    return()
endif()
foreach(_obj IN LISTS _objs)
    execute_process(
        COMMAND "${OBJCOPY}"
            --rename-section .text=.time_critical.${LABEL}
            --rename-section .rodata=.time_critical.${LABEL}.rodata
            "${_obj}"
        RESULT_VARIABLE _rc
    )
    if(_rc)
        message(FATAL_ERROR "objcopy rename-section failed for ${_obj}: exit ${_rc}")
    endif()
endforeach()
