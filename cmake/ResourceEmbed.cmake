function(embed_resources TARGET)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "" "CATEGORY" "")

    if(NOT ARG_CATEGORY)
        set(ARG_CATEGORY "Resources")
    endif()

    foreach(INPUT_FILE IN LISTS ARG_UNPARSED_ARGUMENTS)
        get_filename_component(RESOURCE_NAME "${INPUT_FILE}" NAME_WE)
        set(OUTPUT_CPP "${CMAKE_CURRENT_BINARY_DIR}/${RESOURCE_NAME}_resource.cpp")

        add_custom_command(
            OUTPUT "${OUTPUT_CPP}"
            COMMAND ResourceGenerator "${CMAKE_CURRENT_SOURCE_DIR}/${INPUT_FILE}" "${OUTPUT_CPP}" "${RESOURCE_NAME}" "${ARG_CATEGORY}"
            DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${INPUT_FILE}" ResourceGenerator
            COMMENT "Embedding resource ${RESOURCE_NAME} from ${INPUT_FILE} [${ARG_CATEGORY}]"
        )

        target_sources(${TARGET} PRIVATE "${OUTPUT_CPP}")
    endforeach()
endfunction()
