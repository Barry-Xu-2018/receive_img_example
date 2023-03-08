## Build

Precondition:

Install dependent packages
```
$ sudo apt install libmosquitto-dev libopencv-dev
```

Note that build-essential and cmake are also needed.

```bash
$ cd receive_img_example
$ mkdir build && cd build
$ cmake ..
$ cmake --build .
```

## How to run

Precondition:  

The MQTT broker for image recevier is launched.  

Usage 
```
./img_viewer -a MQTT_Broker_IP_Addr -p Server_TCP_Port -t Topic [-o Output_BMP_FILE_PATH]
```
After run, a window will be poped up. While image is recevied, it will showed on this window.  

If you want to save BMP files, please run with parameter `-o PATH`.  
After running, image is showed on window and is saved to this path at the same time.
