# FairWindSK
A QT based browser designed to be a companion of the Signal K server

# Introduction
FairWindSK is under its IFT1 (cit.). I deployed a production setup and I’m using it under an OpenPlotter install. I updated the (very bad) README.md on https://github.com/openfairwind/FairWindSK 

Just as recap: FairWindSK is part of DYNAMO research projects (now supported by the DataX4Sea project, a research grant from NEC Laboratory of America to test its DataX framework in data crowdsourcing for coastal environmental protection). The final goal of all the DYNAMO projects is data crowdsourcing for coastal environment protection and weather/ocean forecasting (numerical) and prediction (AI) models. All results of projects funded by public institutions are open source and open data.

Now the cool things for sailors and boaters:

FairWindSK is a browser with steroids designed to host SK applications. It is written in C++ 17 and QT6. It runs on Mac, Linux, and raspberry Pi OS. Probably it works on Windows too, but I never built under this OS. I’d like to port it under Android and iOS QT Apps with same source approach, but for now I’m focused on stabilizing it on the already supported platforms.
What is working: the desktop (of course a far remembrance of other similar user interface), the execution environments, the setup and the configuration of units, QT virtual keyboard, signal K paths, Applications.
What is implemented as user interface but the business logic is still missing: autopilot bar, MOB bar, alarms bar. Those bars will relay on SK web apps invoking their APIs or will access directly to the Signal K APIs. 
What need to be fixed: on raspberry Pi (not on MacOS), tapping on Cancel or Save in the Settings it is rises a segmentation fault (actually it saves the settings, but then it crashes). The same it happens sometimes hitting Quit or Restart from the bottom of the Main settings page.
Last words about apps: Applications are SK Web Apps hosted by the SK server, but also can be other web applications manually configured providing the url. Applications can be also local (as OpenCPN) characterized by a local path url with the schema file://. A mechanism to be back on the FairWindSK desktop after launched, for example, OpenCPN, is not available for now.

# Compile on Raspberry Pi

Install the compiling environment

```
sudo apt install qml6-module-qt-labs-folderlistmodel qml6-module-qtquick-window qml6-module-qtquick-layouts qml6-module-qtqml-workerscript libnss-mdns avahi-utils libavahi-compat-libdnssd-dev libxkbcommon-dev qt6-base-dev qt6-base-dev-tools qt6-webengine-dev qt6-webengine-dev-tools qt6-websockets-dev qt6-virtualkeyboard-dev libqt6virtualkeyboard6 qt6-virtualkeyboard-plugin qmake6 qmake6-bin build-essential cmake
```

Fix a file missing issue

```
sudo touch /usr/lib/aarch64-linux-gnu/cmake/Qt6VirtualKeyboard/Qt6VirtualKeyboardConfigVersionImpl.cmake
```

Clone the repository

```
git clone https://github.com/OpenFairWind/FairWindSK.git
```

Enter in the repository directory

```
cd FairWindSK
```

Create the build directory

```
mkdir build
```

Enter the build directory

```
cd build
```

Configure the building environment

```
cmake ..
```

Make FairWindSK

```
make
```

# Run on Raspberry Pi

```
sudo apt install qml6-module-qt-labs-folderlistmodel qml6-module-qtquick-window qml6-module-qtquick-layouts qml6-module-qtqml-workerscript libnss-mdns avahi-utils libavahi-compat-libdnssd libqt6websockets6 libqt6webenginewidgets6 libqt6webenginecore6 libqt6positioningquick6 libqt6widgets6 libqt6network6 libqt6gui6 libqt6core6 libqt6quickwidgets6 libqt6quickwidgets6 libqt6webchannel6 libqt6qml6 libqt6dbus6 libqt6qmlmodels6 libqt6opengl6 libqt6virtualkeyboard6 qt6-virtualkeyboard-plugin
```

# Setup for production

Copy the executable

```
sudo cp FairwindSK /usr/local/bin
```

Copy the icons in home

```
cp icons $HOME/
```

# Run in production

```
FairWindSK
```
