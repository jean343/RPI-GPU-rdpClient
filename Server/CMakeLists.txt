cmake_minimum_required(VERSION 2.8)

project( server )

find_package( OpenCV REQUIRED )
find_package( Boost REQUIRED )
LINK_DIRECTORIES( "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v5.5/lib/Win32")

add_executable( server server.cpp )
include_directories(${OpenCV_INCLUDE_DIRS} ${Boost_INCLUDE_DIR})
target_link_libraries( server ${OpenCV_LIBS} ${Boost_LIBRARIES} )

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
set(FFMPEG_ROOT "" CACHE FILEPATH "Root of the FFMPEG directory, which has README.txt")
include_directories(${FFMPEG_ROOT}/include)
LINK_DIRECTORIES(${FFMPEG_ROOT}/lib)
add_executable( FFMPEGSample transcoding.cpp )
target_link_libraries( FFMPEGSample avcodec avutil)