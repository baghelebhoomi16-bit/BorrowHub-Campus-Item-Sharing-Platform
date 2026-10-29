/**
 * BorrowHub Polling Chat Controller
 * 3-second polling interval with exponential backoff and document visibility awareness.
 * Strict XSS defense: User message content rendered exclusively using textContent.
 */

class ChatClient {
    constructor(apiClient) {
        this.api = apiClient;
        this.activeThreadId = null;
        this.lastMessageId = 0;
        this.pollTimer = null;
        this.pollIntervalMs = 3000;
        this.currentDelayMs = 3000;
        this.maxDelayMs = 30000;
        this.isPolling = false;
        this.containerElement = null;

        // Pause polling when document tab is hidden
        document.addEventListener('visibilitychange', () => {
            if (document.hidden) {
                this.pause();
            } else if (this.activeThreadId) {
                this.resume();
            }
        });
    }

    start(threadId, containerElement) {
        this.stop();
        this.activeThreadId = threadId;
        this.containerElement = containerElement;
        this.lastMessageId = 0;
        this.currentDelayMs = this.pollIntervalMs;
        this.isPolling = true;

        this.renderChatShell();
        this.fetchMessages();
    }

    stop() {
        this.isPolling = false;
        if (this.pollTimer) {
            clearTimeout(this.pollTimer);
            this.pollTimer = null;
        }
        this.activeThreadId = null;
        this.lastMessageId = 0;
    }

    pause() {
        if (this.pollTimer) {
            clearTimeout(this.pollTimer);
            this.pollTimer = null;
        }
    }

    resume() {
        if (this.isPolling && this.activeThreadId && !this.pollTimer) {
            this.fetchMessages();
        }
    }

    renderChatShell() {
        if (!this.containerElement) return;

        this.containerElement.innerHTML = `
            <div class="chat-container">
                <div class="chat-messages" id="chat-msg-list" aria-live="polite"></div>
                <form class="chat-input-bar" id="chat-form">
                    <input type="text" id="chat-input" placeholder="Type a message to negotiate..." maxlength="2000" autocomplete="off" required>
                    <button type="submit" class="btn btn-primary" id="chat-send-btn">Send</button>
                </form>
            </div>
        `;

        const form = this.containerElement.querySelector('#chat-form');
        const input = this.containerElement.querySelector('#chat-input');

        form.addEventListener('submit', async (e) => {
            e.preventDefault();
            const text = input.value.trim();
            if (!text || !this.activeThreadId) return;

            try {
                input.disabled = true;
                await this.api.post(`/api/threads/${this.activeThreadId}/messages`, { body: text });
                input.value = '';
                // Immediately poll for new messages
                this.fetchMessages();
            } catch (err) {
                window.BorrowHubApp?.showToast('Failed to send message: ' + err.message, 'error');
            } finally {
                input.disabled = false;
                input.focus();
            }
        });
    }

    async fetchMessages() {
        if (!this.isPolling || !this.activeThreadId || document.hidden) return;

        try {
            const data = await this.api.get(`/api/threads/${this.activeThreadId}/messages?after_id=${this.lastMessageId}`);
            
            if (data && data.messages && data.messages.length > 0) {
                this.appendMessages(data.messages);
                const highestId = data.messages[data.messages.length - 1].id;
                if (highestId > this.lastMessageId) {
                    this.lastMessageId = highestId;
                }
            }
            // Reset delay on successful poll
            this.currentDelayMs = this.pollIntervalMs;
        } catch (err) {
            // Exponential backoff up to 30 seconds
            this.currentDelayMs = Math.min(this.currentDelayMs * 2, this.maxDelayMs);
            console.warn(`[CHAT] Polling error, backing off to ${this.currentDelayMs}ms:`, err.message);
        } finally {
            if (this.isPolling) {
                this.pollTimer = setTimeout(() => this.fetchMessages(), this.currentDelayMs);
            }
        }
    }

    appendMessages(messages) {
        const list = document.getElementById('chat-msg-list');
        if (!list) return;

        const currentUserId = window.BorrowHubApp?.currentUser?.id;

        for (const msg of messages) {
            // Deduplicate if already present in DOM
            if (document.getElementById(`msg-${msg.id}`)) continue;

            const bubble = document.createElement('div');
            bubble.id = `msg-${msg.id}`;
            bubble.className = `chat-bubble ${msg.senderId === currentUserId ? 'mine' : 'theirs'}`;

            const senderElem = document.createElement('div');
            senderElem.className = 'chat-sender';
            // CRITICAL: Use textContent for sender and body (prevents XSS)
            senderElem.textContent = msg.senderName || `User #${msg.senderId}`;

            const bodyElem = document.createElement('div');
            bodyElem.className = 'chat-body';
            bodyElem.textContent = msg.body;

            bubble.appendChild(senderElem);
            bubble.appendChild(bodyElem);
            list.appendChild(bubble);
        }

        // Auto-scroll to latest message
        list.scrollTop = list.scrollHeight;
    }
}

window.ChatClient = ChatClient;
