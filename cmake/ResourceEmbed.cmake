function(embed_resources TARGET)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "" "CATEGORY" "FILES")

    if(NOT ARG_CATEGORY)
        set(ARG_CATEGORY "Resources")
    endif()

    get_target_property(RESOURCE_INDEX ${TARGET} RESOURCE_EMBED_INDEX)
    if(NOT RESOURCE_INDEX)
        set(RESOURCE_INDEX 0)
    endif()

    get_target_property(EXTERN_DECLS ${TARGET} RESOURCE_EMBED_EXTERN_DECLS)
    if(NOT EXTERN_DECLS)
        set(EXTERN_DECLS "")
    endif()

    get_target_property(ENTRY_LIST ${TARGET} RESOURCE_EMBED_ENTRY_LIST)
    if(NOT ENTRY_LIST)
        set(ENTRY_LIST "")
    endif()

    set(GENERATED_C_FILES "")

    foreach(INPUT_FILE IN LISTS ARG_FILES)
        cmake_path(IS_ABSOLUTE INPUT_FILE IS_ABS)
        if(IS_ABS)
            set(ABSOLUTE_INPUT "${INPUT_FILE}")
        else()
            set(ABSOLUTE_INPUT "${CMAKE_CURRENT_SOURCE_DIR}/${INPUT_FILE}")
        endif()
        get_filename_component(RESOURCE_NAME "${INPUT_FILE}" NAME)
        set(VAR_PREFIX "resource_${RESOURCE_INDEX}")
        set(OUTPUT_C "${CMAKE_CURRENT_BINARY_DIR}/BinaryResource${RESOURCE_INDEX}.c")

        add_custom_command(
            OUTPUT "${OUTPUT_C}"
            COMMAND ResourceGenerator "${ABSOLUTE_INPUT}" "${OUTPUT_C}" "${VAR_PREFIX}"
            DEPENDS "${ABSOLUTE_INPUT}" ResourceGenerator
            COMMENT "Embedding resource ${RESOURCE_NAME} from ${INPUT_FILE} [${ARG_CATEGORY}]"
        )

        string(APPEND EXTERN_DECLS "extern const unsigned char ${VAR_PREFIX}_data[];\nextern const unsigned long ${VAR_PREFIX}_size;\n")
        string(APPEND ENTRY_LIST "    {${VAR_PREFIX}_data, ${VAR_PREFIX}_size, \"${RESOURCE_NAME}\", \"${ARG_CATEGORY}\"},\n")

        list(APPEND GENERATED_C_FILES "${OUTPUT_C}")
        math(EXPR RESOURCE_INDEX "${RESOURCE_INDEX} + 1")
    endforeach()

    set_target_properties(${TARGET} PROPERTIES
        RESOURCE_EMBED_INDEX ${RESOURCE_INDEX}
        RESOURCE_EMBED_EXTERN_DECLS "${EXTERN_DECLS}"
        RESOURCE_EMBED_ENTRY_LIST "${ENTRY_LIST}"
    )

    set(HEADER_FILE "${CMAKE_CURRENT_BINARY_DIR}/ResourceData.h")
    file(WRITE "${HEADER_FILE}"
        "#pragma once\n\n"
        "#include \"ResourceEmbedLib.h\"\n\n"
        "${EXTERN_DECLS}\n"
        "namespace Resources\n"
        "{\n"
        "static const Entries resource_entries = {\n"
        "${ENTRY_LIST}"
        "};\n\n"
        "static const Initializer resourceInitializer {resource_entries};\n"
        "}\n"
    )

    set(INIT_CPP "${CMAKE_CURRENT_BINARY_DIR}/ResourceInit.cpp")
    file(WRITE "${INIT_CPP}" "#include \"ResourceData.h\"\n")

    get_target_property(INIT_ADDED ${TARGET} RESOURCE_EMBED_INIT_ADDED)
    if(NOT INIT_ADDED)
        target_sources(${TARGET} PRIVATE "${INIT_CPP}")
        target_include_directories(${TARGET} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
        set_target_properties(${TARGET} PROPERTIES RESOURCE_EMBED_INIT_ADDED TRUE)
    endif()

    target_sources(${TARGET} PRIVATE ${GENERATED_C_FILES})
endfunction()

function(embed_resource_directory TARGET)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "" "CATEGORY;DIRECTORY" "")

    file(GLOB_RECURSE FILES "${ARG_DIRECTORY}/*.*")

    set(FORWARD_ARGS FILES ${FILES})
    if(DEFINED ARG_CATEGORY)
        list(APPEND FORWARD_ARGS CATEGORY "${ARG_CATEGORY}")
    endif()

    embed_resources(${TARGET} ${FORWARD_ARGS})
endfunction()
