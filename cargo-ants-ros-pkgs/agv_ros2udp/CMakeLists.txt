cmake_minimum_required (VERSION 2.6)
if (COMMAND cmake_policy)
  cmake_policy (SET CMP0003 NEW)
  cmake_policy (SET CMP0005 NEW)
  if (POLICY CMP0011)
    cmake_policy (SET CMP0011 NEW)
  endif (POLICY CMP0011)
  if (POLICY CMP0042)
    cmake_policy (SET CMP0042 NEW)
  endif (POLICY CMP0042)
endif (COMMAND cmake_policy)

project (agv_ros2udp)
set (CMAKE_VERBOSE_MAKEFILE ON)

find_package (catkin REQUIRED COMPONENTS
  cargo_ants_msgs
  roscpp
)

catkin_package (
  INCLUDE_DIRS include
  LIBRARIES #udp_volvo
  CATKIN_DEPENDS cargo_ants_msgs roscpp
  #  DEPENDS system_lib
)

include_directories (
  include
  ${catkin_INCLUDE_DIRS}
  )


add_executable (agv_ros2udp src/agv_ros2udp.cpp)
target_link_libraries (agv_ros2udp ${catkin_LIBRARIES})
add_dependencies(agv_ros2udp ${catkin_EXPORTED_TARGETS})
