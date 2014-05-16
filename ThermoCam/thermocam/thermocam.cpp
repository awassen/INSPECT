#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <stdio.h>
#include <PvSampleUtils.h>
#include <PvDevice.h>
#include <PvBuffer.h>
#include <PvStream.h>
#include <PvInterface.h>
#include <PvSystem.h>
#include <PvBufferWriter.h>
#include <PvPixelType.h>
#include <PvString.h>
#include <string>
#include <cstdio>
#include <vector>
#include <iostream>
#include <PvBufferWriter.h>
#include <PvBufferConverter.h>
#include <PvSystem.h>
#include <fstream>
#include <ctime>
#include <iostream>

///////////////////// ORF CODE ////////////////////
//#include <libMesaSR.h>
///////////////////////////////////////////////////

PV_INIT_SIGNAL_HANDLER();

#define BUFFER_COUNT ( 16 )

bool AcquireImages()
{

	

	PvResult lResult;	
	PvDeviceInfo *lDeviceInfo = NULL;
	PvSystem lSystem;
	lSystem.SetDetectionTimeout( 100 );
	lResult = lSystem.Find();
	if( !lResult.IsOK() )
	{
		cout << "PvSystem::Find Error: " << lResult.GetCodeString().GetAscii();
		return -1;
	}
	PvUInt32 lInterfaceCount = lSystem.GetInterfaceCount();

	for( PvUInt32 x = 0; x < lInterfaceCount; x++ )
	{
		PvInterface * lInterface = lSystem.GetInterface( x );
		cout << "Ethernet Interface " << endl;
		cout << "IP Address: " << lInterface->GetIPAddress().GetAscii() << endl;
		cout << "Subnet Mask: " << lInterface->GetSubnetMask().GetAscii() << endl << endl;
		PvUInt32 lDeviceCount = lInterface->GetDeviceCount();
		for( PvUInt32 y = 0; y < lDeviceCount ; y++ )
		{
			lDeviceInfo = lInterface->GetDeviceInfo( y );
			cout << "ThermoCam " << endl;
			cout << "IP Address: " << lDeviceInfo->GetIPAddress().GetAscii() << endl;
		}
	}
	if( lDeviceInfo != NULL )
	{
		cout << "Connecting to " << lDeviceInfo->GetIPAddress().GetAscii() << endl;
		PvDevice lDevice;
		lResult = lDevice.Connect( lDeviceInfo );
		if ( !lResult.IsOK() )
		{
			cout << "Unable to connect to " << lDeviceInfo->GetIPAddress().GetAscii() << endl;
		}
		else
		{
			cout << "Successfully connected to " << lDeviceInfo->GetIPAddress().GetAscii() << endl;
///////////////////////////////////// ORF CODE /////////////////////////////////
//   printf("ORF pid %d\n", (int) getpid());
//  SRCAM cam;
//    int ret = SR_OpenETH(&cam, "169.254.1.33");
//    int ret2 = -1;
//     //if(ret<=0) return -1; // ret holds the number of cameras found
//    cv::Size imsize(SR_GetCols(cam), SR_GetRows(cam)); // SR image size
//    int sizebytes = 2 * imsize.area() * sizeof(unsigned short); // number of bytes sent from the SR 
//    int sizestep = sizeof(float)*3; // size step from one xyz component to the next
//   if(ret<=0) {
//	printf("No ORF found\n"); // ret holds the number of cameras found 
//    } else {
//	ret2 = 1;
//    	SR_SetMode(cam,AM_SW_TRIGGER);
//    }
/////////////////////////////////////////////////////////////////////////////////
    			PvGenParameterArray *lDeviceParams = lDevice.GetGenParameters();
    			PvGenInteger *lPayloadSize = dynamic_cast<PvGenInteger *>( lDeviceParams->Get( "PayloadSize" ) );
    			PvGenCommand *lStart = dynamic_cast<PvGenCommand *>( lDeviceParams->Get( "AcquisitionStart" ) );
    			PvGenCommand *lStop = dynamic_cast<PvGenCommand *>( lDeviceParams->Get( "AcquisitionStop" ) );
    			lDevice.NegotiatePacketSize();
    			PvStream lStream;
    			cout << "Opening stream to ThermoCam" << endl;
    			lStream.Open( lDeviceInfo->GetIPAddress() );
    			PvInt64 lSize = 0;
    			lPayloadSize->GetValue( lSize );
    			PvUInt32 lBufferCount = ( lStream.GetQueuedBufferMaximum() < BUFFER_COUNT ) ? 
        		lStream.GetQueuedBufferMaximum() : 
        		BUFFER_COUNT;
    			PvBuffer *lBuffers = new PvBuffer[ lBufferCount ];
    			for ( PvUInt32 i = 0; i < lBufferCount; i++ )
    			{
        			lBuffers[ i ].Alloc( static_cast<PvUInt32>( lSize ) );
    			}
    			lDevice.SetStreamDestination( lStream.GetLocalIPAddress(), lStream.GetLocalPort() );
    			PvGenParameterArray *lStreamParams = lStream.GetParameters();
    			PvGenInteger *lCount = dynamic_cast<PvGenInteger *>( lStreamParams->Get( "ImagesCount" ) );
    			PvGenFloat *lFrameRate = dynamic_cast<PvGenFloat *>( lStreamParams->Get( "AcquisitionRate" ) );
    			PvGenFloat *lBandwidth = dynamic_cast<PvGenFloat *>( lStreamParams->Get( "Bandwidth" ) );
    			for ( PvUInt32 i = 0; i < lBufferCount; i++ )
    			{
        			lStream.QueueBuffer( lBuffers + i );
    			}
    			PvGenCommand *lResetTimestamp = dynamic_cast<PvGenCommand *>( lDeviceParams->Get( "GevTimestampControlReset" ) );
    			lResetTimestamp->Execute();
    			cout << "Sending StartAcquisition command to ThermoCam" << endl;
    			lResult = lStart->Execute();		
			PvInt64 lWidth = 0, lHeight = 0;
			lDeviceParams->GetIntegerValue( "Width", lWidth);
			lDeviceParams->GetIntegerValue( "Height", lHeight);			
			cvNamedWindow("OpenCV: ThermoCam",CV_WINDOW_NORMAL);
			cv::Mat raw_lImage(cv::Size(lWidth,lHeight),CV_8U);  
/////////////////////////////////
//std::time_t start_time = std::time(0);
//////////////////////////////////  			
			char lDoodle[] = "|\\-|-/";
    			int lDoodleIndex = 0;
    			PvInt64 lImageCountVal = 0;
    			double lFrameRateVal = 0.0;
    			double lBandwidthVal = 0.0;
			cout << endl;
    			cout << "<press a key to stop streaming>" << endl;
    			while ( !PvKbHit() )
    			{
///////////////////////////////////////////////// ORF CODE //////////////////////////////////////
//			if (ret2>0){
//				printf("taking ORF picture\n\n");
//    				ret = SR_Acquire(cam);
//	        	        cv::Mat xyz(imsize, CV_32FC3, cv::Scalar::all(0));
//	        	        if(ret!=sizebytes) {
//					printf("ret != sizebytes\n");
//					//break;
//				}
//	        	        // the coordinates are stored as three channels in the format
//	        	        // (x1, y1, z1, x2, y2, z2, ... ) squentially row by row
//	        	        SR_CoordTrfFlt( cam, &((float*)xyz.ptr())[0], // pointer to first x
//	       	                        &((float*)xyz.ptr())[1], // pointer to first y
//	       	                        &((float*)xyz.ptr())[2], // pointer to first z
//	                                sizestep, sizestep, sizestep); // increments to next element
//				cv::FileStorage storage("ORF.yml",cv::FileStorage::WRITE);
//				storage << "ORF" << xyz;
//				storage.release();
//				sleep(1);
//
//			}
/////////////////////////////////////////////////////////////////////////////////////////////////

        			PvBuffer *lBuffer = NULL;
				PvImage *lImage = NULL;
				PvResult lOperationResult;	
        			PvResult lResult = lStream.RetrieveBuffer( &lBuffer, &lOperationResult, 1000 );
        			if ( lResult.IsOK() )
        			{
////////////////////////
timeval curTime;
gettimeofday(&curTime, NULL);
int milli = curTime.tv_usec / 1000;
time_t rawtime;
struct tm * timeinfo;
char buffer [80];
time(&rawtime);
timeinfo=localtime(&rawtime);
strftime(buffer,80,"%Y_%m_%d_%H_%M_%S",timeinfo);
char currentTime[84]="";
sprintf(currentTime,"%s_%d",buffer,milli);
printf("current time: %s \n",currentTime);
//////////////////
        				if(lOperationResult.IsOK())
        				{

         					lCount->GetValue( lImageCountVal );
						lFrameRate->GetValue( lFrameRateVal );
						lBandwidth->GetValue( lBandwidthVal );
						if ( lBuffer->GetPayloadType() == PvPayloadTypeImage )
						{
							lImage=lBuffer->GetImage();
							lBuffer->GetImage()->Alloc(lImage->GetWidth(),lImage->GetHeight(),PvPixelMono8);
							
						}
						cout << fixed << setprecision( 1 );
						cout << lDoodle[ lDoodleIndex ];
						cout << " W: " << dec << lWidth << " H: " << lHeight << " " << lFrameRateVal << " FPS " << ( lBandwidthVal / 1000000.0 ) << " Mb/s  \r";
        				}
					lImage->Attach(raw_lImage.data,lImage->GetWidth(),lImage->GetHeight(),PvPixelMono8);
					cv::imshow("OpenCV: ThermoCam",raw_lImage);
					PvBufferWriter lBufferWriter;
					stringstream filename;
					filename << "ThermoCam" << currentTime << ".bmp";					cv::imwrite(filename.str(),raw_lImage);
					//string file = filename.str();
					//lBufferWriter.Store(lBuffer,file,PvBufferFormatBMP);
///////////////////////////////////////////////////					
//					std::time_t t = std::time() - start_time;
//					printf("ThermoCam data taken at %d seconds\n",t);
//					std::stringstream filename;
//					filename << "./ThermoCam_photos/" << t << ".bmp";
//					cv::imwrite(filename.str(),raw_lImage);
////////////////////////////////////////////////////
					stringstream file;
					file << "ThermoCam" << currentTime << ".xml";
					cv::FileStorage fs(file.str(),cv::FileStorage::WRITE);
					fs << "raw_lImage" << raw_lImage;
					fs.release();
					//sleep(1);
					if(cv::waitKey(1000) >= 0) break;
					lStream.QueueBuffer( lBuffer );
					
					
			        }
        			else
        			{
            				cout << lDoodle[ lDoodleIndex ] << " Timeout\r";
        			}
			        ++lDoodleIndex %= 6;
    			}
			PvGetChar(); 
    			cout << endl << endl;
    			cout << "Sending AcquisitionStop command to ThermoCam" << endl;
    			lStop->Execute();
    			cout << "Aborting buffers still in stream" << endl;
    			lStream.AbortQueuedBuffers();
    			while ( lStream.GetQueuedBufferCount() > 0 )
    			{
        			PvBuffer *lBuffer = NULL;
        			PvResult lOperationResult;
			        lStream.RetrieveBuffer( &lBuffer, &lOperationResult );
			}
    			cout << "Releasing buffers" << endl;
    			delete []lBuffers;
    			cout << "Closing stream" << endl;
    			lStream.Close();
    			cout << "Disconnecting ThermoCam" << endl;
    			lDevice.Disconnect();
////////////////////////////////////////////// ORF CODE ////////////////////////////////////////////////
//			if (ret2>0) SR_Close(cam);
////////////////////////////////////////////////////////////////////////////////////////////////////////
			return true;
		}
	}
	else
	{
		cout << "No device found" << endl;
	}
	return 0;
}

int main()
{
   	cout << "Connecting to and streaming data from ThermoCam" << endl << endl;
	printf("pid %d\n", (int) getpid());    	
	AcquireImages();
	cout << endl;
    	cout << "<press a key to exit>" << endl;
    	PvWaitForKeyPress();
	return 0;
}


