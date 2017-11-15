cmake_minimum_required(VERSION 2.8)

project(dockd)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -pthread)

set(srcs
    "main.cpp"
    "crtc.cpp"
)

set(hdrs
    "crtc.h"
)

add_executable(${PROJECT_NAME} ${srcs} ${hdrs})

target_link_libraries(${PROJECT_NAME} X11 Xrandr udev thinkpad)

install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION bin)

