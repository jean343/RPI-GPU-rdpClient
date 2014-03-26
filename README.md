RPI-GPU-rdpClient
=================

Hardware accelerated raspberry pi client for windows PC.

###To compile the client on the Raspberry PI###

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

###To run the client###
./client

###To compile the server in windows###

###NOTES###
From https://github.com/Hexxeh/rpi-update, update your pi:
```
sudo rpi-update
```
Update software:
```
sudo apt-get update && sudo apt-get upgrade
```

###Known issues and limitations###
- Optimized to use a graphic card running CUDA on the windows machine. Works without an encoder, but it is slower.
- Uses ```GetDC``` and ```BitBlt``` to capture the screen, it works well on Win 7 and 8, but it is slow on XP.
  - It does not work for full screen games using DirectX
  - It could be improved by adding the hook from https://github.com/spazzarama/Direct3DHook
  - It could be improved by using Win 8 DuplicateDisplay
