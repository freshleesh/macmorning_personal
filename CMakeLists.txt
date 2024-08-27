cmake_minimum_required(VERSION 3.5)
project(my_package)

# Default to C99
if(NOT CMAKE_C_STANDARD)
  set(CMAKE_C_STANDARD 99)
endif()

# Default to C++14
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(PCL REQUIRED)
find_package(pcl_conversions REQUIRED)
find_package(visualization_msgs REQUIRED)
# uncomment the following section in order to fill in
# further dependencies manually.
# find_package(<dependency> REQUIRED)
include_directories(
  include
  ${PCL_INCLUDE_DIRS}
)


#
# add_executable, ament_target_dependencies plus each node
#

add_executable(pointcloud2_handler src/pointcloud2_handler.cpp)
ament_target_dependencies(pointcloud2_handler rclcpp std_msgs sensor_msgs)
target_link_libraries(pointcloud2_handler ${PCL_LIBRARIES})

add_executable(pcl_test src/pcl_test.cpp)
ament_target_dependencies(pcl_test rclcpp std_msgs sensor_msgs)
target_link_libraries(pcl_test ${PCL_LIBRARIES})

add_executable(map_handler src/map_handler.cpp)
ament_target_dependencies(map_handler rclcpp std_msgs sensor_msgs)
target_link_libraries(map_handler ${PCL_LIBRARIES})

add_executable(relocalization src/relocalization.cpp)
ament_target_dependencies(relocalization rclcpp std_msgs sensor_msgs PCL pcl_conversions visualization_msgs)
target_link_libraries(relocalization ${PCL_LIBRARIES})

add_executable(map2d src/map2d.cpp)
ament_target_dependencies(map2d rclcpp std_msgs sensor_msgs PCL pcl_conversions)
target_link_libraries(map2d ${PCL_LIBRARIES})


target_include_directories(pointcloud2_handler PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)


#
# install TARGETS back plus each node
#  
install(TARGETS 
  pointcloud2_handler
  pcl_test
  map_handler
  relocalization
  map2d
  DESTINATION lib/${PROJECT_NAME})

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # the following line skips the linter which checks for copyrights
  # uncomment the line when a copyright and license is not present in all source files
  #set(ament_cmake_copyright_FOUND TRUE)
  # the following line skips cpplint (only works in a git repo)
  # uncomment the line when this package is not in a git repo
  #set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
