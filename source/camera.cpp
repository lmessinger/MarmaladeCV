/*camera.cpp
 *
 */

#include "ExampleMain.h"


#include "s3eDebug.h"
#include <sys/param.h>
#include "math.h"
#include "../opencv2/opencv.hpp"
#include "camera.h"


static s3eCameraCaptureResult* g_BufferedCapture = 0;;
 char buf[128];
//Alexsandr Evmenchik function

//init static of camera class
Camera* Camera::CameraPtr = NULL; 


CIwTexture* Camera::m_ImageTexture = NULL;
s3eCameraFrameRotation Camera::m_FrameRotation = S3E_CAMERA_FRAME_ROTNORMAL;//S3E_CAMERA_FRAME_ROT90;


CameraState Camera::m_Status = kUnSupported;

//Function defenition of class Camera






//=======================================================
// Function name   : Camera::cameraUpdate
// Description     : 
// Return type     : int32 
// Argument        : void* systemData
// Argument        : void* userData
//=======================================================

int32 Camera::cameraUpdate(void* systemData, void* userData)
{
	
	if (m_Status != kStarted)
        return 0;

    s3eCameraFrameData *data = (s3eCameraFrameData*)systemData;

    // If there is no texture, create one. 
    // This is a slow operation compared to memcpy so we don't want to do it every frame.
    if (m_ImageTexture == NULL)
    {
        m_ImageTexture = new CIwTexture();
        m_ImageTexture->SetMipMapping(false);
        m_ImageTexture->SetModifiable(true);

        m_ImageTexture->CopyFromBuffer(data->m_Width, data->m_Height, CIwImage::RGB_565, data->m_Pitch, (uint8*)data->m_Data, NULL);
        m_ImageTexture->Upload();

		
    }

    // Copy the camera image data into the texture. Note that it does not get copied to VRAM at this point.
    memcpy(m_ImageTexture->GetTexels(), data->m_Data, data->m_Height * data->m_Pitch);
    
	m_FrameRotation = data->m_Rotation;

    return 0;
}



 


//=======================================================
// Function name   : Camera::InitCamera
// Description     : 
// Return type     : bool 
//=======================================================
bool Camera::InitCamera()
{
	if(s3eCameraAvailable())
	{
		if (S3E_RESULT_ERROR == s3eCameraRegister(S3E_CAMERA_UPDATE_STREAMING, Camera::cameraUpdate, 0))
			return false;
			m_Status = kStopped;
			
	
	}
	else
		return false;

	return true;
}





//=======================================================
// Function name   : Camera::CameraStart
// Description     : 
// Return type     : bool 
//=======================================================

bool Camera::CameraStart()
{
	if(m_Status == kStopped)
		if (s3eCameraStart(S3E_CAMERA_STREAMING_SIZE_HINT_LARGEST, S3E_CAMERA_PIXEL_TYPE_RGB565_CONVERTED) == S3E_RESULT_SUCCESS)
			return true;
	return false;
}

 void Camera::StopCamera()
{
	if (m_Status == kStarted)
	{
		s3eCameraStop();
	 s3eCameraUnRegister(S3E_CAMERA_UPDATE_STREAMING, Camera::cameraUpdate);
		m_Status = kStopped;
	}

}




 //=======================================================
 // Function name   : Camera::GetCameraState
 // Description     : 
 // Return type     : CameraState 
 //=======================================================

CameraState Camera::GetCameraState()
{
	return m_Status;
}


//=======================================================
// Function name   : Camera::GetCamFrame
// Description     : 
// Return type     : bool 
// Argument        : CIwImage& frame
//=======================================================
bool Camera::GetCamFrame(CIwImage& frame)
{
	if(m_Status == kStarted)
	{
		frame = this->m_ImageTexture->GetImage();
		return true;
	}

	return false;
}





//=======================================================
// Function name   : Camera::CameraScreenShot
// Description     : 
// Return type     : bool 
// Argument        : char* buff
// Argument        : int buffSize
//=======================================================
bool Camera::CameraScreenShot(char* buff,int buffSize)
{
	bool res = false;
	s3eResult result;

	if(m_Status == kStarted)
	s3eCameraStop();
	// s3eCameraUnRegister(S3E_CAMERA_UPDATE_STREAMING, Camera::cameraUpdate);
	if(s3eCameraCaptureAvailable())
	{
			
			res = s3eCameraCaptureIsFormatSupported(S3E_CAMERACAPTURE_FORMAT_JPG);
			if(res)
			{
				result = s3eCameraCaptureToFile(buff,buffSize,S3E_CAMERACAPTURE_FORMAT_JPG);
				if(result!=S3E_RESULT_SUCCESS)
				{
					char buf[128];
					 sprintf (buf,"capture have error %d ",result);
					 s3eDebugOutputString(buf);
					res = false;
				}
				
	
			}
			
	}
	
	if(m_Status == kStarted)
		s3eCameraStart(S3E_CAMERA_STREAMING_SIZE_HINT_LARGEST, S3E_CAMERA_PIXEL_TYPE_RGB565_CONVERTED);
	return res;
}




//****************************************
//Border detect functions
//*************************************


//=======================================================
// Function name   : angle
// Description     : 
// Return type     : double 
// Argument        :  cv::Point pt1
// Argument        : cv::Point pt2
// Argument        : cv::Point pt0
//=======================================================
double angle( cv::Point pt1, cv::Point pt2, cv::Point pt0 )
{
    double dx1 = pt1.x - pt0.x;
    double dy1 = pt1.y - pt0.y;
    double dx2 = pt2.x - pt0.x;
    double dy2 = pt2.y - pt0.y;
    return (dx1*dx2 + dy1*dy2)/sqrt((dx1*dx1 + dy1*dy1)*(dx2*dx2 + dy2*dy2) + 1e-10);
}


//=======================================================
// Function name   : histogramEqualize
// Description     : 
// Return type     : void 
// Argument        : const IplImage * const pSource
// Argument        : IplImage * pTarget
//=======================================================
void histogramEqualize(const IplImage * const pSource, IplImage * pTarget)
{
	assert(pSource!=NULL);
	assert(pTarget!=NULL);
	assert(pSource->nChannels ==1);
	assert(pTarget->nChannels ==1); 
	assert(pSource->width == pTarget->width);
	assert(pSource->height == pTarget->height);
	CvHistogram *hist;
	uchar lut[1024];
	double lut1[1024];
	CvMat* lut_mat;
	int hist_size = 256;
	float range_0[]={0,256};
	float* ranges[] = { range_0 };
		
	int high=0;
	int low =0;
	float hist_value = 0.0;
	
	hist = cvCreateHist(1, &hist_size, CV_HIST_ARRAY, ranges, 1);
	lut_mat = cvCreateMatHeader( 1, 256, CV_8UC1 );
	cvSetData( lut_mat, lut, 0 );
	//cvCalcHist( &pSource, hist, 0, NULL );
	cvCalcHist( const_cast<IplImage**>(&pSource), hist, 0, NULL );
	
	//CUMULATIVE ARRAY
	lut1[0] = 0;
	for(int index = 0; index != 256; ++index)
	 {
	    //  hist_value = cvQueryHistValue_1D(hist,index);
		hist_value = ::cvGetReal1D(hist->bins,index);
		lut1[index+1]= lut1[index] + hist_value;
	 }
	
	
	//CALCULATE THE NEW LUT
	float scale_factor;
	scale_factor = 256.0f / (float) (pSource->width * pSource->height);
	for (int index=0; index!=256; ++index)
	{
		lut[index]= (unsigned char)((float)lut1[index]*scale_factor);
	}
	//PERFORM IT ON THE CHANNEL
	cvLUT( pSource, pTarget, lut_mat );
	cvReleaseMat( &lut_mat);
	cvReleaseHist(&hist);
}


//=======================================================
// Function name   : DrawContour
// Description     : 
// Return type     : bool 
// Argument        : IplImage* colorRes
// Argument        : bool contrast
// Argument        : bool smooth
// Argument        : bool dilate
// Argument        : CIwSVec2* coords
//=======================================================
bool DrawContour(IplImage* colorRes, bool contrast,bool smooth, bool dilate,CIwSVec2* coords)
{
	bool result = true;
	double maxSquare = 0.0;
	double maxCosine = 0;
	double square = 0;
	CvSeq* pointsMax = NULL;
	IplImage* gray = NULL;
	IplImage* colored = NULL;
	CvMemStorage* storage2 = NULL;
	CvMemStorage* storageHull = NULL;
	static char dbuf[256];


	gray = cvCreateImage(::cvGetSize(colorRes), IPL_DEPTH_8U, 1);
	storage2 = cvCreateMemStorage(0);
	storageHull = cvCreateMemStorage(0);
	
	
	/*colored = cvCreateImage( cvSize(colorRes->width,colorRes->height),IPL_DEPTH_8U, 3 );
	IplImage* out = cvCreateImage( cvSize(colored->width/2,colored->height/2), colored->depth, colored->nChannels );	
	//if(pyrDown)
	{
		::cvPyrDown(colorRes,out);
		::cvPyrUp(out,colored);
	}
	cvFree_(out);*/
	CIw2DImage* image;

	s3eResult ret = s3eDebugSetInt(S3E_DEBUG_TRACE_TO_FILE,1);

	for( int c = 0; c < colorRes->nChannels; c++ )
    {
		int ch[] = {c, 0};
		const CvArr* in[] = {colorRes};
		CvArr* out[] = {gray};
		::cvMixChannels(in,1,out,1,ch,1);
	
		if(contrast)
			histogramEqualize(gray,gray);

		if(smooth)
			cvSmooth( gray, gray, CV_BLUR, 9, 9, 2, 2);
		
		int thresh = 50, N = 11;
		cvCanny(gray, gray, 0, thresh, 3);
		if(dilate)
			::cvDilate(gray,gray);
	

		int total = 0; 
		int counter =0;
		CvSeq* contour2;	
		


		total = cvFindContours(gray, storage2, &contour2, sizeof(CvContour), CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE);
		if(total > 1)
		{
			while (contour2 != NULL) 
			{
				if (contour2->elem_size > 0) 
				{
					 CvSeq* points = cvApproxPoly(contour2,sizeof(CvContour), storage2, CV_POLY_APPROX_DP,cvContourPerimeter(contour2)*0.02, 0);
					 int point_num = points->total;
					 int square =  cvContourArea(points);
					 int convexity = cvCheckContourConvexity(points);

					 sprintf (dbuf,"points: point_num=%d area=%d convexity=%d ",point_num, square, convexity);
					 s3eDebugOutputString(dbuf);
					 if(points->total == 4 && fabs(square)>1000 && convexity > 0)
					 {
						::CvSeqBlock * seg = points->first;
						::CvPoint poiunt[4];
						
						for(int i=0; i<4; i++)
						{
							poiunt[i] = *((CvPoint*)(points->first->data)+i);
					
						}
						maxCosine = 0;
						for( int j = 2; j < 5; j++ )
						{
							// find the maximum cosine of the angle between joint edges
							double cosine = fabs(angle(poiunt[j%4], poiunt[j-2],poiunt[j-1]));
							maxCosine = MAX(maxCosine, cosine);
							sprintf (dbuf,"cosine=%f max=%f ",cosine, maxCosine);
							s3eDebugOutputString(dbuf);
						}
						s3eDebugOutputString("here");
						s3eDebugTracePrintf ("max is=%f ",maxCosine);

						if( maxCosine < 0.7 )
						{ 
							sprintf (dbuf,"max found=%f ",maxCosine);
							s3eDebugOutputString(dbuf);
							if (maxSquare < square)
							{
								maxSquare = square;
								pointsMax = points;
								
								sprintf (dbuf,"maxS=%f pointsMax=%p",maxSquare,pointsMax );
								s3eDebugOutputString(dbuf);
							}
					
						}
					}
					
				}
				
				 contour2 = contour2->h_next;
			}//end while
			::cvFree_(contour2);
		}//end if (total > 1)
		
	}//end for
	//cvCvtColor(colorRes, gray, CV_RGB2GRAY);
	if(pointsMax)
	{
		::CvPoint poiunt[4];
			 for(int i=0; i<4; i++){
					   poiunt[i] = *((CvPoint*)(pointsMax->first->data)+i);
					   (coords+i)->x = poiunt[i].x;
					   (coords+i)->y = poiunt[i].y;
			 }
		
	}
	else
		result = false;

	cvFree_(storage2);
	cvFree_(storageHull);
	cvFree_(gray);
	//cvFree_(colored);
	
	//cvFree_(pointsMax);
	sprintf (dbuf,"result=%d pointsMax=%p maxSquare=%f (%d,%d) (%d,%d) (%d,%d) (%d,%d)",result, pointsMax, maxSquare, (coords)->x, (coords)->y, 
					(coords+1)->x, (coords+1)->y, (coords+2)->x, (coords+2)->y, (coords+3)->x, (coords+3)->y );
	s3eDebugOutputString(dbuf);
	s3eDebugTraceFlush ();
	return result;
}







//=======================================================
// Function name   : FindContour
// Description     : 
// Return type     : bool 
// Argument        : CIwImage* m_ImageRes
// Argument        : CIwSVec2* coords
//=======================================================

bool FindContour(CIwImage* m_ImageRes,CIwSVec2* coords)
{

	
	IplImage* colorRes = NULL;
	

	//  CIwImage tempBGR;
	//  CIwImage oldFormat(*m_ImageRes);
	//  tempBGR.SetFormat(CIwImage::BGR_888);
	//  oldFormat.ConvertToImage(&tempBGR);

	
	//IplImage* colorRes = cvCreateImage( cvSize(tempBGR.GetWidth(),tempBGR.GetHeight()),IPL_DEPTH_8U, 3 );
	CIwImage::Format format = m_ImageRes->GetFormat();
	
	
	colorRes = cvCreateImage( cvSize(m_ImageRes->GetWidth(),m_ImageRes->GetHeight()),IPL_DEPTH_8U, m_ImageRes->GetPitch()/m_ImageRes->GetWidth() );
		

	if(colorRes==NULL||coords==NULL)
	{
		s3eDebugOutputString("Bad, no coords or color Res");
		return false;
	}
	//colorRes = cvCreateImage( cvSize(m_ImageRes->GetWidth(),m_ImageRes->GetHeight()),IPL_DEPTH_8U, 3 );
	//memcpy(colorRes->imageData,tempBGR.GetTexels(),colorRes->imageSize);
	memcpy(colorRes->imageData,m_ImageRes->GetTexels(),colorRes->imageSize);
	
	bool res = false;
	// DrawContour(IplImage* colorRes, bool contrast,bool smooth, bool dilate,CIwSVec2* coords)
	res = DrawContour(colorRes,false,true,true,coords);

	if(!res)
		res = DrawContour(colorRes,true,false,false,coords);
	if(!res)
		res = DrawContour(colorRes,true,true,false,coords);
	if(!res)
		res = DrawContour(colorRes,true,true,true,coords);
	
	::cvFree_(colorRes);

	return res;
}