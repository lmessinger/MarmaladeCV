

#ifndef _AE_CAMERA_H
#define _AE_CAMERA_H

#include "s3eCamera.h"
#include "s3eCameraCapture.h" 

//Declaration Camera class


typedef enum 
{
    kStarted,
    kStopped,
    kUnSupported
} CameraState;
class Camera
{
//variable
	static Camera* CameraPtr;
	
	static CameraState m_Status;

	static CIwTexture* m_ImageTexture;
	static s3eCameraFrameRotation m_FrameRotation;
	

//functions
	Camera()  
	{};

public:

	//functions
	static Camera* Instance()
	{
		if(!CameraPtr)
		CameraPtr = new Camera;

		return CameraPtr;
	}
	bool InitCamera();
	static void StopCamera();
	static bool CameraStart();
	CIwTexture* GetFrameTexture()
	{
		 if (m_ImageTexture != NULL)
		 {
			m_ImageTexture->ChangeTexels(m_ImageTexture->GetTexels(), CIwImage::RGB_565);
		 }
		return m_ImageTexture;
	}
	static int32 cameraUpdate(void* systemData, void* userData);	
	bool GetCamFrame(CIwImage& frame);

	bool CameraScreenShot(char* buff,int buffSize);
	static CameraState GetCameraState();

};


#endif