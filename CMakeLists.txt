cmake_minimum_required(VERSION 3.0)

project(wm LANGUAGES C)

add_executable(wm main.c)

# add X11
find_package(X11 REQUIRED)
include_directories(${X11_INCLUDE_DIR})
link_directories(${X11_LIBRARIES})
target_link_libraries(wm ${X11_LIBRARIES} m)


install(TARGETS wm RUNTIME DESTINATION bin)
