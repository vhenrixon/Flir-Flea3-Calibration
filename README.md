# Flir-Flea3-Calibration-dataset

The Flir Flea3 Calibration dataset is a program that allows for the collection of a photometric calibration dataset for the Flir Flea3 camera using the Spinnaker SDK.

## Installation

Please follow the installation instruction of the Spinnaker SDK at:
https://www.flir.com/products/spinnaker-sdk/

You will need to download the SDK from the Flir box link and then extract it. After extracting, navigate to the folder and select your OS. After selecting your OS, you will need to run the .exe, tar gzip a folder, or run a .dmg.

NOTE: This calibration script has only been tested on a Ubuntu system.

After installing the SDK, clone this repository, and follow the build instructions.


## Building

- mkdir build
- cd /build
- cmake ..
- cmake --build .
- cd src/
- ./run_calibration  --response or --vignette (based on which calibration dataset you would like to gather)

The program will create a folder based on the selected photometric and dump the images into it. The images in the folder can be used to generate a calibration file using the 
this git repository: https://github.com/tum-vision/mono_dataset_code

## NOTE

 After running the program, a folder will be made in the build/src, which will contain the images. If you want to rerun the dataset collection, you must delete the dataset folder so that a new folder can be created.

 Additionally, If the exposure values are breaking, the value of 1.0651 was tested at mode 0 for the Flir Flea3 camera.