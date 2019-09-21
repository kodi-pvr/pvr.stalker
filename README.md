[![Build Status](https://travis-ci.org/kodi-pvr/pvr.stalker.svg?branch=Leia)](https://travis-ci.org/kodi-pvr/pvr.stalker/branches)
[![Build Status](https://ci.appveyor.com/api/projects/status/github/kodi-pvr/pvr.stalker?branch=Leia&svg=true)](https://ci.appveyor.com/project/kodi-pvr/pvr-dvblink?branch=Leia)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/5120/badge.svg)](https://scan.coverity.com/projects/5120)

# Stalker PVR
Stalker Middleware PVR client addon for [Kodi] (https://kodi.tv)

## Build instructions

### Linux

1. `git clone --branch Leia https://github.com/xbmc/xbmc.git`
2. `git clone https://github.com/kodi-pvr/pvr.stalker.git`
3. `cd pvr.stalker && mkdir build && cd build`
4. `cmake -DADDONS_TO_BUILD=pvr.stalker -DADDON_SRC_PREFIX=../.. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=../../xbmc/addons -DPACKAGE_ZIP=1 ../../xbmc/cmake/addons`
5. `make`

##### Useful links

* [Kodi's PVR user support] (https://forum.kodi.tv/forumdisplay.php?fid=167)
* [Kodi's PVR development support] (https://forum.kodi.tv/forumdisplay.php?fid=136)
