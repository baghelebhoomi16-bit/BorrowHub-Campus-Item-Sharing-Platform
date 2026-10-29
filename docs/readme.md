# Documentation (`docs/`)

## Purpose
This directory houses architecture documents, database entity-relationship models, API specifications, and operational policies for BorrowHub.

## Files
- `api.md`: Comprehensive REST API route specifications, request/response JSON schemas, and HTTP status codes.
- `er-diagram.md`: Entity-relationship diagram representing table relationships, constraints, and audit trails.
- `dispute-policy.md`: Rules and mathematical settlement formulas governing peer dispute resolution.
- `readme.md`: Documentation catalog and index.
- `implementation.md`: Procedures for documentation validation and keeping documentation synchronized with code.

## How to Validate Documentation
To ensure all API routes registered in `src/api/Router.cpp` are documented in `docs/api.md`:
```bash
./scripts/check_api_docs.sh
```

## Dependencies
- Standard Markdown and Mermaid-compatible renderers (GitHub, VS Code, or IDE viewers).

## Callers
- Developers, examiners, reviewers, and CI integrity scripts (`check_api_docs.sh`).
