#include "src/api/Server.hpp"
#include "src/api/JsonMapper.hpp"
#include "src/util/Errors.hpp"

#if __has_include(<httplib.h>)
#include <httplib.h>
#define BH_HAVE_HTTPLIB 1
#else
// Forward declare minimal httplib types if header not found at parse time
namespace httplib {
    struct Request {
        std::string method;
        std::string path;
        std::string body;
        std::string remote_addr;
        std::multimap<std::string, std::string> headers;
        std::string get_header_value(const char* key, size_t id = 0) const {
            auto it = headers.find(key);
            if (it != headers.end()) return it->second;
            return "";
        }
    };
    struct Response {
        int status = -1;
        std::string body;
        std::multimap<std::string, std::string> headers;
        void set_content(const std::string& s, const char* content_type) {
            body = s;
            set_header("Content-Type", content_type);
        }
        void set_header(const char* key, const char* val) {
            headers.emplace(key, val);
        }
    };
    class Server {
    public:
        void Get(const std::string&, std::function<void(const Request&, Response&)>) {}
        void Post(const std::string&, std::function<void(const Request&, Response&)>) {}
        void Put(const std::string&, std::function<void(const Request&, Response&)>) {}
        void Delete(const std::string&, std::function<void(const Request&, Response&)>) {}
        void set_mount_point(const std::string&, const std::string&) {}
        void set_payload_max_length(size_t) {}
        void set_default_headers(const std::multimap<std::string, std::string>&) {}
        void set_exception_handler(std::function<void(const Request&, Response&, std::exception_ptr)>) {}
        void set_error_handler(std::function<void(const Request&, Response&)>) {}
        bool listen(const char*, int) { return true; }
        void stop() {}
    };
}
#define BH_HAVE_HTTPLIB 0
#endif

#include <iostream>

namespace borrowhub::api {

Server::Server(util::Config config,
               const util::Clock& clock,
               db::Database& db,
               services::AuthService& auth,
               services::ItemService& items,
               services::SearchService& search,
               services::ChatService& chat,
               services::BorrowService& borrow,
               services::SettlementService& settlement,
               services::RatingService& ratings,
               services::DisputeService& disputes,
               services::NotificationService& notifications)
    : config_(std::move(config)),
      clock_(clock),
      db_(db),
      auth_(auth),
      items_(items),
      search_(search),
      chat_(chat),
      borrow_(borrow),
      settlement_(settlement),
      ratings_(ratings),
      disputes_(disputes),
      notifications_(notifications),
      rateLimiter_(clock),
      authMiddleware_(auth),
      httpServer_(std::make_unique<httplib::Server>()) {
    setup();
}

Server::~Server() {
    stop();
}

void Server::setup() {
    // 1. Payload size limit (16 KB per blueprint)
    httpServer_->set_payload_max_length(16384);

    // 2. Security Headers (CSP, X-Content-Type-Options)
    httpServer_->set_default_headers({
        {"Content-Security-Policy", "default-src 'self' 'unsafe-inline'; font-src https://fonts.gstatic.com; style-src 'self' 'unsafe-inline' https://fonts.googleapis.com;"},
        {"X-Content-Type-Options", "nosniff"},
        {"X-Frame-Options", "DENY"},
        {"X-XSS-Protection", "1; mode=block"}
    });

    // 3. Static files for frontend
    httpServer_->set_mount_point("/", config_.staticDir.c_str());

    // 4. Global Exception Handler
    httpServer_->set_exception_handler([](const httplib::Request&, httplib::Response& res, std::exception_ptr ep) {
        try {
            if (ep) std::rethrow_exception(ep);
        } catch (const util::AppError& e) {
            res.status = e.httpStatus();
            res.set_content(JsonMapper::errorJson(e.codeString(), e.message()), "application/json");
        } catch (const std::exception& e) {
            res.status = 500;
            // No stack trace emitted per blueprint
            res.set_content(JsonMapper::errorJson("INTERNAL", "An internal server error occurred"), "application/json");
        } catch (...) {
            res.status = 500;
            res.set_content(JsonMapper::errorJson("INTERNAL", "An unexpected fatal error occurred"), "application/json");
        }
    });

    // 5. Global Error Handler (e.g. 404 or 413)
    httpServer_->set_error_handler([](const httplib::Request&, httplib::Response& res) {
        if (res.status == 404 && res.body.empty()) {
            res.set_content(JsonMapper::errorJson("NOT_FOUND", "The requested resource was not found"), "application/json");
        } else if (res.status == 413) {
            res.set_content(JsonMapper::errorJson("VALIDATION", "Payload exceeds maximum allowed limit (16 KB)"), "application/json");
        }
    });

    // 6. Register all REST API endpoints
    registerRoutes();
}

void Server::start() {
    std::cout << "[BORROWHUB] Server listening on http://0.0.0.0:" << config_.port << "\n";
    httpServer_->listen("0.0.0.0", config_.port);
}

void Server::stop() {
    if (httpServer_) {
        httpServer_->stop();
    }
}

} // namespace borrowhub::api
