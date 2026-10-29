#pragma once

#include "src/db/Repositories.hpp"

namespace borrowhub::services {

class SearchService {
public:
    explicit SearchService(db::ItemRepo& items);

    [[nodiscard]] db::ItemSearchResult search(const db::ItemFilter& filter);

private:
    db::ItemRepo& items_;
};

} // namespace borrowhub::services
