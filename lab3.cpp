#pragma comment(lib, "PowrProf.lib")
#include <Windows.h>
#include <WinBase.h>
#include <iostream>
#include <powrprof.h>
#include <thread>   
#include "conio.h"
#include <Poclass.h>
#include <Setupapi.h>
#include<devguid.h>
#pragma comment (lib, "setupapi.lib")
#include <iostream>
#include <conio.h>
#include <Windows.h>
#include <Setupapi.h>
#include <Poclass.h>
#include <batclass.h>
#include<devguid.h>
using namespace std;

int a;
bool BatteryHim()
{
	HDEVINFO DeviceInfoSet;

	if ((DeviceInfoSet = SetupDiGetClassDevs(&GUID_DEVCLASS_BATTERY, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE)) == INVALID_HANDLE_VALUE)
	{
		cout << "Error: " << GetLastError() << endl;
		exit(1);
	}

	SP_DEVICE_INTERFACE_DATA DeviceInterfaceData = { 0 };
	ZeroMemory(&DeviceInterfaceData, sizeof(SP_DEVINFO_DATA));
	DeviceInterfaceData.cbSize = sizeof(SP_DEVINFO_DATA);

	if (SetupDiEnumDeviceInterfaces(DeviceInfoSet, NULL, &GUID_DEVCLASS_BATTERY, 0, &DeviceInterfaceData))
	{
		DWORD cbRequired = 0;

		SetupDiGetDeviceInterfaceDetail(DeviceInfoSet, &DeviceInterfaceData, NULL, NULL, &cbRequired, NULL);
		if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
		{
			PSP_DEVICE_INTERFACE_DETAIL_DATA pdidd = (PSP_DEVICE_INTERFACE_DETAIL_DATA)LocalAlloc(LPTR, cbRequired);
			if (pdidd)
			{
				pdidd->cbSize = sizeof(*pdidd);
				if (SetupDiGetDeviceInterfaceDetail(DeviceInfoSet, &DeviceInterfaceData, pdidd, cbRequired, &cbRequired, NULL))
				{
					HANDLE hBattery = CreateFile(pdidd->DevicePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
					if (INVALID_HANDLE_VALUE != hBattery)
					{
						BATTERY_QUERY_INFORMATION BatteryQueryInformation = { 0 };

						DWORD bytesWait = 0;
						DWORD bytesReturned = 0;

						if (DeviceIoControl(hBattery, IOCTL_BATTERY_QUERY_TAG, &bytesWait, sizeof(bytesWait), &BatteryQueryInformation.BatteryTag,
							sizeof(BatteryQueryInformation.BatteryTag), &bytesReturned, NULL) && BatteryQueryInformation.BatteryTag)
						{
							BATTERY_INFORMATION BatteryInfo = { 0 };
							BatteryQueryInformation.InformationLevel = BatteryInformation;

							if (DeviceIoControl(hBattery, IOCTL_BATTERY_QUERY_INFORMATION, &BatteryQueryInformation, sizeof(BatteryQueryInformation),
								&BatteryInfo, sizeof(BatteryInfo), &bytesReturned, NULL))
							{
								cout << "Type: " << BatteryInfo.Chemistry << endl;
							}
						}
						else
						{
							cout << "Error: " << GetLastError() << endl;
							CloseHandle(hBattery);
							return false;
						}
						CloseHandle(hBattery);
					}
					else
					{
						cout << "Error: " << GetLastError() << endl;
						return false;
					}
				}
				LocalFree(pdidd);
			}
		}
	}

	SetupDiDestroyDeviceInfoList(DeviceInfoSet);
	return true;
}

void getinfo() {

	SYSTEM_POWER_STATUS powerStatus;

	char ac[2][8] = { "Offline", "Online" };
	char saver[2][8] = { "is off", "on" };

	while (a != 2) {

		system("cls");

		cout << "Press 0 to Sleep" << endl;
		cout << "Press 1 to Hibernate" << endl;
		cout << "Press 2 to Exit" << endl << endl;

		GetSystemPowerStatus(&powerStatus);

	
		cout << "Battery percentage: " << (int)powerStatus.BatteryLifePercent << endl;
		cout << "Battery saver " << saver[powerStatus.SystemStatusFlag] << endl;
		cout << "AC line status: " << ac[powerStatus.ACLineStatus] << endl;
		
		BatteryHim();

		Sleep(1000);
	}
}


int main() {

	thread log(getinfo);
	setlocale(LC_ALL, "RU");
	while (a != 2) {

		if (a = _getch()) {

			a -= '0';

			switch (a) {

			case(0):
				SetSuspendState(FALSE, FALSE, FALSE);
				break;

			case(1):
				SetSuspendState(TRUE, FALSE, FALSE);
				break;

			}
		}
	}

	log.join();

	return 0;
}
