function(embed_resources TARGET)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "" "CATEGORY;HEADER" "FILES")

    if(NOT ARG_CATEGORY)
        set(ARG_CATEGORY "Resources")
    endif()

    if(NOT ARG_HEADER)
        set(ARG_HEADER "ResourceData.h")
    endif()

    get_target_property(RESOURCE_INDEX ${TARGET} RESOURCE_EMBED_INDEX)
    if(NOT RESOURCE_INDEX)
        set(RESOURCE_INDEX 0)
    endif()

    set(HEADER_FILE "${CMAKE_CURRENT_BINARY_DIR}/${ARG_HEADER}")
    set(INIT_CPP "${CMAKE_CURRENT_BINARY_DIR}/ResourceInit.cpp")
    set(GENERATED_C_FILES "")
    set(GENERATOR_ARGS "${HEADER_FILE}" "${ARG_CATEGORY}")
    set(INPUT_DEPS "")

    foreach(INPUT_FILE IN LISTS ARG_FILES)
        cmake_path(IS_ABSOLUTE INPUT_FILE IS_ABS)
        if(IS_ABS)
            set(ABSOLUTE_INPUT "${INPUT_FILE}")
        else()
            set(ABSOLUTE_INPUT "${CMAKE_CURRENT_SOURCE_DIR}/${INPUT_FILE}")
        endif()
        get_filename_component(RESOURCE_NAME "${INPUT_FILE}" NAME)
        set(OUTPUT_C "${CMAKE_CURRENT_BINARY_DIR}/BinaryResource${RESOURCE_INDEX}.c")

        list(APPEND GENERATOR_ARGS "${ABSOLUTE_INPUT}" "${OUTPUT_C}" "${RESOURCE_NAME}")
        list(APPEND GENERATED_C_FILES "${OUTPUT_C}")
        list(APPEND INPUT_DEPS "${ABSOLUTE_INPUT}")
        math(EXPR RESOURCE_INDEX "${RESOURCE_INDEX} + 1")
    endforeach()

    set_target_properties(${TARGET} PROPERTIES
        RESOURCE_EMBED_INDEX ${RESOURCE_INDEX}
    )

    add_custom_command(
        OUTPUT ${GENERATED_C_FILES} "${HEADER_FILE}"
        COMMAND ResourceGenerator embed ${GENERATOR_ARGS}
        DEPENDS ${INPUT_DEPS} ResourceGenerator
        COMMENT "Embedding ${ARG_CATEGORY} resources"
    )

    get_target_property(INIT_ADDED ${TARGET} RESOURCE_EMBED_INIT_ADDED)
    if(NOT INIT_ADDED)
        add_custom_command(
            OUTPUT "${INIT_CPP}"
            COMMAND ResourceGenerator init "${INIT_CPP}" "${ARG_HEADER}"
            DEPENDS ResourceGenerator
            COMMENT "Generating ResourceInit.cpp"
        )
        target_sources(${TARGET} PRIVATE "${INIT_CPP}")
        target_include_directories(${TARGET} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
        set_target_properties(${TARGET} PROPERTIES RESOURCE_EMBED_INIT_ADDED TRUE)
    endif()

    target_sources(${TARGET} PRIVATE ${GENERATED_C_FILES})
endfunction()

function(embed_resource_directory TARGET)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "" "CATEGORY;DIRECTORY;HEADER" "")

    file(GLOB_RECURSE FILES "${ARG_DIRECTORY}/*.*")

    set(FORWARD_ARGS FILES ${FILES})
    if(DEFINED ARG_CATEGORY)
        list(APPEND FORWARD_ARGS CATEGORY "${ARG_CATEGORY}")
    endif()
    if(DEFINED ARG_HEADER)
        list(APPEND FORWARD_ARGS HEADER "${ARG_HEADER}")
    endif()

    embed_resources(${TARGET} ${FORWARD_ARGS})
endfunction()
