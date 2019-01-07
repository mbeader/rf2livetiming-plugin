# Plugin for rFactor 2 Live Timing

Companion plugin for [rf2livetiming](https://github.com/mbeader/rf2livetiming) based on the [example plugin by Marcel Offermans](https://bitbucket.org/marrs/rfactor/src/e13b7d85bb48cb353af58f4223ecd41a47316f24/cpp/DataPlugin_v2/?at=master).

## Usage
* Place the compiled .dll in the 64-bit rF2 plugins folder
* Place rf2livetiming.ini in the rF2 root folder
* Configure rf2livetiming.ini
* Start the rF2 dedicated server
* Logs to UserData\Log\rf2livetiming.log

## rf2livetiming.ini
```
USE LOCALHOST="0" // 0=use IP, 1=use destination as localhost
DEST IP="127.0.0.1" // location of rf2livetiming, ignored if USE LOCALHOST is set to 1
DEST PORT="6789" // port rf2livetiming is listening on
```

## Compilation

* Open the Visual Studio solution
* Use Visual Studio 2012 build tools
* Build

## Notes

* Currently only tested with 64-bit rFactor 2