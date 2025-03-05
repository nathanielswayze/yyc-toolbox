#include "memory.h"
#ifdef _WIN64
#include "pe64.h"
#else
#include <winternl.h>
#endif
#include <vector>
#include <winnt.h>
#include <TlHelp32.h>
#include "../utils/crt.h"
#include <locale>
#include <iostream>
#include <string>
#include <sstream>
#include <filesystem>
#include "../utils/logging.h"

std::wstring widen(const std::string& str)
{
    std::wostringstream wstm{};
    const std::ctype<wchar_t>& ctfacet = std::use_facet<std::ctype<wchar_t>>(wstm.getloc());
    for (size_t i = 0; i < str.size(); ++i)
        wstm << ctfacet.widen(str[i]);
    return wstm.str();
}

std::string narrow(const std::wstring& str)
{
    std::ostringstream stm;
    const std::ctype<wchar_t>& ctfacet = std::use_facet<std::ctype<wchar_t>>(stm.getloc());

    for (size_t i = 0; i < str.size(); ++i)
        stm << ctfacet.narrow(str[i], 0);
    return stm.str();
}

std::unordered_map<std::string, std::uint8_t*> patterns = {
    /* XREF inside CREATE_RVALUE_MUTEX to g_rvalueMutex */
    {"48 83 3D ? ? ? ? ? 75 ? B9 ? ? ? ? E8 ? ? ? ? 48 8D 15", 0},
    /* XREF inside Variable_Builtin_Add to builtin_variables */
    {"4C 8D 35 ? ? ? ? 49 03 DE", 0},
    /* XREF inside Variable_GetValue to Variable_GetValue_Direct */
    {"E9 ? ? ? ? 85 C9 78 ? 41 81 FA", 0},
    /* Variable_BuiltIn_Find */
    {"48 89 5C 24 ? 57 48 83 EC ? 48 8B 3D ? ? ? ? E8 ? ? ? ? 44 8B 4F", 0},
    /* BOOL_RValue */
    {"40 53 48 83 EC ? 44 8B 49 ? 45 32 C0", 0},
    /* REAL_RValue_Ex */
    {"40 53 48 83 EC ? 8B 41 ? 0F 57 C0", 0},
    /* XREF inside F_Method to the_functions */
    {"48 8B 05 ? ? ? ? C7 47", 0},
    /* YYGML_CallLegacyFunction */
    {"40 55 41 54 41 55 41 56 41 57 48 83 EC ? 48 8D 6C 24 ? 48 89 5D ? 48 89 75 ? 48 89 7D ? 48 8B 05 ? ? ? ? 48 33 C5 48 89 45 ? 48 63 45", 0}, // backup zalupa 1337
    /* DeterminePotentialRoot */
    {"48 89 5C 24 ? 57 48 83 EC ? 80 3D ? ? ? ? ? 48 8B FA", 0},
    /* GetContextStackTop */
    {"8B 05 ? ? ? ? 85 C0 7E ? FF C8 48 63 C8 48 8B 05 ? ? ? ? 48 8B 04 C8", 0},
    /* XREF inside F_StringDelete to YYAlloc */
    {"E8 ? ? ? ? 48 89 03 89 6B", 0},
    /* YYRealloc */
    {"48 83 EC ? 48 63 D2", 0},
    /* XREF inside SGamepadMapping::ToString to YYFree */
    {"E8 ? ? ? ? 90 48 8B C3 EB", 0},
    /* XREF inside InputQuery::Input to YYStrDup */
    {"E8 ? ? ? ? 49 89 06 B0", 0},
    /* YYstrnlen */
    {"48 89 5C 24 ? 57 48 83 EC ? 48 63 DA 48 8B F9 4C 8B C3", 0},
    /* XREF inside HTTP_REQ_CONTEXT::SetResponseHeaders to YYCreateString */
    {"E8 ? ? ? ? 2B FB", 0},
    /* XREF inside CreateDsMap to YYSetString */
    {"E8 ? ? ? ? F2 41 0F 10 46", 0},
    /* YYConstString */
    {"48 89 5C 24 ? 57 48 83 EC ? 48 8B F9 48 8B DA B9 ? ? ? ? E8 ? ? ? ? 48 89 44 24", 0},
    /* XREF inside F_AnimcurveGetChannelIndex to YYGetString */
    {"E8 ? ? ? ? 48 63 8E", 0},
    /* Code_Function_Find */
    {"48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? C7 02", 0},
    /* YYError */
    {"48 89 4C 24 ? 48 89 54 24 ? 4C 89 44 24 ? 4C 89 4C 24 ? 53 48 81 EC", 0},
    /* VMError */
    {"48 89 54 24 ? 4C 89 44 24 ? 4C 89 4C 24 ? 53 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 84 24 ? ? ? ? 80 3D ? ? ? ? ? 48 8B DA", 0},
    /* ResourceGetTypeIndex */
    {"48 89 5C 24 ? 57 48 83 EC ? 48 8B DA 48 8B F9 E8 ? ? ? ? 85 C0", 0},
    /* ResourceGetName */
    {"40 53 48 83 EC ? FF C2", 0},
    /* XREF inside Command_ChangeAt to Object_Exists */
    {"E8 ? ? ? ? 84 C0 75 ? 45 33 F6 4C 8D 3D", 0},
    /* Room_Exists */
    {"85 C9 78 ? 48 63 C9", 0},
    /* Script_Exists */
    {"81 F9 ? ? ? ? 8D 81 ? ? ? ? 0F 4C C1 32 D2", 0},
    /* Object_GetInstance */
    {"4C 63 C9 41 83 F9 ? 75", 0},
    /* Code_Variable_Find */
    {"40 53 48 83 EC ? 48 8B D9 E8 ? ? ? ? 85 C0 0F 89", 0},
    /* Code_Variable_Find_Slot_From_Name */
    {"48 83 EC ? 48 8B 0D ? ? ? ? E8 ? ? ? ? 48 85 C0", 0},
    /* XREF inside CreateColPairs to g_ObjectHash */
    {"4C 8B 1D ? ? ? ? E9", 0},
    /* XREF inside Variable_Global_SetVar to g_pGlobal */
    {"4C 8B 05 ? ? ? ? 8B 5E", 0},
    /* XREF inside CCode::CCode to g_pLLVMVars */
    {"48 8B 05 ? ? ? ? 44 8D 45", 0},
    /* Code_Variable_Find_Name */
    {"48 83 EC ? 81 FA ? ? ? ? 0F 8C", 0},
    /* ExecuteIt */
    {"48 89 5C 24 ? 48 89 6C 24 ? 48 89 7C 24 ? 41 56 48 83 EC ? 80 3D", 0},
    /* XREF inside Path_Duplicate to Path_Main */
    {"48 8B 0D ? ? ? ? 48 89 04 D1 B9 ? ? ? ? E8 ? ? ? ? 48 89 44 24 ? 48 85 C0 74 ? 48 8B C8 E8 ? ? ? ? 48 8B D0 EB ? 33 D2 8B 05 ? ? ? ? FF C8 48 63 C8 48 8B 05 ? ? ? ? 48 89 14 C8 8B 05 ? ? ? ? FF C8 4C 63 C0", 0}, // long zalupa
    /* CPath::Draw */
    {"4C 8B DC 49 89 5B ? 57 48 81 EC", 0},
    /* XREF inside CAnimCurve::`vector deleting destructor'(uint) to CAnimCurveManager */
    {"FF 0D ? ? ? ? 48 8B CB", 0},
    /* XREF inside Script_Prepare to Script_Main (void *g_ppGlobalScripts) */
    {"48 89 05 ? ? ? ? 41 8B F7", 0},
};

/*
     * @brief Scan for a given byte pattern on a module
     *
     * @Param module    Base of the module to search
     * @Param signature IDA-style byte array pattern
     *
     * @Returns Address of the first occurence
*/
std::uint8_t* MEM::PatternScan(const char* modName, const char* signature)
{
    std::string search(signature);
    decltype(patterns)::iterator it = patterns.find(search);
    if (it != patterns.end()) {
        std::uint8_t* addy = it->second;
        if (addy)
            return addy;
    }
    
    void* module = GetModuleBaseHandle(modName);
    if (!module)
        return nullptr;
    L_PRINT(LOG_INFO) << "Scanning memory for sig: " << signature;
    static auto pattern_to_byte = [](const char* pattern) {
        auto bytes = std::vector<int>{};
        auto start = const_cast<char*>(pattern);
        auto end = const_cast<char*>(pattern) + strlen(pattern);

        for (auto current = start; current < end; ++current) {
            if (*current == '?') {
                ++current;
                if (*current == '?')
                    ++current;
                bytes.push_back(-1);
            }
            else {
                bytes.push_back(strtoul(current, &current, 16));
            }
        }
        return bytes;
        };

    auto dosHeader = (PIMAGE_DOS_HEADER)module;
    auto ntHeaders = (PIMAGE_NT_HEADERS)((std::uint8_t*)module + dosHeader->e_lfanew);

    auto sizeOfImage = ntHeaders->OptionalHeader.SizeOfImage;
    auto patternBytes = pattern_to_byte(signature);
    auto scanBytes = reinterpret_cast<std::uint8_t*>(module);

    auto s = patternBytes.size();
    auto d = patternBytes.data();

    for (auto i = 0ul; i < sizeOfImage - s; ++i) {
        bool found = true;
        for (auto j = 0ul; j < s; ++j) {
            if (scanBytes[i + j] != d[j] && d[j] != -1) {
                found = false;
                break;
            }
        }
        if (found) {
            if (it == patterns.end()) {
                L_PRINT(LOG_WARNING) << "Unmapped signature: " << signature;
                patterns.insert({ search, &scanBytes[i] });
            }
            L_PRINT(LOG_INFO) << "Match found for signature at: " << (uint64_t)&scanBytes[i];
            return &scanBytes[i];
        }
    }
    std::string error_msg = std::format("COULD NOT FIND SIGNATURE!\nThis will most likely lead to a crash.\nPlease report this and attach the game along with this error message.\nPress CTRL + C on this error to copy it.\nsig {} / modname {} / compile date {} / time {}", search, modName == nullptr ? "nullptr" : modName, __DATE__, __TIME__);
    MessageBoxA(NULL, error_msg.c_str(), "YYC Toolbox - whoops!", MB_OK | MB_ICONERROR);
    L_PRINT(LOG_ERROR) << "Invalid signature. Could not find anything.";
    return nullptr;
}

void MEM::CachePatterns(unsigned int* result)
{
    unsigned int fails = 0;
    for (auto& pattern : patterns) {
        if (pattern.second == 0) {
            std::uint8_t* address = MEM::PatternScan(nullptr, pattern.first.c_str());
            if (!address)
                fails++;
            else
                pattern.second = address;
        }
    }
    *result = fails;
}

double MEM::CalculateCompatibility()
{
    unsigned int finds = 0;
    for (auto& pattern : patterns) {
        std::uint8_t* address = MEM::PatternScan(nullptr, pattern.first.c_str());
        if (address != nullptr)
            finds++;
    }
    return static_cast<double>(finds) / (unsigned int)patterns.size() * 100;
}

void* MEM::GetModuleBaseHandle(const char* szModuleName) {
    char moduleName[MAX_PATH]{};
    if (!szModuleName) {
        TCHAR szFileName[MAX_PATH];
        GetModuleFileName(NULL, szFileName, MAX_PATH);
        wcstombs(moduleName, szFileName, MAX_PATH);
        std::filesystem::path p(moduleName);
        strcpy(moduleName, p.filename().string().c_str());
    }
    else
        strcpy(moduleName, szModuleName);

    return GetModuleBaseHandle(widen(moduleName).c_str());
}

#ifdef _WIN64
void* MEM::GetModuleBaseHandle(const wchar_t* wszModuleName)
{
    const _PEB* pPEB = reinterpret_cast<_PEB*>(__readgsqword(0x60));

    if (wszModuleName == nullptr)
        return pPEB->ImageBaseAddress;

    void* pModuleBase = nullptr;
    for (LIST_ENTRY* pListEntry = pPEB->Ldr->InMemoryOrderModuleList.Flink; pListEntry != &pPEB->Ldr->InMemoryOrderModuleList; pListEntry = pListEntry->Flink)
    {
        const _LDR_DATA_TABLE_ENTRY* pEntry = CONTAINING_RECORD(pListEntry, _LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);

        if (pEntry->FullDllName.Buffer != nullptr && CRT::StringCompare(wszModuleName, pEntry->BaseDllName.Buffer) == 0)
        {
            pModuleBase = pEntry->DllBase;
            break;
        }
    }

    if (pModuleBase == nullptr)
        L_PRINT(LOG_ERROR) << "Unable to find module: " << wszModuleName;

    return pModuleBase;
}
#else
typedef struct _LDR_DATA_TABLE_ENTRY_x86 {
    PVOID Reserved1[2];
    LIST_ENTRY InMemoryOrderLinks;
    PVOID Reserved2[2];
    PVOID DllBase;
    PVOID Reserved3[2];
    UNICODE_STRING FullDllName;
    UNICODE_STRING BaseDllName;
    PVOID Reserved5[3];
#pragma warning(push)
#pragma warning(disable: 4201) // we'll always use the Microsoft compiler
    union {
        ULONG CheckSum;
        PVOID Reserved6;
    } DUMMYUNIONNAME;
#pragma warning(pop)
    ULONG TimeDateStamp;
};

void* MEM::GetModuleBaseHandle(const wchar_t* wszModuleName)
{
    // Get the PEB in x86
    const _PEB* pPEB = reinterpret_cast<_PEB*>(__readfsdword(0x30));

    if (!pPEB || !pPEB->Ldr)
        return nullptr;

    LIST_ENTRY* pListHead = &pPEB->Ldr->InMemoryOrderModuleList;
    LIST_ENTRY* pListEntry = pListHead->Flink;

    if (wszModuleName == nullptr)
    {
        _LDR_DATA_TABLE_ENTRY_x86* pEntry = CONTAINING_RECORD(pListEntry, _LDR_DATA_TABLE_ENTRY_x86, InMemoryOrderLinks);
        return pEntry->DllBase;
    }

    void* pModuleBase = nullptr;
    for (; pListEntry != pListHead; pListEntry = pListEntry->Flink)
    {
        _LDR_DATA_TABLE_ENTRY_x86* pEntry = CONTAINING_RECORD(pListEntry, _LDR_DATA_TABLE_ENTRY_x86, InMemoryOrderLinks);

        if (pEntry->BaseDllName.Buffer != nullptr && CRT::StringCompare(wszModuleName, pEntry->BaseDllName.Buffer) == 0)
        {
            pModuleBase = pEntry->DllBase;
            break;
        }
    }

    if (pModuleBase == nullptr)
        L_PRINT(LOG_ERROR) << "Unable to find module: " << wszModuleName;

    return pModuleBase;
}
#endif