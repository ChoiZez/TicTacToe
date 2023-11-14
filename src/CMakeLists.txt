find_package(FLTK REQUIRED)

add_executable(client tcpclient.cpp)
add_executable(server tcpserver.cpp)

add_subdirectory(logger)
add_subdirectory(tictactoe)
add_subdirectory(userData)
add_subdirectory(clientLib)

target_include_directories(client PRIVATE ${FLTK_INCLUDE_DIR})
target_link_libraries(client
        PRIVATE
        ${FLTK_LIBRARIES}
        logger
        clientLib
)

target_link_libraries(client PRIVATE X11 Xext pthread Xinerama Xfixes Xcursor Xft Xrender fontconfig)
target_link_libraries(server
        PRIVATE
        logger
        tictactoe
        userData
)
configure_file(.db ${CMAKE_CURRENT_BINARY_DIR}/.db COPYONLY)
configure_file(client.config ${CMAKE_CURRENT_BINARY_DIR}/client.config COPYONLY)
configure_file(server.config ${CMAKE_CURRENT_BINARY_DIR}/server.config COPYONLY)
