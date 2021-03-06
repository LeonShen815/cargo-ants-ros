cmake_minimum_required(VERSION 2.8.3)
project(ants_example)

#Set compiler according C++11 support
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
        message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has C++11 support.")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
        message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has C++0x support.")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
        message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

## Find catkin macros and libraries
find_package( catkin REQUIRED COMPONENTS 
    roscpp
    rospy
    std_msgs
    sensor_msgs)

## Find here Library dependencies. System dependencies are found with CMake's conventions
# find_package(my_library REQUIRED)

#SRCS files. ROS node
SET(SRCS_ROS_NODE 
      src/lib_wrapper.cpp
      src/ants_example_main.cpp
)

#dynamic reconfigure cfg file
# generate_dynamic_reconfigure_options(cfg/my_config_file.cfg)

## The catkin_package macro generates cmake config files for your package
catkin_package ( CATKIN_DEPENDS 
        roscpp 
        rospy 
        std_msgs )

# add include paths
include_directories(${catkin_INCLUDE_DIRS})
# include_directories(${my_library_INCLUDE_DIR} ${catkin_INCLUDE_DIRS})

# build executable and link with libraries 
add_executable(ants_example ${SRCS_ROS_NODE})
target_link_libraries(ants_example ${catkin_LIBRARIES})
#target_link_libraries(ants_example ${my_library_LIBRARY} ${catkin_LIBRARIES})

