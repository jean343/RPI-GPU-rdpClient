RPI-GPU-rdpClient
=================

Hardware accelerated raspberry pi client for windows PC.

###To compile the client on the Raspberry PI###

It needs the following packages. I started on a clean version of the Raspberian OS.

```
sudo apt-get install cmake
sudo apt-get install libboost-thread-dev
```

```
git clone https://github.com/jean343/RPI-GPU-rdpClient.git
cd RPI-GPU-rdpClient/RPI-Client
mkdir build && cd build/
cmake ..
make
```


###To compile the server in windows###

###NOTES###
From https://github.com/Hexxeh/rpi-update, update your pi.

sudo rpi-update

Update software

sudo apt-get update && sudo apt-get upgrade

###Known issues and limitations###
