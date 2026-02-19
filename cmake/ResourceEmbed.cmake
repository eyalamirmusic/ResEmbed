function(embed_resources TARGET)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "" "CATEGORY;NAMESPACE" "FILES")

    if(NOT ARG_CATEGORY)
        set(ARG_CATEGORY "Resources")
    endif()

    if(NOT ARG_NAMESPACE)
        set(ARG_NAMESPACE "Resources")
    endif()

    set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/${ARG_NAMESPACE}")
    file(MAKE_DIRECTORY "${GENERATED_DIR}")

    set(HEADER_FILE "${GENERATED_DIR}/${ARG_NAMESPACE}.h")
    set(ENTRIES_CPP "${GENERATED_DIR}/${ARG_NAMESPACE}.cpp")
    set(GENERATED_C_FILES "")
    set(ABSOLUTE_FILES "")

    set(FILE_INDEX 0)
    foreach(INPUT_FILE IN LISTS ARG_FILES)
        cmake_path(IS_ABSOLUTE INPUT_FILE IS_ABS)
        if(IS_ABS)
            set(ABSOLUTE_INPUT "${INPUT_FILE}")
        else()
            set(ABSOLUTE_INPUT "${CMAKE_CURRENT_SOURCE_DIR}/${INPUT_FILE}")
        endif()

        list(APPEND ABSOLUTE_FILES "${ABSOLUTE_INPUT}")
        math(EXPR FILE_INDEX "${FILE_INDEX} + 1")
    endforeach()

    set(INPUT_FILE_LIST "${GENERATED_DIR}/input_file_list")
    set(newline_delimited_input "")
    foreach(F IN LISTS ABSOLUTE_FILES)
        string(APPEND newline_delimited_input "${F}\n")
    endforeach()

    set(old_input_file_list "")
    if(EXISTS "${INPUT_FILE_LIST}")
        file(READ "${INPUT_FILE_LIST}" old_input_file_list)
    endif()
    if(NOT "${old_input_file_list}" STREQUAL "${newline_delimited_input}")
        file(WRITE "${INPUT_FILE_LIST}" "${newline_delimited_input}")
    endif()

    set(FILE_INDEX 0)
    foreach(ABSOLUTE_INPUT IN LISTS ABSOLUTE_FILES)
        set(OUTPUT_C "${GENERATED_DIR}/BinaryResource${FILE_INDEX}.c")

        target_sources(${TARGET} PRIVATE ${ABSOLUTE_INPUT})

        set_source_files_properties(
                ${ABSOLUTE_INPUT}
                PROPERTIES HEADER_FILE_ONLY TRUE
        )

        add_custom_command(
            OUTPUT "${OUTPUT_C}"
            COMMAND ResourceGenerator embed "${ABSOLUTE_INPUT}" "${OUTPUT_C}" "${FILE_INDEX}" "${ARG_NAMESPACE}"
            DEPENDS "${ABSOLUTE_INPUT}" ResourceGenerator "${INPUT_FILE_LIST}"
            COMMENT "Embedding ${ABSOLUTE_INPUT}"
            VERBATIM
        )

        list(APPEND GENERATED_C_FILES "${OUTPUT_C}")
        math(EXPR FILE_INDEX "${FILE_INDEX} + 1")
    endforeach()

    add_custom_command(
        OUTPUT "${HEADER_FILE}" "${ENTRIES_CPP}"
        COMMAND ResourceGenerator init "${GENERATED_DIR}" "${ARG_NAMESPACE}" "${ARG_CATEGORY}" "${INPUT_FILE_LIST}"
        DEPENDS ResourceGenerator "${INPUT_FILE_LIST}"
        COMMENT "Generating ${ARG_NAMESPACE}.h and ${ARG_NAMESPACE}.cpp"
        VERBATIM
    )

    target_include_directories(${TARGET} PUBLIC "${GENERATED_DIR}")
    target_link_libraries(${TARGET} PUBLIC ResourceEmbedLib)
    target_sources(${TARGET} PRIVATE ${GENERATED_C_FILES} "${ENTRIES_CPP}" "${HEADER_FILE}")
    #set_source_files_properties(${GENERATED_C_FILES} PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
endfunction()

function(embed_resource_directory TARGET)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "" "CATEGORY;DIRECTORY;NAMESPACE" "")

    file(GLOB_RECURSE FILES "${ARG_DIRECTORY}/*.*")

    set(FORWARD_ARGS FILES ${FILES})
    if(DEFINED ARG_CATEGORY)
        list(APPEND FORWARD_ARGS CATEGORY "${ARG_CATEGORY}")
    endif()
    if(DEFINED ARG_NAMESPACE)
        list(APPEND FORWARD_ARGS NAMESPACE "${ARG_NAMESPACE}")
    endif()

    embed_resources(${TARGET} ${FORWARD_ARGS})
endfunction()
