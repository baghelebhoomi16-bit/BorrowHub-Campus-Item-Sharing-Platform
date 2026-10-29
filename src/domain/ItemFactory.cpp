#include "src/domain/ItemFactory.hpp"
#include "src/domain/Book.hpp"
#include "src/domain/Electronic.hpp"
#include "src/domain/Equipment.hpp"
#include "src/util/Errors.hpp"

#if __has_include(<nlohmann/json.hpp>)
#include <nlohmann/json.hpp>
using json = nlohmann::json;
#define BH_HAVE_NLOHMANN 1
#else
#define BH_HAVE_NLOHMANN 0
#endif

namespace borrowhub::domain {

namespace {

std::string extractJsonField(std::string_view jsonStr, std::string_view fieldName) {
#if BH_HAVE_NLOHMANN
    try {
        auto j = json::parse(jsonStr);
        if (j.contains(std::string(fieldName))) {
            return j[std::string(fieldName)].get<std::string>();
        }
    } catch (...) {}
#endif
    // Lightweight string extractor fallback
    std::string key = "\"" + std::string(fieldName) + "\":\"";
    auto pos = jsonStr.find(key);
    if (pos == std::string_view::npos) {
        // Try without quotes around value or spaces
        key = "\"" + std::string(fieldName) + "\": \"";
        pos = jsonStr.find(key);
    }
    if (pos != std::string_view::npos) {
        auto start = pos + key.length();
        auto end = jsonStr.find('"', start);
        if (end != std::string_view::npos) {
            return std::string(jsonStr.substr(start, end - start));
        }
    }
    return "";
}

} // namespace

std::unique_ptr<Item> ItemFactory::create(
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
    std::string_view attrsJson) {

    if (typeStr == "BOOK") {
        std::string author = extractJsonField(attrsJson, "author");
        std::string isbn = extractJsonField(attrsJson, "isbn");
        if (author.empty() || isbn.empty()) {
            throw util::AppError(util::ErrorCode::VALIDATION, "Book requires author and isbn attributes");
        }
        return std::make_unique<Book>(
            id, ownerId, std::move(title), std::move(category), std::move(itemCondition),
            std::move(location), deposit, availFrom, availTo, status, createdAt,
            std::move(author), std::move(isbn)
        );
    }

    if (typeStr == "ELECTRONIC") {
        std::string brand = extractJsonField(attrsJson, "brand");
        std::string model = extractJsonField(attrsJson, "model");
        if (brand.empty() || model.empty()) {
            throw util::AppError(util::ErrorCode::VALIDATION, "Electronic requires brand and model attributes");
        }
        return std::make_unique<Electronic>(
            id, ownerId, std::move(title), std::move(category), std::move(itemCondition),
            std::move(location), deposit, availFrom, availTo, status, createdAt,
            std::move(brand), std::move(model)
        );
    }

    if (typeStr == "EQUIPMENT") {
        std::string equipmentType = extractJsonField(attrsJson, "equipmentType");
        std::string brand = extractJsonField(attrsJson, "brand");
        if (equipmentType.empty()) {
            throw util::AppError(util::ErrorCode::VALIDATION, "Equipment requires equipmentType attribute");
        }
        return std::make_unique<Equipment>(
            id, ownerId, std::move(title), std::move(category), std::move(itemCondition),
            std::move(location), deposit, availFrom, availTo, status, createdAt,
            std::move(equipmentType), std::move(brand)
        );
    }

    throw util::AppError(util::ErrorCode::VALIDATION, "Unknown item type: " + std::string(typeStr));
}

} // namespace borrowhub::domain
