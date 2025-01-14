#include <stdint.h>
#include <stack>
#include <string>
#include <fstream>
#include "keyvalues.h"
#include "ntro.h"
#include "rerl.h"

using std::ios;

KeyValues* pLastNTROInfo = NULL;

void* SearchNTROInfo(uint32_t nID, void*(*pFunction)(char*, char*))
{
	if (!pLastNTROInfo)
		throw std::string("No NTRO information was found. (Did you forget to process the NTRO block first?)");

	for (uint32_t i = 0; i < pLastNTROInfo->size; ++i)
	{
		if ((nID == *(uint32_t*)&pLastNTROInfo->name[i][strlen(pLastNTROInfo->name[i]) + 1]) || (nID == (uint32_t)pLastNTROInfo->data[i]))
		{
			return pFunction(pLastNTROInfo->name[i], pLastNTROInfo->data[i]);
		}
	}
	return 0;
}

void* GetData(char* szName, char* szData)
{
	return szData;
}

void* GetName(char* szName, char* szData)
{
	return szName;
}

void* GetBaseStructID(char* szName, char* szData)
{
	return (void*)*(uint32_t*)&szName[strlen(szName) + 5];
}

KeyValues* FindNTROResourceData(uint32_t nID)
{
	return (KeyValues*)SearchNTROInfo(nID, GetData);
}

const char* FindNTROResourceName(uint32_t nID)
{
	return (const char*)SearchNTROInfo(nID, GetName);
}

uint16_t FindNTROResourceSize(uint32_t nID)
{
	if (!pLastNTROInfo)
		throw std::string("No NTRO information was found. (Did you forget to process the NTRO block first?)");

	KeyValues* pStructSizeTable = (KeyValues*)pLastNTROInfo->data[pLastNTROInfo->size - 1];
	for (uint32_t i = 0; i < pStructSizeTable->size; ++i)
	{
		if (nID == *(uint32_t*)pStructSizeTable->name[i])
		{
			return *(uint16_t*)pStructSizeTable->data[i];
		}
	}
	return 0;
}

uint32_t FindNTROBaseStructID(uint32_t nID)
{
	return (uint32_t)SearchNTROInfo(nID, GetBaseStructID);
}

void ProcessNTROBlock(std::fstream& f, KeyValues& NTROInfo)
{
	char szBuffer[4];

	uint32_t i, j, k;
	uint32_t nStructSize, nEnumSize;

	std::streamoff p1, p2, p3, p4;

	f.read(szBuffer, 4);
	p1 = f.tellg();
	f.seekg(*(int32_t*)szBuffer + 4, ios::cur);
	f.read((char*)&nStructSize, 4);
	p4 = f.tellg();
	f.seekg(4, ios::cur);
	f.read((char*)&nEnumSize, 4);

	f.seekg(-16, ios::cur);
	f.read(szBuffer, 4);
	f.seekg(*(int32_t*)szBuffer - 4, ios::cur);

	NTROInfo = KeyValues(nStructSize + nEnumSize + 1);

	KeyValues* pStructSizeTable = new KeyValues(nStructSize);
	NTROInfo.name[nStructSize + nEnumSize] = new char[1];
	NTROInfo.name[nStructSize + nEnumSize][0] = '\0';
	NTROInfo.data[nStructSize + nEnumSize] = (char*)pStructSizeTable;
	NTROInfo.type[nStructSize + nEnumSize] = KV_DATA_TYPE_STRUCT;
	for (i = 0; i < nStructSize; ++i)
	{
		uint16_t nDiskSize;
		uint16_t nDiskOffset = 0;
		uint32_t nResourceID;
		uint32_t nBaseStructID;

		f.seekg(4, ios::cur);
		f.read((char*)&nResourceID, 4);
		f.read(szBuffer, 4);
		p2 = f.tellg();
		f.seekg(*(int32_t*)szBuffer - 4, ios::cur);
		for (j = 0; f.get() != '\0'; ++j);
		f.seekg(-(int32_t)(j + 1), ios::cur);
		NTROInfo.name[i] = new char[j + 9];
		f.read(NTROInfo.name[i], j);
		NTROInfo.name[i][j] = '\0';
		memcpy(&NTROInfo.name[i][j + 1], &nResourceID, 4);
		f.seekg(p2 + 8);
		f.read((char*)&nDiskSize, 2);
		f.seekg(2, ios::cur);
		f.read((char*)&nBaseStructID, 4);
		memcpy(&NTROInfo.name[i][j + 5], &nBaseStructID, 4);
		f.seekg(4, ios::cur);
		f.read(szBuffer, 4);

		KeyValues* pResourceDiskStruct = new KeyValues(*(uint32_t*)szBuffer);

		NTROInfo.data[i] = (char*)pResourceDiskStruct;
		NTROInfo.type[i] = KV_DATA_TYPE_STRUCT;

		pStructSizeTable->name[i] = new char[4];
		*(uint32_t*)pStructSizeTable->name[i] = *(uint32_t*)&nResourceID;
		pStructSizeTable->data[i] = new char[2];
		*(uint16_t*)pStructSizeTable->data[i] = *(uint16_t*)&nDiskSize;

		p2 = f.tellg();
		f.seekg(-8, ios::cur);
		f.read(szBuffer, 4);
		f.seekg(*(int32_t*)szBuffer - 4, ios::cur);

		//ResourceDiskStruct data:
		//  2 bytes: disk offset
		//  2 bytes: size of the struct
		//  2 bytes: struct type
		//  4 bytes: struct ID
		//  4 bytes: indirection level
		//  (up to) 10 bytes: indirection bytes (3 = pointer, 4 = array)
		for (j = 0; j < pResourceDiskStruct->size; ++j)
		{
			ReadOffsetString(f, pResourceDiskStruct->name[j]);
			f.seekg(2, ios::cur);
			f.read(szBuffer, 2);
			pResourceDiskStruct->data[j] = new char[24];
			*(uint16_t*)&pResourceDiskStruct->data[j][0] = *(uint16_t*)szBuffer;
			if (j != 0)
				*(uint16_t*)&pResourceDiskStruct->data[j - 1][2] = *(uint16_t*)szBuffer - nDiskOffset;
			nDiskOffset = *(uint16_t*)szBuffer;
			f.seekg(4, ios::cur);
			f.read(szBuffer, 4);
			*(uint32_t*)&pResourceDiskStruct->data[j][10] = *(uint32_t*)szBuffer;
			f.seekg(-8, ios::cur);
			f.read(szBuffer, 4);
			p3 = f.tellg();
			f.seekg(*(int32_t*)szBuffer - 4, ios::cur);
			memset(&pResourceDiskStruct->data[j][14], 0, 6);
			for (k = 0; k < 10 && k < *(uint32_t*)&pResourceDiskStruct->data[j][10]; ++k)
				pResourceDiskStruct->data[j][14 + k] = (uint8_t)f.get();
			f.seekg(p3 + 4);
			f.read(szBuffer, 4);
			*(uint32_t*)&pResourceDiskStruct->data[j][6] = *(uint32_t*)szBuffer;
			f.read(szBuffer, 4);
			*(uint16_t*)&pResourceDiskStruct->data[j][4] = *(uint16_t*)szBuffer;
		}

		if (j != 0)
			*(uint16_t*)&pResourceDiskStruct->data[j - 1][2] = nDiskSize - nDiskOffset;

		f.seekg(p2 + 4);
	}

	f.seekg(p4);
	f.read(szBuffer, 4);
	f.seekg(*(int32_t*)szBuffer - 4, ios::cur);
	for (i = nStructSize; i < nStructSize + nEnumSize; ++i)
	{
		uint32_t nResourceID;

		f.seekg(4, ios::cur);
		f.read((char*)&nResourceID, 4);
		f.read(szBuffer, 4);
		p2 = f.tellg();
		f.seekg(*(int32_t*)szBuffer - 4, ios::cur);
		for (j = 0; f.get() != '\0'; ++j);
		f.seekg(-(int32_t)(j + 1), ios::cur);
		NTROInfo.name[i] = new char[j + 5];
		f.read(NTROInfo.name[i], j);
		NTROInfo.name[i][j] = '\0';
		memcpy(&NTROInfo.name[i][j + 1], &nResourceID, 4);
		f.seekg(p2 + 12);
		f.read(szBuffer, 4);

		KeyValues* pResourceDiskEnum = new KeyValues(*(uint32_t*)szBuffer + 1);
		pResourceDiskEnum->size--;

		NTROInfo.data[i] = (char*)pResourceDiskEnum;
		NTROInfo.type[i] = KV_DATA_TYPE_STRUCT;

		f.seekg(-8, ios::cur);
		f.read(szBuffer, 4);
		p2 = f.tellg();
		f.seekg(*(int32_t*)szBuffer - 4, ios::cur);
		for (j = 0; j < pResourceDiskEnum->size; ++j)
		{
			ReadOffsetString(f, pResourceDiskEnum->name[j]);
			pResourceDiskEnum->data[j] = new char[4];
			f.read(pResourceDiskEnum->data[j], 4);
		}
		f.seekg(p2 + 4);
	}

	f.seekg(p1 + 4);

	pLastNTROInfo = &NTROInfo;
}

void ClearLastNTROEntry()
{
	pLastNTROInfo = NULL;
}

KeyValues* ReadIndirectionData(std::fstream& f, char* szArgs, uint32_t nDepth)
{
	char szBuffer[4];

	uint32_t nOffset = 0;
	uint16_t nDataType = *(uint16_t*)&szArgs[4];
	uint32_t nStructID = *(uint32_t*)&szArgs[6];
	uint32_t nIndirectionLevel = *(uint32_t*)&szArgs[10] - nDepth;
	uint8_t* pIndirectionBytes = (uint8_t*)&szArgs[14 + nDepth];

	if (nIndirectionLevel == 0)
		throw std::string("Tried to read indirection data from data without indirection.");

	//Note: Indirection byte value 0x05 doesn't actually exist in real NTRO data as far as I know...
	//      In this implementation, indirection byte value 0x05 indicates a polymorphic pointer;
	//      Specifically, it expects a type value and an offset value, both 4 bytes, such that the 
	//      type value is added to the struct ID of the current struct to obtain the new child
	//      struct ID. The offset is just a normal offset, as per a 0x03 indirection.
	//      
	//      It's a very messy implementation overall and prone to ID collisions (unlikely but still),
	//      but without it we would have no way of handling polymorphic data (looking at you VTEX...)
	if ((pIndirectionBytes[0] == 0x03) || (pIndirectionBytes[0] == 0x05))
	{
		uint32_t nOriginalStructID = nStructID;
		if (pIndirectionBytes[0] == 0x05)
		{
			f.read(szBuffer, 4);
			KeyValues* pBaseStruct = FindNTROResourceData(*(uint32_t*)szBuffer + nStructID);
			if (pBaseStruct)
			{
				nOffset = FindNTROResourceSize(nStructID) - 8;
				nStructID = *(uint32_t*)&szArgs[6] = *(uint32_t*)szBuffer + nStructID;
			}
		}

		f.read(szBuffer, 4);
		std::streamoff p = f.tellg();
		f.seekg(*(int32_t*)szBuffer - 4, ios::cur);
		KeyValues* pSubStruct = new KeyValues(1);
		f.read(szBuffer, 4);
		const char* szStructName = FindNTROResourceName(*(uint32_t*)szBuffer);
		if ((!szStructName) && (nDataType == KV_DATA_TYPE_STRUCT))
			szStructName = FindNTROResourceName(nStructID);
		if (szStructName)
		{
			pSubStruct->name[0] = new char[strlen(szStructName) + 1];
			memcpy(pSubStruct->name[0], szStructName, strlen(szStructName));
			pSubStruct->name[0][strlen(szStructName)] = '\0';
		}
		f.seekg(-4, ios::cur);

		char* szNewArgs = new char[24];
		memcpy(szNewArgs, szArgs, 24);
		*(uint16_t*)&szNewArgs[2] = KeyValues::_snDataSize[nDataType];
		ReadDataField(f, pSubStruct->data[0], szNewArgs, nDepth + 1);
		pSubStruct->type[0] = (uint8_t)nDataType;
		delete[] szNewArgs;

		*(uint32_t*)&szArgs[6] = nOriginalStructID;
		f.seekg(p + nOffset);
		return pSubStruct;
	}
	else if (pIndirectionBytes[0] == 0x04)
	{
		f.seekg(4, ios::cur);
		f.read(szBuffer, 4);
		KeyValues* pSubStruct = new KeyValues(*(uint32_t*)szBuffer);
		f.seekg(-8, ios::cur);
		f.read(szBuffer, 4);
		std::streamoff p = f.tellg();
		f.seekg(*(int32_t*)szBuffer - 4, ios::cur);
		char* szNewArgs = new char[24];
		memcpy(szNewArgs, szArgs, 24);
		*(uint16_t*)&szNewArgs[2] = KeyValues::_snDataSize[nDataType];
		for (uint32_t i = 0; i < pSubStruct->size; ++i)
		{
			f.read(szBuffer, 4);
			const char* szStructName = FindNTROResourceName(*(uint32_t*)szBuffer);
			if ((!szStructName) && (nDataType == KV_DATA_TYPE_STRUCT))
				szStructName = FindNTROResourceName(nStructID);
			if (szStructName)
			{
				pSubStruct->name[i] = new char[strlen(szStructName) + 1];
				memcpy(pSubStruct->name[i], szStructName, strlen(szStructName));
				pSubStruct->name[i][strlen(szStructName)] = '\0';
			}
			f.seekg(-4, ios::cur);

			ReadDataField(f, pSubStruct->data[i], szNewArgs, nDepth + 1);
			pSubStruct->type[i] = (uint8_t)nDataType;
		}
		delete[] szNewArgs;
		f.seekg(p + 4);
		return pSubStruct;
	}
	else
	{
		throw std::string("Unknown indirection value \"" + std::to_string(pIndirectionBytes[0]) + "\".");
	}

}

void ReadDataField(std::fstream& f, char*& szDestination, char* szArgs, uint32_t nDepth)
{
	uint16_t nDataSize = *(uint16_t*)&szArgs[2];
	uint16_t nDataType = *(uint16_t*)&szArgs[4];
	uint32_t nStructID = *(uint32_t*)&szArgs[6];
	uint32_t nIndirectionLevel = *(uint32_t*)&szArgs[10] - nDepth;

	if (nIndirectionLevel > 0)
	{
		szDestination = (char*)ReadIndirectionData(f, szArgs, nDepth);
		return;
	}

	if ((nDataType == KV_DATA_TYPE_STRING) || (nDataType == KV_DATA_TYPE_NAME))
	{
		std::streamoff p = f.tellg();
		ReadOffsetString(f, szDestination);
	}
	else if (nDataType == KV_DATA_TYPE_STRUCT)
	{
		KeyValues* pSubStruct = new KeyValues;
		KeyValues* pResourceStruct = FindNTROResourceData(nStructID);
		ReadStructuredData(f, *pSubStruct, pResourceStruct);
		szDestination = (char*)pSubStruct;
	}
	else if (nDataType == KV_DATA_TYPE_HANDLE)
	{
		char szBuffer2[8];

		f.read(szBuffer2, 8);
		const char* szResourceName = GetExternalResourceName(szBuffer2);
		if (!szResourceName)
			szResourceName = "";
		uint32_t nNameLength = strlen(szResourceName);
		szDestination = new char[nNameLength + 1];
		memcpy(szDestination, szResourceName, nNameLength);
		szDestination[nNameLength] = '\0';
	}
	else
	{
		szDestination = new char[nDataSize];
		f.read(szDestination, nDataSize);
	}
}

void ReadStructuredData(std::fstream& f, KeyValues& Destination, KeyValues* pSourceStruct)
{
	char szBuffer[4];

	uint32_t nIndex = 0;
	uint32_t nTotalOffset = 0;

	f.read(szBuffer, 4);
	KeyValues* pHeaderResource = FindNTROResourceData(*(uint32_t*)szBuffer);
	if (!pHeaderResource)
		f.seekg(-4, ios::cur);
	else
	{
		pSourceStruct = pHeaderResource;
		nTotalOffset += 4;
	}

	if (!pSourceStruct)
		throw std::string("No matching NTRO structure could be found.");

	//KeyValues aren't dynamically sized, so we need to know how much to allocate ahead of time
	//This includes the struct and all base structs that it encompasses
	uint32_t nTotalStructSize = 0;
	std::stack<KeyValues*> pStructStack;
	pStructStack.push(pSourceStruct);
	while (pStructStack.top())
	{
		nTotalStructSize += pStructStack.top()->size;
		pStructStack.push(FindNTROResourceData(FindNTROBaseStructID((uint32_t)pStructStack.top())));
	}
	pStructStack.pop();    //Get rid of the terminating null value

	Destination = KeyValues(nTotalStructSize);

	while (!pStructStack.empty())
	{
		pSourceStruct = pStructStack.top();
		for (uint32_t i = 0; i < pSourceStruct->size; ++i)
		{
			uint32_t nNameLength = strlen(pSourceStruct->name[i]);
			uint16_t nDiskOffset = *(uint16_t*)&pSourceStruct->data[i][0];
			uint16_t nDataSize = *(uint16_t*)&pSourceStruct->data[i][2];
			uint16_t nDataType = *(uint16_t*)&pSourceStruct->data[i][4];

			Destination.name[nIndex] = new char[nNameLength + 3];
			memcpy(Destination.name[nIndex], pSourceStruct->name[i], nNameLength);
			Destination.name[nIndex][nNameLength] = '\0';
			memcpy(&Destination.name[nIndex][nNameLength + 1], &nDataType, 2);

			if (nTotalOffset != nDiskOffset)
			{
				f.seekg((int32_t)nDiskOffset - (int32_t)nTotalOffset, ios::cur);
				nTotalOffset = nDiskOffset;
			}

			std::streamoff p = f.tellg();
			ReadDataField(f, Destination.data[nIndex], pSourceStruct->data[i], 0);
			Destination.type[nIndex] = (uint8_t)nDataType;
			f.seekg(p + nDataSize);
			nTotalOffset += nDataSize;
			nIndex++;
		}
		pStructStack.pop();
	}
}