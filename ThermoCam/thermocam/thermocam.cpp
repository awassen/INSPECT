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

PV_INIT_SIGNAL_HANDLER();

#define BUFFER_COUNT (16)

bool AcquireImages()
{

	PvResult lResult;	
	PvDeviceInfo *lDeviceInfo = NULL;
	PvSystem lSystem;
	lSystem.SetDetectionTimeout( 100 );
	lResult = lSystem.Find();
	if( !lResult.IsOK() ) //searech for ThermoCam
	{
		cout << "PvSystem::Find Error: " << lResult.GetCodeString().GetAscii();
		return -1;
	}
	PvUInt32 lInterfaceCount = lSystem.GetInterfaceCount();

	for(PvUInt32 x = 0; x < lInterfaceCount; x++)
	{
		PvInterface * lInterface = lSystem.GetInterface(x);
		cout << "Ethernet Interface " << endl;
		cout << "IP Address: " << lInterface->GetIPAddress().GetAscii() << endl;
		cout << "Subnet Mask: " << lInterface->GetSubnetMask().GetAscii() << endl << endl;
		PvUInt32 lDeviceCount = lInterface->GetDeviceCount();
		for(PvUInt32 y = 0; y < lDeviceCount ; y++)
		{
			lDeviceInfo = lInterface->GetDeviceInfo(y);
			cout << "ThermoCam " << endl;
			cout << "IP Address: " << lDeviceInfo->GetIPAddress().GetAscii() << endl;
		}
	}
	if(lDeviceInfo != NULL)
	{
		cout << "Connecting to " << lDeviceInfo->GetIPAddress().GetAscii() << endl;
		PvDevice lDevice;
		lResult = lDevice.Connect(lDeviceInfo); // connect to ThermoCam
		if ( !lResult.IsOK() )
		{
			cout << "Unable to connect to " << lDeviceInfo->GetIPAddress().GetAscii() << endl;
		}
		else
		{
			cout << "Successfully connected to " << lDeviceInfo->GetIPAddress().GetAscii() << endl;
    			PvGenParameterArray *lDeviceParams = lDevice.GetGenParameters();
    			PvGenInteger *lPayloadSize = dynamic_cast<PvGenInteger *>(lDeviceParams->Get("PayloadSize"));
    			PvGenCommand *lStart = dynamic_cast<PvGenCommand *>(lDeviceParams->Get("AcquisitionStart"));
    			PvGenCommand *lStop = dynamic_cast<PvGenCommand *>(lDeviceParams->Get("AcquisitionStop"));
    			lDevice.NegotiatePacketSize();
    			PvStream lStream;
    			cout << "Opening stream to ThermoCam" << endl;
    			lStream.Open(lDeviceInfo->GetIPAddress()); // open stream
    			PvInt64 lSize = 0; // read device payload size
    			lPayloadSize->GetValue(lSize);
    			PvUInt32 lBufferCount = (lStream.GetQueuedBufferMaximum() < BUFFER_COUNT) ? //set buffer size and count
        		lStream.GetQueuedBufferMaximum() : 
        		BUFFER_COUNT;
    			PvBuffer *lBuffers = new PvBuffer[lBufferCount];
    			for (PvUInt32 i = 0; i < lBufferCount; i++)
    			{
        			lBuffers[i].Alloc(static_cast<PvUInt32>(lSize));
    			}
    			lDevice.SetStreamDestination(lStream.GetLocalIPAddress(), lStream.GetLocalPort()); //set device IP destination to the stream
    			PvGenParameterArray *lStreamParams = lStream.GetParameters(); // get stream parameters
    			PvGenInteger *lCount = dynamic_cast<PvGenInteger *>(lStreamParams->Get("ImagesCount"));
    			PvGenFloat *lFrameRate = dynamic_cast<PvGenFloat *>(lStreamParams->Get("AcquisitionRate"));
    			PvGenFloat *lBandwidth = dynamic_cast<PvGenFloat *>(lStreamParams->Get("Bandwidth"));
    			for (PvUInt32 i = 0; i < lBufferCount; i++)
    			{
        			lStream.QueueBuffer(lBuffers + i);
    			}
    			PvGenCommand *lResetTimestamp = dynamic_cast<PvGenCommand *>(lDeviceParams->Get("GevTimestampControlReset"));
    			lResetTimestamp->Execute();
    			cout << "Sending StartAcquisition command to ThermoCam" << endl; // stream open
    			lResult = lStart->Execute();		
			PvInt64 lWidth = 0, lHeight = 0;
			lDeviceParams->GetIntegerValue("Width", lWidth);
			lDeviceParams->GetIntegerValue("Height", lHeight);			
			cvNamedWindow("OpenCV: ThermoCam", CV_WINDOW_NORMAL); //create OpenCV display window
			cv::Mat raw_lImage(cv::Size(lWidth, lHeight), CV_8U);  // create OpenCV Mat object
    			cout << "<press a key to stop streaming>" << endl; // stream images until key pressed
    			while (!PvKbHit()) 
    			{
        			PvBuffer *lBuffer = NULL; //obatin next buffer
				PvImage *lImage = NULL;
				PvResult lOperationResult;	
        			PvResult lResult = lStream.RetrieveBuffer(&lBuffer, &lOperationResult, 1000);
        			if (lResult.IsOK())
        			{
					timeval curTime;
					gettimeofday(&curTime, NULL);
					int milli = curTime.tv_usec / 1000;
					time_t rawtime;
					struct tm * timeinfo;
					char buffer [80];
					time(&rawtime);
					timeinfo=localtime(&rawtime);
					strftime(buffer,80, "%Y_%m_%d_%H_%M_%S", timeinfo);
					char currentTime[84]="";
					sprintf(currentTime, "%s_%d", buffer, milli);
					printf("current time: %s \n", currentTime);
        				if(lOperationResult.IsOK())
        				{
						lCount->GetValue(lImageCountVal);
						lFrameRate->GetValue(lFrameRateVal);
						lBandwidth->GetValue(lBandwidthVal);
						if (lBuffer->GetPayloadType() == PvPayloadTypeImage)
						{
							lImage=lBuffer->GetImage();
							lBuffer->GetImage()->Alloc(lImage->GetWidth(), lImage->GetHeight(), PvPixelMono8); // read image parameters
							
						}
					}
					lImage->Attach(raw_lImage.data, lImage->GetWidth(), lImage->GetHeight(), PvPixelMono8);
					cv::imshow("OpenCV: ThermoCam",raw_lImage); // display image stream
					PvBufferWriter lBufferWriter;
					stringstream filename;
					filename << "ThermoCam" << currentTime << ".bmp"; 				
					cv::imwrite(filename.str(), raw_lImage); // store image as bitmap file	
					stringstream file;
					file << "ThermoCam" << currentTime << ".xml";
					cv::FileStorage fs(file.str(),cv::FileStorage::WRITE); //store raw image data
					fs << "raw_lImage" << raw_lImage;
					fs.release();
					if(cv::waitKey(1000) >= 0) break; //sets image frequency
					lStream.QueueBuffer(lBuffer); // queue buffer
					
					
			        }
        			else
        			{
            				cout << "Timeout" << endl; // timeout
        			}
    			}
			PvGetChar(); // flush key buffer for next step
    			cout << endl << endl;
    			cout << "Sending AcquisitionStop command to ThermoCam" << endl;
    			lStop->Execute();
    			lStream.AbortQueuedBuffers();
    			while (lStream.GetQueuedBufferCount() > 0)
    			{
        			PvBuffer *lBuffer = NULL;
        			PvResult lOperationResult;
			        lStream.RetrieveBuffer(&lBuffer, &lOperationResult);
			}
    			cout << "Releasing buffers" << endl;
    			delete []lBuffers; 
    			cout << "Closing stream" << endl;
    			lStream.Close(); // close the stream
    			cout << "Disconnecting ThermoCam" << endl;
    			lDevice.Disconnect(); //disconnect ThermoCam
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


