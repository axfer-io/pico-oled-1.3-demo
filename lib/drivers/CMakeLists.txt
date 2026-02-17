add_library(drivers STATIC
    drivers.c
)

target_include_directories(drivers PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(drivers
    pico_stdlib
)
