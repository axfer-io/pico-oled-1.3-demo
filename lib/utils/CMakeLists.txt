add_library(utils STATIC
    utils.c
)

target_include_directories(utils PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)
