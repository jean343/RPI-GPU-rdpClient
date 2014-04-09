cmake_minimum_required(VERSION 2.8)

project( server )

find_package( OpenCV )

find_package( Boost REQUIRED )
if(Boost_FOUND)
    message("Boost found!")
endif()

LINK_DIRECTORIES( "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v5.5/lib/Win32")

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${OpenCV_INCLUDE_DIRS} ${Boost_INCLUDE_DIR})
LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})

set(FFMPEG_ROOT "" CACHE FILEPATH "Root of the FFMPEG directory, which has README.txt")
FIND_PATH( FFMPEG_INCLUDE_DIR libavcodec/avcodec.h
  ${FFMPEG_ROOT}/include
)
include_directories(${FFMPEG_INCLUDE_DIR})

FIND_LIBRARY( FFMPEG_LIBRARY_avcodec avcodec
  ${FFMPEG_ROOT}/lib
)
FIND_LIBRARY( FFMPEG_LIBRARY_avutil avutil
  ${FFMPEG_ROOT}/lib
)

add_executable( server server.cpp )
target_link_libraries( server ${OpenCV_LIBS} ${Boost_LIBRARIES} ${FFMPEG_LIBRARY_avcodec} ${FFMPEG_LIBRARY_avutil})

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/config.h)