#include "src/db/Repositories.hpp"
#include "src/db/Database.hpp"
#include "src/domain/Admin.hpp"
#include "src/domain/ItemFactory.hpp"
#include "src/util/Errors.hpp"

#include <algorithm>
#include <sstream>

namespace borrowhub::db {

// --- SqliteUserRepo ---
class SqliteUserRepo : public UserRepo {
public:
    explicit SqliteUserRepo(Database& db) : db_(db) {}

    std::unique_ptr<domain::User> getById(int64_t id) override {
        auto stmt = db_.prepare(
            "SELECT id, email, prn, password_hash, name, phone, department, year, division, role, verified, blocked, reputation, created_at "
            "FROM users WHERE id = ?;");
        stmt.bind(1, id);
        if (!stmt.step()) return nullptr;
        return mapUser(stmt);
    }

    std::unique_ptr<domain::User> getByEmail(std::string_view email) override {
        auto stmt = db_.prepare(
            "SELECT id, email, prn, password_hash, name, phone, department, year, division, role, verified, blocked, reputation, created_at "
            "FROM users WHERE email = ?;");
        stmt.bind(1, email);
        if (!stmt.step()) return nullptr;
        return mapUser(stmt);
    }

    std::unique_ptr<domain::User> getByPrn(std::string_view prn) override {
        auto stmt = db_.prepare(
            "SELECT id, email, prn, password_hash, name, phone, department, year, division, role, verified, blocked, reputation, created_at "
            "FROM users WHERE prn = ?;");
        stmt.bind(1, prn);
        if (!stmt.step()) return nullptr;
        return mapUser(stmt);
    }

    int64_t insert(const domain::User& user, std::string_view prn, std::string_view department, int year, std::string_view division) override {
        auto stmt = db_.prepare(
            "INSERT INTO users (email, prn, password_hash, name, phone, department, year, division, role, verified, blocked, reputation, created_at) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);");
        stmt.bind(1, user.email());
        if (prn.empty()) stmt.bindNull(2); else stmt.bind(2, prn);
        stmt.bind(3, user.passwordHash());
        stmt.bind(4, user.name());
        if (user.phone().empty()) stmt.bindNull(5); else stmt.bind(5, user.phone());
        if (department.empty()) stmt.bindNull(6); else stmt.bind(6, department);
        if (year <= 0) stmt.bindNull(7); else stmt.bind(7, year);
        if (division.empty()) stmt.bindNull(8); else stmt.bind(8, division);
        stmt.bind(9, domain::userRoleToString(user.role()));
        stmt.bind(10, user.isVerified() ? 1 : 0);
        stmt.bind(11, user.isBlocked() ? 1 : 0);
        stmt.bind(12, 62.0); // Baseline authoritative reputation
        stmt.bind(13, user.createdAt());
        stmt.step();
        return db_.lastInsertRowid();
    }

    void update(const domain::User& user) override {
        auto stmt = db_.prepare(
            "UPDATE users SET name = ?, phone = ?, verified = ?, blocked = ? WHERE id = ?;");
        stmt.bind(1, user.name());
        stmt.bind(2, user.phone());
        stmt.bind(3, user.isVerified() ? 1 : 0);
        stmt.bind(4, user.isBlocked() ? 1 : 0);
        stmt.bind(5, user.id());
        stmt.step();
    }

    void setVerified(int64_t userId, bool verified) override {
        auto stmt = db_.prepare("UPDATE users SET verified = ? WHERE id = ?;");
        stmt.bind(1, verified ? 1 : 0);
        stmt.bind(2, userId);
        stmt.step();
    }

    void setBlocked(int64_t userId, bool blocked) override {
        auto stmt = db_.prepare("UPDATE users SET blocked = ? WHERE id = ?;");
        stmt.bind(1, blocked ? 1 : 0);
        stmt.bind(2, userId);
        stmt.step();
    }

    void updateReputation(int64_t userId, double reputation) override {
        auto stmt = db_.prepare("UPDATE users SET reputation = ? WHERE id = ?;");
        stmt.bind(1, reputation);
        stmt.bind(2, userId);
        stmt.step();
    }

    void updateCounters(int64_t /*userId*/, int /*totalTxns*/, int /*onTime*/, int /*damageFree*/) override {
        // Derived from transaction logs in real time
    }

private:
    std::unique_ptr<domain::User> mapUser(const Statement& s) {
        int64_t id = s.columnInt64(0);
        std::string email = s.columnText(1);
        std::string prn = s.columnIsNull(2) ? "" : s.columnText(2);
        std::string passwordHash = s.columnText(3);
        std::string name = s.columnText(4);
        std::string phone = s.columnIsNull(5) ? "" : s.columnText(5);
        std::string department = s.columnIsNull(6) ? "" : s.columnText(6);
        int year = s.columnIsNull(7) ? 1 : s.columnInt(7);
        std::string division = s.columnIsNull(8) ? "" : s.columnText(8);
        std::string roleStr = s.columnText(9);
        bool verified = s.columnInt(10) == 1;
        bool blocked = s.columnInt(11) == 1;
        double reputation = s.columnDouble(12);
        int64_t createdAt = s.columnInt64(13);

        if (roleStr == "ADMIN") {
            return std::make_unique<domain::Admin>(id, std::move(email), std::move(passwordHash), std::move(name), std::move(phone), createdAt);
        }

        // Count historical transactions
        int totalTxns = 0, onTime = 0, damageFree = 0;
        auto countStmt = db_.prepare("SELECT COUNT(*) FROM transactions WHERE borrower_id = ? AND status = 'SETTLED';");
        countStmt.bind(1, id);
        if (countStmt.step()) totalTxns = countStmt.columnInt(0);

        auto onTimeStmt = db_.prepare("SELECT COUNT(*) FROM transactions WHERE borrower_id = ? AND status = 'SETTLED' AND returned_at <= due_date;");
        onTimeStmt.bind(1, id);
        if (onTimeStmt.step()) onTime = onTimeStmt.columnInt(0);

        auto dfStmt = db_.prepare("SELECT COUNT(*) FROM transactions WHERE borrower_id = ? AND status = 'SETTLED' AND (claim_paise = 0 OR condition_report = 'GOOD');");
        dfStmt.bind(1, id);
        if (dfStmt.step()) damageFree = dfStmt.columnInt(0);

        return std::make_unique<domain::Student>(
            id, std::move(email), std::move(passwordHash), std::move(name), std::move(phone),
            std::move(prn), std::move(department), year, std::move(division),
            reputation, verified, blocked, createdAt, totalTxns, onTime, damageFree
        );
    }

    Database& db_;
};

// --- SqliteSessionRepo ---
class SqliteSessionRepo : public SessionRepo {
public:
    explicit SqliteSessionRepo(Database& db) : db_(db) {}

    void insert(const Session& session) override {
        auto stmt = db_.prepare("INSERT OR REPLACE INTO sessions (token_hash, user_id, expires_at) VALUES (?, ?, ?);");
        stmt.bind(1, session.tokenHash);
        stmt.bind(2, session.userId);
        stmt.bind(3, session.expiresAt);
        stmt.step();
    }

    std::optional<Session> getByTokenHash(std::string_view tokenHash) override {
        auto stmt = db_.prepare("SELECT token_hash, user_id, expires_at FROM sessions WHERE token_hash = ?;");
        stmt.bind(1, tokenHash);
        if (!stmt.step()) return std::nullopt;
        return Session{stmt.columnText(0), stmt.columnInt64(1), stmt.columnInt64(2)};
    }

    void deleteByTokenHash(std::string_view tokenHash) override {
        auto stmt = db_.prepare("DELETE FROM sessions WHERE token_hash = ?;");
        stmt.bind(1, tokenHash);
        stmt.step();
    }

    void deleteExpired(int64_t now) override {
        auto stmt = db_.prepare("DELETE FROM sessions WHERE expires_at < ?;");
        stmt.bind(1, now);
        stmt.step();
    }

private:
    Database& db_;
};

// --- SqliteVerificationTokenRepo ---
class SqliteVerificationTokenRepo : public VerificationTokenRepo {
public:
    explicit SqliteVerificationTokenRepo(Database& db) : db_(db) {}

    void insert(const VerificationToken& token) override {
        auto stmt = db_.prepare("INSERT OR REPLACE INTO verification_tokens (token_hash, user_id, expires_at) VALUES (?, ?, ?);");
        stmt.bind(1, token.tokenHash);
        stmt.bind(2, token.userId);
        stmt.bind(3, token.expiresAt);
        stmt.step();
    }

    std::optional<VerificationToken> getByTokenHash(std::string_view tokenHash) override {
        auto stmt = db_.prepare("SELECT token_hash, user_id, expires_at FROM verification_tokens WHERE token_hash = ?;");
        stmt.bind(1, tokenHash);
        if (!stmt.step()) return std::nullopt;
        return VerificationToken{stmt.columnText(0), stmt.columnInt64(1), stmt.columnInt64(2)};
    }

    void deleteByTokenHash(std::string_view tokenHash) override {
        auto stmt = db_.prepare("DELETE FROM verification_tokens WHERE token_hash = ?;");
        stmt.bind(1, tokenHash);
        stmt.step();
    }

    void deleteExpired(int64_t now) override {
        auto stmt = db_.prepare("DELETE FROM verification_tokens WHERE expires_at < ?;");
        stmt.bind(1, now);
        stmt.step();
    }

private:
    Database& db_;
};

// --- SqliteItemRepo ---
class SqliteItemRepo : public ItemRepo {
public:
    explicit SqliteItemRepo(Database& db) : db_(db) {}

    std::unique_ptr<domain::Item> getById(int64_t id) override {
        auto stmt = db_.prepare(
            "SELECT id, owner_id, type, title, category, item_condition, location, deposit_paise, avail_from, avail_to, attrs_json, status, created_at "
            "FROM items WHERE id = ?;");
        stmt.bind(1, id);
        if (!stmt.step()) return nullptr;
        return mapItem(stmt);
    }

    int64_t insert(const domain::Item& item) override {
        auto stmt = db_.prepare(
            "INSERT INTO items (owner_id, type, title, category, item_condition, location, deposit_paise, avail_from, avail_to, attrs_json, status, created_at) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);");
        stmt.bind(1, item.ownerId());
        stmt.bind(2, domain::itemTypeToString(item.type()));
        stmt.bind(3, item.title());
        stmt.bind(4, item.category());
        stmt.bind(5, item.itemCondition());
        stmt.bind(6, item.location());
        stmt.bind(7, item.deposit().paise());
        stmt.bind(8, item.availFrom());
        stmt.bind(9, item.availTo());
        stmt.bind(10, item.attributesJson());
        stmt.bind(11, domain::itemStatusToString(item.status()));
        stmt.bind(12, item.createdAt());
        stmt.step();
        return db_.lastInsertRowid();
    }

    void update(const domain::Item& item) override {
        auto stmt = db_.prepare(
            "UPDATE items SET title = ?, category = ?, item_condition = ?, location = ?, deposit_paise = ?, avail_from = ?, avail_to = ?, attrs_json = ?, status = ? "
            "WHERE id = ?;");
        stmt.bind(1, item.title());
        stmt.bind(2, item.category());
        stmt.bind(3, item.itemCondition());
        stmt.bind(4, item.location());
        stmt.bind(5, item.deposit().paise());
        stmt.bind(6, item.availFrom());
        stmt.bind(7, item.availTo());
        stmt.bind(8, item.attributesJson());
        stmt.bind(9, domain::itemStatusToString(item.status()));
        stmt.bind(10, item.id());
        stmt.step();
    }

    void setStatus(int64_t itemId, domain::ItemStatus status) override {
        auto stmt = db_.prepare("UPDATE items SET status = ? WHERE id = ?;");
        stmt.bind(1, domain::itemStatusToString(status));
        stmt.bind(2, itemId);
        stmt.step();
    }

    ItemSearchResult search(const ItemFilter& filter) override {
        std::ostringstream sql;
        sql << "FROM items i JOIN users u ON i.owner_id = u.id WHERE i.status != 'REMOVED' ";

        std::vector<std::pair<std::string, std::string>> textParams;
        std::vector<std::pair<std::string, int64_t>> intParams;
        std::vector<std::pair<std::string, double>> doubleParams;

        if (filter.query.has_value() && !filter.query.value().empty()) {
            sql << "AND (i.title LIKE ? OR i.category LIKE ?) ";
            std::string q = "%" + filter.query.value() + "%";
            textParams.push_back({"q1", q});
            textParams.push_back({"q2", q});
        }
        if (filter.category.has_value() && !filter.category.value().empty()) {
            sql << "AND i.category = ? ";
            textParams.push_back({"cat", filter.category.value()});
        }
        if (filter.location.has_value() && !filter.location.value().empty()) {
            sql << "AND i.location LIKE ? ";
            textParams.push_back({"loc", "%" + filter.location.value() + "%"});
        }
        if (filter.condition.has_value() && !filter.condition.value().empty()) {
            sql << "AND i.item_condition = ? ";
            textParams.push_back({"cond", filter.condition.value()});
        }
        if (filter.maxDepositPaise.has_value()) {
            sql << "AND i.deposit_paise <= ? ";
            intParams.push_back({"dep", filter.maxDepositPaise.value()});
        }
        if (filter.availableOnDate.has_value()) {
            sql << "AND i.status = 'AVAILABLE' AND (i.avail_from IS NULL OR i.avail_from <= ?) AND (i.avail_to IS NULL OR i.avail_to >= ?) ";
            intParams.push_back({"d1", filter.availableOnDate.value()});
            intParams.push_back({"d2", filter.availableOnDate.value()});
        }
        if (filter.minOwnerRating.has_value()) {
            sql << "AND u.reputation >= ? ";
            doubleParams.push_back({"rep", filter.minOwnerRating.value()});
        }

        // Count total matching
        std::string countSql = "SELECT COUNT(*) " + sql.str() + ";";
        auto countStmt = db_.prepare(countSql);
        int bindIdx = 1;
        for (const auto& tp : textParams) countStmt.bind(bindIdx++, tp.second);
        for (const auto& ip : intParams) countStmt.bind(bindIdx++, ip.second);
        for (const auto& dp : doubleParams) countStmt.bind(bindIdx++, dp.second);

        int totalCount = 0;
        if (countStmt.step()) {
            totalCount = countStmt.columnInt(0);
        }

        // Paginated results (capped at 50 per blueprint)
        int limit = std::clamp(filter.pageSize, 1, 50);
        int offset = (std::max(filter.page, 1) - 1) * limit;

        std::string querySql = "SELECT i.id, i.owner_id, i.type, i.title, i.category, i.item_condition, i.location, i.deposit_paise, i.avail_from, i.avail_to, i.attrs_json, i.status, i.created_at "
                               + sql.str() + "ORDER BY i.id DESC LIMIT ? OFFSET ?;";
        auto stmt = db_.prepare(querySql);
        bindIdx = 1;
        for (const auto& tp : textParams) stmt.bind(bindIdx++, tp.second);
        for (const auto& ip : intParams) stmt.bind(bindIdx++, ip.second);
        for (const auto& dp : doubleParams) stmt.bind(bindIdx++, dp.second);
        stmt.bind(bindIdx++, limit);
        stmt.bind(bindIdx++, offset);

        ItemSearchResult result;
        result.totalCount = totalCount;
        while (stmt.step()) {
            result.items.push_back(mapItem(stmt));
        }
        return result;
    }

    std::vector<std::unique_ptr<domain::Item>> listByOwner(int64_t ownerId) override {
        auto stmt = db_.prepare(
            "SELECT id, owner_id, type, title, category, item_condition, location, deposit_paise, avail_from, avail_to, attrs_json, status, created_at "
            "FROM items WHERE owner_id = ? AND status != 'REMOVED' ORDER BY id DESC;");
        stmt.bind(1, ownerId);
        std::vector<std::unique_ptr<domain::Item>> list;
        while (stmt.step()) {
            list.push_back(mapItem(stmt));
        }
        return list;
    }

private:
    std::unique_ptr<domain::Item> mapItem(const Statement& s) {
        int64_t id = s.columnInt64(0);
        int64_t ownerId = s.columnInt64(1);
        std::string typeStr = s.columnText(2);
        std::string title = s.columnText(3);
        std::string category = s.columnText(4);
        std::string cond = s.columnText(5);
        std::string location = s.columnText(6);
        int64_t dep = s.columnInt64(7);
        int64_t availFrom = s.columnIsNull(8) ? 0 : s.columnInt64(8);
        int64_t availTo = s.columnIsNull(9) ? 0 : s.columnInt64(9);
        std::string attrsJson = s.columnText(10);
        std::string statusStr = s.columnText(11);
        int64_t createdAt = s.columnInt64(12);

        return domain::ItemFactory::create(
            id, ownerId, typeStr, std::move(title), std::move(category), std::move(cond),
            std::move(location), domain::Money::fromPaise(dep), availFrom, availTo,
            domain::stringToItemStatus(statusStr), createdAt, attrsJson
        );
    }

    Database& db_;
};

// --- SqliteThreadRepo ---
class SqliteThreadRepo : public ThreadRepo {
public:
    explicit SqliteThreadRepo(Database& db) : db_(db) {}

    std::optional<ChatThread> getById(int64_t id) override {
        auto stmt = db_.prepare("SELECT id, item_id, borrower_id FROM chat_threads WHERE id = ?;");
        stmt.bind(1, id);
        if (!stmt.step()) return std::nullopt;
        return ChatThread{stmt.columnInt64(0), stmt.columnInt64(1), stmt.columnInt64(2)};
    }

    std::optional<ChatThread> getByItemAndBorrower(int64_t itemId, int64_t borrowerId) override {
        auto stmt = db_.prepare("SELECT id, item_id, borrower_id FROM chat_threads WHERE item_id = ? AND borrower_id = ?;");
        stmt.bind(1, itemId);
        stmt.bind(2, borrowerId);
        if (!stmt.step()) return std::nullopt;
        return ChatThread{stmt.columnInt64(0), stmt.columnInt64(1), stmt.columnInt64(2)};
    }

    int64_t getOrCreate(int64_t itemId, int64_t borrowerId) override {
        auto existing = getByItemAndBorrower(itemId, borrowerId);
        if (existing.has_value()) {
            return existing.value().id;
        }
        auto stmt = db_.prepare("INSERT INTO chat_threads (item_id, borrower_id) VALUES (?, ?);");
        stmt.bind(1, itemId);
        stmt.bind(2, borrowerId);
        stmt.step();
        return db_.lastInsertRowid();
    }

    std::vector<ChatThread> listForUser(int64_t userId) override {
        auto stmt = db_.prepare(
            "SELECT t.id, t.item_id, t.borrower_id FROM chat_threads t "
            "JOIN items i ON t.item_id = i.id "
            "WHERE t.borrower_id = ? OR i.owner_id = ? ORDER BY t.id DESC;");
        stmt.bind(1, userId);
        stmt.bind(2, userId);
        std::vector<ChatThread> list;
        while (stmt.step()) {
            list.push_back(ChatThread{stmt.columnInt64(0), stmt.columnInt64(1), stmt.columnInt64(2)});
        }
        return list;
    }

private:
    Database& db_;
};

// --- SqliteMessageRepo ---
class SqliteMessageRepo : public MessageRepo {
public:
    explicit SqliteMessageRepo(Database& db) : db_(db) {}

    int64_t insert(int64_t threadId, int64_t senderId, std::string_view body, int64_t createdAt) override {
        auto stmt = db_.prepare("INSERT INTO messages (thread_id, sender_id, body, created_at) VALUES (?, ?, ?, ?);");
        stmt.bind(1, threadId);
        stmt.bind(2, senderId);
        stmt.bind(3, body);
        stmt.bind(4, createdAt);
        stmt.step();
        return db_.lastInsertRowid();
    }

    std::vector<Message> listForThreadAfter(int64_t threadId, int64_t afterId, int limit) override {
        auto stmt = db_.prepare(
            "SELECT m.id, m.thread_id, m.sender_id, u.name, m.body, m.created_at "
            "FROM messages m JOIN users u ON m.sender_id = u.id "
            "WHERE m.thread_id = ? AND m.id > ? ORDER BY m.id ASC LIMIT ?;");
        stmt.bind(1, threadId);
        stmt.bind(2, afterId);
        stmt.bind(3, std::min(limit, 100));

        std::vector<Message> list;
        while (stmt.step()) {
            list.push_back(Message{
                stmt.columnInt64(0),
                stmt.columnInt64(1),
                stmt.columnInt64(2),
                stmt.columnText(3),
                stmt.columnText(4),
                stmt.columnInt64(5)
            });
        }
        return list;
    }

    int countMessagesInWindow(int64_t senderId, int64_t windowSeconds, int64_t now) override {
        auto stmt = db_.prepare("SELECT COUNT(*) FROM messages WHERE sender_id = ? AND created_at >= ?;");
        stmt.bind(1, senderId);
        stmt.bind(2, now - windowSeconds);
        if (stmt.step()) {
            return stmt.columnInt(0);
        }
        return 0;
    }

private:
    Database& db_;
};

// --- SqliteTxnRepo ---
class SqliteTxnRepo : public TxnRepo {
public:
    explicit SqliteTxnRepo(Database& db) : db_(db) {}

    std::unique_ptr<domain::BorrowTransaction> getById(int64_t id) override {
        auto stmt = db_.prepare(
            "SELECT id, item_id, thread_id, owner_id, borrower_id, start_date, due_date, deposit_paise, created_at, updated_at, "
            "status, returned_at, condition_report, claim_paise, penalty_paise, refund_paise, owner_confirmed, borrower_confirmed "
            "FROM transactions WHERE id = ?;");
        stmt.bind(1, id);
        if (!stmt.step()) return nullptr;
        return mapTxn(stmt);
    }

    int64_t insert(const domain::BorrowTransaction& txn) override {
        auto stmt = db_.prepare(
            "INSERT INTO transactions (item_id, thread_id, owner_id, borrower_id, start_date, due_date, deposit_paise, "
            "status, owner_confirmed, borrower_confirmed, created_at, updated_at) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);");
        stmt.bind(1, txn.itemId());
        if (txn.threadId().has_value()) stmt.bind(2, txn.threadId().value()); else stmt.bindNull(2);
        stmt.bind(3, txn.ownerId());
        stmt.bind(4, txn.borrowerId());
        stmt.bind(5, txn.startDate());
        stmt.bind(6, txn.dueDate());
        stmt.bind(7, txn.deposit().paise());
        stmt.bind(8, domain::transactionStatusToString(txn.status()));
        stmt.bind(9, txn.isOwnerConfirmed() ? 1 : 0);
        stmt.bind(10, txn.isBorrowerConfirmed() ? 1 : 0);
        stmt.bind(11, txn.createdAt());
        stmt.bind(12, txn.updatedAt());
        stmt.step();
        return db_.lastInsertRowid();
    }

    void update(const domain::BorrowTransaction& txn) override {
        auto stmt = db_.prepare(
            "UPDATE transactions SET status = ?, returned_at = ?, condition_report = ?, claim_paise = ?, penalty_paise = ?, "
            "refund_paise = ?, owner_confirmed = ?, borrower_confirmed = ?, updated_at = ? WHERE id = ?;");
        stmt.bind(1, domain::transactionStatusToString(txn.status()));
        if (txn.returnedAt().has_value()) stmt.bind(2, txn.returnedAt().value()); else stmt.bindNull(2);
        stmt.bind(3, txn.conditionReport());
        stmt.bind(4, txn.claim().paise());
        stmt.bind(5, txn.penalty().paise());
        if (txn.refund().has_value()) stmt.bind(6, txn.refund().value().paise()); else stmt.bindNull(6);
        stmt.bind(7, txn.isOwnerConfirmed() ? 1 : 0);
        stmt.bind(8, txn.isBorrowerConfirmed() ? 1 : 0);
        stmt.bind(9, txn.updatedAt());
        stmt.bind(10, txn.id());
        stmt.step();
    }

    std::vector<std::unique_ptr<domain::BorrowTransaction>> listForUser(int64_t userId) override {
        auto stmt = db_.prepare(
            "SELECT id, item_id, thread_id, owner_id, borrower_id, start_date, due_date, deposit_paise, created_at, updated_at, "
            "status, returned_at, condition_report, claim_paise, penalty_paise, refund_paise, owner_confirmed, borrower_confirmed "
            "FROM transactions WHERE borrower_id = ? OR owner_id = ? ORDER BY id DESC;");
        stmt.bind(1, userId);
        stmt.bind(2, userId);
        std::vector<std::unique_ptr<domain::BorrowTransaction>> list;
        while (stmt.step()) {
            list.push_back(mapTxn(stmt));
        }
        return list;
    }

    std::vector<std::unique_ptr<domain::BorrowTransaction>> findRequestedFor(int64_t itemId) override {
        auto stmt = db_.prepare(
            "SELECT id, item_id, thread_id, owner_id, borrower_id, start_date, due_date, deposit_paise, created_at, updated_at, "
            "status, returned_at, condition_report, claim_paise, penalty_paise, refund_paise, owner_confirmed, borrower_confirmed "
            "FROM transactions WHERE item_id = ? AND status = 'REQUESTED';");
        stmt.bind(1, itemId);
        std::vector<std::unique_ptr<domain::BorrowTransaction>> list;
        while (stmt.step()) {
            list.push_back(mapTxn(stmt));
        }
        return list;
    }

    std::vector<std::unique_ptr<domain::BorrowTransaction>> findOverdue(int64_t now) override {
        auto stmt = db_.prepare(
            "SELECT id, item_id, thread_id, owner_id, borrower_id, start_date, due_date, deposit_paise, created_at, updated_at, "
            "status, returned_at, condition_report, claim_paise, penalty_paise, refund_paise, owner_confirmed, borrower_confirmed "
            "FROM transactions WHERE status = 'ACTIVE' AND due_date < ?;");
        stmt.bind(1, now);
        std::vector<std::unique_ptr<domain::BorrowTransaction>> list;
        while (stmt.step()) {
            list.push_back(mapTxn(stmt));
        }
        return list;
    }

    std::vector<std::unique_ptr<domain::BorrowTransaction>> findStaleAccepted(int64_t thresholdTime) override {
        auto stmt = db_.prepare(
            "SELECT id, item_id, thread_id, owner_id, borrower_id, start_date, due_date, deposit_paise, created_at, updated_at, "
            "status, returned_at, condition_report, claim_paise, penalty_paise, refund_paise, owner_confirmed, borrower_confirmed "
            "FROM transactions WHERE status = 'ACCEPTED' AND updated_at < ?;");
        stmt.bind(1, thresholdTime);
        std::vector<std::unique_ptr<domain::BorrowTransaction>> list;
        while (stmt.step()) {
            list.push_back(mapTxn(stmt));
        }
        return list;
    }

    std::vector<std::unique_ptr<domain::BorrowTransaction>> findAutoSettleable(int64_t thresholdTime) override {
        auto stmt = db_.prepare(
            "SELECT t.id, t.item_id, t.thread_id, t.owner_id, t.borrower_id, t.start_date, t.due_date, t.deposit_paise, t.created_at, t.updated_at, "
            "t.status, t.returned_at, t.condition_report, t.claim_paise, t.penalty_paise, t.refund_paise, t.owner_confirmed, t.borrower_confirmed "
            "FROM transactions t LEFT JOIN disputes d ON t.id = d.txn_id "
            "WHERE t.status = 'RETURNED' AND t.returned_at < ? AND (d.id IS NULL OR d.status != 'OPEN');");
        stmt.bind(1, thresholdTime);
        std::vector<std::unique_ptr<domain::BorrowTransaction>> list;
        while (stmt.step()) {
            list.push_back(mapTxn(stmt));
        }
        return list;
    }

    std::vector<std::unique_ptr<domain::BorrowTransaction>> findDueWithin(int64_t windowSeconds, int64_t now) override {
        auto stmt = db_.prepare(
            "SELECT id, item_id, thread_id, owner_id, borrower_id, start_date, due_date, deposit_paise, created_at, updated_at, "
            "status, returned_at, condition_report, claim_paise, penalty_paise, refund_paise, owner_confirmed, borrower_confirmed "
            "FROM transactions WHERE status = 'ACTIVE' AND due_date - ? < ? AND due_date > ?;");
        stmt.bind(1, now);
        stmt.bind(2, windowSeconds);
        stmt.bind(3, now);
        std::vector<std::unique_ptr<domain::BorrowTransaction>> list;
        while (stmt.step()) {
            list.push_back(mapTxn(stmt));
        }
        return list;
    }

    bool hasLiveTransactionForUserAndItem(int64_t itemId, int64_t borrowerId) override {
        auto stmt = db_.prepare(
            "SELECT COUNT(*) FROM transactions WHERE item_id = ? AND borrower_id = ? AND status IN ('REQUESTED', 'ACCEPTED', 'ACTIVE', 'OVERDUE');");
        stmt.bind(1, itemId);
        stmt.bind(2, borrowerId);
        if (stmt.step()) {
            return stmt.columnInt(0) > 0;
        }
        return false;
    }

private:
    std::unique_ptr<domain::BorrowTransaction> mapTxn(const Statement& s) {
        int64_t id = s.columnInt64(0);
        int64_t itemId = s.columnInt64(1);
        std::optional<int64_t> threadId = s.columnIsNull(2) ? std::nullopt : std::optional<int64_t>(s.columnInt64(2));
        int64_t ownerId = s.columnInt64(3);
        int64_t borrowerId = s.columnInt64(4);
        int64_t startDate = s.columnInt64(5);
        int64_t dueDate = s.columnInt64(6);
        int64_t depPaise = s.columnInt64(7);
        int64_t createdAt = s.columnInt64(8);
        int64_t updatedAt = s.columnInt64(9);
        std::string statusStr = s.columnText(10);
        std::optional<int64_t> returnedAt = s.columnIsNull(11) ? std::nullopt : std::optional<int64_t>(s.columnInt64(11));
        std::string cond = s.columnIsNull(12) ? "" : s.columnText(12);
        int64_t claimPaise = s.columnInt64(13);
        int64_t penaltyPaise = s.columnInt64(14);
        std::optional<domain::Money> refund = s.columnIsNull(15) ? std::nullopt : std::optional<domain::Money>(domain::Money::fromPaise(s.columnInt64(15)));
        bool ownerConfirmed = s.columnInt(16) == 1;
        bool borrowerConfirmed = s.columnInt(17) == 1;

        // Check if ledger has hold
        bool depositHeld = false;
        auto holdStmt = db_.prepare("SELECT COUNT(*) FROM ledger_entries WHERE txn_id = ? AND kind = 'HOLD';");
        holdStmt.bind(1, id);
        if (holdStmt.step()) depositHeld = holdStmt.columnInt(0) > 0;

        return std::make_unique<domain::BorrowTransaction>(
            id, itemId, threadId, ownerId, borrowerId, startDate, dueDate,
            domain::Money::fromPaise(depPaise), createdAt, updatedAt,
            domain::stringToTransactionStatus(statusStr), returnedAt, cond,
            domain::Money::fromPaise(claimPaise), domain::Money::fromPaise(penaltyPaise),
            refund, ownerConfirmed, borrowerConfirmed, depositHeld, updatedAt
        );
    }

    Database& db_;
};

// --- SqliteLedgerRepo ---
class SqliteLedgerRepo : public LedgerRepo {
public:
    explicit SqliteLedgerRepo(Database& db) : db_(db) {}

    int64_t insert(int64_t txnId, int64_t userId, std::string_view kind, domain::Money amount, int64_t createdAt) override {
        auto stmt = db_.prepare("INSERT INTO ledger_entries (txn_id, user_id, kind, amount_paise, created_at) VALUES (?, ?, ?, ?, ?);");
        stmt.bind(1, txnId);
        stmt.bind(2, userId);
        stmt.bind(3, kind);
        stmt.bind(4, amount.paise());
        stmt.bind(5, createdAt);
        stmt.step();
        return db_.lastInsertRowid();
    }

    std::vector<LedgerEntry> listForTransaction(int64_t txnId) override {
        auto stmt = db_.prepare("SELECT id, txn_id, user_id, kind, amount_paise, created_at FROM ledger_entries WHERE txn_id = ? ORDER BY id ASC;");
        stmt.bind(1, txnId);
        std::vector<LedgerEntry> list;
        while (stmt.step()) {
            list.push_back(LedgerEntry{
                stmt.columnInt64(0),
                stmt.columnInt64(1),
                stmt.columnInt64(2),
                stmt.columnText(3),
                domain::Money::fromPaise(stmt.columnInt64(4)),
                stmt.columnInt64(5)
            });
        }
        return list;
    }

    domain::Money getSumForTxnAndKind(int64_t txnId, std::string_view kind) override {
        auto stmt = db_.prepare("SELECT COALESCE(SUM(amount_paise), 0) FROM ledger_entries WHERE txn_id = ? AND kind = ?;");
        stmt.bind(1, txnId);
        stmt.bind(2, kind);
        if (stmt.step()) {
            return domain::Money::fromPaise(stmt.columnInt64(0));
        }
        return domain::Money::fromPaise(0);
    }

private:
    Database& db_;
};

// --- SqliteRatingRepo ---
class SqliteRatingRepo : public RatingRepo {
public:
    explicit SqliteRatingRepo(Database& db) : db_(db) {}

    int64_t insert(int64_t txnId, int64_t raterId, int64_t rateeId, int stars, std::string_view comment) override {
        auto stmt = db_.prepare("INSERT INTO ratings (txn_id, rater_id, ratee_id, stars, comment) VALUES (?, ?, ?, ?, ?);");
        stmt.bind(1, txnId);
        stmt.bind(2, raterId);
        stmt.bind(3, rateeId);
        stmt.bind(4, stars);
        stmt.bind(5, comment);
        stmt.step();
        return db_.lastInsertRowid();
    }

    bool hasRated(int64_t txnId, int64_t raterId) override {
        auto stmt = db_.prepare("SELECT COUNT(*) FROM ratings WHERE txn_id = ? AND rater_id = ?;");
        stmt.bind(1, txnId);
        stmt.bind(2, raterId);
        if (stmt.step()) {
            return stmt.columnInt(0) > 0;
        }
        return false;
    }

    std::vector<int> getStarsForUser(int64_t userId) override {
        auto stmt = db_.prepare("SELECT stars FROM ratings WHERE ratee_id = ?;");
        stmt.bind(1, userId);
        std::vector<int> stars;
        while (stmt.step()) {
            stars.push_back(stmt.columnInt(0));
        }
        return stars;
    }

private:
    Database& db_;
};

// --- SqliteNotificationRepo ---
class SqliteNotificationRepo : public NotificationRepo {
public:
    explicit SqliteNotificationRepo(Database& db) : db_(db) {}

    int64_t insert(int64_t userId, std::string_view kind, std::string_view payloadJson, int64_t createdAt) override {
        auto stmt = db_.prepare("INSERT INTO notifications (user_id, kind, payload_json, is_read, created_at) VALUES (?, ?, ?, 0, ?);");
        stmt.bind(1, userId);
        stmt.bind(2, kind);
        stmt.bind(3, payloadJson);
        stmt.bind(4, createdAt);
        stmt.step();
        return db_.lastInsertRowid();
    }

    std::vector<Notification> listForUser(int64_t userId, bool unreadOnly) override {
        std::string sql = "SELECT id, user_id, kind, payload_json, is_read, created_at FROM notifications WHERE user_id = ? ";
        if (unreadOnly) sql += "AND is_read = 0 ";
        sql += "ORDER BY id DESC LIMIT 50;";

        auto stmt = db_.prepare(sql);
        stmt.bind(1, userId);
        std::vector<Notification> list;
        while (stmt.step()) {
            list.push_back(Notification{
                stmt.columnInt64(0),
                stmt.columnInt64(1),
                stmt.columnText(2),
                stmt.columnText(3),
                stmt.columnInt(4) == 1,
                stmt.columnInt64(5)
            });
        }
        return list;
    }

    void markRead(int64_t notificationId, int64_t userId) override {
        auto stmt = db_.prepare("UPDATE notifications SET is_read = 1 WHERE id = ? AND user_id = ?;");
        stmt.bind(1, notificationId);
        stmt.bind(2, userId);
        stmt.step();
    }

private:
    Database& db_;
};

// --- SqliteDisputeRepo ---
class SqliteDisputeRepo : public DisputeRepo {
public:
    explicit SqliteDisputeRepo(Database& db) : db_(db) {}

    int64_t insert(int64_t txnId, int64_t raisedBy, std::string_view reason, int64_t createdAt) override {
        auto stmt = db_.prepare("INSERT INTO disputes (txn_id, raised_by, reason, status, created_at) VALUES (?, ?, ?, 'OPEN', ?);");
        stmt.bind(1, txnId);
        stmt.bind(2, raisedBy);
        stmt.bind(3, reason);
        stmt.bind(4, createdAt);
        stmt.step();
        return db_.lastInsertRowid();
    }

    std::optional<Dispute> getById(int64_t id) override {
        auto stmt = db_.prepare("SELECT id, txn_id, raised_by, reason, status, admin_id, owner_award_paise, decision_note, created_at, decided_at FROM disputes WHERE id = ?;");
        stmt.bind(1, id);
        if (!stmt.step()) return std::nullopt;
        return mapDispute(stmt);
    }

    std::optional<Dispute> getByTxnId(int64_t txnId) override {
        auto stmt = db_.prepare("SELECT id, txn_id, raised_by, reason, status, admin_id, owner_award_paise, decision_note, created_at, decided_at FROM disputes WHERE txn_id = ?;");
        stmt.bind(1, txnId);
        if (!stmt.step()) return std::nullopt;
        return mapDispute(stmt);
    }

    std::vector<Dispute> listAll() override {
        auto stmt = db_.prepare("SELECT id, txn_id, raised_by, reason, status, admin_id, owner_award_paise, decision_note, created_at, decided_at FROM disputes ORDER BY id DESC;");
        std::vector<Dispute> list;
        while (stmt.step()) {
            list.push_back(mapDispute(stmt));
        }
        return list;
    }

    void decide(int64_t disputeId, int64_t adminId, domain::Money ownerAward, std::string_view note, int64_t now) override {
        auto stmt = db_.prepare(
            "UPDATE disputes SET status = 'DECIDED', admin_id = ?, owner_award_paise = ?, decision_note = ?, decided_at = ? WHERE id = ?;");
        stmt.bind(1, adminId);
        stmt.bind(2, ownerAward.paise());
        stmt.bind(3, note);
        stmt.bind(4, now);
        stmt.bind(5, disputeId);
        stmt.step();
    }

private:
    Dispute mapDispute(const Statement& s) {
        int64_t id = s.columnInt64(0);
        int64_t txnId = s.columnInt64(1);
        int64_t raisedBy = s.columnInt64(2);
        std::string reason = s.columnText(3);
        std::string status = s.columnText(4);
        std::optional<int64_t> adminId = s.columnIsNull(5) ? std::nullopt : std::optional<int64_t>(s.columnInt64(5));
        std::optional<domain::Money> award = s.columnIsNull(6) ? std::nullopt : std::optional<domain::Money>(domain::Money::fromPaise(s.columnInt64(6)));
        std::string note = s.columnIsNull(7) ? "" : s.columnText(7);
        int64_t createdAt = s.columnInt64(8);
        std::optional<int64_t> decidedAt = s.columnIsNull(9) ? std::nullopt : std::optional<int64_t>(s.columnInt64(9));

        return Dispute{id, txnId, raisedBy, std::move(reason), std::move(status), adminId, award, std::move(note), createdAt, decidedAt};
    }

    Database& db_;
};

// Factory functions for creating repository implementations
std::unique_ptr<UserRepo> makeUserRepo(Database& db) { return std::make_unique<SqliteUserRepo>(db); }
std::unique_ptr<SessionRepo> makeSessionRepo(Database& db) { return std::make_unique<SqliteSessionRepo>(db); }
std::unique_ptr<VerificationTokenRepo> makeVerificationTokenRepo(Database& db) { return std::make_unique<SqliteVerificationTokenRepo>(db); }
std::unique_ptr<ItemRepo> makeItemRepo(Database& db) { return std::make_unique<SqliteItemRepo>(db); }
std::unique_ptr<ThreadRepo> makeThreadRepo(Database& db) { return std::make_unique<SqliteThreadRepo>(db); }
std::unique_ptr<MessageRepo> makeMessageRepo(Database& db) { return std::make_unique<SqliteMessageRepo>(db); }
std::unique_ptr<TxnRepo> makeTxnRepo(Database& db) { return std::make_unique<SqliteTxnRepo>(db); }
std::unique_ptr<LedgerRepo> makeLedgerRepo(Database& db) { return std::make_unique<SqliteLedgerRepo>(db); }
std::unique_ptr<RatingRepo> makeRatingRepo(Database& db) { return std::make_unique<SqliteRatingRepo>(db); }
std::unique_ptr<NotificationRepo> makeNotificationRepo(Database& db) { return std::make_unique<SqliteNotificationRepo>(db); }
std::unique_ptr<DisputeRepo> makeDisputeRepo(Database& db) { return std::make_unique<SqliteDisputeRepo>(db); }

} // namespace borrowhub::db
