cmake_minimum_required(VERSION 2.8.3)
project(rdi_explorer_dvl)
find_package(catkin REQUIRED COMPONENTS nodelet roscpp uf_common)
catkin_package(
    DEPENDS
    CATKIN_DEPENDS nodelet roscpp uf_common
    INCLUDE_DIRS include
    LIBRARIES
)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
include_directories(include ${Boost_INCLUDE_DIR} ${catkin_INCLUDE_DIRS})
add_library(rdi_explorer_dvl_nodelet src/nodelet.cpp)
target_link_libraries(rdi_explorer_dvl_nodelet ${catkin_LIBRARIES})
add_dependencies(rdi_explorer_dvl_nodelet ${catkin_EXPORTED_TARGETS})
set_target_properties(rdi_explorer_dvl_nodelet PROPERTIES COMPILE_FLAGS -std=c++11)

install(PROGRAMS scripts/fake_dvl DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION})
