# BorrowHub Mechanical Dispute Policy

## 1. Overview & Objective
Disputes in peer-to-peer campus item sharing arise primarily during return inspection when an owner claims damage or late deductions, and the borrower contends the assessment.
To minimize subjective friction and administrative overhead, BorrowHub enforces a **mechanical, evidence-based dispute policy** with strict time bounds.

---

## 2. Windows of Action
1. **Return Report Submission**:
   - When an item is returned, the owner marks condition (`GOOD`, `MINOR_DAMAGE`, `MAJOR_DAMAGE`, `LOST`) and specifies a `claim_paise` amount (up to `deposit_paise - penalty_paise`).
2. **Borrower 48-Hour Review Window**:
   - The borrower has exactly **48 hours (172,800 seconds)** from `returned_at` to:
     - **Accept Settlement**: Immediately executes deposit refund and payment deductions.
     - **Raise Dispute**: Freezes remaining deposit funds, transitions transaction state to `DISPUTED`, and routes the matter to administrators.
3. **Auto-Settlement Default**:
   - If the borrower takes no action within 48 hours, the background `Scheduler` automatically invokes `SettlementService::settle()` in favor of the owner's filed report.

---

## 3. Evidence Requirements
When a dispute is raised, the following audit trail is reviewed by the campus administrator:
1. **Thread Chat Log**: Unalterable, append-only negotiation records and initial condition disclosures made prior to handover.
2. **Handover Confirmations**: Dual-party digital sign-offs confirming the item was in borrower possession.
3. **Dispute Reason & Return Notes**: Condition statements recorded by both parties.

---

## 4. Decision Options & Financial Outcomes
Admins choose from three deterministic financial split formulas:

| Outcome | Owner Award (`owner_award_paise`) | Borrower Refund (`refund_paise`) | Rationale / Mapping |
| :--- | :--- | :--- | :--- |
| **Full Refund** | `0` | `deposit_paise - penalty_paise` | Claim lacks evidence or item returned in pre-agreed condition. |
| **Split Award** | `X` (where `0 < X < deposit - penalty`) | `deposit_paise - penalty_paise - X` | Partial shared responsibility or minor wear-and-tear. |
| **Full Award** | `claim_paise` | `deposit_paise - penalty_paise - claim_paise` | Clear evidence of borrower damage or missing components. |

### Conservation of Funds Invariant
In every outcome:
$$\text{owner\_award\_paise} + \text{refund\_paise} + \text{penalty\_paise} = \text{deposit\_paise}$$
Under no circumstances can an admin award exceed `deposit_paise - penalty_paise` or create a negative refund balance.

---

## 5. Tie-Break & Default Rules
1. If the owner provided no notes/evidence at return time and the condition was marked `GOOD`, any subsequent claim is rejected and full refund is granted.
2. If late penalty applies (`due_date < returned_at`), the late penalty is mathematically non-negotiable and deducted automatically prior to dispute allocation.
3. Once an admin submits a decision with `decision_note`, the transaction is marked `SETTLED`, ledger entries are immutably written (`DEDUCT`, `PAYOUT`, `REFUND`), and both parties are notified.
