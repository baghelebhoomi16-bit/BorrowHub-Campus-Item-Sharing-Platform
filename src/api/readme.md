# REST API Layer (`src/api/`)

## Purpose
This directory manages HTTP transport, routes incoming requests to service methods, enforces authentication via session cookies, tracks rate limits, and maps domain entities into JSON responses.

## Request Pipeline
```
Browser / HTTP Client
        |
        v
+-------------------------------+
| Payload Limiter (max 16 KB)   |  -> Returns 413 if oversized
+-------------------------------+
        |
        v
+-------------------------------+
| Security Headers & CORS       |  -> Adds CSP, nosniff, X-Frame-Options
+-------------------------------+
        |
        v
+-------------------------------+
| RateLimiter (Sliding Window)  |  -> Returns 429 if limit exceeded
+-------------------------------+
        |
        v
+-------------------------------+
| AuthMiddleware                |  -> Resolves bh_session cookie -> User
+-------------------------------+
        |
        v
+-------------------------------+
| Router.cpp (Route Handlers)   |  -> Maps JSON -> Service Calls
+-------------------------------+
        |
        v
+-------------------------------+
| JsonMapper (Privacy Filtering)|  -> Hides phone numbers & password hashes
+-------------------------------+
```

## Error Response Taxonomy
| Error Code | HTTP Status | Meaning |
| :--- | :--- | :--- |
| `VALIDATION` | 400 Bad Request | Invalid parameter, negative amount, ill-formed email |
| `UNAUTHENTICATED` | 401 Unauthorized | Missing, invalid, or expired session cookie |
| `FORBIDDEN` | 403 Forbidden | Blocked user, stranger accessing thread, non-admin |
| `NOT_FOUND` | 404 Not Found | Item, user, thread, or transaction does not exist |
| `CONFLICT` | 409 Conflict | Duplicate email/PRN, double-booking attempt |
| `ILLEGAL_STATE` | 409 Conflict | Invalid state machine transition |
| `RATE_LIMITED` | 429 Too Many Requests | Over 5 logins/min or 20 messages/min |
| `INTERNAL` | 500 Internal Error | Unexpected database or system error |

## Files
- `Server.hpp` / `Server.cpp`: HTTP server wrapper (`cpp-httplib`), security headers, payload boundaries.
- `Router.cpp`: Pure endpoint bindings for all REST routes.
- `AuthMiddleware.hpp` / `AuthMiddleware.cpp`: Session cookie extraction and role validation.
- `RateLimiter.hpp` / `RateLimiter.cpp`: In-memory thread-safe sliding window rate limiter.
- `JsonMapper.hpp` / `JsonMapper.cpp`: Privacy-preserving JSON serializer.
- `readme.md`: Request pipeline and error taxonomy.
- `implementation.md`: Security architecture and cookie policies.
