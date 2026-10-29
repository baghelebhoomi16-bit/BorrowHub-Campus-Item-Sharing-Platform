# Domain Layer (`src/domain/`)

## Purpose
The domain layer encapsulates core campus business models, state machines, monetary value objects, and penalty policies. It is completely isolated from I/O, database systems, and networking protocols, making every business rule 100% testable in memory.

## Class Diagram
```
         +-------------------+
         |       User        | (abstract)
         +-------------------+
               ^       ^
               |       |
     +---------+       +---------+
     |                           |
+---------+                 +---------+
| Student |                 |  Admin  |
+---------+                 +---------+

         +-------------------+
         |       Item        | (abstract)
         +-------------------+
          ^        ^        ^
          |        |        |
    +-----+   +----+----+   +-----+
    |         |         |         |
+------+ +------------+ +-----------+
| Book | | Electronic | | Equipment |
+------+ +------------+ +-----------+

+-------------------+     +-------------------+
|       Money       |     |   PenaltyPolicy   |
+-------------------+     +-------------------+

+-------------------+     +-------------------+
| BorrowTransaction |     |    Reputation     |
|  (State Machine)  |     +-------------------+
+-------------------+
```

## Files
- `Money.hpp`: 64-bit integer paise monetary value object with overflow prevention.
- `User.hpp` / `User.cpp`: Abstract user entity.
- `Student.hpp` / `Student.cpp`: Student entity with borrowing eligibility rule (`canRequest`).
- `Admin.hpp` / `Admin.cpp`: Administrator entity with dispute and moderation roles.
- `Item.hpp` / `Item.cpp`: Abstract item hierarchy defining polymorphic attributes.
- `Book.hpp`: Book item subclass (7 max days, 5 INR/day late penalty, 100 INR min deposit).
- `Electronic.hpp`: Electronic item subclass (3 max days, 50 INR/day late penalty, 500 INR min deposit).
- `Equipment.hpp`: Equipment item subclass (5 max days, 20 INR/day late penalty, 200 INR min deposit).
- `ItemFactory.hpp` / `ItemFactory.cpp`: Factory creating polymorphic Item subclasses from JSON.
- `PenaltyPolicy.hpp` / `PenaltyPolicy.cpp`: Ceiling day penalty calculation with deposit caps.
- `BorrowTransaction.hpp` / `BorrowTransaction.cpp`: Definitive lifecycle state machine.
- `Reputation.hpp` / `Reputation.cpp`: Bayesian rating average with Laplace smoothed reliability rates.
- `readme.md`: Domain catalog and class hierarchy overview.
- `implementation.md`: Explicit documentation of OOP principles and design decisions.

## How to Build and Run
Compiled as part of `borrowhub_core` and tested via `tests/unit/test_*.cpp`.

## Dependencies
- Standard C++17 library and `src/util/`. Zero external dependencies.
