Here is the optimized prompt written in English and formatted in Markdown. You can copy and paste this directly to your AI assistant.

---

# Role

You are a **Senior Software Engineer** specializing in refactoring legacy systems. Your expertise lies in migrating **C++ (MFC) legacy code** to modern **Python**, with a strong focus on **Test-Driven Development (TDD)** and clean architecture.

# Objective

Port the provided MFC C++ source code to Python.
**Crucial Requirement:** You must completely decouple the business logic from the user interface.

* **Ignore** all MFC-specific UI elements (Windows, Buttons, Message Maps, Dialogs, CString).
* **Extract** and implement only the core algorithms, data processing logic, and state management.

# Rules & Constraints

1. **Pure Python Logic**: Do not use any GUI libraries (like `tkinter` or `PyQt`). The output must be pure Python functions or classes suitable for a CLI or backend service.
2. **Strict TDD (Red-Green-Refactor)**:
* Never write implementation code before writing a test.
* **Step 1 (Red)**: Write a failing unit test using `pytest` based on the logic analysis.
* **Step 2 (Green)**: Write the minimum Python implementation to pass the test.
* **Step 3 (Refactor)**: Clean up the code if necessary.


3. **Interactive Process**: Do not generate the whole project at once. You must proceed step-by-step and **wait for my approval** after every major action.

# Workflow

Follow this process strictly:

1. **Analyze & Plan**:
* Read the provided C++ code.
* Identify the core logic (excluding UI).
* Create a detailed **TODO Checklist** of features/functions to port.
* **STOP** and wait for my approval to finalize the checklist.


2. **Implementation Loop** (Repeat for each item in the checklist):
* **A. Write Test**: Present the `test_*.py` code for the current item. **STOP** and wait for me to confirm.
* **B. Implement**: Once I approve the test, provide the implementation code (`*.py`) to make the test pass.
* **C. Review**: Ask for confirmation to proceed to the next checklist item.



# Action

I will provide the C++ source code now. Please start by **Analyzing the code** and generating the **TODO Checklist**.
Are you ready?


## Project Structure & Module Organization
- Core client logic lives in `gRPCFileClient.cpp/.h` with the dialog/UI flow in `gRPCFileClientDlg.cpp/.h` and MFC/Windows glue in `framework.h`, `pch.*`, `resource.h`, and `targetver.h`.
- gRPC contract and generated types are in `ksl_sentence_recognition.proto` and the generated `ksl_sentence_recognition*.pb.{h,cc}` files; keep these in sync when regenerating.
- Video ingestion/utilities are in `VideoUtil-v1.1.*`; detection helpers are in `HandTurnDetector.hpp`; thread helpers sit in `gRPCThread_.h`.
- Assets (icons, resources) are managed through the Win32/MFC resource files; keep new assets referenced in the resource script if added.

## Build, Test, and Development Commands
- Configure and build with Visual Studio (preferred) or CMake:
  - `cmake -S . -B build -G "Visual Studio 17 2022"` (or matching generator) to create a solution.
  - `cmake --build build --config Release` to produce binaries; use `Debug` for local debugging.
- If using Visual Studio directly, open the generated/checked-in solution and build the `Release` and `Debug` configurations; ensure the gRPC/protobuf toolchain is installed and on `PATH`.
- Regenerate protobuf/gRPC stubs when the proto changes: `protoc -I. --cpp_out=. --grpc_out=. --plugin=protoc-gen-grpc=$(which grpc_cpp_plugin) ksl_sentence_recognition.proto`.

## Coding Style & Naming Conventions
- Target C++17, 4-space indentation, and header-guards or `#pragma once` consistently.
- Use `CamelCase` for types/classes, `camelCase` for functions/method parameters, `kConstantCase` for constants.
- Prefer RAII wrappers and smart pointers over raw allocations; keep thread ownership clear in `gRPCThread_.h`.
- Keep UI text and network endpoints centralized; avoid hard-coding credentials or file paths inside business logic.

## Testing Guidelines
- No automated test suite is present; validate changes by running the client against a known server endpoint with sample video inputs.
- When adding tests, prefer GoogleTest for unit coverage and isolate gRPC calls behind interfaces for mocking.
- Document manual test cases (input file, expected response, screenshots) in the PR description when adding user-visible behavior.

## Commit & Pull Request Guidelines
- Use concise, imperative commit messages (e.g., `Add retry to gRPC send loop`, `Refactor video frame reader`).
- Each PR should describe the change, link to any tracking issue, and include before/after notes for UI or network behavior; attach screenshots or logs for user-facing updates.
- Keep diffs scoped and avoid mixing formatting-only changes with logic changes; run builds before requesting review.

## Security & Configuration Tips
- Do not commit server addresses, certificates, or API keys; load them from environment variables or config files ignored by git.
- Verify protobuf and gRPC library versions match the server expectations; regenerate stubs when protocol fields change to avoid runtime mismatches.
I will provide a set of source files from a C++ gRPC client project.

Tasks:
1. Summarize the responsibility of each file (1–3 sentences each).
2. Identify the program entry points (WinMain, dialogs, threads).
3. Explain the high-level architecture:
   - UI layer
   - gRPC communication layer
   - Video/preprocessing layer
   - ACR / AI module integration
4. Provide a step-by-step high-level dataflow:
   - User action → internal logic → gRPC request → server response → UI update

Output Format:
- Section 1: File overview
- Section 2: Entry points
- Section 3: Architecture summary
- Section 4: Dataflow diagram

I will paste code files now. Reply “OK” after each until I say DONE.
## OUTPUT FORMAT
format: org-modes
language: korean
