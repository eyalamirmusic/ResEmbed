function(embed_resources TARGET)
    foreach(INPUT_FILE IN LISTS ARGN)
        get_filename_component(RESOURCE_NAME "${INPUT_FILE}" NAME_WE)
        set(OUTPUT_CPP "${CMAKE_CURRENT_BINARY_DIR}/${RESOURCE_NAME}_resource.cpp")

        add_custom_command(
            OUTPUT "${OUTPUT_CPP}"
            COMMAND ResourceGenerator "${CMAKE_CURRENT_SOURCE_DIR}/${INPUT_FILE}" "${OUTPUT_CPP}" "${RESOURCE_NAME}"
            DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${INPUT_FILE}" ResourceGenerator
            COMMENT "Embedding resource ${RESOURCE_NAME} from ${INPUT_FILE}"
        )

        target_sources(${TARGET} PRIVATE "${OUTPUT_CPP}")
    endforeach()
endfunction()
