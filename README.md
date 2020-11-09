# Sonic Pact
This is work in progress code for research and testing only. Please do not use this in production.

## Build Instructions

We have the following dependencies. Put them in the **same** directory you cloned this repo:
1. Download opencv's android SDK from [here](https://opencv.org/releases/), specifically version 4.3.0, and decompress.
2. Download [Oboe](https://github.com/google/oboe), most recent version should work.
3. Open the project in android studio. From this point on, Gradle should do the rest of the work.

## Running
You need two devices to test this on. Set one device as the "leader" and another device as the "follower". When ready, press the "play" button, and the two devices will begin to take measurements repeatedly every 30 seconds. 

NOTE: There is currently no visual output of the calculated range. Distance measurements are printed over `logcat` via the console at the leader's side.


## Caveats

This has been tested on a pair of Pixel2's. 

It's likely that this code will not work on your device by default. Current thresholds are set for the Pixel2, and there are potential device-specific timing issues. If you have questions or problems, please submit an issue and PR's are encouraged!

