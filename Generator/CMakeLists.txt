cmake_minimum_required(VERSION 3.21)

if(PROJECT_NAME)
    set(host_dir "${CMAKE_CURRENT_BINARY_DIR}/_host")
    file(MAKE_DIRECTORY "${host_dir}")

    execute_process(
        COMMAND ${CMAKE_COMMAND}
            "${CMAKE_CURRENT_SOURCE_DIR}"
            -DCMAKE_BUILD_TYPE=Release
        WORKING_DIRECTORY "${host_dir}"
        RESULT_VARIABLE result)

    if(result)
        message(FATAL_ERROR "Failed to configure ResourceGenerator")
    endif()

    execute_process(
        COMMAND ${CMAKE_COMMAND} --build "${host_dir}" --config Release
        RESULT_VARIABLE result)

    if(_result)
        message(FATAL_ERROR "Failed to build ResourceGenerator")
    endif()

    add_executable(ResourceGenerator IMPORTED GLOBAL)
    set_target_properties(ResourceGenerator PROPERTIES
        IMPORTED_LOCATION "${host_dir}/ResourceGenerator")
else()
    project(ResourceGenerator CXX)
    set(CMAKE_CXX_STANDARD 20)
    add_executable(ResourceGenerator ResourceGenerator.cpp)
endif()
