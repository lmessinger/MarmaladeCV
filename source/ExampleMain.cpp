/*C-ScanMain.cpp
 * 
 */
#include "ExampleMain.h"
#include <malloc.h>
#include <vector>
#include <math.h>
#include <string.h>
#include "../opencv2/opencv.hpp"
#include "../opencv2/core/core.hpp"

#include "s3eImagePicker.h"
#include "Iw2D.h"
#include "IwJPEG.h"

#include "camera.h"

#define SCREEN_SIZE_X 320.0 
#define SCREEN_SIZE_Y 480.0

CIw2DImage* g_2dimage = NULL;
CIwTexture* g_Texture = NULL;

CIwImage* ConvtImage(CIwImage* image) 
{
	CIwImage::Format format = image->GetFormat();
	
	int nChannels = image->GetPitch()/image->GetWidth();
	IplImage * colorRes = cvCreateImage( cvSize(image->GetWidth(),image->GetHeight()),IPL_DEPTH_8U, nChannels);
		
	if (colorRes == NULL)
		return false;
	
	memcpy(colorRes->imageData,image->GetTexels(),colorRes->imageSize);

	IplImage * dstImage = cvCreateImage( cvSize(image->GetWidth(),image->GetHeight()),IPL_DEPTH_8U, 3 );
	//cvCvtColor(colorRes,dstImage,  CV_BGR2GRAY);
	cvNot(colorRes,dstImage);
	// create a new image
	uint32 * new_buf = new uint32[ image->GetWidth()* image->GetHeight()];
	memcpy( new_buf,  (uchar *)dstImage->imageData, ( image->GetWidth()* image->GetHeight() * nChannels ) );
	
	static CIwImage imageDest;
	imageDest.SetFormat( format );
	imageDest.SetWidth( image->GetWidth() );
	imageDest.SetHeight( image->GetHeight() );
	imageDest.SetOwnedBuffers( (uint8*)new_buf, NULL );
	g_2dimage = Iw2DCreateImage( imageDest );
	//Iw2DDrawImage( image2D, CIwSVec2(0,0) );
	// delete image2D;
	//Texture->CopyFromBuffer(Width, Height, CIwImage::PVRTC_2, Width / 8, texture_data, 0);
	
	return &imageDest;
}

void GetScreen();

//=======================================================
// Function name   : GetScreen
// Description     : 
// Return type     : void 
//=======================================================
void GetScreen()
{
	char path[1024];
	CIwTexture* g_Texture = new CIwTexture;
	if(!Camera::Instance()->CameraScreenShot(path,1024))
		return;
	else
	{		
		s3eDebugOutputString("Ok, jpg pass it:");
		s3eDebugOutputString(path);
		void* data = NULL;
		int len = 0;
		s3eFile *f = 0;

		if (!(f = s3eFileOpen( path, "rb")))
			return;
		len = (int)s3eFileGetSize(f);
		if (len <= 0)
			return;

		data = s3eMalloc(len);
		if (!data)
		{
			s3eFileClose(f);
			return;
		}

		uint32 rtn = s3eFileRead(data, 1, len, f);
		s3eFileClose(f);

		if (rtn != (uint32)len)
		{
			s3eFree(data);
		}
									
		JPEGTexture((char*) data,len,*g_Texture);
	}
			
	//g_2dimage = Iw2DCreateImage(g_Texture->GetImage());
		
	if(g_Texture )
	{			
		// convert
		CIwImage* imgDest = ConvtImage(&g_Texture->GetImage()); 
		s3eDebugOutputString("Ok, end convert");
		float indexX = (float)s3eSurfaceGetInt(S3E_SURFACE_WIDTH)/(float)g_Texture->GetImage().GetWidth();
		float indexY = (float)s3eSurfaceGetInt(S3E_SURFACE_HEIGHT)/(float)g_Texture->GetImage().GetHeight();
		
	}
}





//=======================================================
// Function name   : MainUpdate
// Description     : 
// Return type     : bool 
//=======================================================
bool MainUpdate()
{
	s3eDeviceYield(0);
    s3eKeyboardUpdate();
    s3ePointerUpdate();
	
    return true;
}


//=======================================================
// Function name   : MainTerm
// Description     : 
// Return type     : void 
//=======================================================

void MainTerm()
{
   

	IwGxTerminate();

	
}


//=======================================================
// Function name   : MainInit
// Description     : 
// Return type     : void 
//=======================================================
void MainInit()
{
	// initializing global environments
	s3eDebugSetInt(S3E_DEBUG_TRACE_TO_FILE,1);
	Iw2DInit();
	IwGxInit();
	
	// initializing camera and taking a screenshot
	Camera::Instance()->InitCamera();

	GetScreen();

}




//=======================================================
// Function name   : UIRender
// Description     : 
// Return type     : void 
//=======================================================
void UIRender()
{
    IwGxClear(IW_GX_COLOUR_BUFFER_F | IW_GX_DEPTH_BUFFER_F);
 
	if(g_2dimage)
	{
		Iw2DDrawImage(g_2dimage,CIwSVec2(0,0),CIwSVec2(s3eSurfaceGetInt(S3E_SURFACE_WIDTH),s3eSurfaceGetInt(S3E_SURFACE_HEIGHT)));
	}
		
	IwGxFlush();
    IwGxSwapBuffers();

}
