find_program(OBJCOPY arm-none-eabi-objcopy REQUIRED)

# relocate_to_ram(TARGET SOURCE_NAME)
#
# SOURCE_NAME: source filename without path, e.g. "resample.cpp" or "queue.c"
#
# At PRE_LINK, locates every compiled object whose name ends with
# <SOURCE_NAME>.obj or <SOURCE_NAME>.o under the build tree and renames its
# .text / .rodata sections to .time_critical.* so the linker places the code
# in SRAM rather than XIP flash.  This prevents XIP bus contention between
# core0 and core1 on RP2350.
function(relocate_to_ram TARGET SOURCE_NAME)
    string(MAKE_C_IDENTIFIER "${SOURCE_NAME}" _label)
    add_custom_command(TARGET ${TARGET} PRE_LINK
        COMMAND "${CMAKE_COMMAND}"
            "-DOBJCOPY=${OBJCOPY}"
            "-DBUILD_DIR=${CMAKE_CURRENT_BINARY_DIR}"
            "-DOBJ_NAME=${SOURCE_NAME}"
            "-DLABEL=${_label}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/do_relocate.cmake"
        COMMENT "RAM-relocating hot path: ${SOURCE_NAME}"
        VERBATIM
    )
endfunction()
