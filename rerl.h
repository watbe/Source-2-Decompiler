#ifndef INC_S2DC_RERL_BLOCK_H
#define INC_S2DC_RERL_BLOCK_H

/*RERL Block (8 bytes)
     4 bytes:  Offset to resource entry list (always 0x00000008, the size of the RERL block)
       -> 8 bytes:  Resource ID
       -> 8 bytes:  Offset to resource name
     4 bytes:  Number of resource entries
*/

#include <fstream>
#include "keyvalues.h"

void ProcessRERLBlock(std::fstream& f, KeyValues& sRERLInfo);

#endif