# Plugin for rFactor 2 Live Timing

Companion plugin for [rf2livetiming](https://github.com/mbeader/rf2livetiming) based on the [example plugin by Marcel Offermans](https://bitbucket.org/marrs/rfactor/src/e13b7d85bb48cb353af58f4223ecd41a47316f24/cpp/DataPlugin_v2/?at=master).

## rFactor 2

### Usage

* Place the compiled `rf2livetiming.dll` in the 64-bit rF2 plugins folder
* Place `rf2livetiming.ini` in the rF2 root folder
* Configure `rf2livetiming.ini`
* Start the rF2 dedicated server
* Logs to `UserData\Log\rf2livetiming.log`

### rf2livetiming.ini
```
USE LOCALHOST="0" // 0=use IP, 1=use destination as localhost
DEST IP="127.0.0.1" // location of rf2livetiming, ignored if USE LOCALHOST is set to 1
DEST PORT="6789" // port rf2livetiming is listening on
```

## rFactor/Automobilista

### Usage

* Place the compiled `rf1livetiming.dll` in the plugins folder
* Place `rf1livetiming.ini` in the rF1/AMS root folder
* Configure `rf1livetiming.ini`
* Start the rF1/AMS dedicated server
* Logs to `UserData\Log\rf1livetiming.log`

### rf2livetiming.ini
```
USE LOCALHOST="0" // 0=use IP, 1=use destination as localhost
DEST IP="127.0.0.1" // location of rf1livetiming, ignored if USE LOCALHOST is set to 1
DEST PORT="6789" // port rf1livetiming is listening on
SERVER NAME="Test server" // name of the server
SERVER IP="10.0.0.1" // public IP of the server
SERVER PORT="55143" // port server is listening on
```

## Build

* Requires VS2022 (v143)
* Open the solution
* Build
* Output can be found in `bin`

## Notes

* rf2livetiming.dll supports 64-bit rFactor 2
* rf1livetiming.dll supports last versions of rFactor and Automobilista
* Tested on Windows 10 and Windows Server 2012 R2
