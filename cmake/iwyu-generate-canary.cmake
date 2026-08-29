if(NOT DEFINED HEADER_REL OR NOT DEFINED CANARY_FILE)
    message(FATAL_ERROR "iwyu-generate-canary.cmake requires -DHEADER_REL and -DCANARY_FILE")
endif()

file(WRITE "${CANARY_FILE}" "#include \"${HEADER_REL}\"\n")
