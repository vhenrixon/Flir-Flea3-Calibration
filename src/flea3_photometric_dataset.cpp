#include "spinnaker/Spinnaker.h"
#include "spinnaker/SpinGenApi/SpinnakerGenApi.h"
#include <iostream>
#include <sstream>
#include <chrono>
#include <thread>
#include <boost/filesystem.hpp>
#include <fstream>
#include <math.h>

using namespace Spinnaker;
using namespace Spinnaker::GenApi;
using namespace Spinnaker::GenICam;
using namespace std;

/*
    This function sets the cameras exposure time to a set time. 
    The exposure time is in microseconds and this function will turn off 
    the automatic exposure of the camera meaning that it will need to be reset 
    through the ResetExposure() function.

    @param pCam the camera pointer that the exposure that will have its exposure changed
    @param exposureTime the exposure time that is wanted(Microseconds)
*/
void setExposure(CameraPtr pCam, double exposureTime)
{

    cout << endl << endl << "*** CONFIGURING EXPOSURE ***" << endl << endl;

    try
    {
        if (!IsReadable(pCam->ExposureAuto) || !IsWritable(pCam->ExposureAuto))
        {
            cout << "Unable to disable automatic exposure. Aborting..." << endl << endl;
            throw std::exception();
        }
        pCam->ExposureAuto.SetValue(ExposureAuto_Off);
        pCam->ExposureMode.SetValue(ExposureMode_Timed);
        cout << "Automatic exposure disabled..." << endl;
        // Set exposure time manually; exposure time recorded in microseconds
        if (!IsReadable(pCam->ExposureTime) || !IsWritable(pCam->ExposureTime))
        {
            cout << "Unable to set exposure time. Aborting..." << endl << endl;
            throw std::exception();
        }
        // Ensure desired exposure time does not exceed the maximum
        const double exposureTimeMax = pCam->ExposureTime.GetMax();
        // Making sure that the exposure that is going to be set is less than the max exposure of the camera 
        if (exposureTime > exposureTimeMax)
        {
            exposureTime = exposureTimeMax;
        }
        pCam->ExposureTime.SetValue(exposureTime);
        cout << std::fixed << "Shutter time set to " << exposureTime << " us..." << endl << endl;
    }
    catch (Spinnaker::Exception& e)
    {
        cout << "Error: " << e.what() << endl;
        throw std::exception();
    }

}
/*
    This function will reset the exposure on the camera so that it will return to
    Automatic exposure settings

    @param pCam the camera pointer that the exposure that will have its exposure settings changed
*/
void ResetExposure(CameraPtr pCam)
{
    try
    {
        if (!IsReadable(pCam->ExposureAuto) || !IsWritable(pCam->ExposureAuto))
        {
            cout << "Unable to enable automatic exposure (node retrieval). Non-fatal error..." << endl << endl;
        }

        pCam->ExposureAuto.SetValue(ExposureAuto_Continuous);

        cout << "Automatic exposure enabled..." << endl << endl;
    }
    catch (Spinnaker::Exception& e)
    {
        cout << "Error: " << e.what() << endl;
        throw std::exception();
    }

}
/*
    This function allows for camera to send images at a continuous rate 

    @param pCam the camera pointer
*/
void setCameraToContinuous(CameraPtr pCam){
    /*
        This function sets the camera to continuous mode
    */
    if (!IsReadable(pCam->AcquisitionMode) || !IsWritable(pCam->AcquisitionMode))
    {
        cout << "Unable to set acquisition mode to continuous. Aborting..." << endl << endl;
        throw std::exception();
    }
    pCam->AcquisitionMode.SetValue(AcquisitionMode_Continuous);
    cout << "Acquisition mode set to continuous..." << endl;
}

void createDirectory(string path){
    /*
        This function creates in directory in the current path and then switchs the current path to the newly created directory
    */
    boost::filesystem::path p{path};
    if(!(boost::filesystem::exists(p))){
        boost::filesystem::create_directory(p);
        cout << "Directory has been created for the images!" << endl;
        // Changing the current path of the program so that files can be saved in the directory that was just created 
        boost::filesystem::current_path(p);
    }
}

gcstring getSerialNumber(CameraPtr pCam){
    /*
        This function returns the serial number for a camera object.
    */
    gcstring deviceSerialNumber("");

    if (IsReadable(pCam->TLDevice.DeviceSerialNumber))
    {
        deviceSerialNumber = pCam->TLDevice.DeviceSerialNumber.GetValue();

        cout << "Device serial number retrieved as " << deviceSerialNumber << "..." << endl;
    }
    cout << endl;

    return deviceSerialNumber;
}

long currentTime(){
    // current time for the name
    auto time = chrono::system_clock::now();
    auto since_epoch = time.time_since_epoch();
    auto milli = chrono::duration_cast<chrono::milliseconds>(since_epoch);
    return milli.count(); 
}


ostringstream getUniqueName(gcstring serialNumber){
    /*
        This function generates a unique name for the picture based on serial number, time(millie seconds), and image count.
    */
    ostringstream filename;
    filename << "ExposureQS";
    if (serialNumber != "")
    {
        filename << serialNumber.c_str();
    }
    filename << "-" << currentTime() << ".jpg";
    return filename;
}

void acquireXImages(CameraPtr pCam, int amountOfImages, int totalImgCnt, ofstream &file, double currentExposure)
{
    /*
        This function acquires X number of images and saves them in current directory with a unique name. 
    */
    cout << endl << "*** IMAGE ACQUISITION ***" << endl << endl;
    try{
        // Set acquisition mode to continuous
        setCameraToContinuous(pCam);
        cout << "Acquiring images..." << endl;
        pCam->BeginAcquisition();
        // Get device serial number for filename
        gcstring deviceSerialNumber = getSerialNumber(pCam);
        int img_collected = 0;
        while(img_collected < amountOfImages){
            try{
                // Retrieve next received image and ensure image completion
                ImagePtr pResultImage = pCam->GetNextImage(1000);
                if(pResultImage->IsIncomplete()){
                    // The Image has failed to be collected 
                    continue;
                }else{
                    // Convert image to mono 8
                    ImagePtr convertedImage = pResultImage->Convert(PixelFormat_Mono8);
                    // Create a unique filename
                    ostringstream filename = getUniqueName(deviceSerialNumber);
                    // Save image
                    convertedImage->Save(filename.str().c_str());
                    cout << "Image saved at " << filename.str() << endl;
                    // Adding information to the times.txt file
                    // Image_Number Time current_exposure     <- how the information is stored in the time file
                    file << (totalImgCnt+img_collected) << " " << (currentTime()/1000)<< " " << (currentExposure/1000) << "\n";
                    img_collected++;
                }
            }catch(Spinnaker::Exception& e){
                cout << "Error: " << e.what() << endl;
            }
        }
        // End acquisition
        pCam->EndAcquisition();
    }
    catch (Spinnaker::Exception& e)
    {
        cout << "Error: " << e.what() << endl;
        throw std::exception();
    }
}

void vignetteDatasetCollection(CameraPtr pCam){    
    /*
        This function collects the image used in the vignette photometric calibratin
    */
    try{
        int waitTime = 10; 
        int totalImg = 800;             // Simillar to the amount of the images in the vignette TUMS sample

        // Initialize camera
        pCam->Init();
        createDirectory("vignette-dataset");
        cout << "You have 10 seconds to position your camera!" << endl;
        sleep(waitTime);
        // times.txt file 
        ofstream timeFile; 
        timeFile.open("times.txt");

        createDirectory("images");

        for(int pictureTaken=0; pictureTaken < totalImg; ++pictureTaken)
        {
            auto currentExp = pCam->ExposureTime.GetValue();
            acquireXImages(pCam, 1,pictureTaken,timeFile,currentExp);
        }

        cout << "Completed gathering 800 pictures for the vignette Dataset." << endl;

        // Deinitialize the camera
        pCam->DeInit();
        // Closing time file 
        timeFile.close();
    }catch (Spinnaker::Exception& e){
        cout << "A error occurred while doing the vignette dataset collection" << endl;
        cout << "Error:" << e.what() << endl;
    }
}

vector<double> getExposureVector(CameraPtr pCam){
    /*

    This function returns a vector with the exposure that will be used in the photometric response data collection.
    */
    try{
        double cam_min = pCam->ExposureTime.GetMin();   // Min exposure of the Flir Flea3 camera in Microseconds
        double cam_max = pCam->ExposureTime.GetMax();   // Max exposure of the Flir Flea3 camera in Microseconds
        cout << "Min: " << cam_min << " Max: " << cam_max << endl;
        // NOTE: This is a last minute adititon that cannot be tested. This is suppose to combat the changing of modes
        // The mult_increment = 1.0651 was used with mode 0 if this breaks 
        double mult_increment = pow((cam_max/cam_min),1/120);    // This value was derived from finding the multiplicative gain from going to from min to max in 120 exposures 
        double current_exposure = cam_min; 
        int exposure_cnt = 0;  

        vector<double> calibration_exposure; 

        while(exposure_cnt <= 120){
            calibration_exposure.push_back(current_exposure);
            current_exposure *= mult_increment; 
            exposure_cnt++;
        }
        cout << "Finished creating exposure vector." << endl;
        return calibration_exposure;
    }catch(Spinnaker::Exception& e){
        cout << "Error occurred when creating the exposure vector!" << endl;
        cout << "Error: " << e.what() << endl;
        throw std::exception();
    }
}

void responseDatasetCollection(CameraPtr pCam){
    /*
        This function runs the gathering of 1000 images at 120 different exposures in order to create a photometric
        response dataset. 
    */
    try{     
        // Initialize camera
        pCam->Init();   
        createDirectory("response-dataset");

        // times.txt file 
        ofstream timeFile; 
        timeFile.open("times.txt");

        createDirectory("images");

        vector<double> exposures = getExposureVector(pCam);     // Gather the 120 different exposure that will be used in the dataset

        // Looping for the 120 exposures that must be checked
        int imgCnt = 0;
        for(double exposure : exposures)
        {
            setExposure(pCam, exposure);
            acquireXImages(pCam, 8, imgCnt, timeFile, exposure);        
            imgCnt += 8;                    // The 1000 images at 120 exposures; 1000/120 = 8.333 Images/exposure
        }
        ResetExposure(pCam);                                   
        cout << "Gathered and saved 1000 images. The response Dataset collection is complete. " << endl;

        // Deinitialize the camera
        pCam->DeInit();

        // Closing file 
        timeFile.close();
    }catch (Spinnaker::Exception& e){
        cout << "A error occurred while doing the response dataset collection" << endl;
        cout << "Error:" << e.what() << endl;
    }

}

void parseArgument(int argc,char** argv, CameraList camList){
    /*
        This function parse the users argument to decide which photometric calibration dateset will be gathered.
    */
    if(argc < 2)
    {
        cout << "This program requires you to detail which kind of photometric calibration dataset collection is wanted. Either vignette or response." << endl;    
        throw std::exception();
    }else
    {
        if(string(argv[1]) == "--vignette")
        {
            cout << "Running vignette dataset collection!" << endl;
            vignetteDatasetCollection(camList.GetByIndex(0));
        }else if(string(argv[1]) == "--response")
        {
            cout << "Running response dataset collection!" << endl;
            responseDatasetCollection(camList.GetByIndex(0));
        }else{
            cout << "Argument is not understood! Please use either --vignette or --response. Exiting!" << endl;
            throw std::exception();
        }
    }
}

bool cameraCheck(CameraList camList,SystemPtr system){
    /*
        This function checks to see if there are enough cameras to attempt to gather a calibration dataset.
    */
    unsigned int numCameras = camList.GetSize();
    cout << "Number of cameras detected: " << numCameras << endl << endl;
    if (numCameras == 0)
    {
        // Clear camera list before releasing system
        camList.Clear();

        // Release system
        system->ReleaseInstance();

        cout << "Not enough cameras!" << endl;
        cout << "Done! Press Enter to exit..." << endl;
        getchar();
        throw std::exception();
    }else{
        return true;
    }
}

CameraList getCameras(SystemPtr system){
    /*
        This function returns the camera that are available to use. 
    */
    CameraList camList = system->GetCameras();
    if(cameraCheck(camList, system))
        return camList;
    
}

int main(int argc, char** argv)
{

    // Retrieve singleton reference to system object
    SystemPtr system = System::GetInstance();

    CameraList camList = getCameras(system);
    parseArgument(argc, argv, camList);

    // Clear camera list before releasing system
    camList.Clear();
    
    // Release system
    system->ReleaseInstance();

    cout << endl << "Done! Press Enter to exit..." << endl;
    getchar();

    return -1;
}