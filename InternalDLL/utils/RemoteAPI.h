#pragma once
#include "../dependencies/Zydis/Zydis.h"
#include <string>
#include <vector>

namespace REMOTE {
    std::vector<unsigned char> GetBytes(std::uint8_t* start);

    std::vector<ZydisDisassembledInstruction> DisassembleFn(std::uint8_t* start);
    /*
    arg 0: const char* modName
    arg 1: std::vector<ZydisDisassembledInstruction>* result
    */
    void DisassembleModule(void** threadInfo);

    /*
    arg 0: std::uint8_t* start
    arg 1: std::string* result
    */
    void DecompileFn(void** threadInfo);

    std::string ResolveFunctionName(std::uint8_t* ptr);
}