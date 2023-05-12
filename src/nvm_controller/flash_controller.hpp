#pragma once
#include "misc/compiler_compatibility_workaround.hpp"
#include "compiler.h"
#include <cstdlib>
#include <cstdint>
#include <concepts>
#include "FreeRTOS.h"
#include "flash_efc.h"

template<std::regular ValueT, typename AddressT>
class FlashController {
  public:
    using MemoryIntegrityCheckType = AddressT;
    using FlashCheckType           = uint32_t;

    static std::pair<ValueT, bool> Recall(AddressT address) noexcept
    {
        SafeStorage *flash_storage = reinterpret_cast<SafeStorage *>(address);

        if (flash_storage->memoryIntegrityMarker != memoryIntegrityMarkerValue)
            return { ValueT{}, false };

        ValueT data = flash_storage->userData;

        return { data, true };
    }

    static bool Store(ValueT const &data, AddressT at_address) noexcept
    {
        bool           operation_result{ false };
        FlashCheckType zero_if_ok{ 1 };
        auto           data_to_be_saved = SafeStorage{ data, memoryIntegrityMarkerValue };
        uint32_t constexpr erase_first_flag{ 0 };

        portDISABLE_INTERRUPTS();

        zero_if_ok = flash_unlock(at_address, at_address + sizeof(SafeStorage) - 1, nullptr, nullptr);
        if (zero_if_ok != 0)
            goto failed;

        zero_if_ok = flash_erase_page(at_address, 1);
        if (zero_if_ok != 0)
            goto failed;

        zero_if_ok = flash_write(at_address, &data_to_be_saved, sizeof(data_to_be_saved), erase_first_flag);
        if (zero_if_ok != 0)
            goto failed;

        zero_if_ok = flash_lock(at_address, at_address + sizeof(SafeStorage) - 1, nullptr, nullptr);
        if (zero_if_ok != 0)
            goto failed;

        operation_result = true;

failed:
        portENABLE_INTERRUPTS();

        return operation_result;
    }

  private:
    struct SafeStorage {
        ValueT                   userData;
        MemoryIntegrityCheckType memoryIntegrityMarker;
    };

    MemoryIntegrityCheckType static constexpr memoryIntegrityMarkerValue = 0xa5a5a5a5;
};