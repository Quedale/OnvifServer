# OnvifServer
Onvif Server for Linux <b>(NOT FUNCTIONAL YET, DONT WAST YOUR TIME)</b>


# Description
Although I loved the [rpos](https://github.com/Quedale/rpos) project, it is built on nodejs which I'm really not a fan of. Still, I must give my thanks for such a great introduction to the ONVIF protocol.
I personally prefer working on very small embeded devices for these kind of things, so a small footprint is a priority. 
This is why I decided to start my own project built on C.
The result should be a considerably smaller memory footprint overhead for the ONVIF HTTP service.

The first goal is implementing NetworkVideoTransmitter Profile S followed by Profile T.
I may consider adding additional ONVIF device types like NetworkVideoDisplay, NetworkVideoStorage, NetworkVideoAnalytics, etc...

# Working
- WS-Security and WS-Discovery (gsoap)
- Barebone device and media service
- Stand-alone (No external webserver required)

# TODO
- RTSP Server
- Configuration storage
- Complete ONVIF Implementations (Status breakdown will eventually be documented)
- And a lot more...

# Use-case
Turning any linux device (PC, laptop, SBC) into an ONVIF compatible device.

# How to build
### Clone repository
```
git clone https://github.com/Quedale/OnvifServer.git
mkdir OnvifServer/build
cd OnvifServer/build
```

### Autogen, Configure, Download and build dependencies
**[Mandatory]** The following package dependencies are mandatory and are not yet automatically built:
```
sudo apt install git
sudo apt install pkg-config
sudo apt install make
sudo apt install g++
```
**[Optional]** The following package are optional, but will reduce the runtime of autogen.sh if installed.
```
sudo apt install python3-pip
python3 -m pip install cmake
sudo apt install zlib1g-dev
```

Once the mandatory dependencies are installed, configure and compile the project with cmake.
Note that cmake will automatically invoke the `autogen.sh` script.
```
cmake ..
make -j$(nproc)
```
At this point, you should be able to execute the server without installing it on the system

```
./onvifserver -p 8080
```

### Install
<b>DO NOT INSTALL THIS YET. ESSENTIAL FEATURES ARE NOT IMPLEMENTED YET.</b>

# 
# Feedback is more than welcome