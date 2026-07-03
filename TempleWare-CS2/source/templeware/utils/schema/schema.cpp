#include "schema.h"
#include <vector>
#include <algorithm>
#include <Shlobj.h>
#include <shlobj_core.h>


#include "../fnv1a/fnv1a.h"

#include "../math/utlstring/utlstring.h"
#include "../memory/Interface/Interface.h"

struct SchemaDumpedData_t
{
    uint32_t hashedName = 0x0ULL;
    std::uint32_t uOffset = 0x0U;
};

static std::vector<SchemaDumpedData_t> dumped_data;

bool Schema::init(const char* ModuleName, int module_type)
{
    schema_system = I::Get<ISchemaSystem>("schemasystem.dll", "SchemaSystem_00");
    if (!schema_system)
        return false;

    CSchemaSystemTypeScope* pTypeScope = schema_system->FindTypeScopeForModule(ModuleName);
    if (!pTypeScope)
        return false;

    uint8_t* pScope = reinterpret_cast<uint8_t*>(pTypeScope);

    // CORRECT: Use uint16_t for the count
    uint16_t nClasses = *reinterpret_cast<uint16_t*>(pScope + 0x470);
    void* pArray = *reinterpret_cast<void**>(pScope + 0x478);

    printf("[Schema] Found %u classes\n", nClasses);

    if (nClasses == 0 || !pArray)
        return false;

    uint8_t* pEntries = reinterpret_cast<uint8_t*>(pArray);

    for (int i = 0; i < nClasses; i++)
    {
        uint8_t* entry = pEntries + (i * 0x18);
        void* pDecl = *reinterpret_cast<void**>(entry + 0x10);
        if (!pDecl) continue;

        const char* szClassName = *reinterpret_cast<const char**>(reinterpret_cast<uint8_t*>(pDecl) + 0x08);
        SchemaClassInfoData_t* pClassInfo = *reinterpret_cast<SchemaClassInfoData_t**>(reinterpret_cast<uint8_t*>(pDecl) + 0x20);

        if (!pClassInfo || pClassInfo->nFieldSize == 0)
            continue;

        for (int j = 0; j < pClassInfo->nFieldSize; j++)
        {
            SchemaClassFieldData_t* pFields = pClassInfo->pFields;
            if (!pFields[j].szName) continue;

            std::string szFieldClassBuffer = std::string(szClassName) + "->" + std::string(pFields[j].szName);
            dumped_data.emplace_back(hash_32_fnv1a_const(szFieldClassBuffer.c_str()), pFields[j].nSingleInheritanceOffset);
        }

        // Debug: Print all hashes for C_BaseEntity fields
        if (strcmp(szClassName, "C_BaseEntity") == 0) {
            for (int j = 0; j < pClassInfo->nFieldSize; j++) {
                SchemaClassFieldData_t* pFields = pClassInfo->pFields;
                std::string fieldName = std::string(szClassName) + "->" + std::string(pFields[j].szName);
                uint32_t hash = hash_32_fnv1a_const(fieldName.c_str());
                printf("  Stored: %s -> hash 0x%08X (%u) offset %u\n",
                    fieldName.c_str(), hash, hash, pFields[j].nSingleInheritanceOffset);
            }
        }

        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        printf("[Schema] ");
        SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
        printf("Dumped Class: %s fields: %i\n", szClassName, pClassInfo->nFieldSize);
    }

    bool found = false;
    for (const auto& data : dumped_data) {
        if (data.hashedName == 0x0FD14016) {
            printf("[Schema] Verified: hash 0x0FD14016 stored with offset %u\n", data.uOffset);
            found = true;
            break;
        }
    }
    if (!found) {
        printf("[Schema] WARNING: Hash 0x0FD14016 was NOT stored!\n");
    }

    return true;
}

std::uint32_t SchemaFinder::Get(const uint32_t hashedName)
{
    for (size_t i = 0; i < dumped_data.size(); i++) {
        if (dumped_data[i].hashedName == hashedName) {
            return dumped_data[i].uOffset;
        }
    }

    if (hashedName == 0x0FD14016) {
        printf("[SchemaFinder] DEBUG: Hash 0x0FD14016 not found in %zu entries\n", dumped_data.size());

        for (size_t i = 0; i < min(dumped_data.size(), 1000); i++) {
            if (dumped_data[i].hashedName == 0x0FD14016) {
                printf("  FOUND at index %zu! offset %u\n", i, dumped_data[i].uOffset);
                return dumped_data[i].uOffset;
            }
        }

        printf("  Hash definitely not in vector!\n");
    }

    return 0U;
}

