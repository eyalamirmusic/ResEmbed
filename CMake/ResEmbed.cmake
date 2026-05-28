# res_embed_add — embed resources into ${TARGET}.
#
# Modes (mutually exclusive):
#   FILES <file1> ...           explicit list. The set of files is fixed at
#                               configure time, but each file's CONTENTS are
#                               watched at build time via a depfile.
#
#   MANIFEST <file>             newline-separated list of paths, produced by
#                               an upstream rule. Re-runs whenever the
#                               manifest changes; depfile catches content
#                               changes.
#
#   SCAN_DIR <dir>              walk the directory at build time. Combine
#                               with DEPENDS pointing at the stamp that
#                               produces <dir>'s contents so structural
#                               changes (new/removed files) propagate.
#
#   DIRECTORY <dir>             back-compat alias for SCAN_DIR.
#
# Common:
#   NAMESPACE <ns>              C++ namespace + generated file basename
#                               (default: Resources).
#   CATEGORY <cat>              runtime category key (default: Resources).
#   BASE_DIRECTORY <dir>        compute resource keys relative to this dir
#                               (default: filename only).
#   DEPENDS <targets|files>     extra dependencies (typically the stamp of
#                               an upstream code-generation step).
function(res_embed_add TARGET)
    cmake_parse_arguments(PARSE_ARGV 1 ARG ""
            "CATEGORY;NAMESPACE;SCAN_DIR;MANIFEST;DIRECTORY;BASE_DIRECTORY"
            "FILES;DEPENDS")

    if(NOT ARG_CATEGORY)
        set(ARG_CATEGORY "Resources")
    endif()

    if(NOT ARG_NAMESPACE)
        set(ARG_NAMESPACE "Resources")
    endif()

    if(ARG_DIRECTORY AND NOT ARG_SCAN_DIR)
        set(ARG_SCAN_DIR "${ARG_DIRECTORY}")
    endif()

    set(_mode_count 0)
    if(ARG_FILES)
        math(EXPR _mode_count "${_mode_count} + 1")
    endif()
    if(ARG_SCAN_DIR)
        math(EXPR _mode_count "${_mode_count} + 1")
    endif()
    if(ARG_MANIFEST)
        math(EXPR _mode_count "${_mode_count} + 1")
    endif()
    if(NOT _mode_count EQUAL 1)
        message(FATAL_ERROR
                "res_embed_add(${TARGET}): specify exactly one of FILES, "
                "SCAN_DIR (or DIRECTORY), MANIFEST")
    endif()

    get_target_property(TARGET_BINARY_DIR ${TARGET} BINARY_DIR)
    set(GENERATED_DIR "${TARGET_BINARY_DIR}/${TARGET}-${ARG_NAMESPACE}-Generated")
    file(MAKE_DIRECTORY "${GENERATED_DIR}")

    set(DATA_CPP "${GENERATED_DIR}/${ARG_NAMESPACE}.cpp")
    set(HEADER "${GENERATED_DIR}/${ARG_NAMESPACE}.h")
    set(REGISTER_CPP "${GENERATED_DIR}/${ARG_NAMESPACE}_Register.cpp")
    set(DEPFILE "${GENERATED_DIR}/${ARG_NAMESPACE}.d")

    set(GEN_ARGS
            --namespace "${ARG_NAMESPACE}"
            --category "${ARG_CATEGORY}"
            --output-cpp "${DATA_CPP}"
            --output-h "${HEADER}"
            --output-register "${REGISTER_CPP}"
            --depfile "${DEPFILE}")

    if(ARG_BASE_DIRECTORY)
        list(APPEND GEN_ARGS --base-directory "${ARG_BASE_DIRECTORY}")
    endif()

    set(EXTRA_DEPS "")

    if(ARG_FILES)
        set(_abs_files "")
        foreach(_f IN LISTS ARG_FILES)
            cmake_path(ABSOLUTE_PATH _f
                    BASE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" NORMALIZE)
            list(APPEND _abs_files "${_f}")
        endforeach()

        foreach(_f IN LISTS _abs_files)
            target_sources(${TARGET} PRIVATE "${_f}")
            set_source_files_properties("${_f}"
                    PROPERTIES HEADER_FILE_ONLY TRUE)
        endforeach()

        set(_manifest "${GENERATED_DIR}/${ARG_NAMESPACE}.manifest")
        list(JOIN _abs_files "\n" _manifest_body)
        file(WRITE "${_manifest}" "${_manifest_body}\n")

        list(APPEND GEN_ARGS --manifest "${_manifest}")
        list(APPEND EXTRA_DEPS "${_manifest}")
    elseif(ARG_MANIFEST)
        list(APPEND GEN_ARGS --manifest "${ARG_MANIFEST}")
        list(APPEND EXTRA_DEPS "${ARG_MANIFEST}")
    else()
        list(APPEND GEN_ARGS --scan-dir "${ARG_SCAN_DIR}")

        # Legacy DIRECTORY mode preserves the old "auto-detect files
        # added or removed" behavior via a discarded CONFIGURE_DEPENDS
        # glob. The glob result is intentionally unused — the build-time
        # scan produces the file list. We just want CMake to notice
        # directory churn between builds so the next `cmake --build`
        # reconfigures.
        #
        # SCAN_DIR mode skips this on purpose: build-output directories
        # (vite dist, codegen output) should not be globbed at configure
        # time. Callers pass DEPENDS pointing at the upstream stamp
        # instead.
        if(ARG_DIRECTORY)
            file(GLOB_RECURSE _res_embed_dir_watch
                    CONFIGURE_DEPENDS "${ARG_DIRECTORY}/*")
        endif()
    endif()

    add_custom_command(
            OUTPUT "${DATA_CPP}" "${HEADER}" "${REGISTER_CPP}"
            COMMAND ResourceGenerator generate ${GEN_ARGS}
            DEPENDS ${EXTRA_DEPS} ${ARG_DEPENDS} ResourceGenerator
            DEPFILE "${DEPFILE}"
            COMMENT "Embedding ${ARG_NAMESPACE} resources for ${TARGET}"
            VERBATIM)

    target_include_directories(${TARGET} PUBLIC "${GENERATED_DIR}")
    target_link_libraries(${TARGET} PUBLIC ResEmbed)
    target_sources(${TARGET} PRIVATE "${DATA_CPP}" "${HEADER}")

    # The register cpp holds the static initializer that registers the
    # resources at startup. For static libraries the linker may dead-strip
    # an object whose only symbols are referenced by a static initializer,
    # so we propagate it as an INTERFACE source — the consuming executable
    # (or shared library) compiles it directly, which guarantees the
    # initializer ends up in the final binary.
    #
    # The generator expression filters out static-library consumers in
    # the propagation chain: a static lib that compiles the registrar
    # produces an archive member with no externally-visible symbols
    # (only the anonymous-namespace registrar instance), which trips a
    # `ranlib: has no symbols` warning. Skipping static-lib consumers
    # avoids the empty archive members; executables, shared libs,
    # modules, and object libs all still compile the TU, so the static
    # initializer still lands in the final binary.
    get_target_property(TARGET_TYPE ${TARGET} TYPE)
    if(TARGET_TYPE STREQUAL "STATIC_LIBRARY")
        target_sources(${TARGET} INTERFACE
            $<$<NOT:$<STREQUAL:$<TARGET_PROPERTY:TYPE>,STATIC_LIBRARY>>:${REGISTER_CPP}>)
    else()
        target_sources(${TARGET} PRIVATE ${REGISTER_CPP})
    endif()
endfunction()
