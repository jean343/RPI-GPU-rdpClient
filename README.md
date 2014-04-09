RPI-GPU-rdpClient
=================

Video on youtube: http://youtu.be/3HJuHhiXxuE

Hardware accelerated raspberry pi client for windows PC.
It is more a proof-of-concept to show that OpenMAX can be used as a RDP viewer rather than a finished product.
There is no authentication, use at your own risk.

It uses a NVIDIA graphic card to encode H.264 video, and OpenMAX to display the video. It can achieve 1080P 30FPS RDP on a RPI with a relatively low latency of ~200ms.
When the GPU is not accessible on the server, it falls back to CPU encoding.
It can work in a Virtual machine in order to be a true thin client.

### To compile the client on the Raspberry PI ###

It needs the following packages. I started on a clean version of the Raspberian OS.

```
sudo apt-get install cmake
sudo apt-get install libboost-thread-dev libboost-system-dev
sudo apt-get install libx11-dev
```

To compile ilclient:
```
cd /opt/vc/src/hello_pi
sudo ./rebuild.sh
```

To compile the RDP client:
```
git clone https://github.com/jean343/RPI-GPU-rdpClient.git
cd RPI-GPU-rdpClient/RPI-Client
mkdir build && cd build/
cmake ..
make
```

### To run the client ###
./client <host> <port>

###To compile the server in windows###
- Install CMAKE and Boost
- In cmake under the Server folder
  - Set BOOST_ROOT to the root of the Boost folder
- Build, I used Visual Studio
- Optional, either OpenCV or FFMPEG
  - Install OpenCV (3.0 from GIT) in order to use the encoder in an Nvidia GPU
    - Set OpenCV_DIR to the root of OpenCV build folder
  - Download FFMPEG from http://ffmpeg.zeranoe.com/builds/, need the dev and shared
    - Set FFMPEG_ROOT to the root of FFMPEG dev folder
    - Add the bin folder of the shared zip to your path, or copy the DLLs

### To run the server ###
./server monitor 0 port 8080

### Contribute ###

Want to be part of the project? Great! All are welcome! We will get there quicker together :)
Whether you find a bug, have a great feature request feel free to get in touch.

### Known issues and limitations ###
- There is no audio
- There is no authentication, use only in a local LAN or under a VPN.
- The software falls back to CPU encoding in a Virtual Machine, it is fast as it uses the x264 superfast preset, but the H.264 quality is reduced.
- Uses ```GetDC``` and ```BitBlt``` to capture the screen, it works well on Win 7 and 8, but it is slow on XP.
  - It does not work for full screen games using DirectX or OpenGL
  - It could be improved by adding the hook from https://github.com/spazzarama/Direct3DHook
  - It could be improved by using Win 8 ```DuplicateOutput``` WDDM 1.2

### NOTES ###
From https://github.com/Hexxeh/rpi-update, update your pi:
```
sudo rpi-update
```
Update software:
```
sudo apt-get update && sudo apt-get upgrade
```
