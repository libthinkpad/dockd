cmake_minimum_required(VERSION 2.8)

project(dockd)

set(CMAKE_CXX_STANDARD 11)
add_definitions(-pthread)

set(srcs
    "main.cpp"
    "crtc.cpp"
)

set(hdrs
    "crtc.h"
)

add_executable(${PROJECT_NAME} ${srcs} ${hdrs})

target_link_libraries(${PROJECT_NAME} X11 Xrandr thinkpad)

install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION bin)

install(FILES dockd.desktop DESTINATION /etc/xdg/autostart)

set(CPACK_PACKAGE_VENDOR "Ognjen Galic")
set(CPACK_PACKAGE_VERSION_MAJOR 1)
set(CPACK_PACKAGE_VERSION_MINOR 20)
set(CPACK_SOURCE_PACKAGE_FILE_NAME ${PROJECT_NAME}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR})
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_IGNORE_FILES "doc/out;.git;.idea;CMakeLists.txt.user")

include(CPack)
