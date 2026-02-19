function(res_embed_add TARGET)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "" "CATEGORY;DIRECTORY;NAMESPACE" "FILES")

    if(NOT ARG_CATEGORY)
        set(ARG_CATEGORY "Resources")
    endif()

    if(NOT ARG_NAMESPACE)
        set(ARG_NAMESPACE "Resources")
    endif()

    if(DEFINED ARG_DIRECTORY AND DEFINED ARG_FILES)
        message(FATAL_ERROR "res_embed_add: DIRECTORY and FILES are mutually exclusive")
    elseif(NOT DEFINED ARG_DIRECTORY AND NOT DEFINED ARG_FILES)
        message(FATAL_ERROR "res_embed_add: either DIRECTORY or FILES must be specified")
    endif()

    if(DEFINED ARG_DIRECTORY)
        file(GLOB_RECURSE ARG_FILES
                CONFIGURE_DEPENDS
                "${ARG_DIRECTORY}/*.*")
    endif()

    get_target_property(TARGET_BINARY_DIR ${TARGET} BINARY_DIR)
    set(GENERATED_DIR "${TARGET_BINARY_DIR}/${TARGET}-${ARG_NAMESPACE}-Generated")
    file(MAKE_DIRECTORY "${GENERATED_DIR}")

    set(ABSOLUTE_FILES "")
    foreach(INPUT_FILE IN LISTS ARG_FILES)
        cmake_path(ABSOLUTE_PATH INPUT_FILE BASE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" NORMALIZE)
        list(APPEND ABSOLUTE_FILES "${INPUT_FILE}")

        target_sources(${TARGET} PRIVATE ${INPUT_FILE})
        set_source_files_properties(
                ${INPUT_FILE}
                PROPERTIES HEADER_FILE_ONLY TRUE
        )
    endforeach()

    set(GENERATED_FILES "${GENERATED_DIR}/${ARG_NAMESPACE}.h" "${GENERATED_DIR}/${ARG_NAMESPACE}.cpp")
    list(LENGTH ABSOLUTE_FILES FILE_COUNT)
    if(FILE_COUNT GREATER 0)
        math(EXPR LAST_INDEX "${FILE_COUNT} - 1")
        foreach(I RANGE ${LAST_INDEX})
            list(APPEND GENERATED_FILES "${GENERATED_DIR}/BinaryResource${I}.c")
        endforeach()
    endif()

    add_custom_command(
        OUTPUT ${GENERATED_FILES}
        COMMAND ResourceGenerator generate "${GENERATED_DIR}" "${ARG_NAMESPACE}" "${ARG_CATEGORY}" ${ABSOLUTE_FILES}
        COMMAND ${CMAKE_COMMAND} -E touch ${GENERATED_FILES}
        DEPENDS ${ABSOLUTE_FILES} ResourceGenerator
        COMMENT "Generating embedded resources for ${ARG_NAMESPACE}"
        VERBATIM
    )

    target_include_directories(${TARGET} PUBLIC "${GENERATED_DIR}")
    target_link_libraries(${TARGET} PUBLIC ResourceEmbedLib)
    target_sources(${TARGET} PRIVATE ${GENERATED_FILES})
endfunction()
