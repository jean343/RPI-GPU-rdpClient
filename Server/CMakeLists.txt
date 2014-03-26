cmake_minimum_required(VERSION 2.8)
project( server )

find_package( OpenCV REQUIRED )
find_package( Boost REQUIRED )
LINK_DIRECTORIES( "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v5.5/lib/Win32")

add_executable( server server.cpp )
include_directories(${OpenCV_INCLUDE_DIRS} ${Boost_INCLUDE_DIR})
target_link_libraries( server ${OpenCV_LIBS} ${Boost_LIBRARIES} )

message( server ${OpenCV_LIBS} ${Boost_LIBRARIES} )