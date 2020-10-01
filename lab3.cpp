#pragma comment (lib, "setupapi.lib")
#include <iostream>
#include <Windows.h>
#include <map>
#include <string>
#include <iomanip>
#include <Setupapi.h>
#include <devguid.h>
#include <Poclass.h>
#include <sstream>


using namespace std;

const map<int, string> ACLineStatuses
{
	{0 ,"Offline"},
	{1 ,"Online"},
	{255 ,"Unknown status"}
};

const map<int, string> BatteryChargeStatus
{
	{0 ,"Battery capacity is between low and high"},
	{1 ,"High—the battery capacity is at more than 66 percent"},
	{2 ,"Low—the battery capacity is at less than 33 percent"},
	{4 ,"Critical—the battery capacity is at less than five percent"},
	{8 ,"Charging"},
	{128 ,"No system battery"},
	{255 ,"Unknown status—unable to read the battery flag information"}
};

string getBatteryInfo()
{

	HDEVINFO batteryHandleSet;
	if ((batteryHandleSet = SetupDiGetClassDevs(&GUID_DEVCLASS_BATTERY, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE)) == INVALID_HANDLE_VALUE) return "Error getting info\n";
	SP_DEVICE_INTERFACE_DATA batteryDeviceInterfaceData = { 0 };
	batteryDeviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
	SetupDiEnumDeviceInterfaces(batteryHandleSet, 0, &GUID_DEVCLASS_BATTERY, 0, &batteryDeviceInterfaceData);
	DWORD bufferSize = 0;
	SetupDiGetDeviceInterfaceDetail(batteryHandleSet, &batteryDeviceInterfaceData, 0, 0, &bufferSize, 0);
	PSP_DEVICE_INTERFACE_DETAIL_DATA batteryDeviceInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)new BYTE[bufferSize];

	batteryDeviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
	SetupDiGetDeviceInterfaceDetail(batteryHandleSet, &batteryDeviceInterfaceData, batteryDeviceInterfaceDetailData, bufferSize, &bufferSize, 0);
	HANDLE batteryHandle = CreateFile(batteryDeviceInterfaceDetailData->DevicePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	DWORD bytesReturned = 0;
	DWORD batteryTag = 0;
	DWORD waitTime = 1000;
	DeviceIoControl(batteryHandle, IOCTL_BATTERY_QUERY_TAG, &waitTime, sizeof(DWORD), &batteryTag, sizeof(DWORD), &bytesReturned, NULL);


	BATTERY_INFORMATION batteryInfo = {};
	BATTERY_QUERY_INFORMATION batteryQueryInfo = { 0 };
	batteryQueryInfo.BatteryTag = batteryTag;
	DeviceIoControl(batteryHandle, IOCTL_BATTERY_QUERY_INFORMATION, &batteryQueryInfo, sizeof(BATTERY_QUERY_INFORMATION), &batteryInfo, sizeof(BATTERY_INFORMATION), &bytesReturned, NULL);


	BYTE buffer[BUFSIZ];
	batteryQueryInfo.InformationLevel = BatteryManufactureName;
	ZeroMemory(buffer, BUFSIZ);
	DeviceIoControl(batteryHandle, IOCTL_BATTERY_QUERY_INFORMATION, &batteryQueryInfo, sizeof(BATTERY_QUERY_INFORMATION), buffer, BUFSIZ, &bytesReturned, NULL);
	wstring manufactureName((wchar_t*)buffer);

	batteryQueryInfo.InformationLevel = BatteryDeviceName;
	ZeroMemory(buffer, BUFSIZ);
	DeviceIoControl(batteryHandle, IOCTL_BATTERY_QUERY_INFORMATION, &batteryQueryInfo, sizeof(BATTERY_QUERY_INFORMATION), buffer, BUFSIZ, &bytesReturned, NULL);
	wstring batteryName((wchar_t*)buffer);

	string batteryChemistry = (char*)batteryInfo.Chemistry;
	batteryChemistry = batteryChemistry.substr(0, 4);

	wstring model = L"Model: " + manufactureName + batteryName;
	string modelSTR(model.begin(), model.end());
	stringstream ss;

	ss << modelSTR << '\n'
		<< "Battery type: " << batteryChemistry << '\n'
		<< "Cycle count: " << batteryInfo.CycleCount << '\n'
		<< "Designed capacity: " << batteryInfo.DesignedCapacity << '\n'
		<< "Full charge capacity: " << batteryInfo.FullChargedCapacity << "\n\n";

	delete[] batteryDeviceInterfaceDetailData;
	return ss.str();
}

int main()
{
	string staticBatteryInfo = getBatteryInfo();
	while (true)
	{
		cout << staticBatteryInfo;
		SYSTEM_POWER_STATUS powerStatus;
		GetSystemPowerStatus(&powerStatus);
		cout << "AC power status: " << ACLineStatuses.find(powerStatus.ACLineStatus)->second
			<< "\nBattery charge status: " << BatteryChargeStatus.find(powerStatus.BatteryFlag)->second
			<< "\nBattery charge: " << (powerStatus.BatteryLifePercent != 255 ? to_string(powerStatus.BatteryLifePercent) : "Unknown")
			<< "\nBattery saver: " << (powerStatus.SystemStatusFlag ? "On" : "Off")
			<< "\nTotal battery lifetime: " << fixed << setprecision(2) << (powerStatus.BatteryFullLifeTime == MAXDWORD ? "UNAVAILABLE" : to_string((int)powerStatus.BatteryFullLifeTime / 60)) << " minutes"
			<< "\nRemain battery lifetime: " << (powerStatus.BatteryLifeTime == MAXDWORD ? "UNAVAILABLE" : to_string((int)powerStatus.BatteryLifeTime / 60)) << " minutes" << '\n';
		Sleep(250);
		system("cls");
	}
}
