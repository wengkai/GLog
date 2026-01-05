cmake_minimum_required(VERSION 3.10)

set(LIB_NAME adif)

aux_source_directory(. ADIF_FILES)

add_library(${LIB_NAME} STATIC
        ${ADIF_FILES}

)

