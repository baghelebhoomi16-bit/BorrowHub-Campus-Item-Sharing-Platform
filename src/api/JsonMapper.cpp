#include "src/api/JsonMapper.hpp"
#include "src/domain/Student.hpp"

#include <iomanip>
#include <sstream>

namespace borrowhub::api {

std::string JsonMapper::escape(std::string_view str) {
    std::ostringstream oss;
    for (char c : str) {
        switch (c) {
            case '"': oss << "\\\""; break;
            case '\\': oss << "\\\\"; break;
            case '\b': oss << "\\b"; break;
            case '\f': oss << "\\f"; break;
            case '\n': oss << "\\n"; break;
            case '\r': oss << "\\r"; break;
            case '\t': oss << "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    oss << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c);
                } else {
                    oss << c;
                }
                break;
        }
    }
    return oss.str();
}

std::string JsonMapper::toJson(const domain::User& user, bool revealPhone) {
    std::ostringstream oss;
    oss << "{"
        << "\"id\":" << user.id() << ","
        << "\"email\":\"" << escape(user.email()) << "\","
        << "\"name\":\"" << escape(user.name()) << "\","
        << "\"role\":\"" << domain::userRoleToString(user.role()) << "\","
        << "\"verified\":" << (user.isVerified() ? "true" : "false") << ","
        << "\"blocked\":" << (user.isBlocked() ? "true" : "false");

    if (revealPhone && !user.phone().empty()) {
        oss << ",\"phone\":\"" << escape(user.phone()) << "\"";
    }

    if (user.role() == domain::UserRole::STUDENT) {
        const auto* s = dynamic_cast<const domain::Student*>(&user);
        if (s) {
            oss << ",\"prn\":\"" << escape(s->prn()) << "\","
                << "\"department\":\"" << escape(s->department()) << "\","
                << "\"year\":" << s->year() << ","
                << "\"division\":\"" << escape(s->division()) << "\","
                << "\"reputation\":" << std::fixed << std::setprecision(1) << s->reputation() << ","
                << "\"canRequest\":" << (s->canRequest() ? "true" : "false");
        }
    }
    oss << "}";
    return oss.str();
}

std::string JsonMapper::toJson(const domain::Item& item) {
    std::ostringstream oss;
    oss << "{"
        << "\"id\":" << item.id() << ","
        << "\"ownerId\":" << item.ownerId() << ","
        << "\"type\":\"" << item.typeName() << "\","
        << "\"title\":\"" << escape(item.title()) << "\","
        << "\"category\":\"" << escape(item.category()) << "\","
        << "\"condition\":\"" << escape(item.itemCondition()) << "\","
        << "\"location\":\"" << escape(item.location()) << "\","
        << "\"depositPaise\":" << item.deposit().paise() << ","
        << "\"depositFormatted\":\"" << escape(item.deposit().format()) << "\","
        << "\"maxBorrowDays\":" << item.maxBorrowDays() << ","
        << "\"penaltyPerDayPaise\":" << item.penaltyPerDay().paise() << ","
        << "\"availFrom\":" << item.availFrom() << ","
        << "\"availTo\":" << item.availTo() << ","
        << "\"attrs\":" << item.attributesJson() << ","
        << "\"status\":\"" << domain::itemStatusToString(item.status()) << "\""
        << "}";
    return oss.str();
}

std::string JsonMapper::toJson(const std::vector<std::unique_ptr<domain::Item>>& items, int totalCount, int page, int pageSize) {
    std::ostringstream oss;
    oss << "{\"items\":[";
    for (size_t i = 0; i < items.size(); ++i) {
        if (i > 0) oss << ",";
        oss << toJson(*items[i]);
    }
    oss << "],\"total\":" << totalCount << ",\"page\":" << page << ",\"pageSize\":" << pageSize << "}";
    return oss.str();
}

std::string JsonMapper::toJson(const db::Message& msg) {
    std::ostringstream oss;
    oss << "{"
        << "\"id\":" << msg.id << ","
        << "\"threadId\":" << msg.threadId << ","
        << "\"senderId\":" << msg.senderId << ","
        << "\"senderName\":\"" << escape(msg.senderName) << "\","
        << "\"body\":\"" << escape(msg.body) << "\","
        << "\"createdAt\":" << msg.createdAt
        << "}";
    return oss.str();
}

std::string JsonMapper::toJson(const std::vector<db::Message>& msgs) {
    std::ostringstream oss;
    oss << "{\"messages\":[";
    for (size_t i = 0; i < msgs.size(); ++i) {
        if (i > 0) oss << ",";
        oss << toJson(msgs[i]);
    }
    oss << "]}";
    return oss.str();
}

std::string JsonMapper::toJson(const db::ChatThread& thread) {
    std::ostringstream oss;
    oss << "{"
        << "\"id\":" << thread.id << ","
        << "\"itemId\":" << thread.itemId << ","
        << "\"borrowerId\":" << thread.borrowerId
        << "}";
    return oss.str();
}

std::string JsonMapper::toJson(const std::vector<db::ChatThread>& threads) {
    std::ostringstream oss;
    oss << "{\"threads\":[";
    for (size_t i = 0; i < threads.size(); ++i) {
        if (i > 0) oss << ",";
        oss << toJson(threads[i]);
    }
    oss << "]}";
    return oss.str();
}

std::string JsonMapper::toJson(const domain::BorrowTransaction& txn,
                               int64_t currentUserId,
                               const std::string& ownerPhone,
                               const std::string& borrowerPhone) {
    std::ostringstream oss;
    oss << "{"
        << "\"id\":" << txn.id() << ","
        << "\"itemId\":" << txn.itemId() << ","
        << "\"ownerId\":" << txn.ownerId() << ","
        << "\"borrowerId\":" << txn.borrowerId() << ","
        << "\"startDate\":" << txn.startDate() << ","
        << "\"dueDate\":" << txn.dueDate() << ","
        << "\"depositPaise\":" << txn.deposit().paise() << ","
        << "\"status\":\"" << domain::transactionStatusToString(txn.status()) << "\","
        << "\"ownerConfirmed\":" << (txn.isOwnerConfirmed() ? "true" : "false") << ","
        << "\"borrowerConfirmed\":" << (txn.isBorrowerConfirmed() ? "true" : "false") << ","
        << "\"depositHeld\":" << (txn.isDepositHeld() ? "true" : "false") << ","
        << "\"claimPaise\":" << txn.claim().paise() << ","
        << "\"penaltyPaise\":" << txn.penalty().paise() << ","
        << "\"conditionReport\":\"" << escape(txn.conditionReport()) << "\"";

    if (txn.returnedAt().has_value()) {
        oss << ",\"returnedAt\":" << txn.returnedAt().value();
    }
    if (txn.refund().has_value()) {
        oss << ",\"refundPaise\":" << txn.refund().value().paise();
    }

    // Blueprint 2.2: "Phone revealed only after accept"
    bool showPhone = (txn.status() != domain::TransactionStatus::REQUESTED &&
                      txn.status() != domain::TransactionStatus::REJECTED &&
                      txn.status() != domain::TransactionStatus::CANCELLED);

    if (showPhone) {
        if (currentUserId == txn.borrowerId() && !ownerPhone.empty()) {
            oss << ",\"counterpartyPhone\":\"" << escape(ownerPhone) << "\"";
        } else if (currentUserId == txn.ownerId() && !borrowerPhone.empty()) {
            oss << ",\"counterpartyPhone\":\"" << escape(borrowerPhone) << "\"";
        }
    }

    oss << "}";
    return oss.str();
}

std::string JsonMapper::toJson(const std::vector<std::unique_ptr<domain::BorrowTransaction>>& txns, int64_t currentUserId) {
    std::ostringstream oss;
    oss << "{\"transactions\":[";
    for (size_t i = 0; i < txns.size(); ++i) {
        if (i > 0) oss << ",";
        oss << toJson(*txns[i], currentUserId);
    }
    oss << "]}";
    return oss.str();
}

std::string JsonMapper::toJson(const db::Notification& n) {
    std::ostringstream oss;
    oss << "{"
        << "\"id\":" << n.id << ","
        << "\"kind\":\"" << escape(n.kind) << "\","
        << "\"payload\":" << (n.payloadJson.empty() ? "{}" : n.payloadJson) << ","
        << "\"isRead\":" << (n.isRead ? "true" : "false") << ","
        << "\"createdAt\":" << n.createdAt
        << "}";
    return oss.str();
}

std::string JsonMapper::toJson(const std::vector<db::Notification>& notifications) {
    std::ostringstream oss;
    oss << "{\"notifications\":[";
    for (size_t i = 0; i < notifications.size(); ++i) {
        if (i > 0) oss << ",";
        oss << toJson(notifications[i]);
    }
    oss << "]}";
    return oss.str();
}

std::string JsonMapper::toJson(const db::Dispute& d) {
    std::ostringstream oss;
    oss << "{"
        << "\"id\":" << d.id << ","
        << "\"txnId\":" << d.txnId << ","
        << "\"raisedBy\":" << d.raisedBy << ","
        << "\"reason\":\"" << escape(d.reason) << "\","
        << "\"status\":\"" << d.status << "\","
        << "\"decisionNote\":\"" << escape(d.decisionNote) << "\","
        << "\"createdAt\":" << d.createdAt;
    if (d.ownerAward.has_value()) {
        oss << ",\"ownerAwardPaise\":" << d.ownerAward.value().paise();
    }
    if (d.decidedAt.has_value()) {
        oss << ",\"decidedAt\":" << d.decidedAt.value();
    }
    oss << "}";
    return oss.str();
}

std::string JsonMapper::toJson(const std::vector<db::Dispute>& disputes) {
    std::ostringstream oss;
    oss << "{\"disputes\":[";
    for (size_t i = 0; i < disputes.size(); ++i) {
        if (i > 0) oss << ",";
        oss << toJson(disputes[i]);
    }
    oss << "]}";
    return oss.str();
}

std::string JsonMapper::errorJson(std::string_view code, std::string_view message) {
    std::ostringstream oss;
    oss << "{\"error\":{\"code\":\"" << escape(code) << "\",\"message\":\"" << escape(message) << "\"}}";
    return oss.str();
}

} // namespace borrowhub::api
