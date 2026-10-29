/**
 * BorrowHub Frontend Application
 * Single Page App using Hash Routing, Fetch Wrapper, and State-Driven Action Controls.
 * Strict XSS Defense: All user text is rendered with textContent.
 */

class ApiClient {
    async request(method, path, body = null) {
        const options = {
            method,
            headers: {
                'Accept': 'application/json',
                'Content-Type': 'application/json'
            },
            credentials: 'same-origin'
        };

        if (body && (method === 'POST' || method === 'PUT')) {
            options.body = JSON.stringify(body);
        }

        const res = await fetch(path, options);
        let data = null;

        try {
            data = await res.json();
        } catch {
            // Non-JSON response
        }

        if (!res.ok) {
            if (res.status === 401) {
                window.BorrowHubApp?.handleUnauthorized();
            }
            const errMsg = data?.error?.message || `HTTP ${res.status}: ${res.statusText}`;
            const err = new Error(errMsg);
            err.status = res.status;
            err.code = data?.error?.code;
            throw err;
        }

        return data;
    }

    get(path) { return this.request('GET', path); }
    post(path, body) { return this.request('POST', path, body); }
    put(path, body) { return this.request('PUT', path, body); }
    delete(path) { return this.request('DELETE', path); }
}

class App {
    constructor() {
        this.api = new ApiClient();
        this.chat = new ChatClient(this.api);
        this.currentUser = null;
        this.viewElement = document.getElementById('view');
        this.authNavElement = document.getElementById('header-auth');
        this.notifBadgeElement = document.getElementById('notif-badge');
        this.activeModal = null;

        window.addEventListener('hashchange', () => this.handleRoute());
    }

    async init() {
        await this.checkAuth();
        this.checkHealth();
        this.handleRoute();
        // Periodically poll unread notifications if logged in
        setInterval(() => this.pollNotifications(), 15000);
    }

    async checkAuth() {
        try {
            const data = await this.api.get('/api/auth/me');
            this.currentUser = data.user;
        } catch {
            this.currentUser = null;
        }
        this.renderHeaderAuth();
    }

    async checkHealth() {
        try {
            const health = await this.api.get('/health');
            const elem = document.getElementById('health-status');
            if (elem && health.status === 'UP') {
                elem.textContent = 'System Operational';
            }
        } catch {
            const elem = document.getElementById('health-status');
            if (elem) elem.textContent = 'Server Offline';
        }
    }

    handleUnauthorized() {
        this.currentUser = null;
        this.renderHeaderAuth();
        if (window.location.hash !== '#login' && window.location.hash !== '#register' && !window.location.hash.startsWith('#verify=')) {
            window.location.hash = '#login';
        }
    }

    showToast(message, type = 'info') {
        const container = document.getElementById('toast-container');
        if (!container) return;

        const toast = document.createElement('div');
        toast.className = `toast ${type === 'error' ? 'toast-error' : ''}`;
        toast.textContent = message;

        container.appendChild(toast);
        setTimeout(() => {
            toast.style.opacity = '0';
            setTimeout(() => toast.remove(), 300);
        }, 4000);
    }

    renderHeaderAuth() {
        if (!this.authNavElement) return;

        this.authNavElement.innerHTML = '';
        const navAdmin = document.getElementById('nav-admin');

        if (this.currentUser) {
            if (navAdmin) {
                navAdmin.style.display = this.currentUser.role === 'ADMIN' ? 'inline-flex' : 'none';
            }

            const badge = document.createElement('div');
            badge.className = 'user-badge';

            const nameSpan = document.createElement('span');
            nameSpan.textContent = this.currentUser.name;

            badge.appendChild(nameSpan);

            if (this.currentUser.role === 'STUDENT' && this.currentUser.reputation !== undefined) {
                const repSpan = document.createElement('span');
                repSpan.className = 'user-rep';
                repSpan.textContent = `★ ${this.currentUser.reputation.toFixed(1)}`;
                repSpan.title = 'Bayesian Campus Reputation Score';
                badge.appendChild(repSpan);
            }

            const logoutBtn = document.createElement('button');
            logoutBtn.className = 'btn btn-secondary';
            logoutBtn.textContent = 'Logout';
            logoutBtn.onclick = () => this.logout();

            this.authNavElement.appendChild(badge);
            this.authNavElement.appendChild(logoutBtn);
            this.pollNotifications();
        } else {
            if (navAdmin) navAdmin.style.display = 'none';

            const loginBtn = document.createElement('a');
            loginBtn.href = '#login';
            loginBtn.className = 'btn btn-secondary';
            loginBtn.textContent = 'Log In';

            const registerBtn = document.createElement('a');
            registerBtn.href = '#register';
            registerBtn.className = 'btn btn-primary';
            registerBtn.textContent = 'Register';

            this.authNavElement.appendChild(loginBtn);
            this.authNavElement.appendChild(registerBtn);
        }
    }

    async logout() {
        try {
            await this.api.post('/api/auth/logout');
        } catch {}
        this.currentUser = null;
        this.renderHeaderAuth();
        this.showToast('Logged out successfully');
        window.location.hash = '#browse';
    }

    async pollNotifications() {
        if (!this.currentUser) return;
        try {
            const data = await this.api.get('/api/notifications?unread=true');
            if (data && data.notifications && this.notifBadgeElement) {
                const count = data.notifications.length;
                if (count > 0) {
                    this.notifBadgeElement.textContent = count;
                    this.notifBadgeElement.style.display = 'inline-block';
                } else {
                    this.notifBadgeElement.style.display = 'none';
                }
            }
        } catch {}
    }

    // --- Hash Router ---
    handleRoute() {
        const hash = window.location.hash || '#browse';
        this.updateActiveNav(hash);

        if (hash.startsWith('#verify=')) {
            const token = hash.substring(8);
            this.renderVerify(token);
        } else if (hash === '#login') {
            this.renderLogin();
        } else if (hash === '#register') {
            this.renderRegister();
        } else if (hash === '#browse' || hash === '') {
            this.renderBrowse();
        } else if (hash === '#my-requests') {
            this.requireAuth(() => this.renderMyBorrows());
        } else if (hash === '#my-listings') {
            this.requireAuth(() => this.renderMyListings());
        } else if (hash === '#list-item') {
            this.requireAuth(() => this.renderListItem());
        } else if (hash === '#notifications') {
            this.requireAuth(() => this.renderNotifications());
        } else if (hash === '#admin') {
            this.requireAuth(() => this.renderAdmin());
        } else if (hash.startsWith('#item/')) {
            const id = hash.substring(6);
            this.renderItemDetail(id);
        } else {
            this.renderBrowse();
        }
    }

    updateActiveNav(hash) {
        document.querySelectorAll('.nav-link').forEach(link => {
            const href = link.getAttribute('href');
            if (href === hash || (hash === '' && href === '#browse')) {
                link.classList.add('active');
            } else {
                link.classList.remove('active');
            }
        });
    }

    requireAuth(callback) {
        if (!this.currentUser) {
            window.location.hash = '#login';
            return;
        }
        callback();
    }

    // --- View Renderers ---

    renderLogin() {
        this.viewElement.innerHTML = `
            <div style="max-width: 420px; margin: 3rem auto;">
                <div class="modal-content" style="box-shadow: var(--shadow-lg);">
                    <h2 style="font-family: var(--font-heading); margin-bottom: 0.5rem;">Welcome Back</h2>
                    <p style="color: var(--text-secondary); margin-bottom: 1.5rem; font-size: 0.95rem;">Log in with your verified campus email</p>
                    <form id="login-form">
                        <div class="form-group">
                            <label class="form-label" for="login-email">Campus Email</label>
                            <input class="form-control" type="email" id="login-email" placeholder="student@campus.edu" required autofocus>
                        </div>
                        <div class="form-group">
                            <label class="form-label" for="login-pwd">Password</label>
                            <input class="form-control" type="password" id="login-pwd" placeholder="••••••••" required>
                        </div>
                        <button type="submit" class="btn btn-primary btn-full" style="margin-top: 1rem;">Log In</button>
                    </form>
                    <p style="text-align: center; margin-top: 1.5rem; font-size: 0.9rem; color: var(--text-secondary);">
                        Don't have an account? <a href="#register" style="color: var(--primary); font-weight: 600;">Register here</a>
                    </p>
                </div>
            </div>
        `;

        document.getElementById('login-form').addEventListener('submit', async (e) => {
            e.preventDefault();
            const email = document.getElementById('login-email').value;
            const password = document.getElementById('login-pwd').value;

            try {
                const res = await this.api.post('/api/auth/login', { email, password });
                this.currentUser = res.user;
                this.renderHeaderAuth();
                this.showToast('Login successful!');
                window.location.hash = '#browse';
            } catch (err) {
                this.showToast(err.message, 'error');
            }
        });
    }

    renderRegister() {
        this.viewElement.innerHTML = `
            <div style="max-width: 520px; margin: 2rem auto;">
                <div class="modal-content" style="box-shadow: var(--shadow-lg);">
                    <h2 style="font-family: var(--font-heading); margin-bottom: 0.5rem;">Create Student Account</h2>
                    <p style="color: var(--text-secondary); margin-bottom: 1.5rem; font-size: 0.95rem;">Verify your PRN and start sharing on campus</p>
                    <form id="reg-form">
                        <div class="form-group">
                            <label class="form-label" for="reg-name">Full Name</label>
                            <input class="form-control" type="text" id="reg-name" placeholder="Aarav Patel" required>
                        </div>
                        <div class="form-group">
                            <label class="form-label" for="reg-email">College Email (.edu)</label>
                            <input class="form-control" type="email" id="reg-email" placeholder="aarav@campus.edu" required>
                        </div>
                        <div style="display: grid; grid-template-columns: 1fr 1fr; gap: 1rem;">
                            <div class="form-group">
                                <label class="form-label" for="reg-prn">PRN / Roll Number</label>
                                <input class="form-control" type="text" id="reg-prn" placeholder="PRN2024001" required>
                            </div>
                            <div class="form-group">
                                <label class="form-label" for="reg-dept">Department</label>
                                <input class="form-control" type="text" id="reg-dept" placeholder="Computer Science">
                            </div>
                        </div>
                        <div style="display: grid; grid-template-columns: 1fr 1fr; gap: 1rem;">
                            <div class="form-group">
                                <label class="form-label" for="reg-year">Year (1-4)</label>
                                <input class="form-control" type="number" id="reg-year" min="1" max="4" value="2">
                            </div>
                            <div class="form-group">
                                <label class="form-label" for="reg-div">Division</label>
                                <input class="form-control" type="text" id="reg-div" placeholder="A">
                            </div>
                        </div>
                        <div class="form-group">
                            <label class="form-label" for="reg-phone">Phone Number (Optional)</label>
                            <input class="form-control" type="tel" id="reg-phone" placeholder="+919876543210">
                        </div>
                        <div class="form-group">
                            <label class="form-label" for="reg-pwd">Password (min 8 chars)</label>
                            <input class="form-control" type="password" id="reg-pwd" minlength="8" required>
                        </div>
                        <button type="submit" class="btn btn-primary btn-full" style="margin-top: 1rem;">Register Account</button>
                    </form>
                    <p style="text-align: center; margin-top: 1.5rem; font-size: 0.9rem; color: var(--text-secondary);">
                        Already registered? <a href="#login" style="color: var(--primary); font-weight: 600;">Log in</a>
                    </p>
                </div>
            </div>
        `;

        document.getElementById('reg-form').addEventListener('submit', async (e) => {
            e.preventDefault();
            const body = {
                name: document.getElementById('reg-name').value,
                email: document.getElementById('reg-email').value,
                prn: document.getElementById('reg-prn').value,
                department: document.getElementById('reg-dept').value,
                year: parseInt(document.getElementById('reg-year').value, 10),
                division: document.getElementById('reg-div').value,
                phone: document.getElementById('reg-phone').value,
                password: document.getElementById('reg-pwd').value
            };

            try {
                const res = await this.api.post('/api/auth/register', body);
                this.showToast('Registration initiated! Check console/email for verification link.');
                window.location.hash = '#login';
            } catch (err) {
                this.showToast(err.message, 'error');
            }
        });
    }

    async renderVerify(token) {
        this.viewElement.innerHTML = `
            <div style="max-width: 480px; margin: 4rem auto; text-align: center;">
                <div class="modal-content">
                    <div class="spinner" style="margin: 0 auto 1.5rem auto;"></div>
                    <h3 style="font-family: var(--font-heading);">Verifying Your Account...</h3>
                    <p style="color: var(--text-muted); margin-top: 0.5rem;">Validating verification token against campus registrar records.</p>
                </div>
            </div>
        `;

        try {
            await this.api.post('/api/auth/verify', { token });
            this.showToast('Account successfully verified! You may now log in.');
            window.location.hash = '#login';
        } catch (err) {
            this.viewElement.innerHTML = `
                <div style="max-width: 480px; margin: 4rem auto; text-align: center;">
                    <div class="modal-content">
                        <h3 style="color: var(--danger); font-family: var(--font-heading);">Verification Failed</h3>
                        <p style="color: var(--text-secondary); margin: 1rem 0;">${err.message}</p>
                        <a href="#login" class="btn btn-secondary">Go to Login</a>
                    </div>
                </div>
            `;
        }
    }

    // --- Catalog Browse View ---
    async renderBrowse() {
        this.viewElement.innerHTML = `
            <div class="hero-banner">
                <h1 class="hero-title">Campus Item Sharing, Reimagined.</h1>
                <p class="hero-subtitle">Borrow books, calculators, and lab equipment securely from fellow students with audited deposits and trust tracking.</p>
                <div class="search-box">
                    <div class="search-input-group">
                        <input type="text" id="search-query" placeholder="Search items (e.g. Arduino, CLRS, Multimeter)...">
                    </div>
                    <div class="search-input-group" style="flex: 0 1 180px;">
                        <select id="search-category">
                            <option value="">All Categories</option>
                            <option value="Academics">Academics</option>
                            <option value="Computer Science">Computer Science</option>
                            <option value="Robotics Lab">Robotics Lab</option>
                            <option value="Calculators">Calculators</option>
                            <option value="Lab Equipment">Lab Equipment</option>
                            <option value="Architecture">Architecture</option>
                        </select>
                    </div>
                    <button class="btn btn-primary" id="search-btn">Search</button>
                </div>
            </div>

            <div class="filter-bar">
                <span class="filter-chip active" data-type="">All Types</span>
                <span class="filter-chip" data-type="BOOK">📚 Books</span>
                <span class="filter-chip" data-type="ELECTRONIC">⚡ Electronics</span>
                <span class="filter-chip" data-type="EQUIPMENT">🔧 Equipment</span>
            </div>

            <div id="items-container" class="item-grid">
                <div class="loading-spinner-container">
                    <div class="spinner"></div>
                    <p>Loading campus listings...</p>
                </div>
            </div>
        `;

        const searchBtn = document.getElementById('search-btn');
        const queryInput = document.getElementById('search-query');
        const catSelect = document.getElementById('search-category');

        const doSearch = () => {
            const q = queryInput.value.trim();
            const cat = catSelect.value;
            this.loadItems({ query: q, category: cat });
        };

        searchBtn.addEventListener('click', doSearch);
        queryInput.addEventListener('keypress', (e) => { if (e.key === 'Enter') doSearch(); });
        catSelect.addEventListener('change', doSearch);

        document.querySelectorAll('.filter-chip').forEach(chip => {
            chip.addEventListener('click', (e) => {
                document.querySelectorAll('.filter-chip').forEach(c => c.classList.remove('active'));
                chip.classList.add('active');
                doSearch();
            });
        });

        this.loadItems({});
    }

    async loadItems(filters) {
        const container = document.getElementById('items-container');
        if (!container) return;

        try {
            let url = '/api/items?';
            if (filters.query) url += `q=${encodeURIComponent(filters.query)}&`;
            if (filters.category) url += `category=${encodeURIComponent(filters.category)}&`;

            const data = await this.api.get(url);
            container.innerHTML = '';

            if (!data.items || data.items.length === 0) {
                container.innerHTML = `
                    <div style="grid-column: 1 / -1; text-align: center; padding: 3rem 1rem; color: var(--text-muted);">
                        <p style="font-size: 1.1rem; margin-bottom: 0.5rem;">No items found matching your filters.</p>
                        <p style="font-size: 0.9rem;">Be the first to list an item for fellow students!</p>
                    </div>
                `;
                return;
            }

            for (const item of data.items) {
                const card = document.createElement('div');
                card.className = 'item-card';
                card.onclick = () => { window.location.hash = `#item/${item.id}`; };

                // Type badge class
                const typeClass = item.type === 'BOOK' ? 'type-book' : (item.type === 'ELECTRONIC' ? 'type-electronic' : 'type-equipment');
                const statusClass = item.status === 'AVAILABLE' ? 'status-available' : (item.status === 'RESERVED' ? 'status-reserved' : 'status-borrowed');

                card.innerHTML = `
                    <div class="card-header">
                        <span class="item-type-badge ${typeClass}">${item.type}</span>
                        <span class="item-status-pill ${statusClass}">${item.status}</span>
                    </div>
                    <div class="card-body">
                        <h3 class="item-title" id="item-title-${item.id}"></h3>
                        <div class="item-location">
                            <span>📍</span> <span id="item-loc-${item.id}"></span>
                        </div>
                        <div class="item-meta-row">
                            <div>
                                <span class="deposit-label">Security Deposit</span>
                                <span class="deposit-amount">${item.depositFormatted}</span>
                            </div>
                            <div style="text-align: right;">
                                <span class="deposit-label">Max Duration</span>
                                <span style="font-weight: 600; font-size: 0.95rem;">${item.maxBorrowDays} Days</span>
                            </div>
                        </div>
                    </div>
                `;

                // Safe textContent insertion (prevents XSS)
                card.querySelector(`#item-title-${item.id}`).textContent = item.title;
                card.querySelector(`#item-loc-${item.id}`).textContent = item.location;

                container.appendChild(card);
            }
        } catch (err) {
            container.innerHTML = `<div style="grid-column: 1 / -1; color: var(--danger); text-align: center; padding: 2rem;">Error: ${err.message}</div>`;
        }
    }

    // --- Item Detail & Negotiation Modal ---
    async renderItemDetail(itemId) {
        try {
            const item = await this.api.get(`/api/items/${itemId}`);
            this.openModal(`
                <div class="modal-header">
                    <div>
                        <span class="item-type-badge type-${item.type.toLowerCase()}">${item.type}</span>
                        <h2 class="item-title" id="modal-item-title" style="font-size: 1.5rem; margin-top: 0.5rem;"></h2>
                    </div>
                    <button class="close-btn" onclick="window.BorrowHubApp.closeModal()">&times;</button>
                </div>
                <div style="margin-bottom: 1.5rem;">
                    <p style="color: var(--text-secondary); margin-bottom: 0.75rem;">
                        <strong>Condition:</strong> <span id="modal-item-cond"></span> | 
                        <strong>Location:</strong> <span id="modal-item-loc"></span>
                    </p>
                    <div id="modal-attrs" style="background: var(--bg-page); padding: 0.75rem 1rem; border-radius: var(--radius-md); font-size: 0.9rem; margin-bottom: 1rem;"></div>
                    <div style="display: flex; gap: 1.5rem; padding: 1rem 0; border-top: 1px solid var(--border); border-bottom: 1px solid var(--border);">
                        <div>
                            <span class="deposit-label">Deposit</span>
                            <span style="font-size: 1.3rem; font-weight: 800; color: var(--primary);">${item.depositFormatted}</span>
                        </div>
                        <div>
                            <span class="deposit-label">Late Penalty</span>
                            <span style="font-weight: 600;">₹${item.penaltyPerDayPaise / 100}/day</span>
                        </div>
                        <div>
                            <span class="deposit-label">Max Allowed</span>
                            <span style="font-weight: 600;">${item.maxBorrowDays} Days</span>
                        </div>
                    </div>
                </div>

                <!-- Chat & Request Workspace -->
                <div style="display: grid; grid-template-columns: 1fr; gap: 1.5rem;">
                    <div>
                        <h4 style="font-family: var(--font-heading); margin-bottom: 0.5rem;">1. Mandatory Negotiation Chat</h4>
                        <p style="font-size: 0.85rem; color: var(--text-muted); margin-bottom: 0.75rem;">In accordance with campus policy, negotiate and agree on terms before sending a borrow request.</p>
                        <div id="chat-workspace"></div>
                    </div>

                    <div style="background: var(--bg-page); padding: 1.25rem; border-radius: var(--radius-md); border: 1px solid var(--border);">
                        <h4 style="font-family: var(--font-heading); margin-bottom: 0.75rem;">2. Send Borrow Request</h4>
                        <form id="borrow-req-form">
                            <div style="display: grid; grid-template-columns: 1fr 1fr; gap: 1rem;">
                                <div class="form-group">
                                    <label class="form-label" for="req-start">Start Date</label>
                                    <input class="form-control" type="date" id="req-start" required>
                                </div>
                                <div class="form-group">
                                    <label class="form-label" for="req-end">Due Date</label>
                                    <input class="form-control" type="date" id="req-end" required>
                                </div>
                            </div>
                            <button type="submit" class="btn btn-primary btn-full" id="btn-submit-req">Send Formal Request</button>
                        </form>
                    </div>
                </div>
            `);

            // Safe textContent population
            document.getElementById('modal-item-title').textContent = item.title;
            document.getElementById('modal-item-cond').textContent = item.condition;
            document.getElementById('modal-item-loc').textContent = item.location;

            // Render type attributes safely
            const attrsContainer = document.getElementById('modal-attrs');
            if (item.type === 'BOOK') {
                attrsContainer.textContent = `Author: ${item.attrs.author || 'N/A'} | ISBN: ${item.attrs.isbn || 'N/A'}`;
            } else if (item.type === 'ELECTRONIC') {
                attrsContainer.textContent = `Brand: ${item.attrs.brand || 'N/A'} | Model: ${item.attrs.model || 'N/A'}`;
            } else if (item.type === 'EQUIPMENT') {
                attrsContainer.textContent = `Equipment Type: ${item.attrs.equipmentType || 'N/A'} | Brand: ${item.attrs.brand || 'N/A'}`;
            }

            // Set default dates (today to +min(3, maxBorrowDays))
            const today = new Date();
            const tomorrow = new Date(today);
            tomorrow.setDate(tomorrow.getDate() + 1);
            const due = new Date(today);
            due.setDate(due.getDate() + Math.min(3, item.maxBorrowDays));

            const formatDate = (d) => d.toISOString().split('T')[0];
            document.getElementById('req-start').value = formatDate(tomorrow);
            document.getElementById('req-end').value = formatDate(due);

            // If user is logged in and not the owner, open/create thread
            if (this.currentUser && this.currentUser.id !== item.ownerId) {
                try {
                    const threadRes = await this.api.post(`/api/items/${item.id}/threads`);
                    this.chat.start(threadRes.thread_id, document.getElementById('chat-workspace'));
                } catch (err) {
                    document.getElementById('chat-workspace').innerHTML = `<p style="color: var(--text-muted); font-size: 0.9rem;">Chat unavailable: ${err.message}</p>`;
                }
            } else if (!this.currentUser) {
                document.getElementById('chat-workspace').innerHTML = `<p style="color: var(--text-muted); font-size: 0.9rem;"><a href="#login" style="color: var(--primary);">Log in</a> to chat and borrow.</p>`;
            } else {
                document.getElementById('chat-workspace').innerHTML = `<p style="color: var(--text-muted); font-size: 0.9rem;">You own this item listing.</p>`;
                document.getElementById('borrow-req-form').style.display = 'none';
            }

            // Bind borrow request form
            document.getElementById('borrow-req-form').addEventListener('submit', async (e) => {
                e.preventDefault();
                if (!this.currentUser) {
                    window.location.hash = '#login';
                    return;
                }

                const start = new Date(document.getElementById('req-start').value).getTime() / 1000;
                const end = new Date(document.getElementById('req-end').value).getTime() / 1000;

                try {
                    await this.api.post('/api/requests', {
                        item_id: item.id,
                        start_date: Math.floor(start),
                        due_date: Math.floor(end)
                    });
                    this.showToast('Borrow request sent to owner!');
                    this.closeModal();
                    window.location.hash = '#my-requests';
                } catch (err) {
                    this.showToast(err.message, 'error');
                }
            });

        } catch (err) {
            this.showToast(err.message, 'error');
        }
    }

    // --- My Borrowings View (Full State Machine Driven Actions) ---
    async renderMyBorrows() {
        this.viewElement.innerHTML = `
            <h2 style="font-family: var(--font-heading); margin-bottom: 0.5rem;">My Borrowings & Requests</h2>
            <p style="color: var(--text-secondary); margin-bottom: 1.5rem;">Track simulated deposits, handovers, returns, and settlements.</p>
            <div id="txns-list" style="display: flex; flex-direction: column; gap: 1.25rem;">
                <div class="spinner" style="margin: 2rem auto;"></div>
            </div>
        `;

        try {
            const dash = await this.api.get('/api/dashboard');
            const list = document.getElementById('txns-list');
            list.innerHTML = '';

            const txns = dash.transactions?.transactions || [];
            const myBorrows = txns.filter(t => t.borrowerId === this.currentUser.id);

            if (myBorrows.length === 0) {
                list.innerHTML = `<div style="text-align: center; color: var(--text-muted); padding: 3rem;">You haven't requested any items yet. <a href="#browse" style="color: var(--primary);">Explore catalog</a></div>`;
                return;
            }

            for (const t of myBorrows) {
                const card = document.createElement('div');
                card.className = 'modal-content';
                card.style.maxWidth = '100%';

                const statusColor = t.status === 'ACTIVE' ? 'var(--primary)' : (t.status === 'OVERDUE' ? 'var(--danger)' : 'var(--text-secondary)');

                card.innerHTML = `
                    <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 0.75rem;">
                        <span style="font-weight: 700; font-size: 1.1rem;">Transaction #${t.id} (Item #${t.itemId})</span>
                        <span style="font-weight: 700; color: ${statusColor};">${t.status}</span>
                    </div>
                    <div style="font-size: 0.9rem; color: var(--text-secondary); margin-bottom: 1rem;">
                        <p><strong>Security Deposit:</strong> ₹${t.depositPaise / 100} (${t.depositHeld ? 'Held in Escrow' : 'Awaiting Payment'})</p>
                        <p><strong>Window:</strong> ${new Date(t.startDate * 1000).toLocaleDateString()} to ${new Date(t.dueDate * 1000).toLocaleDateString()}</p>
                        ${t.counterpartyPhone ? `<p><strong>Owner Phone:</strong> ${t.counterpartyPhone}</p>` : ''}
                        ${t.conditionReport ? `<p><strong>Return Condition:</strong> ${t.conditionReport} (Claim: ₹${t.claimPaise / 100})</p>` : ''}
                    </div>
                    <div class="action-panel" id="action-panel-${t.id}"></div>
                `;

                const panel = card.querySelector(`#action-panel-${t.id}`);
                this.renderBorrowerActions(panel, t);

                list.appendChild(card);
            }
        } catch (err) {
            this.showToast(err.message, 'error');
        }
    }

    renderBorrowerActions(panel, t) {
        // Legal state machine transitions for borrower
        if (t.status === 'ACCEPTED' && !t.depositHeld) {
            const payBtn = document.createElement('button');
            payBtn.className = 'btn btn-primary';
            payBtn.textContent = `Pay Deposit (₹${t.depositPaise / 100})`;
            payBtn.onclick = async () => {
                try {
                    await this.api.post(`/api/requests/${t.id}/pay-deposit`);
                    this.showToast('Simulated security deposit recorded in ledger!');
                    this.renderMyBorrows();
                } catch (err) { this.showToast(err.message, 'error'); }
            };
            panel.appendChild(payBtn);
        }

        if (t.status === 'ACCEPTED' && t.depositHeld && !t.borrowerConfirmed) {
            const confirmBtn = document.createElement('button');
            confirmBtn.className = 'btn btn-primary';
            confirmBtn.textContent = 'Confirm Handover';
            confirmBtn.onclick = async () => {
                try {
                    await this.api.post(`/api/requests/${t.id}/confirm-handover`);
                    this.showToast('Handover confirmed!');
                    this.renderMyBorrows();
                } catch (err) { this.showToast(err.message, 'error'); }
            };
            panel.appendChild(confirmBtn);
        }

        if (t.status === 'RETURNED') {
            const acceptBtn = document.createElement('button');
            acceptBtn.className = 'btn btn-primary';
            acceptBtn.textContent = 'Accept Settlement';
            acceptBtn.onclick = async () => {
                try {
                    await this.api.post(`/api/requests/${t.id}/accept-settlement`);
                    this.showToast('Settlement finalized! Refund credited.');
                    this.renderMyBorrows();
                } catch (err) { this.showToast(err.message, 'error'); }
            };
            panel.appendChild(acceptBtn);

            const disputeBtn = document.createElement('button');
            disputeBtn.className = 'btn btn-danger';
            disputeBtn.textContent = 'Raise Dispute (48h Window)';
            disputeBtn.onclick = () => this.promptDispute(t.id);
            panel.appendChild(disputeBtn);
        }

        if (t.status === 'SETTLED') {
            const rateBtn = document.createElement('button');
            rateBtn.className = 'btn btn-secondary';
            rateBtn.textContent = '★ Rate Owner';
            rateBtn.onclick = () => this.promptRating(t.id);
            panel.appendChild(rateBtn);
        }

        if (t.status === 'REQUESTED' || (t.status === 'ACCEPTED' && !t.borrowerConfirmed)) {
            const cancelBtn = document.createElement('button');
            cancelBtn.className = 'btn btn-secondary';
            cancelBtn.textContent = 'Cancel Request';
            cancelBtn.onclick = async () => {
                try {
                    await this.api.post(`/api/requests/${t.id}/cancel`);
                    this.showToast('Request cancelled.');
                    this.renderMyBorrows();
                } catch (err) { this.showToast(err.message, 'error'); }
            };
            panel.appendChild(cancelBtn);
        }
    }

    // --- My Listings View (Owner Actions) ---
    async renderMyListings() {
        this.viewElement.innerHTML = `
            <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 1.5rem;">
                <div>
                    <h2 style="font-family: var(--font-heading);">My Item Listings</h2>
                    <p style="color: var(--text-secondary);">Manage your items and incoming borrow requests</p>
                </div>
                <a href="#list-item" class="btn btn-primary">+ List New Item</a>
            </div>
            <div id="owner-txns-container" style="margin-bottom: 2rem;"></div>
        `;

        try {
            const dash = await this.api.get('/api/dashboard');
            const container = document.getElementById('owner-txns-container');
            const txns = dash.transactions?.transactions || [];
            const ownerTxns = txns.filter(t => t.ownerId === this.currentUser.id);

            if (ownerTxns.length === 0) {
                container.innerHTML = `<p style="color: var(--text-muted); text-align: center; padding: 2rem;">No active borrow requests on your items.</p>`;
                return;
            }

            container.innerHTML = '<h3 style="font-family: var(--font-heading); margin-bottom: 1rem;">Incoming Requests & Active Borrows</h3>';

            for (const t of ownerTxns) {
                const card = document.createElement('div');
                card.className = 'modal-content';
                card.style.maxWidth = '100%';
                card.style.marginBottom = '1rem';

                card.innerHTML = `
                    <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 0.5rem;">
                        <span style="font-weight: 700;">Request on Item #${t.itemId}</span>
                        <span class="item-status-pill status-${t.status.toLowerCase()}">${t.status}</span>
                    </div>
                    <div style="font-size: 0.9rem; color: var(--text-secondary); margin-bottom: 0.75rem;">
                        <p><strong>Borrower:</strong> User #${t.borrowerId} ${t.counterpartyPhone ? `(Phone: ${t.counterpartyPhone})` : ''}</p>
                        <p><strong>Dates:</strong> ${new Date(t.startDate * 1000).toLocaleDateString()} to ${new Date(t.dueDate * 1000).toLocaleDateString()}</p>
                        <p><strong>Deposit:</strong> ₹${t.depositPaise / 100} ${t.depositHeld ? '✓ Held' : '⏳ Pending'}</p>
                    </div>
                    <div class="action-panel" id="owner-actions-${t.id}"></div>
                `;

                const panel = card.querySelector(`#owner-actions-${t.id}`);
                this.renderOwnerActions(panel, t);
                container.appendChild(card);
            }
        } catch (err) {
            this.showToast(err.message, 'error');
        }
    }

    renderOwnerActions(panel, t) {
        if (t.status === 'REQUESTED') {
            const acceptBtn = document.createElement('button');
            acceptBtn.className = 'btn btn-primary';
            acceptBtn.textContent = 'Accept Request';
            acceptBtn.onclick = async () => {
                try {
                    await this.api.post(`/api/requests/${t.id}/accept`);
                    this.showToast('Request accepted! Waiting for deposit.');
                    this.renderMyListings();
                } catch (err) { this.showToast(err.message, 'error'); }
            };
            panel.appendChild(acceptBtn);

            const rejectBtn = document.createElement('button');
            rejectBtn.className = 'btn btn-danger';
            rejectBtn.textContent = 'Reject';
            rejectBtn.onclick = async () => {
                try {
                    await this.api.post(`/api/requests/${t.id}/reject`);
                    this.showToast('Request rejected.');
                    this.renderMyListings();
                } catch (err) { this.showToast(err.message, 'error'); }
            };
            panel.appendChild(rejectBtn);
        }

        if (t.status === 'ACCEPTED' && t.depositHeld && !t.ownerConfirmed) {
            const confirmBtn = document.createElement('button');
            confirmBtn.className = 'btn btn-primary';
            confirmBtn.textContent = 'Confirm Handover';
            confirmBtn.onclick = async () => {
                try {
                    await this.api.post(`/api/requests/${t.id}/confirm-handover`);
                    this.showToast('Handover confirmed! Status is now ACTIVE.');
                    this.renderMyListings();
                } catch (err) { this.showToast(err.message, 'error'); }
            };
            panel.appendChild(confirmBtn);
        }

        if (t.status === 'ACTIVE' || t.status === 'OVERDUE') {
            const returnBtn = document.createElement('button');
            returnBtn.className = 'btn btn-warning';
            returnBtn.textContent = 'Report Return & Inspect';
            returnBtn.onclick = () => this.promptReturnReport(t);
            panel.appendChild(returnBtn);
        }

        if (t.status === 'SETTLED') {
            const rateBtn = document.createElement('button');
            rateBtn.className = 'btn btn-secondary';
            rateBtn.textContent = '★ Rate Borrower';
            rateBtn.onclick = () => this.promptRating(t.id);
            panel.appendChild(rateBtn);
        }
    }

    // --- List New Item Form ---
    renderListItem() {
        this.viewElement.innerHTML = `
            <div style="max-width: 600px; margin: 2rem auto;">
                <div class="modal-content" style="box-shadow: var(--shadow-lg);">
                    <h2 style="font-family: var(--font-heading); margin-bottom: 0.5rem;">List an Item for Lending</h2>
                    <p style="color: var(--text-secondary); margin-bottom: 1.5rem; font-size: 0.95rem;">Deposit minimums and borrow limits are enforced by item type.</p>
                    <form id="create-item-form">
                        <div class="form-group">
                            <label class="form-label" for="item-type-select">Item Type</label>
                            <select class="form-control" id="item-type-select" required>
                                <option value="BOOK">Book (Max 7 Days, Min Deposit ₹100, Late Fee ₹5/day)</option>
                                <option value="ELECTRONIC">Electronic (Max 3 Days, Min Deposit ₹500, Late Fee ₹50/day)</option>
                                <option value="EQUIPMENT">Equipment (Max 5 Days, Min Deposit ₹200, Late Fee ₹20/day)</option>
                            </select>
                        </div>
                        <div class="form-group">
                            <label class="form-label" for="new-title">Item Title</label>
                            <input class="form-control" type="text" id="new-title" placeholder="e.g. Raspberry Pi 4 Model B" required>
                        </div>
                        <div style="display: grid; grid-template-columns: 1fr 1fr; gap: 1rem;">
                            <div class="form-group">
                                <label class="form-label" for="new-cat">Category</label>
                                <input class="form-control" type="text" id="new-cat" placeholder="Embedded Systems" required>
                            </div>
                            <div class="form-group">
                                <label class="form-label" for="new-cond">Condition</label>
                                <select class="form-control" id="new-cond">
                                    <option value="GOOD">Good / Like New</option>
                                    <option value="MINOR_DAMAGE">Minor Scratches</option>
                                    <option value="MAJOR_DAMAGE">Heavily Used</option>
                                </select>
                            </div>
                        </div>
                        <div class="form-group">
                            <label class="form-label" for="new-loc">Pickup Location</label>
                            <input class="form-control" type="text" id="new-loc" placeholder="Hostel 3, Room 214" required>
                        </div>
                        <div class="form-group">
                            <label class="form-label" for="new-dep">Security Deposit (₹)</label>
                            <input class="form-control" type="number" id="new-dep" min="100" value="500" required>
                        </div>

                        <!-- Dynamic Type Attributes -->
                        <div id="dynamic-attrs" style="background: var(--bg-page); padding: 1rem; border-radius: var(--radius-md); margin-bottom: 1.25rem;"></div>

                        <button type="submit" class="btn btn-primary btn-full">Publish Listing</button>
                    </form>
                </div>
            </div>
        `;

        const typeSelect = document.getElementById('item-type-select');
        const attrsBox = document.getElementById('dynamic-attrs');

        const updateAttrsFields = () => {
            const t = typeSelect.value;
            if (t === 'BOOK') {
                attrsBox.innerHTML = `
                    <div class="form-group" style="margin-bottom: 0.75rem;">
                        <label class="form-label">Author(s)</label>
                        <input class="form-control" type="text" id="attr-author" placeholder="Cormen et al." required>
                    </div>
                    <div class="form-group" style="margin-bottom: 0;">
                        <label class="form-label">ISBN (10 or 13 digits)</label>
                        <input class="form-control" type="text" id="attr-isbn" placeholder="9780262033848" required>
                    </div>
                `;
            } else if (t === 'ELECTRONIC') {
                attrsBox.innerHTML = `
                    <div class="form-group" style="margin-bottom: 0.75rem;">
                        <label class="form-label">Brand</label>
                        <input class="form-control" type="text" id="attr-brand" placeholder="Raspberry Pi" required>
                    </div>
                    <div class="form-group" style="margin-bottom: 0;">
                        <label class="form-label">Model</label>
                        <input class="form-control" type="text" id="attr-model" placeholder="4B (4GB RAM)" required>
                    </div>
                `;
            } else if (t === 'EQUIPMENT') {
                attrsBox.innerHTML = `
                    <div class="form-group" style="margin-bottom: 0.75rem;">
                        <label class="form-label">Equipment Type</label>
                        <input class="form-control" type="text" id="attr-eqtype" placeholder="Digital Multimeter" required>
                    </div>
                    <div class="form-group" style="margin-bottom: 0;">
                        <label class="form-label">Brand / Spec</label>
                        <input class="form-control" type="text" id="attr-brand" placeholder="Mastech" required>
                    </div>
                `;
            }
        };

        typeSelect.addEventListener('change', updateAttrsFields);
        updateAttrsFields();

        document.getElementById('create-item-form').addEventListener('submit', async (e) => {
            e.preventDefault();
            const type = typeSelect.value;
            const attrs = {};

            if (type === 'BOOK') {
                attrs.author = document.getElementById('attr-author').value;
                attrs.isbn = document.getElementById('attr-isbn').value;
            } else if (type === 'ELECTRONIC') {
                attrs.brand = document.getElementById('attr-brand').value;
                attrs.model = document.getElementById('attr-model').value;
            } else if (type === 'EQUIPMENT') {
                attrs.equipmentType = document.getElementById('attr-eqtype').value;
                attrs.brand = document.getElementById('attr-brand').value;
            }

            const body = {
                type,
                title: document.getElementById('new-title').value,
                category: document.getElementById('new-cat').value,
                condition: document.getElementById('new-cond').value,
                location: document.getElementById('new-loc').value,
                deposit_paise: parseInt(document.getElementById('new-dep').value, 10) * 100,
                attrs
            };

            try {
                await this.api.post('/api/items', body);
                this.showToast('Listing created successfully!');
                window.location.hash = '#browse';
            } catch (err) {
                this.showToast(err.message, 'error');
            }
        });
    }

    // --- Notifications Feed ---
    async renderNotifications() {
        this.viewElement.innerHTML = `
            <div style="max-width: 650px; margin: 1.5rem auto;">
                <h2 style="font-family: var(--font-heading); margin-bottom: 1.5rem;">Notifications & Activity Log</h2>
                <div id="notif-feed" style="display: flex; flex-direction: column; gap: 0.75rem;">
                    <div class="spinner" style="margin: 2rem auto;"></div>
                </div>
            </div>
        `;

        try {
            const data = await this.api.get('/api/notifications');
            const feed = document.getElementById('notif-feed');
            feed.innerHTML = '';

            if (!data.notifications || data.notifications.length === 0) {
                feed.innerHTML = `<p style="color: var(--text-muted); text-align: center; padding: 2rem;">No notifications yet.</p>`;
                return;
            }

            for (const n of data.notifications) {
                const item = document.createElement('div');
                item.className = 'modal-content';
                item.style.padding = '1rem 1.25rem';
                item.style.borderLeft = n.isRead ? '1px solid var(--border)' : '4px solid var(--primary)';

                item.innerHTML = `
                    <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 0.35rem;">
                        <span style="font-weight: 700; font-size: 0.95rem;">${n.kind}</span>
                        <span style="font-size: 0.75rem; color: var(--text-muted);">${new Date(n.createdAt * 1000).toLocaleString()}</span>
                    </div>
                    <p style="font-size: 0.9rem; color: var(--text-secondary); margin-bottom: 0.5rem;" id="notif-payload-${n.id}"></p>
                    ${!n.isRead ? `<button class="btn btn-secondary" style="font-size: 0.8rem; padding: 0.25rem 0.6rem;" onclick="window.BorrowHubApp.markNotifRead(${n.id})">Mark as Read</button>` : ''}
                `;

                // Render JSON payload safely
                item.querySelector(`#notif-payload-${n.id}`).textContent = JSON.stringify(n.payload);
                feed.appendChild(item);
            }
        } catch (err) {
            this.showToast(err.message, 'error');
        }
    }

    async markNotifRead(id) {
        try {
            await this.api.post(`/api/notifications/${id}/read`);
            this.renderNotifications();
            this.pollNotifications();
        } catch (err) { this.showToast(err.message, 'error'); }
    }

    // --- Admin Dashboard View ---
    async renderAdmin() {
        this.viewElement.innerHTML = `
            <div style="max-width: 900px; margin: 1.5rem auto;">
                <h2 style="font-family: var(--font-heading); margin-bottom: 0.5rem;">Campus Administration Console</h2>
                <p style="color: var(--text-secondary); margin-bottom: 1.5rem;">Review dispute evidence and execute mechanical deposit resolutions.</p>
                
                <h3 style="font-family: var(--font-heading); margin-bottom: 1rem;">Open Disputes</h3>
                <div id="dispute-list" style="display: flex; flex-direction: column; gap: 1rem;">
                    <div class="spinner" style="margin: 2rem auto;"></div>
                </div>
            </div>
        `;

        try {
            const data = await this.api.get('/api/admin/disputes');
            const list = document.getElementById('dispute-list');
            list.innerHTML = '';

            if (!data.disputes || data.disputes.length === 0) {
                list.innerHTML = `<p style="color: var(--text-muted); padding: 2rem; text-align: center;">No disputes currently pending review.</p>`;
                return;
            }

            for (const d of data.disputes) {
                const card = document.createElement('div');
                card.className = 'modal-content';
                card.style.maxWidth = '100%';

                card.innerHTML = `
                    <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 0.5rem;">
                        <span style="font-weight: 700;">Dispute #${d.id} (Txn #${d.txnId})</span>
                        <span class="item-status-pill status-${d.status.toLowerCase()}">${d.status}</span>
                    </div>
                    <p style="margin-bottom: 0.5rem; font-size: 0.95rem;"><strong>Raised By:</strong> User #${d.raisedBy}</p>
                    <p style="background: var(--bg-page); padding: 0.75rem; border-radius: var(--radius-md); font-size: 0.9rem; margin-bottom: 1rem;" id="disp-reason-${d.id}"></p>
                    ${d.status === 'OPEN' ? `<button class="btn btn-primary" onclick="window.BorrowHubApp.promptAdminDecide(${d.id})">Decide Dispute Split</button>` : `<p style="font-size: 0.85rem; color: var(--text-muted);">Decided by Admin #${d.decisionNote}</p>`}
                `;

                card.querySelector(`#disp-reason-${d.id}`).textContent = d.reason;
                list.appendChild(card);
            }
        } catch (err) {
            this.showToast(err.message, 'error');
        }
    }

    // --- Action Dialogs (Return Inspection, Dispute, Rating) ---

    promptReturnReport(txn) {
        this.openModal(`
            <div class="modal-header">
                <h3 style="font-family: var(--font-heading);">Return Inspection & Damage Assessment</h3>
                <button class="close-btn" onclick="window.BorrowHubApp.closeModal()">&times;</button>
            </div>
            <form id="return-report-form">
                <div class="form-group">
                    <label class="form-label" for="ret-cond">Physical Condition</label>
                    <select class="form-control" id="ret-cond" required>
                        <option value="GOOD">Good (No damage - claim is ₹0)</option>
                        <option value="MINOR_DAMAGE">Minor Damage (Scratches / cosmetic)</option>
                        <option value="MAJOR_DAMAGE">Major Damage (Broken parts / repair needed)</option>
                        <option value="LOST">Lost / Unreturned Item</option>
                    </select>
                </div>
                <div class="form-group">
                    <label class="form-label" for="ret-claim">Damage Claim (₹) - Max ₹${txn.depositPaise / 100}</label>
                    <input class="form-control" type="number" id="ret-claim" min="0" max="${txn.depositPaise / 100}" value="0" required>
                </div>
                <button type="submit" class="btn btn-primary btn-full">Submit Return Assessment</button>
            </form>
        `);

        const condSelect = document.getElementById('ret-cond');
        const claimInput = document.getElementById('ret-claim');

        condSelect.addEventListener('change', () => {
            if (condSelect.value === 'GOOD') {
                claimInput.value = '0';
                claimInput.disabled = true;
            } else {
                claimInput.disabled = false;
            }
        });
        claimInput.disabled = true;

        document.getElementById('return-report-form').addEventListener('submit', async (e) => {
            e.preventDefault();
            const cond = condSelect.value;
            const claimPaise = parseInt(claimInput.value, 10) * 100;

            try {
                await this.api.post(`/api/requests/${txn.id}/report-return`, {
                    condition: cond,
                    claim_paise: claimPaise
                });
                this.showToast('Return report submitted! 48h settlement timer started.');
                this.closeModal();
                this.renderMyListings();
            } catch (err) { this.showToast(err.message, 'error'); }
        });
    }

    promptDispute(txnId) {
        this.openModal(`
            <div class="modal-header">
                <h3 style="font-family: var(--font-heading); color: var(--danger);">Raise Dispute with Campus Admin</h3>
                <button class="close-btn" onclick="window.BorrowHubApp.closeModal()">&times;</button>
            </div>
            <p style="font-size: 0.9rem; color: var(--text-secondary); margin-bottom: 1rem;">
                Raising a dispute freezes held deposit funds and forwards your negotiation chat log and condition reports to campus moderators.
            </p>
            <form id="dispute-form">
                <div class="form-group">
                    <label class="form-label" for="disp-text">Reason & Evidence Statement</label>
                    <textarea class="form-control" id="disp-text" rows="4" maxlength="2000" placeholder="Explain discrepancy in damage claim or handover..." required></textarea>
                </div>
                <button type="submit" class="btn btn-danger btn-full">Submit Formal Dispute</button>
            </form>
        `);

        document.getElementById('dispute-form').addEventListener('submit', async (e) => {
            e.preventDefault();
            const reason = document.getElementById('disp-text').value.trim();

            try {
                await this.api.post(`/api/requests/${txnId}/dispute`, { reason });
                this.showToast('Dispute raised. Funds frozen pending admin decision.');
                this.closeModal();
                this.renderMyBorrows();
            } catch (err) { this.showToast(err.message, 'error'); }
        });
    }

    promptRating(txnId) {
        this.openModal(`
            <div class="modal-header">
                <h3 style="font-family: var(--font-heading);">Submit Mutual Rating</h3>
                <button class="close-btn" onclick="window.BorrowHubApp.closeModal()">&times;</button>
            </div>
            <form id="rating-form">
                <div class="form-group">
                    <label class="form-label" for="star-count">Stars (1 to 5)</label>
                    <select class="form-control" id="star-count" required>
                        <option value="5">★★★★★ (5 Stars - Exceptional)</option>
                        <option value="4">★★★★☆ (4 Stars - Good)</option>
                        <option value="3">★★★☆☆ (3 Stars - Average)</option>
                        <option value="2">★★☆☆☆ (2 Stars - Poor)</option>
                        <option value="1">★☆☆☆☆ (1 Star - Problematic)</option>
                    </select>
                </div>
                <div class="form-group">
                    <label class="form-label" for="rate-comment">Feedback Comment (Optional)</label>
                    <input class="form-control" type="text" id="rate-comment" placeholder="Punctual and kept item in pristine shape.">
                </div>
                <button type="submit" class="btn btn-primary btn-full">Submit Review</button>
            </form>
        `);

        document.getElementById('rating-form').addEventListener('submit', async (e) => {
            e.preventDefault();
            const stars = parseInt(document.getElementById('star-count').value, 10);
            const comment = document.getElementById('rate-comment').value.trim();

            try {
                await this.api.post(`/api/requests/${txnId}/rating`, { stars, comment });
                this.showToast('Rating submitted! Reputation scores updated.');
                this.closeModal();
                this.renderMyBorrows();
            } catch (err) { this.showToast(err.message, 'error'); }
        });
    }

    promptAdminDecide(disputeId) {
        this.openModal(`
            <div class="modal-header">
                <h3 style="font-family: var(--font-heading);">Execute Dispute Decision</h3>
                <button class="close-btn" onclick="window.BorrowHubApp.closeModal()">&times;</button>
            </div>
            <form id="admin-decide-form">
                <div class="form-group">
                    <label class="form-label" for="award-amount">Owner Award (₹) - Balance Refunded to Borrower</label>
                    <input class="form-control" type="number" id="award-amount" min="0" value="0" required>
                </div>
                <div class="form-group">
                    <label class="form-label" for="decide-note">Decision Note / Findings</label>
                    <textarea class="form-control" id="decide-note" rows="3" required placeholder="Evidence demonstrates minor damage. Split approved per dispute policy."></textarea>
                </div>
                <button type="submit" class="btn btn-primary btn-full">Finalize Split & Settle Ledger</button>
            </form>
        `);

        document.getElementById('admin-decide-form').addEventListener('submit', async (e) => {
            e.preventDefault();
            const awardPaise = parseInt(document.getElementById('award-amount').value, 10) * 100;
            const note = document.getElementById('decide-note').value.trim();

            try {
                await this.api.post(`/api/admin/disputes/${disputeId}/decide`, {
                    owner_award_paise: awardPaise,
                    decision_note: note
                });
                this.showToast('Dispute resolved! Ledger settlement executed.');
                this.closeModal();
                this.renderAdmin();
            } catch (err) { this.showToast(err.message, 'error'); }
        });
    }

    openModal(html) {
        this.closeModal();
        const overlay = document.createElement('div');
        overlay.className = 'modal-overlay';
        overlay.id = 'active-modal-overlay';
        overlay.innerHTML = `<div class="modal-content">${html}</div>`;

        overlay.addEventListener('click', (e) => {
            if (e.target === overlay) this.closeModal();
        });

        document.body.appendChild(overlay);
        this.activeModal = overlay;
    }

    closeModal() {
        if (this.activeModal) {
            this.activeModal.remove();
            this.activeModal = null;
        }
    }
}

// Global bootstrap
document.addEventListener('DOMContentLoaded', () => {
    window.BorrowHubApp = new App();
    window.BorrowHubApp.init();
});
