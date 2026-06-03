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
#   TU_COUNT <n>                number of .c data translation units to emit
#                               (see below). Default: one per resource in
#                               FILES mode, 8 in the build-time modes.
#
# Translation-unit layout:
#   The embedded bytes are emitted as plain-C `.c` files (compiled by the C
#   front end, which digests large byte arrays far faster than C++) plus a
#   small `.cpp` registry. Resources are round-robined across N data TUs:
#
#     - FILES mode knows the resource count at configure time, so N defaults
#       to one `.c` per resource — the finest grain, so editing one resource
#       recompiles only its `.c`. TU_COUNT caps N below the resource count.
#
#     - SCAN_DIR/MANIFEST/DIRECTORY resolve their list at build time, so N
#       can't track the resource count; it defaults to 8 (override with
#       TU_COUNT). Resources are distributed across the N buckets; if there
#       are fewer resources than buckets the surplus `.c` files are empty.
#
#   All N `.c` files compile in parallel. To coalesce them into fewer/bigger
#   TUs, turn on CMake's native unity build for the target
#   (`set_target_properties(<t> PROPERTIES UNITY_BUILD ON)`, tune with
#   `UNITY_BUILD_BATCH_SIZE`); the generated `.c` files participate like any
#   other source.
#
#   If the consuming project hasn't enabled the C language, all modes fall
#   back to a single combined `.cpp` compiled as C++.
function(res_embed_add TARGET)
    cmake_parse_arguments(PARSE_ARGV 1 ARG ""
            "CATEGORY;NAMESPACE;SCAN_DIR;MANIFEST;DIRECTORY;BASE_DIRECTORY;TU_COUNT"
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

    # Decide the TU layout (see the header comment). The split layout emits
    # the bytes as plain-C .c files; it needs the C language enabled. When C
    # isn't available we fall back to a single combined .cpp compiled as C++.
    get_property(_res_embed_languages GLOBAL PROPERTY ENABLED_LANGUAGES)
    if("C" IN_LIST _res_embed_languages)
        set(_use_split TRUE)
    else()
        set(_use_split FALSE)
    endif()

    if(DEFINED ARG_TU_COUNT AND NOT ARG_TU_COUNT MATCHES "^[1-9][0-9]*$")
        message(FATAL_ERROR
                "res_embed_add(${TARGET}): TU_COUNT must be a positive "
                "integer, got '${ARG_TU_COUNT}'.")
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

    # Per-resource/per-bucket .c outputs (split layout only); empty otherwise
    # so the ${C_FILES} expansions below vanish.
    set(C_FILES "")

    # File count is only known at configure time in FILES mode; left empty
    # for the build-time discovery modes (used to size the bucket count).
    set(_file_count "")

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

        list(LENGTH _abs_files _file_count)
    elseif(ARG_MANIFEST)
        list(APPEND GEN_ARGS --manifest "${ARG_MANIFEST}")
        list(APPEND EXTRA_DEPS "${ARG_MANIFEST}")
    else()
        # The generator runs in CMAKE_CURRENT_BINARY_DIR, so a relative
        # SCAN_DIR/DIRECTORY would resolve into the build tree and find
        # nothing. Anchor it to the source dir (matching how FILES paths are
        # resolved); already-absolute build-output dirs are left untouched.
        cmake_path(ABSOLUTE_PATH ARG_SCAN_DIR
                BASE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" NORMALIZE)
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

    # Bucket count: the generator round-robins resources across exactly this
    # many .c files (<NS>_0.c ... <NS>_<N-1>.c), which CMake must enumerate
    # now. In FILES mode the resource count is known, so the default is one
    # bucket per resource (finest-grained incremental builds); TU_COUNT caps
    # it. In the build-time discovery modes the count is unknown, so we use a
    # fixed bucket count (TU_COUNT or 8) — surplus buckets become harmless
    # empty TUs when there are fewer resources than buckets.
    if(_use_split)
        if(ARG_FILES)
            if(DEFINED ARG_TU_COUNT AND ARG_TU_COUNT LESS _file_count)
                set(_tu_count ${ARG_TU_COUNT})
            else()
                set(_tu_count ${_file_count})
            endif()
        else()
            if(DEFINED ARG_TU_COUNT)
                set(_tu_count ${ARG_TU_COUNT})
            else()
                set(_tu_count 8)
            endif()
        endif()

        # A FILES set can legitimately be empty of buckets only if it had no
        # files, which the mode guard already rejects; guard anyway so the
        # RANGE below is well-formed.
        if(_tu_count GREATER 0)
            list(APPEND GEN_ARGS --split-count ${_tu_count})

            math(EXPR _last_bucket "${_tu_count} - 1")
            foreach(_b RANGE ${_last_bucket})
                list(APPEND C_FILES
                        "${GENERATED_DIR}/${ARG_NAMESPACE}_${_b}.c")
            endforeach()
        endif()
    endif()

    add_custom_command(
            OUTPUT "${DATA_CPP}" "${HEADER}" "${REGISTER_CPP}" ${C_FILES}
            COMMAND ResourceGenerator generate ${GEN_ARGS}
            DEPENDS ${EXTRA_DEPS} ${ARG_DEPENDS} ResourceGenerator
            DEPFILE "${DEPFILE}"
            COMMENT "Embedding ${ARG_NAMESPACE} resources for ${TARGET}"
            VERBATIM)

    target_include_directories(${TARGET} PUBLIC "${GENERATED_DIR}")
    target_link_libraries(${TARGET} PUBLIC ResEmbed)
    target_sources(${TARGET} PRIVATE "${DATA_CPP}" "${HEADER}" ${C_FILES})

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
