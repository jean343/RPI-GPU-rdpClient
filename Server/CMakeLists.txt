cmake_minimum_required(VERSION 2.8)
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}")

project( server )

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

SET(USE_CUDA on CACHE BOOL "Use CUDA")
SET(USE_WDDM on CACHE BOOL "Use WDDM for screen capture")

SET(USE_NVENC on CACHE BOOL "Use Nvidia encoder")

# BOOST
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
find_package( Boost REQUIRED COMPONENTS thread )
if(Boost_FOUND)
  message("Boost found!")
endif()
include_directories(${Boost_INCLUDE_DIR})
LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})

# FFMPEG
set(FFMPEG_ROOT "" CACHE FILEPATH "Root of the FFMPEG directory, which has README.txt")
if (FFMPEG_ROOT)
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
	if (FFMPEG_LIBRARY_avcodec AND FFMPEG_LIBRARY_avutil)
		set (FFMPEG_FOUND 1)
		set (FFMPEG_LIBRARIES "${FFMPEG_LIBRARY_avcodec};${FFMPEG_LIBRARY_avutil}")
		message("FFMPEG found!")
	endif()
endif()

# DXGI and CUDA
if (USE_CUDA)
  find_package(CUDA)
endif()
if(CUDA_FOUND)
	set (HAS_CUDA 1)
  message("CUDA found!")
  include_directories(${CUDA_TOOLKIT_INCLUDE})
  CUDA_ADD_LIBRARY(cudalib STATIC
      color_conversion.h
      color_conversion.cu
      OPTIONS -arch sm_30
  )
  TARGET_LINK_LIBRARIES(cudalib ${CUDA_LIBRARIES})
  set (CUDA_LINK_LIBRARIES "${CUDA_CUDA_LIBRARY};${CUDA_CUDART_LIBRARY};cudalib")
endif()

if (USE_WDDM OR USE_NVENC)
	find_package( DirectX )
endif()

include_directories(${DXGI_INCLUDES})
include_directories(${Boost_INCLUDE_DIRS})

if (USE_WDDM AND DIRECTX_FOUND)
	set (HAS_WDDM 1)
	SET(Capture_HEADER
	  wddm.h
	  WDDMCapture.h
	)
	SET(Capture_LIBRARIES
	  "${DXGI_LIBRARIES}"
	)
else()
	SET(Capture_HEADER
	  GDICapture.h
	)
endif()

if (DIRECTX_FOUND AND USE_NVENC)
	set (HAS_NVENC 1)
	#files for NVEncoder
	SET(ENCODER_SOURCE
	  NvEncoder/NvHWEncoder.cpp
	)
	SET(ENCODER_HEADERS
	  NV_encoding.hpp
	  NvEncoder/NvEncoder.h
	  NvEncoder/NvHWEncoder.h
	  NvEncoder/nvEncodeAPI.h
	)
elseif (FFMPEG_FOUND)
	set (HAS_FFMPEG 1)
	SET(ENCODER_SOURCE
	)
	SET(ENCODER_HEADERS
	  FFMPEG_encoding.hpp
	)
	SET(ENCODER_LIBRARIES
	  "${FFMPEG_LIBRARIES}"
	)
endif()

SET(COMMON_SOURCE
  config.h
  bounded_buffer.h
  Capture.h
  fps.h
  monitor.h
  params.h
)

add_executable( server server.cpp ${COMMON_SOURCE} ${ENCODER_SOURCE} ${ENCODER_HEADERS} ${Capture_HEADER} )


target_link_libraries( server ${Boost_LIBRARIES} ${ENCODER_LIBRARIES} ${Capture_LIBRARIES} ${CUDA_LINK_LIBRARIES})

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/config.h)