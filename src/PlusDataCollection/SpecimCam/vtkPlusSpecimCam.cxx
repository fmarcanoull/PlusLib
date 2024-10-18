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

#define SPECIM_CHECK_ERROR(nError) \
  if (SI_FAILED(nError)) { \
    showSpecimErrorMessage(SI_GetErrorString(nError)); \
    return PLUS_FAIL; \
  }

int specimStopAcquisition(void)
{
  return(SI_Command(g_hDevice, L"Acquisition.Stop"));
}

int specimStartAcquisition(void)
{
  return(SI_Command(g_hDevice, L"Acquisition.Start"));
}

int specimInitialize(void)
{
  return(SI_Command(g_hDevice, L"Initialize"));
}

int getSpecimImageWidth(size_t *W) {
  SI_64 nWidth = 0;
  int nError = siNoError;

  nError=SI_GetInt(g_hDevice, L"Camera.Image.Width", &nWidth);
  *W = static_cast<size_t>(nWidth);
  return nError; 
}

int getSpecimImageHeight(size_t *H) {
  SI_64 nHeight = 0;
  int nError = siNoError;

  nError=SI_GetInt(g_hDevice, L"Camera.Image.Height", &nHeight);
  *H = static_cast<size_t>(nHeight);
  return nError;
}

SI_64 getSpecimBufferSize(SI_64 * nBufferSize) {
  return(SI_GetInt(g_hDevice, L"Camera.Image.SizeBytes",nBufferSize));
}

void showSpecimErrorMessage(const SI_WC *errMsg)
{
  char errorBuffer[4096];
  std::wcstombs(errorBuffer,errMsg, sizeof(errorBuffer));
  LOG_ERROR("Error: " << errorBuffer);
}

//----------------------------------------------------------------------------
void vtkPlusSpecimCam::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "SpecimCam: Specim Fx17e Camera" << std::endl;
}


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
  int nError = siNoError;

  cameraModel = deviceConfig->GetAttribute("Model");

  std::mbstowcs(ProfilesDirectory, pd, 4096);
  std::wcstombs(buffer, ProfilesDirectory, sizeof(buffer));

  if (int  er = SI_SetString(SI_SYSTEM, L"ProfilesDirectory", ProfilesDirectory) != 0) {
    LOG_DEBUG("(Error " << er << ") Specim " << cameraModel << " Camera Profile not found.");
    return PLUS_FAIL;
  }

  SI_CHK(SI_Load(L""));

  SI_CHK(SI_GetInt(SI_SYSTEM, L"DeviceCount", &nDeviceCount));
  LOG_DEBUG("nDeviceCount = " << nDeviceCount);
  for (int n = 0; n < nDeviceCount; n++)
  {
    SI_CHK(SI_GetEnumStringByIndex(SI_SYSTEM, L"DeviceName", n, szProfileName, 4096));
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

Error:
  SPECIM_CHECK_ERROR(nError);

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
  int nError = siNoError;
  int nDeviceIndex = 0; // Por ahora, solo el primero

  SI_CHK(SI_Open(nDeviceIndex, &g_hDevice));
  SI_CHK(specimInitialize());

  if (S_BufferAddress == nullptr) {
    SI_64 nBufferSize = 0;
    SI_CHK(getSpecimBufferSize(&nBufferSize));
    SI_CHK(SI_CreateBuffer(g_hDevice, nBufferSize, (void**)&S_BufferAddress));
  }

  SI_CHK(specimStartAcquisition());

Error:
  SPECIM_CHECK_ERROR(nError);

  return PLUS_SUCCESS;
}


//----------------------------------------------------------------------------
PlusStatus vtkPlusSpecimCam::InternalDisconnect()
{
  int nError = siNoError;
  SI_CHK(specimStopAcquisition());
  SI_CHK(SI_DisposeBuffer(g_hDevice, S_BufferAddress));
  SI_Close(g_hDevice);

Error:
  SPECIM_CHECK_ERROR(nError);

  return PLUS_SUCCESS;
}



//----------------------------------------------------------------------------
PlusStatus vtkPlusSpecimCam::InternalUpdate()
{

  int pixelType = VTK_UNSIGNED_CHAR;
  int numberOfScalarComponents = 1;
  US_IMAGE_TYPE imageType = US_IMG_BRIGHTNESS;
  int nError = siNoError;
  
  vtkPlusDataSource* aSource(nullptr);
  if (this->GetFirstActiveOutputVideoSource(aSource) == PLUS_FAIL || aSource == nullptr)
  {
    LOG_ERROR("Unable to grab a video source. Skipping frame.");
    return PLUS_FAIL;
  }

  size_t width; 
  size_t height; 
  SI_CHK(getSpecimImageWidth(&width));  
  SI_CHK(getSpecimImageHeight(&height));


  SI_CHK(SI_Wait(g_hDevice, S_BufferAddress, &S_FrameSize, &S_FrameNumber, SI_INFINITE));
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

Error:
  SPECIM_CHECK_ERROR(nError);

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
