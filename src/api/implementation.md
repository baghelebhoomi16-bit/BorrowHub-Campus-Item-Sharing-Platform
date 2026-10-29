# API Implementation Architecture & Security (`src/api/`)

## Design Decisions and Security Policies

### 1. Cookies vs LocalStorage
- **Vulnerability**: Storing session tokens in browser `localStorage` or `sessionStorage` leaves them vulnerable to theft via Cross-Site Scripting (XSS).
- **Decision**: BorrowHub issues session tokens exclusively via `Set-Cookie: bh_session=<token>; HttpOnly; SameSite=Strict; Path=/; Max-Age=86400`.
- **Guarantees**:
  - `HttpOnly`: JavaScript cannot read or exfiltrate the session token.
  - `SameSite=Strict`: Browsers refuse to send the cookie on third-party cross-site requests, mitigating Cross-Site Request Forgery (CSRF).

### 2. Hardened Security Headers
Every HTTP response carries protective defense-in-depth headers:
- `Content-Security-Policy: default-src 'self' 'unsafe-inline'; font-src https://fonts.gstatic.com; style-src 'self' 'unsafe-inline' https://fonts.googleapis.com;`: Restricts resource fetching to origin and vetted font providers.
- `X-Content-Type-Options: nosniff`: Prevents MIME-type sniffing attacks.
- `X-Frame-Options: DENY`: Blocks clickjacking attacks.
- `X-XSS-Protection: 1; mode=block`: Activates legacy browser XSS filters.

### 3. Payload Size Limitation (16 KB)
To prevent memory exhaustion and buffer inflation attacks, incoming request bodies are hard-capped at 16,384 bytes (`16 KB`). Any payload exceeding this size triggers an immediate `413 Payload Too Large` error without allocating backend string buffers.

### 4. Sliding Window Rate Limiting
- Login attempts are bounded to **5 requests per minute per IP address**.
- Chat messages are bounded to **20 messages per minute per user**.
- A sliding window algorithm tracks timestamps in memory, instantly rejecting velocity spikes with HTTP `429 Too Many Requests`.
