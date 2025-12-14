# Repository Guidelines

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
