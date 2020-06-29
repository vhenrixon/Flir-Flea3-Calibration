#include "spinnaker/Spinnaker.h"
#include "spinnaker/SpinGenApi/SpinnakerGenApi.h"
#include <iostream>
#include <sstream>
#include <chrono>
#include <thread>
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
        pCam->ExposureMode.SetValue(ExposureMode_Timed);
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

void setCameraToContinuous(CameraPtr pCam){

    // Set acquisition mode to continuous
        if (!IsReadable(pCam->AcquisitionMode) || !IsWritable(pCam->AcquisitionMode))
        {
            cout << "Unable to set acquisition mode to continuous. Aborting..." << endl << endl;
            throw std::exception();
        }

        pCam->AcquisitionMode.SetValue(AcquisitionMode_Continuous);

        cout << "Acquisition mode set to continuous..." << endl;

}

void createDirectory(string path){

    boost::filesystem::path p{path};
    if(!(boost::filesystem::exists(p))){
        boost::filesystem::create_directory(p);
        cout << "Directory has been created for the images!" << endl;
        // Changing the current path of the program so that files can be saved in the directory that was just created 
        boost::filesystem::current_path(p);
    }
}

gcstring getSerialNumber(CameraPtr pCam){
    
    // Get device serial number for filename
    gcstring deviceSerialNumber("");

    if (IsReadable(pCam->TLDevice.DeviceSerialNumber))
    {
        deviceSerialNumber = pCam->TLDevice.DeviceSerialNumber.GetValue();

        cout << "Device serial number retrieved as " << deviceSerialNumber << "..." << endl;
    }
    cout << endl;

    return deviceSerialNumber;
}

ostringstream getUniqueName(gcstring serialNumber ,unsigned int imageCnt){

    ostringstream filename;
    filename << "ExposureQS-";
    if (serialNumber != "")
    {
        filename << serialNumber.c_str() << "-";
    }

    // current time for the name
    auto time = chrono::system_clock::now();
    auto since_epoch = time.time_since_epoch();
    auto milli = chrono::duration_cast<chrono::milliseconds>(since_epoch);
    long now = milli.count(); 

    filename << imageCnt << "-" << now << ".jpg";
    return filename;
}

// This function acquires and saves 8 images from a device; please see
// Acquisition example for more in-depth comments on the acquisition of images.
void acquireXImages(CameraPtr pCam, int amountOfImages)
{
    /*
    TODO
     - When acquiring image in a fast manner, you might get a error status of 3 which is incomplete packet so make a check if it 
    */

    cout << endl << "*** IMAGE ACQUISITION ***" << endl << endl;

    try
    {
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
                 cout << "getting img" << endl;
                if(pResultImage->IsIncomplete()){
                    continue;
                }else{
                    
                    // Convert image to mono 8
                    ImagePtr convertedImage = pResultImage->Convert(PixelFormat_Mono8);

                    // Create a unique filename
                    ostringstream filename = getUniqueName(deviceSerialNumber, img_collected);

                    // Save image
                    convertedImage->Save(filename.str().c_str());

                    cout << "Image saved at " << filename.str() << endl;
                    
                    img_collected++;
                }



            }catch(Spinnaker::Exception& e){
                cout << "Error: " << e.what() << endl;
            }
        }
        // Error: Spinnaker: Camera is not streaming [-1010] <---- this is the error that is occuring 
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
    
    try{
        int waitTime = 10; 
        int totalImg = 800;          // Simillar to the amount of the images in the vignette TUMS sample
        
        // Initialize camera
        pCam->Init();
        createDirectory("vignette-dataset");

        cout << "You have 10 seconds to position your camera!" << endl;
        sleep(waitTime);
        
        for(int pictureTaken=0; pictureTaken < totalImg; ++pictureTaken){
            acquireXImages(pCam, 1);

        
        }

        cout << "Completed gathering 800 pictures for the vignette Dataset." << endl;

        // Deinitialize the camera
        pCam->DeInit();



    }catch (Spinnaker::Exception& e){
        cout << "A error occurred while doing the vignette dataset collection" << endl;
        cout << "Error:" << e.what() << endl;
    }



}
vector<double> getExposureVector(CameraPtr pCam){
        
    try{
        double cam_min = pCam->ExposureTime.GetMin();   // Min exposure of the Flire Flea3 camera in Microseconds
        double cam_max = pCam->ExposureTime.GetMax();   // Max exposure of the Flire Flea3 camera in Microseconds
        double cam_increment = 10.0;         // Microseconds

        double tum_min = 50.0;              // Microseconds
        double tum_increment = 1.05;        // Microseconds   

        vector<double> cam_exposure_vector; 
        vector<double> tum_exposure_vector; 
        
        double cam_exposure = cam_min; 
        double tum_exposure = tum_min; 

        while(cam_exposure < cam_max){
            cam_exposure += cam_increment; 
            cam_exposure_vector.push_back(cam_exposure);
        }
        while(tum_exposure < cam_max){
            tum_exposure *= tum_increment;
            tum_exposure_vector.push_back(tum_exposure);
        }
        
        vector<double> calibration_exposure;

        int i = 0;
        int j = 0;
        while(i < tum_exposure_vector.size() && j < cam_exposure_vector.size()){
            while(i< tum_exposure_vector.size() && tum_exposure_vector[i] < cam_exposure_vector[j]){
                i++;
            }
            while(j < cam_exposure_vector.size() && cam_exposure_vector[j] < tum_exposure_vector[i]){
                j++;
            }

            auto exposure = cam_exposure_vector[j-1];
            calibration_exposure.push_back(exposure);
        }

        return calibration_exposure;

    }catch(Spinnaker::Exception& e){
        cout << "Error occurred when creating the exposure vector!" << endl;
        cout << "Error: " << e.what() << endl;
        throw std::exception();
    }

}

void responseDatasetCollection(CameraPtr pCam){

    try{     

        // Initialize camera
        pCam->Init();   
        
        createDirectory("response-dataset");
                                                
        vector<double> exposures = getExposureVector(pCam);

        // Looping for the 120 exposures that must be checked
        for(double exposure : exposures){
            
            // **NOTE** 
            // The image retrivale function handles the gathering of photos 

            setExposure(pCam, exposure);
            acquireXImages(pCam, 8);                         // The 1000 images at 120 exposures; 1000/120 = 8.333 Images/exposure
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

// Checking arguments to see what kind of photometric calibration will be used
void parseArgument(int argc,char** argv, CameraList camList){
        // Checking arguments to see what kind of photometric calibration will be used
    if(argc < 2){
        
        cout << "This program requires you to detail which kind of photometric calibration dataset collection is wanted. Either vignette or response." << endl;    
        throw std::exception();
    }else{
        if(string(argv[1]) == "--vignette"){
            
            cout << "Running vignette dataset collection!" << endl;
            vignetteDatasetCollection(camList.GetByIndex(0));

        }else if(string(argv[1]) == "--response"){
            
            cout << "Running response dataset collection!" << endl;
            responseDatasetCollection(camList.GetByIndex(0));

        }else{
            cout << "Argument is not understood! Please use either --vignette or --response. Exiting!" << endl;
            throw std::exception();
        }
    }
}

bool cameraCheck(CameraList camList,SystemPtr system){

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
        return false;
    }else{
        return true;
    }
}

CameraList getCameras(SystemPtr system){

    CameraList camList = system->GetCameras();
    // Finish if there are no cameras
    if(cameraCheck(camList, system)){
        return camList;
    }else{
        throw std::exception();     // Gracefully shutdown the program
    }
}


// Example entry point; please see Enumeration_QuickSpin example for more
// in-depth comments on preparing and cleaning up the system.
int main(int argc, char** argv)
{

    // Print application build information
    cout << "Application build date: " << __DATE__ << " " << __TIME__ << endl << endl;

    // Retrieve singleton reference to system object
    SystemPtr system = System::GetInstance();

    // Print out current library version
    const LibraryVersion spinnakerLibraryVersion = system->GetLibraryVersion();
    cout << "Spinnaker library version: " << spinnakerLibraryVersion.major << "." << spinnakerLibraryVersion.minor
         << "." << spinnakerLibraryVersion.type << "." << spinnakerLibraryVersion.build << endl
         << endl;

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