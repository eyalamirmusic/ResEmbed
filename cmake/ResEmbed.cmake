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

    set(DATA_FILES "")
    list(LENGTH ABSOLUTE_FILES FILE_COUNT)
    if(FILE_COUNT GREATER 0)
        math(EXPR LAST_INDEX "${FILE_COUNT} - 1")
        foreach(I RANGE ${LAST_INDEX})
            list(GET ABSOLUTE_FILES ${I} INPUT_FILE)
            set(OUTPUT_FILE "${GENERATED_DIR}/BinaryResource${I}.c")
            set(VAR_PREFIX "${ARG_NAMESPACE}_${I}")

            add_custom_command(
                OUTPUT "${OUTPUT_FILE}"
                COMMAND ResourceGenerator generate-data "${OUTPUT_FILE}" "${VAR_PREFIX}" "${INPUT_FILE}"
                DEPENDS "${INPUT_FILE}" ResourceGenerator
                COMMENT "Embedding ${INPUT_FILE}"
                VERBATIM
            )

            list(APPEND DATA_FILES "${OUTPUT_FILE}")
        endforeach()
    endif()

    set(CONFIG_FILE "${GENERATED_DIR}/${ARG_NAMESPACE}.cfg")
    list(JOIN ABSOLUTE_FILES "\n" FILE_LIST)
    file(WRITE "${CONFIG_FILE}" "${GENERATED_DIR}\n${ARG_NAMESPACE}\n${ARG_CATEGORY}\n${FILE_LIST}\n")

    set(REGISTRY_FILES "${GENERATED_DIR}/${ARG_NAMESPACE}.h" "${GENERATED_DIR}/${ARG_NAMESPACE}.cpp")

    add_custom_command(
        OUTPUT ${REGISTRY_FILES}
        COMMAND ResourceGenerator generate-registry "${CONFIG_FILE}"
        DEPENDS ${CONFIG_FILE} ResourceGenerator
        COMMENT "Generating resource registry for ${ARG_NAMESPACE}"
        VERBATIM
    )

    target_include_directories(${TARGET} PUBLIC "${GENERATED_DIR}")
    target_link_libraries(${TARGET} PUBLIC ResEmbed)
    target_sources(${TARGET} PRIVATE ${DATA_FILES} ${REGISTRY_FILES})
endfunction()
