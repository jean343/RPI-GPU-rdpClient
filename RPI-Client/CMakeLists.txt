cmake_minimum_required(VERSION 2.8)
project( client )

FIND_PACKAGE( Boost REQUIRED COMPONENTS thread )

include_directories("/opt/vc/include/interface/vcos/pthreads")


add_executable( client client.cpp )
include_directories(/opt/vc/include /opt/vc/src/hello_pi/libs/ilclient/ ${Boost_INCLUDE_DIR})
target_link_libraries( video X11 /opt/vc/src/hello_pi/libs/ilclient/libilclient.a boost_system /opt/vc/lib/libbcm_host.so /opt/vc/lib/libopenmaxil.so /opt/vc/lib/libvcos.so  /opt/vc/lib/libGLESv2.so /opt/vc/lib/libEGL.so ${Boost_LIBRARIES} )