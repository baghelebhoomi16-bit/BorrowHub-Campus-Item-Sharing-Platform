#include "src/db/Database.hpp"
#include "src/db/Repositories.hpp"
#include "src/domain/Admin.hpp"
#include "src/util/Clock.hpp"
#include "src/util/Config.hpp"
#include "src/util/Errors.hpp"
#include "src/util/Token.hpp"

#include <iostream>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

namespace {

std::string readHiddenPassword(const std::string& prompt) {
    std::cout << prompt;
    std::string password;
#if defined(_WIN32)
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(hStdin, &mode);
    SetConsoleMode(hStdin, mode & (~ENABLE_ECHO_INPUT));
    std::getline(std::cin, password);
    SetConsoleMode(hStdin, mode);
    std::cout << "\n";
#else
    termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    std::getline(std::cin, password);
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    std::cout << "\n";
#endif
    return password;
}

void printUsage() {
    std::cerr << "BorrowHub Campus Administrator CLI\n"
              << "Usage:\n"
              << "  borrowhub_admin create-admin <email>\n"
              << "  borrowhub_admin block <userId>\n"
              << "  borrowhub_admin unblock <userId>\n"
              << "  borrowhub_admin list-disputes\n"
              << "  borrowhub_admin export-ledger <txnId>\n";
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage();
        return 2;
    }

    std::string command = argv[1];

    try {
        borrowhub::util::Config config = borrowhub::util::Config::loadFromEnv();
        borrowhub::util::SystemClock clock;
        borrowhub::db::Database db(config.dbPath);

        auto users = borrowhub::db::makeUserRepo(db);
        auto disputes = borrowhub::db::makeDisputeRepo(db);
        auto ledger = borrowhub::db::makeLedgerRepo(db);

        if (command == "create-admin") {
            if (argc < 3) {
                std::cerr << "Error: Email required for create-admin\n";
                printUsage();
                return 2;
            }
            std::string email = argv[2];
            std::string password = readHiddenPassword("Enter administrator password: ");
            if (password.length() < 8) {
                std::cerr << "Error: Password must be at least 8 characters\n";
                return 1;
            }

            std::string hash = borrowhub::util::Token::hashPassword(password);
            borrowhub::domain::Admin admin(0, email, hash, "Campus Admin", "", clock.now());
            int64_t id = users->insert(admin, "", "Administration", 0, "");
            users->setVerified(id, true);

            std::cout << "Successfully created verified administrator account [ID: " << id << "] for " << email << "\n";
            return 0;
        }

        if (command == "block" || command == "unblock") {
            if (argc < 3) {
                std::cerr << "Error: userId required\n";
                printUsage();
                return 2;
            }
            int64_t userId = std::stoll(argv[2]);
            bool blocked = (command == "block");
            users->setBlocked(userId, blocked);
            std::cout << "User " << userId << " is now " << (blocked ? "BLOCKED" : "UNBLOCKED") << ".\n";
            return 0;
        }

        if (command == "list-disputes") {
            auto list = disputes->listAll();
            std::cout << "ID\tTxnID\tRaisedBy\tStatus\tReason\n";
            std::cout << "------------------------------------------------------------\n";
            for (const auto& d : list) {
                std::cout << d.id << "\t" << d.txnId << "\t" << d.raisedBy << "\t\t"
                          << d.status << "\t" << d.reason << "\n";
            }
            return 0;
        }

        if (command == "export-ledger") {
            if (argc < 3) {
                std::cerr << "Error: txnId required for export-ledger\n";
                printUsage();
                return 2;
            }
            int64_t txnId = std::stoll(argv[2]);
            auto entries = ledger->listForTransaction(txnId);
            std::cout << "id,txn_id,user_id,kind,amount_paise,amount_formatted,created_at\n";
            int64_t holdTotal = 0;
            int64_t settledTotal = 0;
            for (const auto& e : entries) {
                std::cout << e.id << "," << e.txnId << "," << e.userId << ","
                          << e.kind << "," << e.amount.paise() << ",\""
                          << e.amount.format() << "\"," << e.createdAt << "\n";
                if (e.kind == "HOLD") holdTotal += e.amount.paise();
                if (e.kind == "REFUND" || e.kind == "DEDUCT") settledTotal += e.amount.paise();
            }
            std::cout << "# Invariant Audit: HOLD=" << holdTotal << " paise, SETTLED=" << settledTotal << " paise. Balance="
                      << (holdTotal - settledTotal) << "\n";
            return 0;
        }

        std::cerr << "Error: Unknown command '" << command << "'\n";
        printUsage();
        return 2;

    } catch (const std::exception& e) {
        std::cerr << "Command failed: " << e.what() << "\n";
        return 1;
    }
}
