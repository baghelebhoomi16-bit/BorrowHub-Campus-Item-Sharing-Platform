#pragma once

#include "src/domain/Item.hpp"
#include <memory>
#include <string>
#include <string_view>

namespace borrowhub::domain {

class ItemFactory {
public:
    // Creates concrete subclass (Book, Electronic, or Equipment) from attributes JSON
    [[nodiscard]] static std::unique_ptr<Item> create(
        int64_t id,
        int64_t ownerId,
        std::string_view typeStr,
        std::string title,
        std::string category,
        std::string itemCondition,
        std::string location,
        Money deposit,
        int64_t availFrom,
        int64_t availTo,
        ItemStatus status,
        int64_t createdAt,
        std::string_view attrsJson);
};

} // namespace borrowhub::domain
