/*=Plus=header=begin======================================================
Program: Plus
Copyright (c) Laboratory for Percutaneous Surgery. All rights reserved.
See License.txt for details.

Developed by ULL & IACTEC-IAC group
=========================================================Plus=header=end*/

#ifndef __vtkPlusSpecimCam_h
#define __vtkPlusSpecimCam_h

#include "vtkPlusDataCollectionExport.h"
#include "vtkPlusDevice.h"
#include "SI_sensor.h"


/*!
\class vtkPlusSpecimCam
\brief Class for interfacing an FLIR Systems Spinnaker capture device and recording frames into a Plus buffer

\ingroup PlusLibDataCollection
*/

class vtkPlusDataCollectionExport vtkPlusSpecimCam : public vtkPlusDevice
{
public:
  static vtkPlusSpecimCam* New();
  vtkTypeMacro(vtkPlusSpecimCam, vtkPlusDevice);
  virtual void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /*! Read configuration from xml data */
  PlusStatus ReadConfiguration(vtkXMLDataElement* config);
  /*! Write configuration to xml data */
  PlusStatus WriteConfiguration(vtkXMLDataElement* config);
  

  /*! Sets video mode */
  //PlusStatus SetPixelFormat(INodeMap& nodeMap);
  /* Autofoco de cámara */
  //PlusStatus AutoFocus();
  
/*! Manage device frozen state */
  PlusStatus FreezeDevice(bool freeze);

  /*! Is this device a tracker */
  bool IsTracker() const { return false; }
  /*! Get an update from the tracking system and push the new transforms to the tools. This function is called by the tracker thread.*/
  PlusStatus InternalUpdate();

  /*! Verify the device is correctly configured */
  virtual PlusStatus NotifyConfigured();


protected:
  vtkPlusSpecimCam();
  ~vtkPlusSpecimCam();

  virtual PlusStatus InternalConnect() VTK_OVERRIDE;
  virtual PlusStatus InternalDisconnect() VTK_OVERRIDE;

protected:
/**
  enum VideoFormatMode : int
  {
    Mono8 = 0,
    Mono16 = 1
  };
 CameraList camList;
  DWORD dwExposure;
  int iVideoFormat;
**/  
  double m_currentTime = UNDEFINED_TIMESTAMP;
};

#endif // __vtkPlusSpecimCam_h
