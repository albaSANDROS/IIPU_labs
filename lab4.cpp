#include <Windows.h>
#include <setupapi.h>
#include <iostream>
#include <wdmguid.h>
#include <devguid.h>
#include <ctime>
#include <sstream>

#include <opencv2/opencv.hpp>
#include <opencv2/videoio.hpp>

using namespace std;
using namespace cv;

#define FRAME_RATE 10
#define FRAME_TIME 1000 / FRAME_RATE
#define HIDDEN_MODE_TIME 10
static string SAVE_PATH = "frames/";

#pragma comment(lib, "setupapi.lib")

string makeName()
{
	time_t now = time(0);
	tm* ltm = new tm;
	localtime_s(ltm, &now);
	stringstream sstream;
	sstream << ltm->tm_year + 1900 << '_'
		<< ltm->tm_mon + 1 << '_'
		<< ltm->tm_mday << '_'
		<< ltm->tm_hour << '_'
		<< ltm->tm_min << '_'
		<< ltm->tm_sec;
	return sstream.str();

}

void printCameraInfo()
{
	SP_DEVINFO_DATA DeviceInfoData = { 0 };
	HDEVINFO DeviceInfoSet = SetupDiGetClassDevsA(&GUID_DEVCLASS_IMAGE, "USB", NULL, DIGCF_PRESENT);
	if (DeviceInfoSet == INVALID_HANDLE_VALUE) {
		exit(1);
	}
	DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
	SetupDiEnumDeviceInfo(DeviceInfoSet, 0, &DeviceInfoData);
	char deviceName[256];
	char deviceMfg[256];
	SetupDiGetDeviceRegistryPropertyA(DeviceInfoSet, &DeviceInfoData, SPDRP_FRIENDLYNAME, NULL, (PBYTE)deviceName, sizeof(deviceName), 0);
	SetupDiGetDeviceRegistryPropertyA(DeviceInfoSet, &DeviceInfoData, SPDRP_MFG, NULL, (PBYTE)deviceMfg, sizeof(deviceMfg), 0);
	SetupDiDestroyDeviceInfoList(DeviceInfoSet);
	cout << "Vendor: " << deviceMfg << '\n'
		<< "Name: " << deviceName << '\n';
}

int main() {
	system("chcp 1251");
	system("cls");
	printCameraInfo();

	VideoCapture webcam;
	if (!webcam.open(0, CAP_DSHOW)) return 1;
	webcam.set(CAP_PROP_FRAME_WIDTH, 1280);
	webcam.set(CAP_PROP_FRAME_HEIGHT, 720);
	Mat frame;

	int choise = 0;
	do
	{
		cout << "Choose action:\n 1 - Take photo\n 2 - Capture video\n 0 - Exit\n";
		cin >> choise;
		switch (choise)
		{
		case 1: {
			webcam >> frame;
			imwrite(SAVE_PATH + "photo_" + makeName() + ".png", frame);
			break;
		}
		case 2:
		{
			long videoLenght;
			cout << "Video lenght in seconds: ";
			cin >> videoLenght;
			VideoWriter videoWriter(SAVE_PATH + "video_" + makeName() + ".mp4", VideoWriter::fourcc('X', 'V', 'I', 'D'), FRAME_RATE, Size(1280, 720), true);
			long long curFrame = 0;
			auto prevClock = clock();
			while (curFrame < FRAME_RATE * videoLenght)
			{
				while (clock() - prevClock < FRAME_TIME);
				prevClock = clock();
				curFrame++;
				webcam >> frame;
				videoWriter.write(frame);
			}
			break;
		}
		case 0:
		{
			
			ShowWindow(GetConsoleWindow(), SW_HIDE);
			long videoLenght;
			videoLenght = 10;
			VideoWriter videoWriter(SAVE_PATH + "video_" + makeName() + ".mp4", VideoWriter::fourcc('X', 'V', 'I', 'D'), FRAME_RATE, Size(1280, 720), true);
			long long curFrame = 0;
			auto prevClock = clock();
			while (curFrame < FRAME_RATE * videoLenght)
			{
				while (clock() - prevClock < FRAME_TIME);
				prevClock = clock();
				curFrame++;
				webcam >> frame;
				videoWriter.write(frame);
			}
			break;
		}
		default: break;
		}
	} while (choise == 1 || choise == 2);
	webcam.release();
	return 0;
}
