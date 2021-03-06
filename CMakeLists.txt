cmake_minimum_required(VERSION 3.5)

project(cpp-skeleton LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall")

set(_PROJECT_PATH_ "./")
set(_DAQI_LIBRARY_PATH_ "./third_party/daqi/build")
# set(_LIBGO_LIBRARY_PATH_ "./third_party/libgo/build")

include_directories(/usr/local/include/opencv4)
include_directories(${_PROJECT_PATH_}/third_party/daqi/da4qi4/include)
include_directories(${_PROJECT_PATH_}/third_party/daqi/da4qi4)
include_directories(${_PROJECT_PATH_}/third_party/daqi/da4qi4/spdlog)
include_directories(${_PROJECT_PATH_}/third_party/daqi/da4qi4/nlohmann_json/include)
# include_directories(${_PROJECT_PATH_}/third_party/libgo/libgo)

include_directories(${_PROJECT_PATH_}/typedef)
include_directories(${_PROJECT_PATH_}/storage)
include_directories(${_PROJECT_PATH_}/controller)
include_directories(${_PROJECT_PATH_}/service)
include_directories(${_PROJECT_PATH_}/manager)
include_directories(${_PROJECT_PATH_}/utils/const)
include_directories(${_PROJECT_PATH_}/utils/error)
include_directories(${_PROJECT_PATH_}/utils/base)
include_directories(${_PROJECT_PATH_}/utils/cache)
include_directories(${_PROJECT_PATH_}/utils/codec)
include_directories(${_PROJECT_PATH_}/utils/serialize)

find_package(OpenCV REQUIRED)
find_package(Boost 1.65.0 REQUIRED COMPONENTS filesystem system)
link_directories(${_DAQI_LIBRARY_PATH_})
# link_directories(${_LIBGO_LIBRARY_PATH_})

link_libraries(da4qi4)
link_libraries(pthread)
link_libraries(ssl)
link_libraries(crypto)
link_libraries(boost_filesystem)
link_libraries(boost_system)
# link_libraries(libgo)

add_subdirectory(controller)
add_subdirectory(service)
add_subdirectory(manager)
add_subdirectory(utils/error)
add_subdirectory(utils/base)
add_subdirectory(utils/codec)

add_executable(cpp-skeleton main.cpp)

target_link_libraries(cpp-skeleton ${OpenCV_LIBS})
target_link_libraries(cpp-skeleton ControllerFunc)
target_link_libraries(cpp-skeleton ServiceFunc)
target_link_libraries(cpp-skeleton ManagerFunc)
target_link_libraries(cpp-skeleton UtilBaseFunc)
target_link_libraries(cpp-skeleton UtilErrFunc)
target_link_libraries(cpp-skeleton UtilCodecFunc)
