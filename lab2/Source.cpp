#include <iostream>
#include <string>
#include <Windows.h>
#include <Setupapi.h>
#include <winioctl.h>
#include <ntddscsi.h>
#include <vector>
#include <nvme.h>
#include <ntddstor.h>
#include "Header.h"
#pragma comment (lib, "Setupapi.lib")

using namespace std;

void printDiskSize(HANDLE handle, int number)
{
	double totalMemory = 0
		, unusedMemory = 0
		, usedMemory = 0;
	DISK_GEOMETRY_EX diskGeometry;
	DeviceIoControl(handle, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, NULL, 0, &diskGeometry, sizeof(diskGeometry), NULL, NULL); 
	totalMemory = (double)diskGeometry.DiskSize.QuadPart / (double)(1024 * 1024 * 1024);

	DWORD logicalDrivesMask = GetLogicalDrives(); 
	for (int i = 0; i < 26; i++)     
	{
		if ((logicalDrivesMask >> i) & 1)    
		{
			char* localBuf = new char[BUFSIZ];
			char symbl = char(65 + i);
			string pathHandle = "\\\\.\\", pathName = ":\\";
			HANDLE logicalDriveHandle = CreateFileA((pathHandle + symbl + ":").c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);  
			DeviceIoControl(logicalDriveHandle, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, sizeof(STORAGE_DEVICE_NUMBER), localBuf, BUFSIZ, NULL, (LPOVERLAPPED)NULL);   
			STORAGE_DEVICE_NUMBER* deviceNumber = (STORAGE_DEVICE_NUMBER*)localBuf; 
			if (number == deviceNumber->DeviceNumber)  
			{
				ULARGE_INTEGER freeSpace;
				GetDiskFreeSpaceExA((symbl + pathName).c_str(), 0, 0, &freeSpace); 
				unusedMemory += (double)freeSpace.QuadPart;
			}
			CloseHandle(logicalDriveHandle);
		}
	}
	unusedMemory /= (double)1024 * 1024 * 1024;  
	usedMemory = totalMemory - unusedMemory; 
	std::cout << "\nMemory:\n - Total - " << totalMemory << " Gb\n"
		<< " - Used - " << usedMemory << " Gb\n"
		<< " - Free - " << unusedMemory << " Gb\n";
	
}

void printAtaSpecs(HANDLE diskHandle) 
{
	DWORD vBufferSize = sizeof(ATA_PASS_THROUGH_EX) + sizeof(IDENTIFY_DEVICE_DATA);
	BYTE* vBuffer = new BYTE[vBufferSize];
	PATA_PASS_THROUGH_EX pATARequest(reinterpret_cast<PATA_PASS_THROUGH_EX>(vBuffer));
	pATARequest->AtaFlags = ATA_FLAGS_DATA_IN | ATA_FLAGS_DRDY_REQUIRED;
	pATARequest->Length = sizeof(ATA_PASS_THROUGH_EX);
	pATARequest->DataBufferOffset = sizeof(ATA_PASS_THROUGH_EX);
	pATARequest->DataTransferLength = sizeof(IDENTIFY_DEVICE_DATA);
	pATARequest->TimeOutValue = 2;
	pATARequest->CurrentTaskFile[6] = ID_CMD;
	PIDENTIFY_DEVICE_DATA pIdentityBlob = 0;
	ULONG ulBytesRead;
	if (DeviceIoControl(diskHandle, IOCTL_ATA_PASS_THROUGH,
		&vBuffer[0], vBufferSize,
		&vBuffer[0], vBufferSize,
		&ulBytesRead, NULL) == FALSE)
	{
		std::cout << "DeviceIoControl(IOCTL_ATA_PASS_THROUGH) failed.  LastError: " << GetLastError() << std::endl;
	}
	else
	{
		
		pIdentityBlob = reinterpret_cast<PIDENTIFY_DEVICE_DATA>(&vBuffer[sizeof(ATA_PASS_THROUGH_EX)]);
	}

	BYTE multiwordDma = *((BYTE*)pIdentityBlob + 126);
	BYTE pio = *((BYTE*)pIdentityBlob + 128);
	BYTE sata = *((BYTE*)pIdentityBlob + 160);
	USHORT commandSet = *((BYTE*)pIdentityBlob + 164);
	BYTE ultraDma = *((BYTE*)pIdentityBlob + 176);

	std::cout << "Modes: \n";
	if (multiwordDma & 7)
	{
		std::cout << " - MWDMA.  Supported versions: ";
		for (int i = 0; i < 4; i++)
			if (multiwordDma & (BYTE)pow(2, i))
				std::cout << 'v' << i << ' ';
		std::cout << '\n';
	}
	if (pio & 3)
	{
		std::cout << " - PIO.  Supported versions: ";
		for (int i = 0; i < 2; i++)
			if (pio & (BYTE)pow(2, i))
				std::cout << 'v' << i + 3 << ' ';
		std::cout << '\n';
	}
	if (sata & 255)
	{
		std::cout << " - ATA.  Supported versions: ";
		for (int i = 1; i < 8; i++)
			if (sata & (BYTE)pow(2, i))
				std::cout << 'v' << i << ' ';
		std::cout << '\n';
	}
	if (ultraDma & 255)
	{
		std::cout << " - UDMA.  Supported versions: ";
		for (int i = 0; i < 7; i++)
			if (ultraDma & (BYTE)pow(2, i))
				std::cout << 'v' << i << ' ';
		std::cout << '\n';
	}
	if (commandSet & 0b11111111111)
	{
		std::cout << "Supported commands:\n";
		for (int i = 0; i < 11; i++)
			if (commandSet & (BYTE)pow(2, i))
				std::cout << " - " << commands[i] << '\n';
		std::cout << '\n';
	}
}



int main()
{
	for (int diskNumber = 0;; diskNumber++)
	{
		string diskName = string("\\\\.\\PhysicalDrive") + to_string(diskNumber);              

		HANDLE diskHandle = CreateFileA(diskName.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
		if (diskHandle == INVALID_HANDLE_VALUE)
			break;

		DWORD bytesReturned;

		STORAGE_PROPERTY_QUERY storagePropertyQuery;            
		storagePropertyQuery.PropertyId = StorageDeviceProperty;         
		storagePropertyQuery.QueryType = PropertyStandardQuery;
		STORAGE_DESCRIPTOR_HEADER  diskDescHeader;          
		DeviceIoControl(diskHandle, IOCTL_STORAGE_QUERY_PROPERTY, &storagePropertyQuery, sizeof(STORAGE_PROPERTY_QUERY), &diskDescHeader, sizeof(STORAGE_DESCRIPTOR_HEADER), &bytesReturned, NULL);  
		BYTE* diskDescBuffer = new BYTE[diskDescHeader.Size];
		DeviceIoControl(diskHandle, IOCTL_STORAGE_QUERY_PROPERTY, &storagePropertyQuery, sizeof(STORAGE_PROPERTY_QUERY), diskDescBuffer, diskDescHeader.Size, &bytesReturned, NULL);
		STORAGE_DEVICE_DESCRIPTOR* diskDesc = (STORAGE_DEVICE_DESCRIPTOR*)diskDescBuffer; 



		string vendor = (char*)diskDesc + diskDesc->VendorIdOffset;
		if (vendor.size() <= 1)
			vendor = "Unknown";
		std::cout << "Number: " << diskNumber << '\n'
			<< "Vendor: " << vendor << '\n'                                                      
			<< "Model: " << (char*)diskDesc + diskDesc->ProductIdOffset << '\n'                  
			<< "Serial: " << (char*)diskDesc + diskDesc->SerialNumberOffset << '\n'              
			<< "Firmware version: " << (char*)diskDesc + diskDesc->ProductRevisionOffset << '\n' 
			<< "Interface type: " << busTypes[diskDesc->BusType] << "\n";
		printDiskSize(diskHandle, diskNumber);  
		std::cout << '\n';
		if (diskDesc->BusType == 3 || diskDesc->BusType == 11)
			printAtaSpecs(diskHandle);

		std::cout << "\n\n";
		CloseHandle(diskHandle);
	}
}
