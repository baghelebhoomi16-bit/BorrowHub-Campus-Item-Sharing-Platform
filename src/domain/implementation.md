# Domain Implementation Notes & OOP Architecture (`src/domain/`)

## Object-Oriented Programming (OOP) Alignment

### 1. Encapsulation
- All fields in `BorrowTransaction`, `Money`, `User`, and `Item` are strictly private or protected.
- Invariant-checking mutators reject invalid states (e.g. `setDeposit()` refuses values below the item type minimum, `Money` addition checks for integer overflow before calculating, `reportReturn()` rejects claims exceeding deposits).

### 2. Inheritance
- `User` is the abstract base for `Student` and `Admin`. Common identity attributes (id, email, password hash, name, phone, verified, blocked) live in `User`, while academic department, PRN, division, and reputation metrics live in `Student`.
- `Item` is the abstract base for `Book`, `Electronic`, and `Equipment`. Common listing fields (title, category, location, deposit, availability window, status) live in `Item`.

### 3. Polymorphism
Rather than checking switch statements or `type == 'BOOK'`, item behavior is purely polymorphic:
- `Item::maxBorrowDays()`: Returns 7 for `Book`, 3 for `Electronic`, 5 for `Equipment`.
- `Item::penaltyPerDay()`: Returns ₹5 for `Book`, ₹50 for `Electronic`, ₹20 for `Equipment`.
- `Item::minDeposit()`: Returns ₹100 for `Book`, ₹500 for `Electronic`, ₹200 for `Equipment`.
- Calling `item->penaltyPerDay()` via an `Item*` executes the derived class method dynamically.

### 4. Abstraction
- Complex state transitions and multi-step checks are abstracted inside `BorrowTransaction`. External callers cannot set status directly; they must call transition methods (`accept`, `payDeposit`, `confirmHandover`, `reportReturn`, `raiseDispute`, `settle`).

## Why Integer Money (`paise`)
- Floating point math introduces representation artifacts (e.g. `100.0 - 99.9 = 0.09999999999999432`). In financial ledgers, audit balance requires `HOLD == REFUND + DEDUCT` exactness. Storing `paise` as signed 64-bit integers guarantees exact math up to trillions of rupees.

## State Machine Invariant
- `BorrowTransaction` is the sole authority for lifecycle progression.
- Any illegal transition throws `AppError(ILLEGAL_STATE)`.
- Unauthorized actors throw `AppError(FORBIDDEN)`.
- Handover confirmation requires dual affirmative sign-offs (`ownerConfirmed && borrowerConfirmed`) before activating.
