# FairWindSK
A QT based browser designed to be a companion of the Signal K server


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
