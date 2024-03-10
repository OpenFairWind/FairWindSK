#! /bin/bash

# Change the current directory where FairWindSK is installed
cd $HOME/fairwindsk

# Get the Signal K Server URL from the fairwindsk.ini file
SIGNALK_SERVER=`cat fairwindsk.ini | grep signalk-server|sed 's/signalk-server=//'`

# Set the HTTP_CODE to an empty string
HTTP_CODE=""

# Wait until the the Signal K server is up and running
while [[ "$HTTP_CODE" != "200" ]]
do
  # Check if the Signal K server is up and running
  HTTP_CODE=`curl -I -s $SIGNALK_SERVER/signalk/ | grep HTTP|awk '{ print $2}'`
done

# Run FairWindSK
./FairWindSK