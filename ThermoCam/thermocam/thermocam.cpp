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

	PvResult iResult;	
	PvDeviceInfo *iDeviceInfo = NULL;
	PvSystem iSystem;
	iSystem.SetDetectionTimeout( 100 );
	iResult = iSystem.Find();
	if( !iResult.IsOK() ) //searech for ThermoCam
	{
		cout << "PvSystem::Find Error: " << iResult.GetCodeString().GetAscii();
		return -1;
	}
	PvUInt32 iInterfaceCount = iSystem.GetInterfaceCount();

	for(PvUInt32 x = 0; x < iInterfaceCount; x++)
	{
		PvInterface * iInterface = iSystem.GetInterface(x);
		cout << "Ethernet Interface " << endl;
		cout << "IP Address: " << iInterface->GetIPAddress().GetAscii() << endl;
		cout << "Subnet Mask: " << iInterface->GetSubnetMask().GetAscii() << endl << endl;
		PvUInt32 lDeviceCount = iInterface->GetDeviceCount();
		for(PvUInt32 y = 0; y < iDeviceCount ; y++)
		{
			iDeviceInfo = iInterface->GetDeviceInfo(y);
			cout << "ThermoCam " << endl;
			cout << "IP Address: " << iDeviceInfo->GetIPAddress().GetAscii() << endl;
		}
	}
	if(iDeviceInfo != NULL)
	{
		cout << "Connecting to " << iDeviceInfo->GetIPAddress().GetAscii() << endl;
		PvDevice iDevice;
		iResult = iDevice.Connect(iDeviceInfo); // connect to ThermoCam
		if ( !iResult.IsOK() )
		{
			cout << "Unable to connect to " << iDeviceInfo->GetIPAddress().GetAscii() << endl;
		}
		else
		{
			cout << "Successfully connected to " << iDeviceInfo->GetIPAddress().GetAscii() << endl;
    			PvGenParameterArray *iDeviceParams = iDevice.GetGenParameters();
    			PvGenInteger *iPayloadSize = dynamic_cast<PvGenInteger *>(iDeviceParams->Get("PayloadSize"));
    			PvGenCommand *iStart = dynamic_cast<PvGenCommand *>(iDeviceParams->Get("AcquisitionStart"));
    			PvGenCommand *iStop = dynamic_cast<PvGenCommand *>(iDeviceParams->Get("AcquisitionStop"));
    			iDevice.NegotiatePacketSize();
    			PvStream iStream;
    			cout << "Opening stream to ThermoCam" << endl;
    			iStream.Open(iDeviceInfo->GetIPAddress()); // open stream
    			PvInt64 iSize = 0; // read device payload size
    			iPayloadSize->GetValue(iSize);
    			PvUInt32 iBufferCount = (iStream.GetQueuedBufferMaximum() < BUFFER_COUNT) ? //set buffer size and count
        		iStream.GetQueuedBufferMaximum() : 
        		BUFFER_COUNT;
    			PvBuffer *iBuffers = new PvBuffer[iBufferCount];
    			for (PvUInt32 i = 0; i < iBufferCount; i++)
    			{
        			iBuffers[i].Alloc(static_cast<PvUInt32>(iSize));
    			}
    			iDevice.SetStreamDestination(iStream.GetLocalIPAddress(), iStream.GetLocalPort()); //set device IP destination to the stream
    			PvGenParameterArray *iStreamParams = iStream.GetParameters(); // get stream parameters
    			PvGenInteger *iCount = dynamic_cast<PvGenInteger *>(iStreamParams->Get("ImagesCount"));
    			PvGenFloat *iFrameRate = dynamic_cast<PvGenFloat *>(iStreamParams->Get("AcquisitionRate"));
    			PvGenFloat *iBandwidth = dynamic_cast<PvGenFloat *>(iStreamParams->Get("Bandwidth"));
    			for (PvUInt32 i = 0; i < iBufferCount; i++)
    			{
        			iStream.QueueBuffer(iBuffers + i);
    			}
    			PvGenCommand *iResetTimestamp = dynamic_cast<PvGenCommand *>(iDeviceParams->Get("GevTimestampControlReset"));
    			iResetTimestamp->Execute();
    			cout << "Sending StartAcquisition command to ThermoCam" << endl; // stream open
    			iResult = iStart->Execute();		
			PvInt64 iWidth = 0, iHeight = 0;
			iDeviceParams->GetIntegerValue("Width", iWidth);
			iDeviceParams->GetIntegerValue("Height", iHeight);			
			cvNamedWindow("OpenCV: ThermoCam", CV_WINDOW_NORMAL); //create OpenCV display window
			cv::Mat raw_iImage(cv::Size(iWidth, iHeight), CV_8U);  // create OpenCV Mat object
    			cout << "<press a key to stop streaming>" << endl; // stream images until key pressed
    			while (!PvKbHit()) 
    			{
        			PvBuffer *iBuffer = NULL; //obatin next buffer
				PvImage *iImage = NULL;
				PvResult iOperationResult;	
        			PvResult iResult = iStream.RetrieveBuffer(&iBuffer, &iOperationResult, 1000);
        			if (iResult.IsOK())
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
						iCount->GetValue(iiImageCountVal);
						iFrameRate->GetValue(iFrameRateVal);
						iBandwidth->GetValue(iBandwidthVal);
						if (iBuffer->GetPayloadType() == PvPayloadTypeImage)
						{
							iImage=iBuffer->GetImage();
							iBuffer->GetImage()->Alloc(iImage->GetWidth(), iImage->GetHeight(), PvPixelMono8); // read image parameters
							
						}
					}
					iImage->Attach(raw_iImage.data, iImage->GetWidth(), iImage->GetHeight(), PvPixelMono8);
					cv::imshow("OpenCV: ThermoCam",raw_lImage); // display image stream
					PvBufferWriter iBufferWriter;
					stringstream filename;
					filename << "ThermoCam" << currentTime << ".bmp"; 				
					cv::imwrite(filename.str(), raw_iImage); // store image as bitmap file	
					stringstream file;
					file << "ThermoCam" << currentTime << ".xml";
					cv::FileStorage fs(file.str(),cv::FileStorage::WRITE); //store raw image data
					fs << "raw_lImage" << raw_iImage;
					fs.release();
					if(cv::waitKey(1000) >= 0) break; //sets image frequency
					iStream.QueueBuffer(iBuffer); // queue buffer
					
					
			        }
        			else
        			{
            				cout << "Timeout" << endl; // timeout
        			}
    			}
			PvGetChar(); // flush key buffer for next step
    			cout << endl << endl;
    			cout << "Sending AcquisitionStop command to ThermoCam" << endl;
    			iStop->Execute();
    			iStream.AbortQueuedBuffers();
    			while (iStream.GetQueuedBufferCount() > 0)
    			{
        			PvBuffer *iBuffer = NULL;
        			PvResult iOperationResult;
			        iStream.RetrieveBuffer(&iBuffer, &iOperationResult);
			}
    			cout << "Releasing buffers" << endl;
    			delete []iBuffers; 
    			cout << "Closing stream" << endl;
    			iStream.Close(); // close the stream
    			cout << "Disconnecting ThermoCam" << endl;
    			iDevice.Disconnect(); //disconnect ThermoCam
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


