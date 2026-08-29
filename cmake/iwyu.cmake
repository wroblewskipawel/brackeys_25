find_program(IWYU_EXECUTABLE NAMES include-what-you-use iwyu NO_CACHE)

if(IWYU_EXECUTABLE)
    include(${CMAKE_SOURCE_DIR}/cmake/iwyu-ignore.cmake)
    execute_process(
        COMMAND ${CMAKE_CXX_COMPILER} --print-resource-dir
        OUTPUT_VARIABLE IWYU_RESOURCE_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
 
    set(IWYU_COMMAND ${IWYU_EXECUTABLE})
    list(APPEND IWYU_COMMAND -Xiwyu --mapping_file=${CMAKE_SOURCE_DIR}/iwyu.imp)
    list(APPEND IWYU_COMMAND -Xiwyu --error_always)
    list(APPEND IWYU_COMMAND -resource-dir=${IWYU_RESOURCE_DIR})

    file(GLOB_RECURSE IWYU_PUBLIC_HEADERS ${CMAKE_SOURCE_DIR}/include/*.h)
    list(REMOVE_ITEM IWYU_PUBLIC_HEADERS ${IWYU_IGNORE_HEADERS})

    set(IWYU_CANARY_DIR ${CMAKE_BINARY_DIR}/iwyu_canaries)
    set(IWYU_CANARY_SOURCES)

    foreach(HEADER ${IWYU_PUBLIC_HEADERS})
        file(RELATIVE_PATH HEADER_REL ${CMAKE_SOURCE_DIR}/include ${HEADER})
        get_filename_component(HEADER_REL_DIR ${HEADER_REL} DIRECTORY)
        get_filename_component(HEADER_STEM ${HEADER_REL} NAME_WE)
        set(CANARY_FILE ${IWYU_CANARY_DIR}/${HEADER_REL_DIR}/${HEADER_STEM}.cpp)
        
        add_custom_command(
            OUTPUT ${CANARY_FILE}
            COMMAND ${CMAKE_COMMAND}
                    -DHEADER_REL=${HEADER_REL}
                    -DCANARY_FILE=${CANARY_FILE}
                    -P ${CMAKE_SOURCE_DIR}/cmake/iwyu-generate-canary.cmake
            DEPENDS ${HEADER} ${CMAKE_SOURCE_DIR}/cmake/iwyu-generate-canary.cmake
            COMMENT "Generating IWYU canary for ${HEADER_REL}"
        )
        list(APPEND IWYU_CANARY_SOURCES ${CANARY_FILE})
    endforeach()

    add_library(iwyu_headers OBJECT EXCLUDE_FROM_ALL ${IWYU_CANARY_SOURCES})
    target_include_directories(iwyu_headers PRIVATE ${PROJECT_INCLUDE_DIRS})
    target_compile_definitions(iwyu_headers PRIVATE GLM_FORCE_QUAT_DATA_XYZW)
    set_target_properties(iwyu_headers PROPERTIES CXX_INCLUDE_WHAT_YOU_USE "${IWYU_COMMAND}")
else()
    message(STATUS "include-what-you-use not found - 'iwyu_headers' target disabled")
endif()
