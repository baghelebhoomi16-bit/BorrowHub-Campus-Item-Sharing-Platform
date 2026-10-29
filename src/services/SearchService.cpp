#include "src/services/SearchService.hpp"

namespace borrowhub::services {

SearchService::SearchService(db::ItemRepo& items) : items_(items) {}

db::ItemSearchResult SearchService::search(const db::ItemFilter& filter) {
    return items_.search(filter);
}

} // namespace borrowhub::services
