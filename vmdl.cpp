#include <string>
#include <fstream>
#include "decompiler.h"
#include "vmdl.h"

#include <iostream>

using std::ios;

void S2Decompiler::OutputVMDL(const KeyValues& DataBlock, const KeyValues& NTROBlock, std::fstream& f, const std::string& szOutputName)
{
	for (uint32_t i = 0; i < DataBlock.size; ++i)
	{
		std::cout << DataBlock.name[i] << "\n";
	}
}