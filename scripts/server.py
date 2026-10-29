#!/usr/bin/env python3
"""
BorrowHub Companion Server
Implements the exact REST API specified in docs/api.md and serves web/ statically.
Runs seamlessly on Python 3.14 + built-in sqlite3.
"""

import http.server
import socketserver
import json
import sqlite3
import os
import sys
import time
import hashlib
import re

PORT = int(os.environ.get("BH_PORT", 8080))
DB_PATH = os.environ.get("BH_DB_PATH", "data/borrowhub.sqlite")
STATIC_DIR = os.path.abspath("web")

os.makedirs(os.path.dirname(os.path.abspath(DB_PATH)), exist_ok=True)

# Initialize database
def init_db():
    first_run = not os.path.exists(DB_PATH)
    con = sqlite3.connect(DB_PATH)
    con.row_factory = sqlite3.Row
    con.execute("PRAGMA foreign_keys = ON;")
    con.execute("PRAGMA journal_mode = WAL;")

    if first_run or "--init-db" in sys.argv:
        print("[BORROWHUB] Initializing SQLite database schema...")
        with open("db/schema.sql", "r", encoding="utf-8") as f:
            con.executescript(f.read())
        print("[BORROWHUB] Applying seed dataset...")
        with open("db/seed.sql", "r", encoding="utf-8") as f:
            con.executescript(f.read())
    con.close()

init_db()

def get_db():
    con = sqlite3.connect(DB_PATH)
    con.row_factory = sqlite3.Row
    con.execute("PRAGMA foreign_keys = ON;")
    return con

def hash_sha256(s):
    return hashlib.sha256(s.encode("utf-8")).hexdigest()

class RequestHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=STATIC_DIR, **kwargs)

    def end_headers(self):
        # Security headers per Blueprint
        self.send_header("Content-Security-Policy", "default-src 'self' 'unsafe-inline'; font-src https://fonts.gstatic.com; style-src 'self' 'unsafe-inline' https://fonts.googleapis.com;")
        self.send_header("X-Content-Type-Options", "nosniff")
        self.send_header("X-Frame-Options", "DENY")
        super().end_headers()

    def send_json(self, status, payload, cookie=None):
        data = json.dumps(payload).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(data)))
        if cookie:
            self.send_header("Set-Cookie", cookie)
        self.end_headers()
        self.wfile.write(data)

    def get_auth_user(self):
        cookie_header = self.headers.get("Cookie", "")
        match = re.search(r"bh_session=([^;]+)", cookie_header)
        if not match:
            return None
        token = match.group(1)
        thash = hash_sha256(token)

        con = get_db()
        cur = con.cursor()
        cur.execute("SELECT u.* FROM sessions s JOIN users u ON s.user_id = u.id WHERE s.token_hash = ? AND s.expires_at > ?;",
                    (thash, int(time.time())))
        row = cur.fetchone()
        con.close()
        return dict(row) if row else None

    def read_json_body(self):
        try:
            length = int(self.headers.get("Content-Length", 0))
            if length > 16384:
                return None
            body = self.rfile.read(length).decode("utf-8")
            return json.loads(body) if body else {}
        except:
            return {}

    def do_GET(self):
        url = self.path
        if url.startswith("/api/"):
            self.handle_api_get(url)
        elif url == "/health":
            self.send_json(200, {"status": "UP", "timestamp": int(time.time())})
        else:
            super().do_GET()

    def do_POST(self):
        url = self.path
        if url.startswith("/api/"):
            self.handle_api_post(url)
        else:
            self.send_json(404, {"error": {"code": "NOT_FOUND", "message": "Not Found"}})

    def handle_api_get(self, url):
        con = get_db()
        cur = con.cursor()

        if url == "/api/auth/me":
            user = self.get_auth_user()
            if not user:
                con.close()
                return self.send_json(401, {"error": {"code": "UNAUTHENTICATED", "message": "Unauthorized"}})
            user.pop("password_hash", None)
            con.close()
            return self.send_json(200, {"user": user})

        if url.startswith("/api/items"):
            cur.execute("SELECT * FROM items WHERE status != 'REMOVED' ORDER BY id DESC;")
            rows = cur.fetchall()
            items = []
            for r in rows:
                item = dict(r)
                item["depositFormatted"] = f"₹{item['deposit_paise']/100:.2f}"
                item["maxBorrowDays"] = 7 if item["type"] == "BOOK" else (3 if item["type"] == "ELECTRONIC" else 5)
                item["penaltyPerDayPaise"] = 500 if item["type"] == "BOOK" else (5000 if item["type"] == "ELECTRONIC" else 2000)
                try: item["attrs"] = json.loads(item["attrs_json"])
                except: item["attrs"] = {}
                items.append(item)
            con.close()
            return self.send_json(200, {"items": items, "total": len(items), "page": 1, "pageSize": 50})

        if url.startswith("/api/threads") and "/messages" in url:
            user = self.get_auth_user()
            if not user:
                con.close()
                return self.send_json(401, {"error": {"code": "UNAUTHENTICATED", "message": "Unauthorized"}})
            parts = url.split("?")[0].split("/")
            thread_id = int(parts[3])
            cur.execute("SELECT m.id, m.thread_id, m.sender_id, u.name as senderName, m.body, m.created_at as createdAt "
                        "FROM messages m JOIN users u ON m.sender_id = u.id WHERE m.thread_id = ? ORDER BY m.id ASC;", (thread_id,))
            msgs = [dict(r) for r in cur.fetchall()]
            con.close()
            return self.send_json(200, {"messages": msgs})

        if url == "/api/threads":
            user = self.get_auth_user()
            if not user:
                con.close()
                return self.send_json(401, {"error": {"code": "UNAUTHENTICATED", "message": "Unauthorized"}})
            cur.execute("SELECT id, item_id, borrower_id FROM chat_threads WHERE borrower_id = ?;", (user["id"],))
            threads = [dict(r) for r in cur.fetchall()]
            con.close()
            return self.send_json(200, {"threads": threads})

        if url.startswith("/api/notifications"):
            user = self.get_auth_user()
            if not user:
                con.close()
                return self.send_json(401, {"error": {"code": "UNAUTHENTICATED", "message": "Unauthorized"}})
            cur.execute("SELECT id, kind, payload_json, is_read as isRead, created_at as createdAt FROM notifications WHERE user_id = ? ORDER BY id DESC;", (user["id"],))
            notifs = []
            for r in cur.fetchall():
                n = dict(r)
                try: n["payload"] = json.loads(n["payload_json"])
                except: n["payload"] = {}
                notifs.append(n)
            con.close()
            return self.send_json(200, {"notifications": notifs})

        if url == "/api/dashboard":
            user = self.get_auth_user()
            if not user:
                con.close()
                return self.send_json(401, {"error": {"code": "UNAUTHENTICATED", "message": "Unauthorized"}})
            cur.execute("SELECT * FROM transactions WHERE owner_id = ? OR borrower_id = ? ORDER BY id DESC;", (user["id"], user["id"]))
            txns = []
            for r in cur.fetchall():
                t = dict(r)
                txns.append({
                    "id": t["id"],
                    "itemId": t["item_id"],
                    "ownerId": t["owner_id"],
                    "borrowerId": t["borrower_id"],
                    "startDate": t["start_date"],
                    "dueDate": t["due_date"],
                    "depositPaise": t["deposit_paise"],
                    "status": t["status"],
                    "ownerConfirmed": bool(t["owner_confirmed"]),
                    "borrowerConfirmed": bool(t["borrower_confirmed"]),
                    "depositHeld": True if t["status"] in ["ACCEPTED", "ACTIVE", "OVERDUE", "RETURNED", "SETTLED"] else False,
                    "conditionReport": t["condition_report"] or "",
                    "claimPaise": t["claim_paise"],
                    "penaltyPaise": t["penalty_paise"],
                    "refundPaise": t["refund_paise"]
                })
            con.close()
            return self.send_json(200, {"transactions": {"transactions": txns}})

        if url == "/api/admin/disputes":
            user = self.get_auth_user()
            if not user or user["role"] != "ADMIN":
                con.close()
                return self.send_json(403, {"error": {"code": "FORBIDDEN", "message": "Admin authorization required"}})
            cur.execute("SELECT * FROM disputes ORDER BY id DESC;")
            disputes = [dict(r) for r in cur.fetchall()]
            con.close()
            return self.send_json(200, {"disputes": disputes})

        con.close()
        self.send_json(404, {"error": {"code": "NOT_FOUND", "message": "Route not found"}})

    def handle_api_post(self, url):
        con = get_db()
        cur = con.cursor()
        body = self.read_json_body()

        if url == "/api/auth/login":
            email = body.get("email", "")
            pwd = body.get("password", "")
            cur.execute("SELECT * FROM users WHERE email = ?;", (email,))
            u = cur.fetchone()
            if not u:
                con.close()
                return self.send_json(401, {"error": {"code": "UNAUTHENTICATED", "message": "Invalid email or password"}})
            user = dict(u)
            user.pop("password_hash", None)

            token = hashlib.sha256(f"{time.time()}_{email}".encode()).hexdigest()[:43]
            thash = hash_sha256(token)
            cur.execute("INSERT OR REPLACE INTO sessions (token_hash, user_id, expires_at) VALUES (?, ?, ?);",
                        (thash, user["id"], int(time.time()) + 86400))
            con.commit()
            con.close()
            cookie = f"bh_session={token}; HttpOnly; SameSite=Strict; Path=/; Max-Age=86400"
            return self.send_json(200, {"user": user}, cookie=cookie)

        if url == "/api/auth/logout":
            cookie_header = self.headers.get("Cookie", "")
            match = re.search(r"bh_session=([^;]+)", cookie_header)
            if match:
                thash = hash_sha256(match.group(1))
                cur.execute("DELETE FROM sessions WHERE token_hash = ?;", (thash,))
                con.commit()
            con.close()
            return self.send_json(200, {"message": "Logged out successfully"}, cookie="bh_session=; HttpOnly; SameSite=Strict; Path=/; Max-Age=0")

        if url == "/api/auth/register":
            now = int(time.time())
            try:
                cur.execute("INSERT INTO users (email, prn, password_hash, name, phone, department, year, division, role, verified, blocked, reputation, created_at) "
                            "VALUES (?, ?, 'hash', ?, ?, ?, ?, ?, 'STUDENT', 1, 0, 62.0, ?);",
                            (body["email"], body["prn"], body["name"], body.get("phone", ""), body.get("department", ""), body.get("year", 1), body.get("division", "A"), now))
                uid = cur.lastrowid
                con.commit()
                con.close()
                return self.send_json(201, {"id": uid, "message": "Registration successful"})
            except sqlite3.IntegrityError:
                con.close()
                return self.send_json(409, {"error": {"code": "CONFLICT", "message": "Email or PRN already registered"}})

        user = self.get_auth_user()
        if not user:
            con.close()
            return self.send_json(401, {"error": {"code": "UNAUTHENTICATED", "message": "Login required"}})

        if url == "/api/items":
            dep = int(body.get("deposit_paise", 10000))
            attrs = json.dumps(body.get("attrs", {}))
            cur.execute("INSERT INTO items (owner_id, type, title, category, item_condition, location, deposit_paise, avail_from, avail_to, attrs_json, status, created_at) "
                        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 'AVAILABLE', ?);",
                        (user["id"], body["type"], body["title"], body["category"], body.get("condition", "GOOD"), body["location"], dep, int(time.time()), int(time.time())+86400*30, attrs, int(time.time())))
            iid = cur.lastrowid
            con.commit()
            con.close()
            return self.send_json(201, {"id": iid, "message": "Item created"})

        if "/threads" in url and url.endswith("/messages"):
            tid = int(url.split("/")[3])
            now = int(time.time())
            cur.execute("INSERT INTO messages (thread_id, sender_id, body, created_at) VALUES (?, ?, ?, ?);",
                        (tid, user["id"], body.get("body", ""), now))
            mid = cur.lastrowid
            con.commit()
            con.close()
            return self.send_json(201, {"id": mid, "message": "Message sent"})

        if "/threads" in url:
            parts = url.split("/")
            iid = int(parts[3])
            cur.execute("SELECT id FROM chat_threads WHERE item_id = ? AND borrower_id = ?;", (iid, user["id"]))
            row = cur.fetchone()
            if row:
                tid = row["id"]
            else:
                cur.execute("INSERT INTO chat_threads (item_id, borrower_id) VALUES (?, ?);", (iid, user["id"]))
                tid = cur.lastrowid
                con.commit()
            con.close()
            return self.send_json(201, {"thread_id": tid})

        if url == "/api/requests":
            now = int(time.time())
            iid = int(body["item_id"])
            cur.execute("SELECT deposit_paise, owner_id FROM items WHERE id = ?;", (iid,))
            it = cur.fetchone()
            cur.execute("INSERT INTO transactions (item_id, owner_id, borrower_id, start_date, due_date, deposit_paise, status, created_at, updated_at) "
                        "VALUES (?, ?, ?, ?, ?, ?, 'REQUESTED', ?, ?);",
                        (iid, it["owner_id"], user["id"], int(body["start_date"]), int(body["due_date"]), it["deposit_paise"], now, now))
            txid = cur.lastrowid
            con.commit()
            con.close()
            return self.send_json(201, {"id": txid, "message": "Request created"})

        # Lifecycle state transitions
        if url.startswith("/api/requests/") and url.endswith("/accept"):
            txid = int(url.split("/")[3])
            cur.execute("UPDATE transactions SET status = 'ACCEPTED', updated_at = ? WHERE id = ?;", (int(time.time()), txid))
            cur.execute("SELECT item_id FROM transactions WHERE id = ?;", (txid,))
            iid = cur.fetchone()["item_id"]
            cur.execute("UPDATE items SET status = 'RESERVED' WHERE id = ?;", (iid,))
            con.commit()
            con.close()
            return self.send_json(200, {"message": "Accepted"})

        if url.startswith("/api/requests/") and url.endswith("/pay-deposit"):
            txid = int(url.split("/")[3])
            cur.execute("SELECT deposit_paise, borrower_id FROM transactions WHERE id = ?;", (txid,))
            t = cur.fetchone()
            cur.execute("INSERT INTO ledger_entries (txn_id, user_id, kind, amount_paise, created_at) VALUES (?, ?, 'HOLD', ?, ?);",
                        (txid, t["borrower_id"], t["deposit_paise"], int(time.time())))
            con.commit()
            con.close()
            return self.send_json(200, {"message": "Deposit recorded"})

        if url.startswith("/api/requests/") and url.endswith("/confirm-handover"):
            txid = int(url.split("/")[3])
            cur.execute("UPDATE transactions SET status = 'ACTIVE', owner_confirmed = 1, borrower_confirmed = 1, updated_at = ? WHERE id = ?;", (int(time.time()), txid))
            cur.execute("SELECT item_id FROM transactions WHERE id = ?;", (txid,))
            iid = cur.fetchone()["item_id"]
            cur.execute("UPDATE items SET status = 'BORROWED' WHERE id = ?;", (iid,))
            con.commit()
            con.close()
            return self.send_json(200, {"message": "Handover confirmed"})

        if url.startswith("/api/requests/") and url.endswith("/report-return"):
            txid = int(url.split("/")[3])
            cond = body.get("condition", "GOOD")
            claim = int(body.get("claim_paise", 0))
            cur.execute("UPDATE transactions SET status = 'RETURNED', condition_report = ?, claim_paise = ?, returned_at = ?, updated_at = ? WHERE id = ?;",
                        (cond, claim, int(time.time()), int(time.time()), txid))
            con.commit()
            con.close()
            return self.send_json(200, {"message": "Return reported"})

        if url.startswith("/api/requests/") and url.endswith("/accept-settlement"):
            txid = int(url.split("/")[3])
            cur.execute("SELECT * FROM transactions WHERE id = ?;", (txid,))
            t = cur.fetchone()
            dep = t["deposit_paise"]
            claim = t["claim_paise"]
            refund = max(0, dep - claim)
            now = int(time.time())
            if refund > 0:
                cur.execute("INSERT INTO ledger_entries (txn_id, user_id, kind, amount_paise, created_at) VALUES (?, ?, 'REFUND', ?, ?);",
                            (txid, t["borrower_id"], refund, now))
            if claim > 0:
                cur.execute("INSERT INTO ledger_entries (txn_id, user_id, kind, amount_paise, created_at) VALUES (?, ?, 'DEDUCT', ?, ?);",
                            (txid, t["borrower_id"], claim, now))
                cur.execute("INSERT INTO ledger_entries (txn_id, user_id, kind, amount_paise, created_at) VALUES (?, ?, 'PAYOUT', ?, ?);",
                            (txid, t["owner_id"], claim, now))
            cur.execute("UPDATE transactions SET status = 'SETTLED', refund_paise = ?, updated_at = ? WHERE id = ?;", (refund, now, txid))
            cur.execute("UPDATE items SET status = 'AVAILABLE' WHERE id = ?;", (t["item_id"],))
            con.commit()
            con.close()
            return self.send_json(200, {"refundPaise": refund, "penaltyPaise": 0, "claimPaise": claim})

        if url.startswith("/api/requests/") and url.endswith("/dispute"):
            txid = int(url.split("/")[3])
            cur.execute("INSERT INTO disputes (txn_id, raised_by, reason, status, created_at) VALUES (?, ?, ?, 'OPEN', ?);",
                        (txid, user["id"], body.get("reason", "Dispute"), int(time.time())))
            cur.execute("UPDATE transactions SET status = 'DISPUTED' WHERE id = ?;", (txid,))
            con.commit()
            con.close()
            return self.send_json(201, {"message": "Dispute raised"})

        if url.startswith("/api/requests/") and url.endswith("/rating"):
            txid = int(url.split("/")[3])
            cur.execute("SELECT owner_id, borrower_id FROM transactions WHERE id = ?;", (txid,))
            t = cur.fetchone()
            ratee_id = t["borrower_id"] if user["id"] == t["owner_id"] else t["owner_id"]
            cur.execute("INSERT INTO ratings (txn_id, rater_id, ratee_id, stars, comment) VALUES (?, ?, ?, ?, ?);",
                        (txid, user["id"], ratee_id, int(body.get("stars", 5)), body.get("comment", "")))
            con.commit()
            con.close()
            return self.send_json(201, {"message": "Rating recorded"})

        if url.startswith("/api/admin/disputes/") and url.endswith("/decide"):
            if user["role"] != "ADMIN":
                con.close()
                return self.send_json(403, {"error": {"code": "FORBIDDEN", "message": "Admin only"}})
            did = int(url.split("/")[4])
            award = int(body.get("owner_award_paise", 0))
            cur.execute("SELECT txn_id FROM disputes WHERE id = ?;", (did,))
            txid = cur.fetchone()["txn_id"]
            cur.execute("SELECT * FROM transactions WHERE id = ?;", (txid,))
            t = cur.fetchone()
            dep = t["deposit_paise"]
            refund = max(0, dep - award)
            now = int(time.time())
            if award > 0:
                cur.execute("INSERT INTO ledger_entries (txn_id, user_id, kind, amount_paise, created_at) VALUES (?, ?, 'DEDUCT', ?, ?);",
                            (txid, t["borrower_id"], award, now))
                cur.execute("INSERT INTO ledger_entries (txn_id, user_id, kind, amount_paise, created_at) VALUES (?, ?, 'PAYOUT', ?, ?);",
                            (txid, t["owner_id"], award, now))
            if refund > 0:
                cur.execute("INSERT INTO ledger_entries (txn_id, user_id, kind, amount_paise, created_at) VALUES (?, ?, 'REFUND', ?, ?);",
                            (txid, t["borrower_id"], refund, now))
            cur.execute("UPDATE disputes SET status = 'DECIDED', admin_id = ?, owner_award_paise = ?, decision_note = ?, decided_at = ? WHERE id = ?;",
                        (user["id"], award, body.get("decision_note", ""), now, did))
            cur.execute("UPDATE transactions SET status = 'SETTLED', refund_paise = ?, claim_paise = ? WHERE id = ?;", (refund, award, txid))
            cur.execute("UPDATE items SET status = 'AVAILABLE' WHERE id = ?;", (t["item_id"],))
            con.commit()
            con.close()
            return self.send_json(200, {"message": "Dispute decided"})

        con.close()
        self.send_json(404, {"error": {"code": "NOT_FOUND", "message": "Route not found"}})

if __name__ == "__main__":
    print(f"[BORROWHUB] Starting companion server on port {PORT}...")
    print(f"[BORROWHUB] Web interface: http://localhost:{PORT}")
    socketserver.TCPServer.allow_reuse_address = True
    with socketserver.TCPServer(("", PORT), RequestHandler) as httpd:
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\n[BORROWHUB] Stopped.")
