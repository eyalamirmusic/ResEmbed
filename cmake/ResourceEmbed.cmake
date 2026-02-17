function(embed_resources TARGET)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "" "CATEGORY;HEADER" "FILES")

    if(NOT ARG_CATEGORY)
        set(ARG_CATEGORY "Resources")
    endif()

    if(NOT ARG_HEADER)
        set(ARG_HEADER "ResourceData.h")
    endif()

    get_filename_component(HEADER_STEM "${ARG_HEADER}" NAME_WE)
    set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/${HEADER_STEM}")
    file(MAKE_DIRECTORY "${GENERATED_DIR}")

    set(HEADER_FILE "${GENERATED_DIR}/${ARG_HEADER}")
    set(ENTRIES_CPP "${GENERATED_DIR}/${HEADER_STEM}.cpp")
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
        list(APPEND GENERATED_C_FILES "${GENERATED_DIR}/BinaryResource${FILE_INDEX}.c")
        math(EXPR FILE_INDEX "${FILE_INDEX} + 1")
    endforeach()

    list(JOIN ABSOLUTE_FILES "," FILES_CSV)

    add_custom_command(
        OUTPUT ${GENERATED_C_FILES} "${ENTRIES_CPP}"
        COMMAND ResourceGenerator embed "${ENTRIES_CPP}" "${ARG_CATEGORY}" "${FILES_CSV}"
        DEPENDS ${ABSOLUTE_FILES} ResourceGenerator
        COMMENT "Embedding ${ARG_CATEGORY} resources"
    )

    get_target_property(INIT_ADDED ${TARGET} RESOURCE_EMBED_INIT_ADDED)
    if(NOT INIT_ADDED)
        add_custom_command(
            OUTPUT "${HEADER_FILE}"
            COMMAND ResourceGenerator init "${HEADER_FILE}"
            DEPENDS ResourceGenerator
            COMMENT "Generating ${ARG_HEADER}"
        )
        target_sources(${TARGET} PRIVATE "${HEADER_FILE}")
        target_include_directories(${TARGET} PUBLIC "${GENERATED_DIR}")
        target_link_libraries(${TARGET} PUBLIC ResourceEmbedLib)
        set_target_properties(${TARGET} PROPERTIES RESOURCE_EMBED_INIT_ADDED TRUE)
    endif()

    target_sources(${TARGET} PRIVATE ${GENERATED_C_FILES} "${ENTRIES_CPP}")
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
