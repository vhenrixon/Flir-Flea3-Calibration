# Flir-Flea3-Calibration

The Flir Flea3 Calibration is program that allows for the photometric calibration of the Flire Flea3 camera.

## Installation

Please follow the installation instruction of the Spinnaker SDK at:
https://www.flir.com/products/spinnaker-sdk/

You will need to download the SDK from the Flir box link and then extract it. After extracting, navigate to the folder and select your OS. After selecting your OS, you will need to run the .exe, tar gzip a folder, or run a .dmg.

NOTE: This calibration script has only been tested on a Ubuntu system.

After installing the SDK, clone this repository and follow the build instructions.

## Building

- mkdir build
- cd /build
- cmake ..
- cmake --build .
- cd src/
- ./run_calibration  --response or --vignette (based on which calibration you would like to preform)
