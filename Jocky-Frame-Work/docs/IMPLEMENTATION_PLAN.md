# Implementation Plan: JOCKY Framework for Enterprise Forensics

## Context
This plan outlines the architecture for JOCKY, a custom Domain-Specific Language (DSL) and execution framework for deep forensic system analysis. Designed as an enterprise-grade incident response tool, JOCKY allows security teams to rapidly deploy forensic queries to endpoints securely and reliably.

## Objective
Create a cross-platform (Windows & Linux) forensic analysis framework that:
1. Implements a custom `.jocky` query language using an LLVM backend.
2. Safely gathers system telemetry (processes, network, files) using authorized OS APIs.
3. Provides a secure central management interface for dispatching scripts and aggregating results.

## Architecture Overview

### Layer 1: Language Parsing & Compilation
- **Parser:** Flex/Bison or Python `ply` to parse `.jocky` scripts into an Abstract Syntax Tree (AST).
- **Backend:** LLVM IR generation ensures that complex forensic queries are executed with compiled C++ performance, critical for scanning large filesystems or memory safely.

### Layer 2: Authorized Telemetry Collection (The Runner)
- **Windows Integration:** Utilizes WMI, standard Win32 APIs, and Event Tracing for Windows (ETW) for process and network visibility.
- **Linux Integration:** Utilizes `/proc` filesystem and standard POSIX APIs.
- **Safety Guarantees:** Script execution is strictly sandboxed to read-only operations to prevent system instability during forensic data collection.

### Layer 3: Secure Management & Transport
- **Protocol:** HTTPS with mutual TLS (mTLS) or JWT-based authentication.
- **Payload Securement:** Scripts are cryptographically signed by the C2 server before dispatch; agents refuse unsigned instructions.
- **Telemetry Aggregation:** Forensic output is serialized to JSON and delivered to the central server for dashboarding.

## Development Phases

### Phase 1: Compiler Foundation (CLI & AST)
- Define the JOCKY language grammar (variables, forensic primitives).
- Implement lexer and parser.
- Build the LLVM IR code generator.

### Phase 2: Agent Telemetry Modules
- Develop the C++/Python runtime environment that executes compiled JOCKY IR.
- Implement read-only OS modules (File hashing, Network connection listing, Process enumeration).

### Phase 3: Central Management Application
- Build the Python/FastAPI backend server.
- Establish secure agent-server communication routing.
- Create script deployment endpoints.

### Phase 4: Integration & CI/CD
- Set up unit tests for compiler output.
- Link the pipeline so changes are documented, tested, and tracked on Git.
