add_library(usermod_uqr INTERFACE)

target_sources(usermod_uqr INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/moduqr.c
)

target_include_directories(usermod_uqr INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(usermod INTERFACE usermod_uqr)
