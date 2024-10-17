/*=Plus=header=begin======================================================
Program: Plus
Copyright (c) Laboratory for Percutaneous Surgery. All rights reserved.
See License.txt for details.

Developed by ULL & IACTEC-IAC group
=========================================================Plus=header=end*/
// Specim
#include "SI_sensor.h"
#include "SI_errors.h"
#include "SI_types.h"


// Local includes
#include "PlusConfigure.h"
#include "vtkPlusChannel.h"
#include "vtkPlusDataSource.h"
#include "vtkPlusSpecimCam.h"

// VTK includes
#include <vtkImageData.h>
#include <vtkObjectFactory.h>

SI_H g_hDevice = 0;
SI_U8 *S_BufferAddress = nullptr;
SI_64 S_FrameSize;
SI_64 S_FrameNumber;

std::string cameraModel;

//----------------------------------------------------------------------------

vtkStandardNewMacro(vtkPlusSpecimCam);

//----------------------------------------------------------------------------
vtkPlusSpecimCam::vtkPlusSpecimCam()
{
  this->RequireImageOrientationInConfiguration = true;
  this->StartThreadForInternalUpdates = true;
}

//----------------------------------------------------------------------------
vtkPlusSpecimCam::~vtkPlusSpecimCam()
{
 // Due to freeze connects & disconnects, Load should be in ReadConfiguration 
 // after the reading of ProfilesDirectory,  and Unload should be here 
 // in the last executed function (this one).
  SI_Unload();
}


PlusStatus specimStopAcquisition(void)
{
  SI_Command(g_hDevice, L"Acquisition.Stop");
  return PLUS_SUCCESS;
}

PlusStatus specimStartAcquisition(void)
{
  SI_Command(g_hDevice, L"Acquisition.Start");
  return PLUS_SUCCESS;
}

PlusStatus specimInitialize(void)
{
  SI_Command(g_hDevice, L"Initialize");
  return PLUS_SUCCESS;
}

size_t getSpecimImageWidth(void) {
  SI_64 nWidth = 0;
  SI_GetInt(g_hDevice, L"Camera.Image.Width", &nWidth);
  return static_cast<size_t>(nWidth);
}

size_t getSpecimImageHeight(void) {
  SI_64 nHeight = 0;
  SI_GetInt(g_hDevice, L"Camera.Image.Height", &nHeight);
  return static_cast<size_t>(nHeight);
}

SI_64 getSpecimBufferSize(void) {
  SI_64 nBufferSize = 0;
  SI_GetInt(g_hDevice, L"Camera.Image.SizeBytes",&nBufferSize);
  return nBufferSize;
}

//----------------------------------------------------------------------------
void vtkPlusSpecimCam::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "SpecimCam: Specim Fx17e Camera" << std::endl;
}



/***
int ConfigureGVCPHeartbeat(CameraPtr pCam, bool enable)
{
  //
  // Write to boolean node controlling the camera's heartbeat
  //
  // *** NOTES ***
  // This applies only to GEV cameras.
  //
  // GEV cameras have a heartbeat built in, but when debugging applications the
  // camera may time out due to its heartbeat. Disabling the heartbeat prevents
  // this timeout from occurring, enabling us to continue with any necessary 
  // debugging.
  //
  // *** LATER ***
  // Make sure that the heartbeat is reset upon completion of the debugging.  
  // If the application is terminated unexpectedly, the camera may not locked
  // to Spinnaker indefinitely due to the the timeout being disabled.  When that 
  // happens, a camera power cycle will reset the heartbeat to its default setting.

  // Retrieve TL device nodemap
  INodeMap& nodeMapTLDevice = pCam->GetTLDeviceNodeMap();

  // Retrieve GenICam nodemap
  INodeMap& nodeMap = pCam->GetNodeMap();

  CEnumerationPtr ptrDeviceType = nodeMapTLDevice.GetNode("DeviceType");
  if (!IsReadable(ptrDeviceType))
  {
    LOG_ERROR("Unable to read DeviceType (node retrieval, !IsReadable).");
    return PLUS_FAIL;
  }

  if (ptrDeviceType->GetIntValue() != DeviceType_GigEVision)
  {
    LOG_DEBUG("Device is not a GigEVision. Skipping.");
    return PLUS_SUCCESS;
  }

  if (enable)
  {
    LOG_DEBUG("Resetting heartbeat.");
  }
  else
  {
    LOG_DEBUG("Disabling heartbeat.");
  }

CBooleanPtr ptrDeviceHeartbeat = nodeMap.GetNode("GevGVCPHeartbeatDisable");
if (!IsWritable(ptrDeviceHeartbeat))
{
  LOG_DEBUG("Unable to configure heartbeat. Continuing with execution as this may be non-fatal.");
  return PLUS_FAIL;
}

  ptrDeviceHeartbeat->SetValue(enable);

  if (!enable)
  {
    LOG_DEBUG("WARNING: Heartbeat has been disabled for the rest of this example run." << endl
    << "         Heartbeat will be reset upon the completion of this run.  If the " << endl
    << "         example is aborted unexpectedly before the heartbeat is reset, the" << endl
    << "         camera may need to be power cycled to reset the heartbeat.");
  }
  else
  {
    LOG_DEBUG("Heartbeat has been reset.");
  }

  return PLUS_SUCCESS;
}
***/
/****
PlusStatus ConfigureExposure(INodeMap& nodeMap,DWORD exposureTimeToSet)
{
  LOG_DEBUG("*** CONFIGURING EXPOSURE ***");

  try
  {
    //
    // Turn off automatic exposure mode
    //
    // *** NOTES ***
    // Automatic exposure prevents the manual configuration of exposure
    // time and needs to be turned off.
    //
    // *** LATER ***
    // Exposure time can be set automatically or manually as needed. This
    // example turns automatic exposure off to set it manually and back
    // on in order to return the camera to its default state.
    //

    CEnumerationPtr ptrExposureAuto = nodeMap.GetNode("ExposureAuto");
    if (!IsReadable(ptrExposureAuto))
    {
      LOG_ERROR("Unable to disable automatic exposure (node retrieval, !IsReadable). Aborting...");
      return PLUS_FAIL;
    }

    if (!IsWritable(ptrExposureAuto))
    {
      LOG_ERROR("Unable to disable automatic exposure (node retrieval, !IsWritable). Aborting...");
      return PLUS_FAIL;
    }

    CEnumEntryPtr ptrExposureAutoOff = ptrExposureAuto->GetEntryByName("Off");
    if (!IsReadable(ptrExposureAutoOff))
    {
      LOG_ERROR("Unable to disable automatic exposure (enum entry retrieval). Aborting...");
      return PLUS_FAIL;
    }

    ptrExposureAuto->SetIntValue(ptrExposureAutoOff->GetValue());

    LOG_DEBUG("Automatic exposure disabled...");

    //
    // Set exposure time manually; exposure time recorded in microseconds
    //
    // *** NOTES ***
    // The node is checked for availability and writability prior to the
    // setting of the node. Further, it is ensured that the desired exposure
    // time does not exceed the maximum. Exposure time is counted in
    // microseconds. This information can be found out either by
    // retrieving the unit with the GetUnit() method or by checking SpinView.
    //
    CFloatPtr ptrExposureTime = nodeMap.GetNode("ExposureTime");
    if (!IsReadable(ptrExposureTime) ||
      !IsWritable(ptrExposureTime))
    {
      LOG_ERROR("Unable to get or set exposure time. Aborting...");
      return PLUS_FAIL;
    }

    // Ensure desired exposure time does not exceed the maximum
    const double exposureTimeMax = ptrExposureTime->GetMax();

    if (exposureTimeToSet > exposureTimeMax)
    {
      exposureTimeToSet = exposureTimeMax;
    }

    ptrExposureTime->SetValue(exposureTimeToSet);

    LOG_DEBUG("Exposure time set to " << exposureTimeToSet << " us...");

  }
  catch (Spinnaker::Exception& e)
  {
    LOG_ERROR("Error Configuring Exposure (Spinnaker SDK): " << e.what());
    return PLUS_FAIL;
  }


  return PLUS_SUCCESS;
}

// This function returns the camera to its default state by re-enabling automatic
// exposure.
PlusStatus ResetExposure(INodeMap& nodeMap)
{
  CEnumerationPtr ptrExposureAuto = nullptr;
  try
  {
    //
    // Turn automatic exposure back on
    //
    // *** NOTES ***
    // Automatic exposure is turned on in order to return the camera to its
    // default state.
    //
    ptrExposureAuto = nodeMap.GetNode("ExposureAuto");

    if (!IsReadable(ptrExposureAuto) ||
      !IsWritable(ptrExposureAuto))
    {
      LOG_DEBUG("Unable to enable automatic exposure (node retrieval). Non-fatal error...");
      return PLUS_SUCCESS;
    }

    CEnumEntryPtr ptrExposureAutoContinuous = ptrExposureAuto->GetEntryByName("Continuous");
    if (!IsReadable(ptrExposureAutoContinuous))
    {
      LOG_DEBUG("Unable to enable automatic exposure (enum entry retrieval). Non-fatal error...");
      return PLUS_SUCCESS;
    }

    ptrExposureAuto->SetIntValue(ptrExposureAutoContinuous->GetValue());

    LOG_DEBUG("Automatic exposure enabled...");
  }
  catch (Spinnaker::Exception& e)
  {
    LOG_ERROR("Error: " << e.what());
    return PLUS_FAIL;
  }

  return PLUS_SUCCESS;
}


int PrintDeviceInfo(INodeMap& nodeMap)
{
  int result = PLUS_SUCCESS;

  LOG_DEBUG("*** DEVICE INFORMATION ***");

  try
  {
    FeatureList_t features;
    CCategoryPtr category = nodeMap.GetNode("DeviceInformation");
    if (IsReadable(category))
    {
      category->GetFeatures(features);

      FeatureList_t::const_iterator it;
      for (it = features.begin(); it != features.end(); ++it)
      {
        CNodePtr pfeatureNode = *it;
        CValuePtr pValue = (CValuePtr)pfeatureNode;
        LOG_DEBUG(pfeatureNode->GetName() << " : " << (IsReadable(pValue) ? pValue->ToString() : "Node not readable"));
       }
    }
    else
    {
      LOG_DEBUG("Device control information not readable.");
    }
  }
  catch (Spinnaker::Exception& e)
  {
    LOG_ERROR("Error: " << e.what());
    return PLUS_FAIL;
  }
  return PLUS_SUCCESS;
}
***/

//-----------------------------------------------------------------------------

PlusStatus vtkPlusSpecimCam::ReadConfiguration(vtkXMLDataElement* rootConfigElement)
{
  XML_FIND_DEVICE_ELEMENT_REQUIRED_FOR_READING(deviceConfig, rootConfigElement);
  int numAttributes = deviceConfig->GetNumberOfAttributes();
  SI_WC ProfilesDirectory[4096];
  const char * pd = deviceConfig->GetAttribute("ProfilesDirectory");
  SI_64 nDeviceCount;
  SI_WC szProfileName[4096];
  std::string profileName;
  bool profileFound = false;
  char buffer[4096];
  char buffer2[4096];

  cameraModel = deviceConfig->GetAttribute("Model");

  std::mbstowcs(ProfilesDirectory, pd, 4096);
  std::wcstombs(buffer, ProfilesDirectory, sizeof(buffer));

  if (int  er = SI_SetString(SI_SYSTEM, L"ProfilesDirectory", ProfilesDirectory) != 0) {
    LOG_DEBUG("(Error " << er << ") Specim " << cameraModel << " Camera Profile not found.");
    return PLUS_FAIL;
  }

  SI_Load(L"");

  SI_GetInt(SI_SYSTEM, L"DeviceCount", &nDeviceCount);
  LOG_DEBUG("nDeviceCount = " << nDeviceCount);
  for (int n = 0; n < nDeviceCount; n++)
  {
    SI_GetEnumStringByIndex(SI_SYSTEM, L"DeviceName", n, szProfileName, 4096);
    std::wcstombs(buffer2, szProfileName, sizeof(buffer));
    profileName = buffer2;
    if (profileName.find(cameraModel) != std::string::npos) {
      LOG_DEBUG("Specim " << cameraModel << " Camera Profile found: " << profileName);
      profileFound = true;
      break;
    }
    else {
      LOG_DEBUG("Specim " << cameraModel << " Probado profileName con: " << profileName << ".");

    }
  }
  if (!profileFound) {
    LOG_DEBUG("Specim " << cameraModel << " Camera Profile not found in directory " << buffer << ".");
    return PLUS_FAIL;
  }

  return PLUS_SUCCESS;
}

//-----------------------------------------------------------------------------
PlusStatus vtkPlusSpecimCam::WriteConfiguration(vtkXMLDataElement* rootConfigElement)
{
  XML_FIND_DEVICE_ELEMENT_REQUIRED_FOR_WRITING(deviceConfig, rootConfigElement);
  return PLUS_SUCCESS;
}

//----------------------------------------------------------------------------
PlusStatus vtkPlusSpecimCam::FreezeDevice(bool freeze)
{
  if (freeze)
  {
    this->Disconnect();
  }
  else
  {
    this->Connect();
  }

  return PLUS_SUCCESS;
}


//----------------------------------------------------------------------------
PlusStatus vtkPlusSpecimCam::InternalConnect()
{

  int nDeviceIndex = 0; // Por ahora, solo el primero

  SI_Open(nDeviceIndex, &g_hDevice);
  specimInitialize();

  if (S_BufferAddress == nullptr) {
    SI_64 nBufferSize = getSpecimBufferSize();
    SI_CreateBuffer(g_hDevice, nBufferSize, (void**)&S_BufferAddress);
  }

  specimStartAcquisition();

/*****
  // Retrieve singleton reference to system object
  psystem = System::GetInstance();

  const LibraryVersion spinnakerLibraryVersion = psystem->GetLibraryVersion();
  LOG_DEBUG("Spinnaker library version: " << spinnakerLibraryVersion.major << "." << spinnakerLibraryVersion.minor
    << "." << spinnakerLibraryVersion.type << "." << spinnakerLibraryVersion.build);
  // CameraList camList = psystem->GetCameras();
  camList = psystem->GetCameras();

  const unsigned int numCameras = camList.GetSize();
  
  if (numCameras == 0)
  {
    // Clear camera list before releasing system
    camList.Clear();
    // Release system
    psystem->ReleaseInstance();
    LOG_ERROR("No FLIR cameras detected!");
    return PLUS_FAIL;
  }
 
  // For this version, takes the first cam in the list.
  pCam = camList.GetByIndex(0);
  INodeMap& nodeMapTLDevice = pCam->GetTLDeviceNodeMap();
  // Mostrar capacidades de la cámara
  PrintDeviceInfo(nodeMapTLDevice);

  pCam->Init();

  INodeMap& nodeMap = pCam->GetNodeMap();

  ConfigureGVCPHeartbeat(pCam, false);

  // Set pixel format
  if (SetPixelFormat(nodeMap) == PLUS_FAIL) {
    camList.Clear();
    psystem->ReleaseInstance();
    return PLUS_FAIL;
  } 

LOG_DEBUG("Number of FLIR cameras detected: " << numCameras);

  try {
    CCommandPtr ptrAutoFocus = nodeMap.GetNode("AutoFocus");
    if (!IsAvailable(ptrAutoFocus)) {
      LOG_DEBUG("AutoFocus not available.");
    } else if (!IsWritable(ptrAutoFocus)) {
      LOG_DEBUG("AutoFocus not writable.");
    } else {
        ptrAutoFocus->Execute();
        LOG_DEBUG("Autofocus On.");
    }
  }
  catch (Spinnaker::Exception& e) {
    LOG_DEBUG("Error: " << e.what());
  }

  try{
    CEnumerationPtr ptrAcquisitionMode = nodeMap.GetNode("AcquisitionMode");
    if (!IsReadable(ptrAcquisitionMode) ||
      !IsWritable(ptrAcquisitionMode))
    {
      LOG_ERROR("Unable to set acquisition mode to continuous (enum retrieval). Aborting.");
      camList.Clear();
      psystem->ReleaseInstance();
      return PLUS_FAIL;
    }

    // Retrieve entry node from enumeration node
    CEnumEntryPtr ptrAcquisitionModeContinuous = ptrAcquisitionMode->GetEntryByName("Continuous");
    if (!IsReadable(ptrAcquisitionModeContinuous))
    {
      LOG_ERROR("Unable to set acquisition mode to continuous (enum retrieval 2). Aborting.");
      camList.Clear();
      psystem->ReleaseInstance();
      return PLUS_FAIL;
    }
    // Retrieve integer value from entry node
    const int64_t acquisitionModeContinuous = ptrAcquisitionModeContinuous->GetValue();

    // Set integer value from entry node as new value of enumeration node
    ptrAcquisitionMode->SetIntValue(acquisitionModeContinuous);

    LOG_DEBUG("Acquisition mode set to continuous...");

    // Acquisition start
    pCam->BeginAcquisition();
    LOG_DEBUG("Acquisition starting...");

    processor.SetColorProcessing(SPINNAKER_COLOR_PROCESSING_ALGORITHM_HQ_LINEAR);

  }
  catch (Spinnaker::Exception& e)
  {
    LOG_ERROR("Error starting acquisition: " << e.what());
    camList.Clear();
    psystem->ReleaseInstance();
    return PLUS_FAIL;
  }
********/
  return PLUS_SUCCESS;
}


//----------------------------------------------------------------------------
PlusStatus vtkPlusSpecimCam::InternalDisconnect()
{
  /****
  // Deinitialize camera
  try {
    pCam->EndAcquisition();
    pCam->DeInit();
    camList.Clear();
    psystem->ReleaseInstance();
  }
  catch (Spinnaker::Exception& e)
  {
    LOG_ERROR("Error: " << e.what());
  }

**/
  specimStopAcquisition();
  SI_DisposeBuffer(g_hDevice, S_BufferAddress);
  SI_Close(g_hDevice);
  return PLUS_SUCCESS;
}



//----------------------------------------------------------------------------
PlusStatus vtkPlusSpecimCam::InternalUpdate()
{

  int pixelType = VTK_UNSIGNED_CHAR;
  int numberOfScalarComponents = 1;
  US_IMAGE_TYPE imageType = US_IMG_BRIGHTNESS;

  
  vtkPlusDataSource* aSource(nullptr);
  if (this->GetFirstActiveOutputVideoSource(aSource) == PLUS_FAIL || aSource == nullptr)
  {
    LOG_ERROR("Unable to grab a video source. Skipping frame.");
    return PLUS_FAIL;
  }

  const size_t width = getSpecimImageWidth();
  const size_t height = getSpecimImageHeight();

  SI_Wait(g_hDevice, S_BufferAddress, &S_FrameSize, &S_FrameNumber, SI_INFINITE);
  this->m_currentTime = vtkIGSIOAccurateTimer::GetSystemTime();

  if (aSource->GetNumberOfItems() == 0)
  {
    // Init the buffer with the metadata from the first frame
    aSource->SetImageType(imageType);
    aSource->SetPixelType(pixelType);
    aSource->SetNumberOfScalarComponents(numberOfScalarComponents);
    aSource->SetInputFrameSize(width, height, 1);
  }
  // Add the frame to the stream buffer
  FrameSizeType frameSize = { static_cast<unsigned int>(width), static_cast<unsigned int>(height), 1 };
  if (aSource->AddItem(S_BufferAddress, aSource->GetInputImageOrientation(), frameSize, pixelType, numberOfScalarComponents, imageType, 0, this->FrameNumber, this->m_currentTime, this->m_currentTime) == PLUS_FAIL)
  {
    return PLUS_FAIL;
  }
  this->FrameNumber++;

/*****
  ImagePtr pResultImage = pCam->GetNextImage(1000);
  int pixelType = VTK_UNSIGNED_CHAR;
  int numberOfScalarComponents = 1;
  US_IMAGE_TYPE imageType = US_IMG_BRIGHTNESS;
  ImagePtr convertedImage;

  this->m_currentTime = vtkIGSIOAccurateTimer::GetSystemTime();

  if (pResultImage->IsIncomplete())
  {
    // Retrieve and print the image status description
    LOG_DEBUG("Image incomplete: " << Image::GetImageStatusDescription(pResultImage->GetImageStatus()));
  }
  else {
    const size_t width = pResultImage->GetWidth();
    const size_t height = pResultImage->GetHeight();


    switch (this->iVideoFormat) {
    case Mono8:
      convertedImage = processor.Convert(pResultImage, PixelFormat_Mono8);
      pixelType = VTK_UNSIGNED_CHAR;
      numberOfScalarComponents = 1;
      imageType = US_IMG_BRIGHTNESS;
      break;
    case Mono16:
    default:
      convertedImage = processor.Convert(pResultImage, PixelFormat_Mono16);
      pixelType = VTK_UNSIGNED_SHORT;
      numberOfScalarComponents = 1;
      imageType = US_IMG_BRIGHTNESS;
      break;
      //aSource->SetImageType(US_IMG_RGB_COLOR);
      //aSource->SetNumberOfScalarComponents(3);
    }

    vtkPlusDataSource* aSource(nullptr);
    if (this->GetFirstActiveOutputVideoSource(aSource) == PLUS_FAIL || aSource == nullptr)
    {
      LOG_ERROR("Unable to grab a video source. Skipping frame.");
      return PLUS_FAIL;
    }

    if (aSource->GetNumberOfItems() == 0)
    {
      // Init the buffer with the metadata from the first frame
      aSource->SetImageType(imageType);
      aSource->SetPixelType(pixelType);
      aSource->SetNumberOfScalarComponents(numberOfScalarComponents);
      aSource->SetInputFrameSize(width, height, 1);
    }

    // Add the frame to the stream buffer
    FrameSizeType frameSize = { static_cast<unsigned int>(width), static_cast<unsigned int>(height), 1 };
    if (aSource->AddItem(convertedImage->GetData(), aSource->GetInputImageOrientation(), frameSize, pixelType, numberOfScalarComponents, imageType, 0, this->FrameNumber, this->m_currentTime, this->m_currentTime) == PLUS_FAIL)
    {
      return PLUS_FAIL;
    }
    this->FrameNumber++;
  }

**/
  return PLUS_SUCCESS;

}

//----------------------------------------------------------------------------
PlusStatus vtkPlusSpecimCam::NotifyConfigured()
{
  if (this->OutputChannels.size() > 1)
  {
    LOG_WARNING("vtkPlusSpecimCam is expecting one output channel and there are " << this->OutputChannels.size() << " channels. First output channel will be used.");
  }

  if (this->OutputChannels.empty())
  {
    LOG_ERROR("No output channels defined for vtkPlusSpecimCam. Cannot proceed.");
    this->CorrectlyConfigured = false;
    return PLUS_FAIL;
  }
  return PLUS_SUCCESS;
}
