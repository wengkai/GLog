set(LIB_NAME adif)

aux_source_directory(. ADIF_FILES)

add_library(${LIB_NAME} STATIC
        ${ADIF_FILES}

)

target_include_directories(${LIB_NAME} PUBLIC
    BEFORE
        .
        ./constants
)
