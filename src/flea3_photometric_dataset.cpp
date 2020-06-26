#include "spinnaker/Spinnaker.h"
#include "spinnaker/SpinGenApi/SpinnakerGenApi.h"
#include <iostream>
#include <sstream>
#include <chrono>
#include <thread>
#include <ctime>
#include <boost/filesystem.hpp>

using namespace Spinnaker;
using namespace Spinnaker::GenApi;
using namespace Spinnaker::GenICam;
using namespace std;


// This function configures a custom exposure time. Automatic exposure is turned
// off in order to allow for the customization, and then the custom setting is
// applied.
int setExposure(CameraPtr pCam, double exposureTime)
{
    int result = 0;

    cout << endl << endl << "*** CONFIGURING EXPOSURE ***" << endl << endl;

    try
    {
        //
        // Turn off automatic exposure mode
        //
        // *** NOTES ***
        // Automatic exposure prevents the manual configuration of exposure
        // times and needs to be turned off for this example. Enumerations
        // representing entry nodes have been added to QuickSpin. This allows
        // for the much easier setting of enumeration nodes to new values.
        //
        // In C++, the naming convention of QuickSpin enums is the name of the
        // enumeration node followed by an underscore and the symbolic of
        // the entry node. Selecting "Off" on the "ExposureAuto" node is
        // thus named "ExposureAuto_Off".
        //
        // *** LATER ***
        // Exposure time can be set automatically or manually as needed. This
        // example turns automatic exposure off to set it manually and back
        // on to return the camera to its default state.
        //
        if (!IsReadable(pCam->ExposureAuto) || !IsWritable(pCam->ExposureAuto))
        {
            cout << "Unable to disable automatic exposure. Aborting..." << endl << endl;
            return -1;
        }

        pCam->ExposureAuto.SetValue(ExposureAuto_Off);

        cout << "Automatic exposure disabled..." << endl;

        //
        // Set exposure time manually; exposure time recorded in microseconds
        //
        // *** NOTES ***
        // Notice that the node is checked for availability and writability
        // prior to the setting of the node. In QuickSpin, availability is
        // ensured by checking for null while writability is ensured by checking
        // the access mode.
        //
        // Further, it is ensured that the desired exposure time does not exceed
        // the maximum. Exposure time is counted in microseconds - this can be
        // found out either by retrieving the unit with the GetUnit() method or
        // by checking SpinView.
        //
        if (!IsReadable(pCam->ExposureTime) || !IsWritable(pCam->ExposureTime))
        {
            cout << "Unable to set exposure time. Aborting..." << endl << endl;
            return -1;
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
        result = -1;
    }

    return result;
}

// This function returns the camera to a normal state by re-enabling automatic
// exposure.
int ResetExposure(CameraPtr pCam)
{
    int result = 0;

    try
    {
        //
        // Turn automatic exposure back on
        //
        // *** NOTES ***
        // Automatic exposure is turned on in order to return the camera to its
        // default state.
        //
        if (!IsReadable(pCam->ExposureAuto) || !IsWritable(pCam->ExposureAuto))
        {
            cout << "Unable to enable automatic exposure (node retrieval). Non-fatal error..." << endl << endl;
            return -1;
        }

        pCam->ExposureAuto.SetValue(ExposureAuto_Continuous);

        cout << "Automatic exposure enabled..." << endl << endl;
    }
    catch (Spinnaker::Exception& e)
    {
        cout << "Error: " << e.what() << endl;
        result = -1;
    }

    return result;
}

// This function prints the device information of the camera from the transport
// layer; please see NodeMapInfo example for more in-depth comments on printing
// device information from the nodemap.
int PrintDeviceInfo(CameraPtr pCam)
{
    int result = 0;

    cout << endl << "*** DEVICE INFORMATION ***" << endl << endl;

    try
    {
        INodeMap& nodeMap = pCam->GetTLDeviceNodeMap();

        FeatureList_t features;
        CCategoryPtr category = nodeMap.GetNode("DeviceInformation");
        if (IsAvailable(category) && IsReadable(category))
        {
            category->GetFeatures(features);

            FeatureList_t::const_iterator it;
            for (it = features.begin(); it != features.end(); ++it)
            {
                CNodePtr pfeatureNode = *it;
                cout << pfeatureNode->GetName() << " : ";
                CValuePtr pValue = (CValuePtr)pfeatureNode;
                cout << (IsReadable(pValue) ? pValue->ToString() : "Node not readable");
                cout << endl;
            }
        }
        else
        {
            cout << "Device control information not available." << endl;
        }
    }
    catch (Spinnaker::Exception& e)
    {
        cout << "Error: " << e.what() << endl;
        result = -1;
    }

    return result;
}


// This function acquires and saves 8 images from a device; please see
// Acquisition example for more in-depth comments on the acquisition of images.
int acquireXImages(CameraPtr pCam, int amountOfImages, string path, bool hasFolder)
{
    int result = 0;

    cout << endl << "*** IMAGE ACQUISITION ***" << endl << endl;

    try
    {
        // Set acquisition mode to continuous
        if (!IsReadable(pCam->AcquisitionMode) || !IsWritable(pCam->AcquisitionMode))
        {
            cout << "Unable to set acquisition mode to continuous. Aborting..." << endl << endl;
            return -1;
        }

        pCam->AcquisitionMode.SetValue(AcquisitionMode_Continuous);

        cout << "Acquisition mode set to continuous..." << endl;

        // Begin acquiring images
        pCam->BeginAcquisition();

        cout << "Acquiring images..." << endl;

        // Get device serial number for filename
        gcstring deviceSerialNumber("");

        if (IsReadable(pCam->TLDevice.DeviceSerialNumber))
        {
            deviceSerialNumber = pCam->TLDevice.DeviceSerialNumber.GetValue();

            cout << "Device serial number retrieved as " << deviceSerialNumber << "..." << endl;
        }
        cout << endl;

        // Creating folder
        if(!hasFolder){
            boost::filesystem::path p{path};
            boost::filesystem::create_directory(p);
            cout << "Directory has been created for the images!" << endl;
            // Changing the current path of the program so that files can be saved in the directory that was just created 
            boost::filesystem::current_path(p);
        }
        
    
        // Retrieve, convert, and save images
        const int k_numImages = amountOfImages;

        for (unsigned int imageCnt = 0; imageCnt < k_numImages; imageCnt++)
        {
            try
            {
                // Retrieve next received image and ensure image completion
                ImagePtr pResultImage = pCam->GetNextImage(1000);

                if (pResultImage->IsIncomplete())
                {
                    cout << "Image incomplete with image status " << pResultImage->GetImageStatus() << "..." << endl
                         << endl;
                }
                else
                {
                    // Print image information
                    cout << "Grabbed image " << imageCnt << ", width = " << pResultImage->GetWidth()
                         << ", height = " << pResultImage->GetHeight() << endl;

                    // Convert image to mono 8
                    ImagePtr convertedImage = pResultImage->Convert(PixelFormat_Mono8);

                    // Create a unique filename
                    ostringstream filename;

                    filename << "ExposureQS-";
                    if (deviceSerialNumber != "")
                    {
                        filename << deviceSerialNumber.c_str() << "-";
                    }

                    // current time for the name
                    time_t current_time;
                    time(&current_time);

                    filename << imageCnt << "-" << current_time << ".jpg";



                    // Save image
                    convertedImage->Save(filename.str().c_str());

                    cout << "Image saved at " << filename.str() << endl;
                }

                // Release image
                pResultImage->Release();

                cout << endl;
            }
            catch (Spinnaker::Exception& e)
            {
                cout << "Error: " << e.what() << endl;
                result = -1;
            }
        }

        // End acquisition
        pCam->EndAcquisition();
    }
    catch (Spinnaker::Exception& e)
    {
        cout << "Error: " << e.what() << endl;
        result = -1;
    }

    return result;
}
void vignetteDatasetCollection(CameraPtr pCam){    
    
    try{
        int waitTime = 10; 
        int totalImg = 800;          // Simillar to the amount of the images in the vignette TUMS sample
        bool hasFolder = false;
        
        // Initialize camera
        pCam->Init();

        cout << "You have 10 seconds to position your camera!" << endl;
        sleep(waitTime);
        for(int pictureTaken=0; pictureTaken < totalImg; ++pictureTaken){
            if(hasFolder){
                acquireXImages(pCam, 1, "vignette-dataset", true);
            }else{
                acquireXImages(pCam, 1, "vignette-dataset", false);
                hasFolder = true;
            }
        
        }

        cout << "Completed gathering 800 pictures for the vignette Dataset." << endl;

        // Deinitialize the camera
        pCam->DeInit();



    }catch (Spinnaker::Exception& e){
        cout << "A error occurred while doing the vignette dataset collection" << endl;
        cout << "Error:" << e.what() << endl;
    }



}
void responseDatasetCollection(CameraPtr pCam){

    try{
        int multiplicativeGain = 1.05;  // The Multiplicative Increment
        int currentExposure = 50;       // The starting point for the exposure is at 50(microseconds)
        int exposureCount = 120;        // The number of times that the exposure of the camera must be changed in the dataset
        bool hasFolder = false;
        // Initialize camera
        pCam->Init();   

        // Looping for the 120 exposures that must be checked
        for(int exposure=0; exposure < exposureCount; ++exposure){
            
            // **NOTE** 
            // The image retrivale function handles the gathering of photos 
            if(hasFolder){

                setExposure(pCam, currentExposure);
                currentExposure = currentExposure + (currentExposure*multiplicativeGain);
                acquireXImages(pCam, 8, "response-dataset", true);                         // The 1000 images at 120 exposures; 1000/120 = 8.333 Images/exposure

            }else{
                
                setExposure(pCam, currentExposure);
                currentExposure = currentExposure + (currentExposure*multiplicativeGain);
                acquireXImages(pCam, 8, "response-dataset", false);                          // The 1000 images at 120 exposures; 1000/120 = 8.333 Images/exposure
                hasFolder = true;
            }

        }
        ResetExposure(pCam);
        cout << "Gathered and saved 1000 images. The response Dataset collection is complete. " << endl;

        // Deinitialize the camera
        pCam->DeInit();



    }catch (Spinnaker::Exception& e){
        cout << "A error occurred while doing the response dataset collection" << endl;
        cout << "Error:" << e.what() << endl;
    }

}



// Example entry point; please see Enumeration_QuickSpin example for more
// in-depth comments on preparing and cleaning up the system.
int main(int argc, char** argv)
{
    int result = 0;

    // Print application build information
    cout << "Application build date: " << __DATE__ << " " << __TIME__ << endl << endl;

    // Retrieve singleton reference to system object
    SystemPtr system = System::GetInstance();

    // Print out current library version
    const LibraryVersion spinnakerLibraryVersion = system->GetLibraryVersion();
    cout << "Spinnaker library version: " << spinnakerLibraryVersion.major << "." << spinnakerLibraryVersion.minor
         << "." << spinnakerLibraryVersion.type << "." << spinnakerLibraryVersion.build << endl
         << endl;

    // Retrieve list of cameras from the system
    CameraList camList = system->GetCameras();

    unsigned int numCameras = camList.GetSize();

    cout << "Number of cameras detected: " << numCameras << endl << endl;

    // Finish if there are no cameras
    if (numCameras == 0)
    {
        // Clear camera list before releasing system
        camList.Clear();

        // Release system
        system->ReleaseInstance();

        cout << "Not enough cameras!" << endl;
        cout << "Done! Press Enter to exit..." << endl;
        getchar();

        return -1;
    }

    // Checking arguments to see what kind of photometric calibration will be used
    if(argc < 2){
        
        cout << "This program requires you to detail which kind of photometric calibration dataset collection is wanted. Either vignette or response." << endl;    
        return -1;
    
    }else{
        if(string(argv[1]) == "--vignette"){
            
            cout << "Running vignette dataset collection!" << endl;
            vignetteDatasetCollection(camList.GetByIndex(0));

        }else if(string(argv[1]) == "--response"){
            
            cout << "Running response dataset collection!" << endl;
            responseDatasetCollection(camList.GetByIndex(0));

        }else{
            cout << "Argument is not understood! Please use either --vignette or --response. Exiting!" << endl;
            return -1; 
        }
    }



    // Clear camera list before releasing system
    camList.Clear();

    // Release system
    system->ReleaseInstance();

    cout << endl << "Done! Press Enter to exit..." << endl;
    getchar();

    return result;
}