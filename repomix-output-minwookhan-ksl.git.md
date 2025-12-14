This file is a merged representation of the entire codebase, combined into a single document by Repomix.
The content has been processed where security check has been disabled.

# File Summary

## Purpose
This file contains a packed representation of the entire repository's contents.
It is designed to be easily consumable by AI systems for analysis, code review,
or other automated processes.

## File Format
The content is organized as follows:
1. This summary section
2. Repository information
3. Directory structure
4. Repository files (if enabled)
5. Multiple file entries, each consisting of:
  a. A header with the file path (## File: path/to/file)
  b. The full contents of the file in a code block

## Usage Guidelines
- This file should be treated as read-only. Any changes should be made to the
  original repository files, not this packed version.
- When processing this file, use the file path to distinguish
  between different files in the repository.
- Be aware that this file may contain sensitive information. Handle it with
  the same level of security as you would the original repository.

## Notes
- Some files may have been excluded based on .gitignore rules and Repomix's configuration
- Binary files are not included in this packed representation. Please refer to the Repository Structure section for a complete list of file paths, including binary files
- Files matching patterns in .gitignore are excluded
- Files matching default ignore patterns are excluded
- Security check has been disabled - content may contain sensitive information
- Files are sorted by Git change count (files with more changes are at the bottom)

# Directory Structure
```
1_propmt_프로젝트구조.md
2_prompt.md
AGENTS.md
anal_note.org
conversation_log.md
framework.h
gptel_conversation.org
gRPCFileClient.cpp
gRPCFileClient.h
gRPCFileClientDlg.cpp
gRPCFileClientDlg.h
gRPCThread_.h
HandTurnDetector.hpp
ksl_sentence_recognition.grpc.pb.cc
ksl_sentence_recognition.grpc.pb.h
ksl_sentence_recognition.pb.cc
ksl_sentence_recognition.pb.h
ksl_sentence_recognition.proto
pch.cpp
pch.h
proto_data_structure.md
README.md
resource.h
targetver.h
VideoUtil-v1.1.cpp
VideoUtil-v1.1.h
```

# Files

## File: 1_propmt_프로젝트구조.md
```markdown
I will provide a set of source files from a C++ gRPC client project.

## Tasks:
1. Summarize the responsibility of each file (1–3 sentences each).
2. Identify the program entry points (WinMain, dialogs, threads).
3. Explain the high-level architecture:
   - UI layer
   - gRPC communication layer
   - Video/preprocessing layer
   - ACR / AI module integration
4. Provide a step-by-step high-level dataflow:
   - User action → internal logic → gRPC request → server response → UI update

### Output Format:
- Section 1: File overview
- Section 2: Entry points
- Section 3: Architecture summary
- Section 4: Dataflow diagram

I will paste code files now. Reply “OK” after each until I say DONE.
```

## File: 2_prompt.md
```markdown
You are analyzing the gRPC and protobuf layer.
## Strong rule
- 서술어 사용하지 말 것.
- 한국어로 작성 
## Tasks:
1. Extract all service and message definitions from the .proto file.
2. For each RPC:
   - Describe its purpose.
   - List its Request and Response fields.
3. From the generated C++ code:
   - Show how the client stub is constructed.
   - Identify how each RPC is invoked.
4. Connect these to the client code:
   - Where the stub is created.
   - Where each request is constructed.
   - Where each response is processed.

## Output:
- Services summary
- RPC details
- C++ stub mapping
- Integration points in gRPCFileClient
```

## File: AGENTS.md
```markdown
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
```

## File: anal_note.org
```
* proto structure
#+begin_src proto
syntax = "proto3";
package vision.raw.v1;

service SequenceService {
  rpc SendFrames (stream Frame) returns (SubmitResultResponse);
}

message Point3 {
  float x = 1;
  float y = 2;
  float z = 3;
}

message Frame {
  string session_id = 1;
  int32 index = 2;
  int32 flag = 3; // 0: START, 1: NORMAL, 2: END
  int32 width = 4;
  int32 height = 5;
  int32 type = 6;
  bytes data = 7;
  repeated Point3 pose_points = 8;
}

message SubmitResultResponse {
  string session_id = 1;
  int32 frame_count = 2;
  string message = 3;
}
 session_id(string): 세션 식별자
  - index(int32): 프레임 번호
  - flag(int32): 상태 플래그(0=START, 1=NORMAL, 2=END)
  - width/height/type(int32): 프레임 크기와 OpenCV 타입 값
  - data(bytes): 프레임 이미지 바이너리
  - pose_points(repeated Point3): 포즈 좌표 목록(x,y,z float)

#+end_src
#+begin_src markdown
**`ksl_sentence_recognition.proto` 파일에 정의된 데이터 구조 및 역할:**

1.  **`Point3` 메시지**
    *   **구조**: `float x`, `float y`, `float z` 세 개의 부동 소수점 필드를 가집니다.
    *   **역할**: 3차원 공간에서의 한 지점(예: MediaPipe로 추출된 스켈레톤 관절의 3D 좌표)을 표현하는 데 사용됩니다.

2.  **`Frame` 메시지**
    *   **구조**:
        *   `string session_id`: 현재 비디오 처리 세션을 식별하는 문자열 ID.
        *   `int32 index`: 비디오 시퀀스 내에서 현재 프레임의 순서 번호.
        *   `int32 flag`: 프레임의 특정 상태나 목적을 나타내는 정수 플래그 (예: 0=시작, 1=일반, 2=종료).
        *   `int32 width`: 프레임 이미지의 가로 픽셀 수.
        *   `int32 height`: 프레임 이미지의 세로 픽셀 수.
        *   `int32 type`: OpenCV `cv::Mat` 객체의 데이터 타입 (예: `CV_8UC3`은 8비트 unsigned char 3채널 이미지를 의미).
        *   `bytes data`: 실제 프레임 이미지의 바이너리 데이터 (압축되지 않은 픽셀 데이터).
        *   `repeated Point3 pose_points`: 이 프레임에서 감지된 여러 `Point3` 객체들의 목록. 주로 MediaPipe와 같은 AI 모델이 추출한 인체 관절의 3D 좌표를 담습니다.
    *   **역할**: 클라이언트가 gRPC 서버로 전송하는 단일 비디오 프레임에 대한 모든 관련 정보를 캡슐화합니다. 이미지 데이터 자체와 함께, 해당 프레임의 메타데이터(세션 ID, 인덱스, 플래그, 크기, 타입) 및 AI 분석 결과(포즈 포인트)를 포함하여 서버가 수어 인식을 수행하는 데 필요한 모든 컨텍스트를 제공합니다.

3.  **`SubmitResultResponse` 메시지**
    *   **구조**:
        *   `string session_id`: 요청 시 클라이언트가 보낸 세션 ID와 동일한 ID.
        *   `int32 frame_count`: 서버가 처리한 총 프레임 수.
        *   `string message`: 서버가 인식한 최종 수어 문장 텍스트 결과.
    *   **역할**: gRPC 서버가 클라이언트에게 수어 인식 처리 결과를 반환할 때 사용하는 메시지입니다. 어떤 세션에 대한 응답인지, 몇 개의 프레임이 처리되었는지, 그리고 가장 중요한 인식된 수어 문장이 무엇인지를 클라이언트에게 전달합니다.

#+end_src
```

## File: conversation_log.md
```markdown
## 대화 로그

**사용자**: @1_propmt_프로젝트구조.md 한글로 대답해

**모델**: 네, 알겠습니다. 제공해주시는 C++ gRPC 클라이언트 프로젝트의 소스 파일을 분석하여 각 파일의 역할, 프로그램 진입점, 전체 아키텍처, 그리고 데이터 흐름에 대해 한글로 설명해 드리겠습니다.

파일을 올려주시면 각 파일마다 "OK"라고 답변 드리겠습니다. 모든 파일 업로드가 끝나면 "DONE"이라고 말씀해주세요.

**사용자**: 현재 폴더에 있는 모든 파일을 추가해서 분석해.

**모델**: 알겠습니다. 현재 폴더에 있는 모든 파일을 읽고 분석하여 요청하신 내용을 정리해 드리겠습니다. 먼저 파일 목록을 확인하겠습니다.

**모델**: 파일 목록을 확인했습니다. 이제 각 파일의 내용을 읽어서 분석을 시작하겠습니다.

**모델**: 파일 분석이 완료되었습니다. 요청하신 내용을 정리하여 알려드립니다.

### 섹션 1: 파일 개요

*   **`gRPCFileClient.h`, `gRPCFileClient.cpp`**: MFC 애플리케이션의 메인 클래스(`CgRPCFileClientApp`)를 정의합니다. 애플리케이션의 시작과 초기화를 담당합니다.
*   **`gRPCFileClientDlg.h`, `gRPCFileClientDlg.cpp`**: 애플리케이션의 메인 다이얼로그(UI) 클래스(`CgRPCFileClientDlg`)입니다. UI 요소(버튼, 에디트 박스 등)를 관리하고 사용자 입력을 처리하며, 비디오 재생 및 AI 분석 스레드를 생성하고 제어하는 핵심 로직을 포함합니다.
*   **`gRPCThread_.h`**: gRPC 클라이언트(`SequenceClient`)를 구현한 파일입니다. 서버와 통신하여 비디오 프레임과 메타데이터를 스트리밍으로 전송하고, 서버로부터 문장 인식 결과를 받는 역할을 합니다.
*   **`VideoUtil-v1.1.h`, `VideoUtil-v1.1.cpp`**: OpenCV와 MFC를 사용하여 비디오 처리 및 UI 출력을 담당하는 유틸리티 클래스(`CVideoUtil`)입니다. 이미지/비디오 파일 입출력, 화면에 이미지 그리기, 좌표 변환, 문자열 처리 등 다양한 보조 기능을 제공합니다.
*   **`HandTurnDetector.hpp`**: 손의 움직임(위치, 속도, 각도)을 분석하여 수어 동작의 '방향 전환'과 '감속'을 감지하는 클래스입니다. 이를 통해 수어 문장의 분절점을 찾는 데 사용됩니다.
*   **`ksl_sentence_recognition.proto`**: Protocol Buffers를 사용하여 gRPC 서비스의 데이터 구조(메시지)와 서비스 인터페이스(RPC)를 정의합니다. `Frame`, `Point3`, `SubmitResultResponse` 등의 메시지가 정의되어 있습니다.
*   **`ksl_sentence_recognition.pb.h`, `ksl_sentence_recognition.pb.cc`, `ksl_sentence_recognition.grpc.pb.h`, `ksl_sentence_recognition.grpc.pb.cc`**: `ksl_sentence_recognition.proto` 파일로부터 자동으로 생성된 C++ 코드입니다. gRPC 통신에 필요한 데이터 직렬화/역직렬화 및 서비스 스텁(stub) 코드를 포함합니다.
*   **`pch.h`, `pch.cpp`, `framework.h`, `targetver.h`, `resource.h`**: Visual Studio MFC 프로젝트의 표준 파일들입니다. 미리 컴파일된 헤더, 프레임워크 포함, 리소스 ID 정의 등의 역할을 합니다.

### 섹션 2: 프로그램 진입점

*   **`WinMain` (애플리케이션 진입점)**: `gRPCFileClient.cpp`의 `CgRPCFileClientApp::InitInstance()` 함수가 실질적인 메인 진입점 역할을 합니다. 이 함수에서 메인 다이얼로그(`CgRPCFileClientDlg`)를 생성하고 실행합니다.
*   **다이얼로그 (UI 진입점)**: `CgRPCFileClientDlg` 클래스가 메인 UI 창을 담당하며, `OnInitDialog()` 함수에서 초기화가 이루어집니다. 사용자의 버튼 클릭과 같은 이벤트는 `OnBnClicked...` 형태의 메시지 핸들러 함수들에서 처리됩니다.
*   **스레드 (백그라운드 작업 진입점)**:
    *   `CgRPCFileClientDlg::VideoPlay`: 비디오 파일을 읽고 화면에 재생하는 스레드입니다.
    *   `CgRPCFileClientDlg::AIThread`: Python으로 작성된 AI 모듈(MediaPipe)을 호출하여 영상 프레임에서 손과 포즈의 랜드마크를 추출하는 스레드입니다.
    *   `CgRPCFileClientDlg::gRPCSend`: 전처리된 프레임 데이터(`Frame` 메시지)를 gRPC 서버로 전송하는 스레드입니다.

### 섹션 3: 아키텍처 요약

*   **UI 계층 (MFC)**: `CgRPCFileClientDlg`가 담당합니다. 사용자는 이 UI를 통해 비디오 파일을 선택하고, 재생을 제어하며, 분석할 ROI(관심 영역)를 설정하고, gRPC 통신을 시작합니다. 결과는 다이얼로그의 정보창에 표시됩니다.
*   **gRPC 통신 계층**: `gRPCThread_.h`의 `SequenceClient` 클래스가 담당합니다. 비디오 프레임과 AI 모듈이 추출한 스켈레톤 데이터를 `Frame` protobuf 메시지로 인코딩하여, 클라이언트 스트리밍 방식으로 서버에 전송합니다. 서버로부터 최종 문장 인식 결과를 받습니다.
*   **비디오/전처리 계층**: `VideoUtil-v1.1.cpp`와 `gRPCFileClientDlg.cpp`의 `VideoPlay` 스레드가 담당합니다. OpenCV를 사용하여 비디오 파일을 프레임 단위로 읽고, 설정된 ROI를 추출합니다. `HandTurnDetector`는 추출된 스켈레톤 데이터를 기반으로 동작의 분절점을 감지하는 데 사용됩니다.
*   **ACR / AI 모듈 통합**: `gRPCFileClientDlg::AIThread` 스레드에서 `pybind11`을 통해 Python으로 구현된 MediaPipe 모델을 호출합니다. 이 모델은 입력된 비디오 프레임(ROI 이미지)으로부터 3D 포즈(스켈레톤) 정보를 추출하여 C++ 코드에 반환합니다.

### 섹션 4: 데이터 흐름

1.  **사용자 액션**: 사용자가 UI에서 비디오 파일을 선택하고 'gRPC Send' 버튼을 클릭합니다.
2.  **내부 로직 (프레임 처리)**:
    *   `gRPCSend` 스레드가 시작됩니다.
    *   지정된 프레임 범위 내에서 비디오를 한 프레임씩 읽습니다.
    *   설정된 ROI 영역을 프레임에서 잘라냅니다.
    *   `AIThread`를 깨워 ROI 이미지를 전달하고, MediaPipe를 통해 스켈레톤 데이터를 추출합니다.
    *   `HandTurnDetector`와 Optical Flow를 사용하여 손의 움직임을 분석하고, 수어 동작의 분절점(전송할 프레임)인지 판단합니다.
3.  **gRPC 요청**: 
    *   분절점으로 판단된 프레임의 이미지 데이터와 스켈레톤 데이터를 `Frame` protobuf 메시지에 담습니다.
    *   `SequenceClient`가 이 `Frame` 메시지를 서버로 스트리밍 전송합니다.
4.  **서버 응답**: 
    *   클라이언트가 스트림 종료를 알리면, 서버는 그동안 수신한 프레임 시퀀스를 종합하여 수어 문장 인식을 수행하고, 결과 텍스트를 `SubmitResultResponse` 메시지에 담아 클라이언트에 반환합니다.
5.  **UI 업데이트**: 
    *   클라이언트는 응답 메시지에서 결과 텍스트를 추출하여 UI의 정보창(`m_info_edit`)에 표시합니다.

**사용자**: protobuf 에 정의된 데이터 구조는?

**모델**: `ksl_sentence_recognition.proto` 파일에 정의된 데이터 구조는 다음과 같습니다:

1.  **`Point3` 메시지**:
    *   3차원 공간의 한 점을 나타냅니다.
    *   필드:
        *   `float x = 1;`
        *   `float y = 2;`
        *   `float z = 3;`

2.  **`Frame` 메시지**:
    *   단일 비디오 프레임과 관련된 데이터를 나타냅니다.
    *   필드:
        *   `string session_id = 1;`: 현재 세션의 고유 ID입니다.
        *   `int32 index = 2;`: 프레임의 인덱스(순서)입니다.
        *   `int32 flag = 3;`: 프레임의 상태를 나타내는 플래그입니다 (예: 0: START, 1: NORMAL, 2: END).
        *   `int32 width = 4;`: 프레임 이미지의 너비입니다.
        *   `int32 height = 5;`: 프레임 이미지의 높이입니다.
        *   `int32 type = 6;`: OpenCV `cv::Mat`의 타입 정보입니다 (예: `CV_8UC3`).
        *   `bytes data = 7;`: 프레임 이미지의 실제 바이너리 데이터입니다.
        *   `repeated Point3 pose_points = 8;`: 프레임 내에서 감지된 여러 3D 포즈(스켈레톤) 포인트들의 목록입니다.

3.  **`SubmitResultResponse` 메시지**:
    *   서버로부터 클라이언트로 전송되는 최종 결과 응답을 나타냅니다.
    *   필드:
        *   `string session_id = 1;`: 요청에 사용된 세션 ID입니다.
        *   `int32 frame_count = 2;`: 처리된 총 프레임 수입니다.
        *   `string message = 3;`: 서버가 인식한 수어 문장 결과 텍스트입니다.

**사용자**: 대답의 결과를 선택하고싶다. 복사하는 방법은?

**모델**: 현재 CLI 환경에서는 직접 텍스트를 선택하고 복사하는 기능이 제한적일 수 있습니다.

**결과를 파일로 저장**해 드릴 수 있습니다. 예를 들어, `analysis_result.md` 와 같은 파일에 이전 답변 내용을 저장해 드릴까요? 그러면 해당 파일을 열어서 편하게 복사하실 수 있습니다.

**사용자**: ok

**모델**: 알겠습니다. 파일 저장이 취소되었습니다.

혹시 다른 방식으로 답변 결과를 제공해 드릴까요? 아니면 다른 질문이 있으신가요?

**사용자**: 다시 파일로 저장하고 클립보드에도 저장해

**모델**: 알겠습니다. 이전 답변 내용을 `proto_data_structure.md` 파일로 저장해 드리겠습니다.

하지만 저는 사용자의 클립보드에 직접 접근하여 내용을 저장할 수 없습니다. 파일로 저장된 내용을 열어서 직접 복사해 주셔야 합니다.

먼저 파일을 저장하겠습니다.

**모델**: `proto_data_structure.md` 파일이 성공적으로 저장되었습니다.

클립보드 저장 기능은 직접 제공해 드릴 수 없으니, 생성된 파일을 열어 내용을 복사해 주시기 바랍니다.

**사용자**: 영상을 하나 추가하고 버튼을 눌렀을 때 최종 전송 메세지가 생성될 때 까지 과정을 함수 중심으로 기술해. 각 과정마다 번호를 붙여

**모델**: 네, 알겠습니다. 사용자가 영상을 선택하고 'gRPC Send' 버튼을 눌렀을 때, 최종적으로 서버에 전송될 `Frame` 메시지가 생성되기까지의 과정을 함수 호출 중심으로 단계별로 설명해 드리겠습니다.

---

1.  **사용자 입력 및 이벤트 핸들러 호출**
    *   사용자가 UI에서 'gRPC Send' 버튼(ID: `IDC_BUTTON9`)을 클릭합니다.
    *   MFC 메시지 맵에 의해 `CgRPCFileClientDlg::OnBnClickedButton9()` 함수가 호출됩니다. 이 함수는 gRPC 통신을 위한 새로운 백그라운드 스레드를 시작시키는 역할을 합니다.

2.  **gRPC 전송 스레드 생성 및 시작**
    *   `OnBnClickedButton9` 함수 내에서 `AfxBeginThread(gRPCSend, this)`가 호출되어 `gRPCSend`라는 스레드 함수를 실행하는 새 스레드를 생성합니다.
    *   `CgRPCFileClientDlg::gRPCSend(LPVOID pParam)` 함수가 새 스레드의 진입점(entry point)으로 실행됩니다.

3.  **gRPC 클라이언트 실행 및 비디오 처리 루프 진입**
    *   `gRPCSend` 함수는 `grpc_client_::SequenceClient` 클래스의 멤버 함수인 `client.SendFrames("SESSION_001")`를 호출합니다. 이 함수가 실질적인 데이터 처리 및 전송의 핵심입니다.
    *   `SendFrames` 함수는 먼저 gRPC 서버와의 스트리밍 연결(`writer`)을 초기화하고, UI에서 비디오 파일 경로와 분석할 프레임 범위를 가져옵니다.
    *   OpenCV의 `VideoCapture cap(...)`을 사용해 비디오 파일을 열고, 지정된 시작 프레임부터 `while` 루프를 통해 프레임 처리를 시작합니다.

4.  **프레임 캡처 및 관심 영역(ROI) 추출**
    *   `while` 루프 내에서 `cap >> img` 코드를 통해 비디오에서 한 프레임을 `cv::Mat img` 객체로 읽어옵니다.
    *   `img(pDlg->m_roi).clone()`을 통해 사용자가 설정한 관심 영역(ROI) 이미지를 `img`에서 잘라내어 `cv::Mat roi` 객체에 복사합니다. 이 `roi` 이미지가 AI 분석의 대상이 됩니다.

5.  **AI 스레드를 통한 스켈레톤 데이터 추출**
    *   `SetEvent(pDlg->hAIStart)`를 호출하여 대기 중인 AI 스레드(`AIThread`)를 깨웁니다. `roi` 이미지는 공유 멤버 변수인 `pDlg->m_cap_img`를 통해 AI 스레드에 전달됩니다.
    *   메인 전송 스레드는 `WaitForSingleObject(pDlg->hAIFinish, INFINITE)`를 호출하여 AI 스레드가 분석을 완료할 때까지 대기합니다.
    *   `CgRPCFileClientDlg::AIThread` 함수는 `pybind11`을 통해 Python으로 작성된 MediaPipe 함수(`func_mp_pose`)를 호출하여 `m_cap_img`로부터 3D 스켈레톤 좌표를 추출하고, 결과를 `pDlg->m_sgcp_vtSkeletonMP` 벡터에 저장한 뒤 `hAIFinish` 이벤트를 발생시켜 메인 스레드를 다시 깨웁니다.

6.  **동작 분석 및 전송 프레임 결정**
    *   AI 분석이 끝나고 스켈레톤 데이터(`m_sgcp_vtSkeletonMP`)가 준비되면, `SendFrames` 함수는 이 프레임을 서버로 전송할지 여부를 결정합니다.
    *   `pDlg->m_Rdetector.update(...)`, `pDlg->m_Ldetector.update(...)` 함수를 호출하여 손의 움직임에서 방향 전환과 감속이 동시에 일어나는 '분절점'을 감지합니다.
    *   `pDlg->GetMotionVauleWithOpticalFlow(roi)` 함수를 호출하여 이미지 전체의 움직임 양을 계산하고, 움직임이 특정 임계값(`OPTICALFLOW_THRESH`) 이하로 줄어드는 순간(정지 구간)을 감지합니다.
    *   이러한 조건들이 충족되면(예: 손의 방향 전환이 감지되거나, 동작 후 정지하는 순간), 해당 프레임을 '전송 대상'으로 결정합니다.

7.  **Protobuf `Frame` 메시지 생성**
    *   프레임을 전송하기로 결정하면, `grpc_client_::EncodeFrame(...)` 함수가 호출됩니다.
    *   이 함수는 새로운 `Frame` 메시지 객체를 생성하고, 현재 프레임의 정보로 필드를 채웁니다.
        *   `session_id`, `index`, `flag`, `width`, `height`, `type` 등 메타데이터를 설정합니다.
        *   `roi` 이미지의 픽셀 데이터를 `data` 필드에 바이트(bytes) 형태로 복사합니다.
        *   5단계에서 추출한 스켈레톤 좌표(`m_sgcp_vtSkeletonMP`)를 `pose_points` 반복 필드에 하나씩 추가합니다.

8.  **메시지 전송**
    *   `EncodeFrame` 함수를 통해 완전히 채워진 `Frame` 메시지가 반환되면, `writer->Write(frame)` 함수가 호출됩니다.
    *   이 함수는 `Frame` 메시지를 Protobuf 형식으로 직렬화(serialize)하여 gRPC 스트림을 통해 서버로 최종 전송합니다.
```

## File: framework.h
```
#pragma once

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // 거의 사용되지 않는 내용은 Windows 헤더에서 제외합니다.
#endif

#include "targetver.h"

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // 일부 CString 생성자는 명시적으로 선언됩니다.

// MFC의 공통 부분과 무시 가능한 경고 메시지에 대한 숨기기를 해제합니다.
#define _AFX_ALL_WARNINGS

#include <afxwin.h>         // MFC 핵심 및 표준 구성 요소입니다.
#include <afxext.h>         // MFC 확장입니다.


#include <afxdisp.h>        // MFC 자동화 클래스입니다.



#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>           // Internet Explorer 4 공용 컨트롤에 대한 MFC 지원입니다.
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>             // Windows 공용 컨트롤에 대한 MFC 지원입니다.
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <afxcontrolbars.h>     // MFC의 리본 및 컨트롤 막대 지원









#ifdef _UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif
```

## File: gptel_conversation.org
```
:PROPERTIES:
:GPTEL_MODEL: gemini-2.5-flash
:GPTEL_BACKEND: Gemini
:GPTEL_SYSTEM: 내가 주는 내용을 org-mode 문서로 출력 형태를 변화하라. 내용은 추가/삭제 하지말고 원본을 유지하라.
:GPTEL_BOUNDS: ((response (668 3411) (3429 5950) (5976 9144) (9162 9754) (9874 12517) (12535 15555)))
:END:

* ** 아래 정보를 이용하여 대답하라. 이 프로젝트를 분석하는 필요한 파일 리스트를 정리하라
### Instruction
1. 2개의 project로 구성되있다.(gRPCKSLDetectorClient, gRPCKSLDetectorServer)
2. 1차로 gRPCKSLDetectorClient 를 분석한다.
3. {Project info} 를 참고하여 대답하라.
### Project info
1. main program은 MFC로 인터페이스를 작성했다.
2. google mediapipe 를 이용하여 특정 동작을 검출한다.
3. 검출한 이미지에서 정보를 빼내서 이미지와 함께 서버로 전달한다
4. gRPC를 이용해 서버와 통신한다.
5. 이미지는 수어를 구분하는 **키프레임** 검출이 기준이다.


#+begin_reasoning
**Delving into Dependencies*

I'm focusing now on understanding the specific requirements and the structure of the =gRPCKSLDetectorClient= project. I'm carefully analyzing the provided file tree and the instructions, pinpointing the key files needed for analysis. I'm thinking about the potential dependencies and the best way to approach this based on the structure.


*Defining Project Boundaries*

I've isolated my focus to =gRPCKSLDetectorClient=, specifically its role as a client interacting with a server. The project structure's clear; MFC handles the UI, while MediaPipe detects gestures, and gRPC facilitates communication. I'm prioritizing the main dialog file, seeing this as the hub where UI actions, MediaPipe output, and gRPC calls intertwine to transmit keyframe data and images. My plan is to explore this client-side logic in detail.


*Investigating Python Integration*

My focus is narrowing in on the MediaPipe integration, especially the Python components. I'm prioritizing the =ACR/= directory, particularly =acr/main.py=, =detect.py=, and =mp_detect.py=. I'm starting to think about how these Python scripts feed into the C++ side, potentially using a system call or shared memory. Understanding =ksl_sentence_recognition.proto= is still very important, as this dictates the data format being exchanged.


*Mapping the gRPC Flow*

I'm now zeroing in on the interaction between the MFC UI and the gRPC communication layer. Specifically, I see =gRPCFileClientDlg.cpp= and =gRPCFileClient.cpp= as the crucial files to trace the main application flow. I'm prioritizing understanding how the MFC UI initiates and handles gRPC calls. =gRPCThread_.h= probably handles the networking in a separate thread, which I will need to understand in order to avoid UI freezing.


*Identifying Key Interactions*

I'm now zeroing in on the critical link between C++ and Python, focusing on how the former calls the latter. The presence of the =pybind11= directory strongly suggests the use of this library. Specifically, I'm anticipating =py::= calls within the C++ code, likely in =gRPCFileClientDlg.cpp=, which should shed light on the Python integration and how MediaPipe functionalities are leveraged. I've eliminated some files from my initial list as irrelevant for the initial analysis.


*Categorizing the Components*

My current focus is on organizing the project's key elements into logical groups: MFC interface, core C++ logic and hand detection, MediaPipe and AI in Python, gRPC networking, and configuration settings. I'm finalizing my categorization and ensuring the file paths in the categorization are correct by cross-checking them against the directory structure. I'm confident in my understanding of each component.



#+end_reasoning

제공된 파일 트리와 프로젝트 정보(MFC, MediaPipe, gRPC, 키프레임 검출)를 바탕으로 *gRPCKSLDetectorClient* 프로젝트 분석에 필수적인 파일 리스트를 정리했습니다.

분석은 *메인 로직(MFC) → 영상 처리 및 검출(MediaPipe/Logic) → 통신(gRPC)* 순서로 진행하는 것이 효율적입니다.

*** 1. 메인 인터페이스 및 프로그램 흐름 (MFC)
가장 먼저 분석해야 할 부분입니다. UI 이벤트 처리, 프로그램의 진입점, 전체적인 데이터 흐름(영상 캡처 -> 검출 -> 전송)을 파악할 수 있습니다.

-   *=gRPCFileClient/gRPCFileClientDlg.cpp=*: (핵심) 메인 다이얼로그 소스. 카메라 캡처 시작, MediaPipe 연동 함수 호출, gRPC 전송 트리거 등 주요 비즈니스 로직이 포함되어 있을 가능성이 높습니다.
-   *=gRPCFileClient/gRPCFileClientDlg.h=*: 다이얼로그 헤더. 주요 멤버 변수와 함수 선언을 통해 클래스 구조를 파악합니다.
-   *=gRPCFileClient/gRPCFileClient.cpp=*: 애플리케이션 초기화 및 인스턴스 생성.
-   *=gRPCFileClient/resource.h=*: UI 컨트롤(버튼, 리스트 등)의 ID 매핑 확인용.

*** 2. 영상 처리 및 핵심 알고리즘 (C++ & Logic)
영상을 획득하고, 수어 인식을 위한 *키프레임*을 판단하는 로직입니다.

-   *=gRPCFileClient/VideoUtil-v1.1.cpp= & =.h=*: OpenCV를 이용한 카메라/동영상 프레임 획득 및 전처리 로직.
-   *=gRPCFileClient/HandTurnDetector.hpp=*: (핵심) 파일명으로 보아 *수어 동작(손의 회전 등)을 감지하여 키프레임을 선별*하는 핵심 알고리즘이 포함된 헤더 전용 라이브러리로 추정됩니다.

*** 3. AI 및 동작 검출 (Python & MediaPipe)
C++(MFC)에서 pybind11 등을 통해 호출되는 실제 AI 추론 부분입니다. Google MediaPipe 관련 로직이 여기 있습니다.

-   *=gRPCFileClient/ACR/mp_detect.py=*: (핵심) MediaPipe를 사용하여 손/포즈를 검출하는 메인 파이썬 스크립트로 예상됩니다.
-   *=gRPCFileClient/ACR/detect.py=*: 동작 검출 또는 추론을 수행하는 진입점 스크립트일 가능성이 있습니다.
-   *=gRPCFileClient/ACR/acr/config.py=*: AI 모델의 설정값(Threshold 등) 확인.

*** 4. 서버 통신 (gRPC)
검출된 정보를 서버로 보내는 데이터 구조와 통신 방식을 분석합니다.

-   *=gRPCFileClient/ksl_sentence_recognition.proto=*: (핵심) 클라이언트와 서버 간에 주고받는 데이터(이미지, 키프레임 정보 등)의 *메시지 규격*이 정의된 파일입니다.
-   *=gRPCFileClient/gRPCThread_.h=*: UI가 멈추지 않도록 백그라운드에서 gRPC 통신을 수행하는 쓰레드 로직으로 추정됩니다.
-   *=gRPCFileClient/ksl_sentence_recognition.grpc.pb.cc=*: 실제 gRPC 호출 구현부(필요시 참조).

*** 5. 설정 및 기타
-   *=gRPCFileClient/cfg-new.txt=*: 프로그램 실행 시 로드하는 설정 파일(서버 IP, 포트, 모델 경로 등).

---

*** 📂 요약: 우선순위별 분석 파일 리스트 (경로 포함)

*Phase 1. 진입점 및 제어 로직*
1. =./gRPCKSLDetectorClient-v1.4-DS_CCTV/gRPCFileClient/gRPCFileClientDlg.cpp=
2. =./gRPCKSLDetectorClient-v1.4-DS_CCTV/gRPCFileClient/gRPCFileClientDlg.h=

*Phase 2. 키프레임 검출 및 영상 처리*
3. =./gRPCKSLDetectorClient-v1.4-DS_CCTV/gRPCFileClient/HandTurnDetector.hpp=
4. =./gRPCKSLDetectorClient-v1.4-DS_CCTV/gRPCFileClient/VideoUtil-v1.1.cpp=

*Phase 3. AI 추론 (MediaPipe)*
5. =./gRPCKSLDetectorClient-v1.4-DS_CCTV/gRPCFileClient/ACR/mp_detect.py=
6. =./gRPCKSLDetectorClient-v1.4-DS_CCTV/gRPCFileClient/ACR/detect.py=

*Phase 4. 통신 프로토콜*
7. =./gRPCKSLDetectorClient-v1.4-DS_CCTV/gRPCFileClient/ksl_sentence_recognition.proto=
8. =./gRPCKSLDetectorClient-v1.4-DS_CCTV/gRPCFileClient/gRPCThread_.h

***


#+begin_reasoning
**Revising the Structure*

Okay, I've adjusted the format. The user seems to have responded to my previous output. I'm taking the =***= as a cue that it's okay to now change direction. I've rethought the initial output - I'm using more detail to make sure the output fulfills all of the original instructions now.


*Defining the Analysis Strategy*

I'm now zeroing in on the next action. My current thought process centers on obtaining the necessary code to begin the analysis. I'm focusing on requesting the contents of the critical files I've already identified, specifically the entry point (MFC) and the gRPC proto file. Providing all files at once might be overwhelming, so I'm planning to start with these two to establish the fundamental framework. I believe this will provide the best starting point for a comprehensive evaluation.


*Confirming File Selection*

I'm solidifying my approach for requesting the initial files. Based on the user's implied file availability and the emphasis on a "start" to the analysis, I've re-evaluated the user's directive. I believe the asterisks indicate readiness to move forward. To lay the groundwork, I will request the content of =gRPCFileClientDlg.h=, =gRPCFileClientDlg.cpp=, and =ksl_sentence_recognition.proto=. This prioritizes the core application structure and data format for a swift and effective evaluation. I'll provide this request now.


*Interpreting User Intent*

I'm now grappling with the user's intent after the =***=.  It's unclear if the user expects me to simulate analysis based on filenames alone, which I've determined is not appropriate. My current thinking is that they want me to proceed with the analysis, but I can't do so without the code. So, I will ask for the code to continue with the analysis of =gRPCFileClientDlg.h=, =gRPCFileClientDlg.cpp=, and =ksl_sentence_recognition.proto=.


*Determining the Priority*

I'm now focusing on the user's input after the =***=. I've re-examined the prompt and I'm reconsidering the user's intent. The user gave me a file structure and asked me to answer using the provided information, not using my own assumptions. Given this, I will request the content of =gRPCFileClientDlg.cpp= and =ksl_sentence_recognition.proto= so I can proceed to analyze the program's core logic. I'm focusing on these files to ensure a focused, manageable analysis.


*Initiating Code Acquisition*

I'm now determining the most effective approach to continue. My current understanding is that the =***= doesn't provide the code, nor does it confirm an analysis can begin.  I will directly request the content of the critical files. I'm prioritizing clarity, so I will request =ksl_sentence_recognition.proto=, =gRPCFileClientDlg.h=, and =gRPCFileClientDlg.cpp= to start. This should allow a comprehensive first step for the analysis.


*Requesting the Core Logic*

I've decided to prioritize the main logic, so I'm requesting the content of =gRPCFileClientDlg.cpp= to understand the core functionality. Once I have this, I plan to move on to the MediaPipe integration and keyframe detection logic. This step-by-step approach seems the most efficient way to proceed.



#+end_reasoning

네, 분석을 시작하기 위해 *코드 내용 확인*이 필요합니다.

효율적인 분석을 위해, 전체 파일을 한 번에 보는 것보다 *데이터 구조(Protocol)*와 *메인 흐름(Main Logic)*을 먼저 파악하는 것이 좋습니다.

다음 *3개 파일의 내용*을 먼저 제공해 주시면 상세 분석을 시작하겠습니다.

1.  *=gRPCFileClient/ksl_sentence_recognition.proto=*
    *   /목적:/ 서버와 어떤 데이터를 주고받는지(이미지 포맷, 키프레임 정보 등) 명세를 먼저 확정합니다.
2.  *=gRPCFileClient/gRPCFileClientDlg.h=*
    *   /목적:/ 메인 다이얼로그 클래스의 멤버 변수(MediaPipe 객체, gRPC 채널 등) 구성을 확인합니다.
3.  *=gRPCFileClient/gRPCFileClientDlg.cpp=*
    *   /목적:/ 카메라 캡처 루프, MediaPipe 호출 시점, gRPC 전송 시점 등 전체 흐름을 분석합니다.

위 파일들의 내용을 붙여넣어 주시면, *"MFC ↔ MediaPipe ↔ gRPC"* 간의 데이터 흐름을 중심으로 분석해 드리겠습니다.

***

** 이 프로젝트를 LLM 이용해 분석하고자 한다.
   1. 최적의 접근 방법과 과정을 제시하라.
   2. 분석을위한 LLM 세팅을 위한 prompt를 추천하라.


#+begin_reasoning
**Refining the Objective*

I'm now zeroing in on the core goal: using the LLM to analyze this C++ project. I've broken down the goal and identified the need to understand the project's structure and functionality. My next step will be to explore different analysis approaches for C++ code within the LLM's capabilities.


*Devising the Analysis Strategy*

I've just landed on the ideal strategy: a Top-Down combined with Data-Flow approach. I think starting with the UI and protocol buffer, then delving into logic and math will be effective. Tracing the video frame from camera to server is key. I've broken the work into digestible phases since LLMs have context limits.


*Defining the Workflow*

Currently, I'm focusing on the sequential steps for code analysis. I'm structuring the process into distinct phases: from the initial protocol to server-side integration. My breakdown will be organized to extract information from the project's skeleton. It's the protocol, then the main loop, the interface with Python, and finally, diving deep into the key business logic with focus on keyframe detection.


*Developing the Prompt*

I'm now iterating on the prompts, aiming for the perfect fit. I've switched gears from a generic 'helper' role to a "Senior Reverse Engineer," which I believe suits the task better. I am focusing on the key constraint of the gRPC, C++, and Python integration, the main thing I need to do is keep the answers concise. The core function is to find bugs and trace data, keeping a structural approach, I believe this is the way. I'm experimenting with two specific tasks: analyzing the =cv::Mat= lifecycle and debugging the C++/Python border with pybind11, I'm working to produce a concise and usable prompt.


*Optimizing Prompting Technique*

I'm now focusing on refining the prompt structure for optimal LLM interaction. My strategy involves a "System Prompt" for the overall context and "Task-Specific Prompts" for individual files to provide clear directives, considering the context window limits. I'm focusing on providing users with clear instructions on which files to feed the LLM and in what order, specifically highlighting =HandTurnDetector.hpp= for keyframe detection and the =proto= files to emphasize client-server interface. I'm also exploring self-correction during drafting and the key terms necessary for the final output. The key will be to balance Top-Down and Bottom-Up with appropriate context window management.


*Revising Output Specifics*

나는 이제 최종 결과의 정교화에 집중하고 있다. 나는 특히 클라이언트 분석(사용자의 2단계 지침)과 서버 분석을 명확히 구분하려고 노력 중이다. 하지만 궁극적으로 전체적인 관점을 포함해야 한다. 생성된 프롬프트는 복사하여 붙여넣을 수 있도록 설계해야 한다.


#+end_reasoning

이 프로젝트는 *Legacy 기술(MFC/C++)*과 *Modern 기술(Python AI/gRPC)*이 혼합된 하이브리드 시스템입니다. 단순한 코드 해석을 넘어 *언어 간 인터페이스*와 *데이터 파이프라인*을 추적하는 것이 분석의 핵심입니다.

LLM을 활용해 효율적으로 분석하기 위한 *접근 로드맵*과 *최적 프롬프트*를 제안합니다.

---

*** 1. 최적의 접근 방법과 과정 (Roadmap)

LLM의 컨텍스트 윈도우(기억 용량) 한계를 고려하여, 전체를 한 번에 넣기보다 *기능 단위(Module-wise)*로 나누어 순차적으로 분석해야 합니다.

**** *Phase 1: 데이터 계약서 분석 (The Contract)*
가장 먼저 클라이언트와 서버가 *무엇을 주고받는지* 정의해야 합니다.
-   *대상 파일:* =ksl_sentence_recognition.proto=
-   *분석 목표:* 이미지 전송 포맷(Bytes? Base64?), 메타데이터(키프레임 인덱스, 유저 ID 등), 응답 구조 파악.

**** *Phase 2: 핵심 루프 & 연결고리 파악 (The Bridge)*
MFC가 어떻게 Python AI를 호출하고 결과를 받는지 파악합니다. 이 프로젝트의 가장 복잡한 부분(C++ ↔ Python)입니다.
-   *대상 파일:* =gRPCFileClientDlg.cpp=, =gRPCFileClientDlg.h=
-   *분석 목표:*
    1.  카메라 영상 취득 시점 (=OnTimer= 또는 스레드).
    2.  =pybind11= 또는 Python API 호출 코드 식별.
    3.  리턴받은 데이터(키프레임 여부) 처리 로직.

**** *Phase 3: AI 및 비즈니스 로직 (The Brain)*
실제 수어 인식과 키프레임 선정 로직을 분석합니다.
-   *대상 파일:* =HandTurnDetector.hpp= (C++), =ACR/mp_detect.py= (Python)
-   *분석 목표:*
    1.  *키프레임 기준:* =HandTurnDetector=가 어떤 수학적/논리적 기준으로 동작을 구분하는지.
    2.  *검출:* =mp_detect.py=가 MediaPipe에서 어떤 랜드마크를 추출하는지.

**** *Phase 4: 통신 파이프라인 (The Pipe)*
데이터가 실제 서버로 날아가는 비동기/동기 로직을 봅니다.
-   *대상 파일:* =gRPCThread_.h=, =gRPCFileClient.cpp=
-   *분석 목표:* UI 블로킹 방지 처리, gRPC Streaming/Unary 방식 확인.

---

*** 2. LLM 세팅을 위한 추천 Prompts

LLM에게 단순한 "코드 설명해줘" 보다는, *역할(Persona)*과 *출력 형식(Format)*을 지정해야 정확도가 높아집니다.

**** *A. 기본 시스템 설정 (System Prompt)*
LLM의 역할을 정의하는 프롬프트입니다. 분석 세션의 맨 처음에 입력하세요.

#+begin_src text
당신은 C++(MFC), Python, gRPC 아키텍처에 정통한 [수석 시스템 아키텍트]입니다.
이 프로젝트는 Windows MFC 클라이언트가 Python MediaPipe를 이용해 수어 영상을 분석하고,
gRPC를 통해 서버로 전송하는 하이브리드 시스템입니다.

당신의 임무는 제공된 코드를 바탕으로 다음을 분석하는 것입니다:
1. Cross-Language Interface: C++에서 Python 모듈을 호출하는 방식 분석.
2. Data Flow: Camera Frame -> Preprocessing -> AI Inference -> gRPC Send 흐름 추적.
3. Key Logic: '키프레임 검출' 알고리즘의 동작 원리 설명.

대답은 항상 [코드의 의도], [잠재적 문제점], [동작 흐름 요약]으로 구조화하여 답변하세요.
#+end_src

**** *B. 단계별 분석 프롬프트 (User Prompts)*

각 파일을 업로드할 때 아래 프롬프트를 함께 사용하세요.

*[상황 1: Proto 파일 분석 시]*
#+begin_src text
이 파일은 프로젝트의 gRPC 프로토콜 명세서(proto)입니다.
이 파일을 분석하여 다음을 표로 정리해 주세요:
1. 클라이언트가 보내는 데이터 필드와 타입.
2. 서버가 응답하는 데이터 필드와 타입.
3. 서비스 방식이 Unary(단일 호출)인지 Streaming(스트리밍)인지 여부.
#+end_src

*[상황 2: C++ ↔ Python 연동 분석 시 (=gRPCFileClientDlg.cpp= 등)]*
#+begin_src text
이 코드는 MFC 메인 다이얼로그입니다. 특히 'Python MediaPipe'와의 연동 부분에 집중해서 분석해 주세요.
1. Python 인터프리터를 초기화하고 스크립트를 로드하는 부분이 어디입니까?
2. C++의 이미지 데이터(cv::Mat 등)가 Python 함수로 전달될 때 어떤 변환 과정을 거치나요?
3. Python 함수의 리턴값(동작 검출 결과)을 C++에서 어떻게 파싱하여 처리합니까?
#+end_src

*[상황 3: 키프레임 로직 분석 시 (=HandTurnDetector.hpp=)]*
#+begin_src text
이 코드는 '키프레임(Keyframe)'을 검출하는 핵심 알고리즘으로 보입니다.
1. 손의 회전(Turn)이나 동작 변화를 감지하기 위해 어떤 변수(각도, 속도, 거리 등)를 계산합니까?
2. 키프레임으로 판정하는 임계값(Threshold) 로직을 수식이나 의사코드(Pseudo-code)로 요약해 주세요.
#+end_src

---

*** *💡 팁: LLM 컨텍스트 관리*
-   *OpenCV/gRPC 헤더 제외:* =opencv-cuda/include=나 =utils/include= 폴더에 있는 라이브러리 헤더 파일들은 LLM에 넣지 마세요. 토큰만 낭비됩니다. LLM은 이미 OpenCV나 gRPC의 기본 사용법을 알고 있습니다.
-   *소스 코드 위주:* =.h=와 =.cpp= (C++), =.py= (Python), =.proto= 파일만 선별하여 입력하세요.

**  아래 내용을 org-mode 문서로 변환해
```

## File: gRPCFileClient.cpp
```cpp
// gRPCFileClient.cpp: 애플리케이션에 대한 클래스 동작을 정의합니다.
//

#include "pch.h"
#include "framework.h"
#include "gRPCFileClient.h"
#include "gRPCFileClientDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CgRPCFileClientApp

BEGIN_MESSAGE_MAP(CgRPCFileClientApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// CgRPCFileClientApp 생성

CgRPCFileClientApp::CgRPCFileClientApp()
{
	// 다시 시작 관리자 지원
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;

	// TODO: 여기에 생성 코드를 추가합니다.
	// InitInstance에 모든 중요한 초기화 작업을 배치합니다.
}


// 유일한 CgRPCFileClientApp 개체입니다.

CgRPCFileClientApp theApp;


// CgRPCFileClientApp 초기화

BOOL CgRPCFileClientApp::InitInstance()
{
	// Windows XP에서는 InitCommonControlsEx()를 필요로 합니다.
	// 사용하도록 지정하는 경우, Windows XP 상에서 반드시 InitCommonControlsEx()가 필요합니다.
	// InitCommonControlsEx()를 사용하지 않으면 창을 만들 수 없습니다.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// 응용 프로그램에서 사용할 모든 공용 컨트롤 클래스를 포함하도록
	// 이 항목을 설정하십시오.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();


	AfxEnableControlContainer();

	// 대화 상자에 셸 트리 뷰 또는
	// 셸 목록 뷰 컨트롤이 포함되어 있는 경우 셸 관리자를 만듭니다.
	CShellManager *pShellManager = new CShellManager;

	// MFC 컨트롤의 테마를 사용하기 위해 "Windows 원형" 비주얼 관리자 활성화
	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));

	// 표준 초기화
	// 이들 기능을 사용하지 않고 최종 실행 파일의 크기를 줄이려면
	// 아래에서 필요 없는 특정 초기화
	// 루틴을 제거해야 합니다.
	// 해당 설정이 저장된 레지스트리 키를 변경하십시오.
	// TODO: 이 문자열을 회사 또는 조직의 이름과 같은
	// 적절한 내용으로 수정해야 합니다.
	SetRegistryKey(_T("로컬 애플리케이션 마법사에서 생성된 애플리케이션"));

	CgRPCFileClientDlg dlg;
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: 여기에 [확인]을 클릭하여 대화 상자가 없어질 때 처리할
		//  코드를 배치합니다.
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: 여기에 [취소]를 클릭하여 대화 상자가 없어질 때 처리할
		//  코드를 배치합니다.
	}
	else if (nResponse == -1)
	{
		TRACE(traceAppMsg, 0, "경고: 대화 상자를 만들지 못했으므로 애플리케이션이 예기치 않게 종료됩니다.\n");
		TRACE(traceAppMsg, 0, "경고: 대화 상자에서 MFC 컨트롤을 사용하는 경우 #define _AFX_NO_MFC_CONTROLS_IN_DIALOGS를 수행할 수 없습니다.\n");
	}

	// 위에서 만든 셸 관리자를 삭제합니다.
	if (pShellManager != nullptr)
	{
		delete pShellManager;
	}

#if !defined(_AFXDLL) && !defined(_AFX_NO_MFC_CONTROLS_IN_DIALOGS)
	ControlBarCleanUp();
#endif

	// 대화 상자가 닫혔으므로 응용 프로그램의 메시지 펌프를 시작하지 않고 응용 프로그램을 끝낼 수 있도록 FALSE를
	// 반환합니다.
	return FALSE;
}
```

## File: gRPCFileClient.h
```
// gRPCFileClient.h: PROJECT_NAME 애플리케이션에 대한 주 헤더 파일입니다.
//

#pragma once

#ifndef __AFXWIN_H__
	#error "PCH에 대해 이 파일을 포함하기 전에 'pch.h'를 포함합니다."
#endif

#include "resource.h"		// 주 기호입니다.


// CgRPCFileClientApp:
// 이 클래스의 구현에 대해서는 gRPCFileClient.cpp을(를) 참조하세요.
//

class CgRPCFileClientApp : public CWinApp
{
public:
	CgRPCFileClientApp();

// 재정의입니다.
public:
	virtual BOOL InitInstance();

// 구현입니다.

	DECLARE_MESSAGE_MAP()
};

extern CgRPCFileClientApp theApp;
```

## File: gRPCFileClientDlg.cpp
```cpp
// gRPCFileClientDlg.cpp: 구현 파일
//

#include "gRPCThread_.h"

//#include "pch.h"
#include "framework.h"
#include "gRPCFileClient.h"
#include "gRPCFileClientDlg.h"

#include "afxdialogex.h"

#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console")

#ifdef _DEBUG
#define new DEBUG_NEW
#endif




// 응용 프로그램 정보에 사용되는 CAboutDlg 대화 상자입니다.

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

// 구현입니다.
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CgRPCFileClientDlg 대화 상자



CgRPCFileClientDlg::CgRPCFileClientDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_GRPCFILECLIENT_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	hPlayPause = CreateEvent(NULL, FALSE, FALSE, NULL);
	hAIStart = CreateEvent(NULL, FALSE, FALSE, NULL);
	hAIFinish = CreateEvent(NULL, FALSE, FALSE, NULL);

	m_exit.store(false);
	m_pause.store(false);
	m_stop.store(false);
	m_back.store(false);
	m_move.store(false);

}

void CgRPCFileClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT1, m_video_file_name_edit);
	DDX_Control(pDX, IDC_CCTV, m_cctv_frame);
	DDX_Control(pDX, IDC_ROICCTV, m_ROIcctv_frame);
	DDX_Control(pDX, IDC_EDIT6, m_frame_index_edit);
	DDX_Control(pDX, IDC_EDIT2, m_roi_edit);
	DDX_Control(pDX, IDC_EDIT5, m_frame_range_edit);
	DDX_Control(pDX, IDC_EDIT3, m_info_edit);
}

BEGIN_MESSAGE_MAP(CgRPCFileClientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, &CgRPCFileClientDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDOK, &CgRPCFileClientDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_BUTTON2, &CgRPCFileClientDlg::OnBnClickedButton2)
	ON_BN_CLICKED(IDC_BUTTON3, &CgRPCFileClientDlg::OnBnClickedButton3)
	ON_BN_CLICKED(IDC_BUTTON4, &CgRPCFileClientDlg::OnBnClickedButton4)
	ON_BN_CLICKED(IDC_BUTTON5, &CgRPCFileClientDlg::OnBnClickedButton5)
	ON_BN_CLICKED(IDC_BUTTON7, &CgRPCFileClientDlg::OnBnClickedButton7)
	ON_BN_CLICKED(IDC_BUTTON8, &CgRPCFileClientDlg::OnBnClickedButton8)
	ON_BN_CLICKED(IDC_BUTTON10, &CgRPCFileClientDlg::OnBnClickedButton10)
	ON_BN_CLICKED(IDC_BUTTON9, &CgRPCFileClientDlg::OnBnClickedButton9)
END_MESSAGE_MAP()


// CgRPCFileClientDlg 메시지 처리기

BOOL CgRPCFileClientDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 시스템 메뉴에 "정보..." 메뉴 항목을 추가합니다.

	// IDM_ABOUTBOX는 시스템 명령 범위에 있어야 합니다.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 이 대화 상자의 아이콘을 설정합니다.  응용 프로그램의 주 창이 대화 상자가 아닐 경우에는
	//  프레임워크가 이 작업을 자동으로 수행합니다.
	SetIcon(m_hIcon, TRUE);			// 큰 아이콘을 설정합니다.
	SetIcon(m_hIcon, FALSE);		// 작은 아이콘을 설정합니다.

	// TODO: 여기에 추가 초기화 작업을 추가합니다.

		// UTF-8 콘솔 설정
	SetConsoleOutputCP(CP_UTF8);
	SetConsoleCP(CP_UTF8);
	// 안전: 실패하면 클래식 로캘로 대체
	try {
		std::wcout.imbue(std::locale(".UTF-8"));   // 또는 ".utf8" / "Korean_Korea.949"
	}
	catch (...) {
		std::wcout.imbue(std::locale::classic());
	}

	ReadCFG();

	OnBnClickedButton2(); //ROI 적용

	if (hAIThread == NULL) hAIThread = AfxBeginThread(AIThread, this);
	


	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

void CgRPCFileClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 대화 상자에 최소화 단추를 추가할 경우 아이콘을 그리려면
//  아래 코드가 필요합니다.  문서/뷰 모델을 사용하는 MFC 애플리케이션의 경우에는
//  프레임워크에서 이 작업을 자동으로 수행합니다.

void CgRPCFileClientDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 그리기를 위한 디바이스 컨텍스트입니다.

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 클라이언트 사각형에서 아이콘을 가운데에 맞춥니다.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 아이콘을 그립니다.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// 사용자가 최소화된 창을 끄는 동안에 커서가 표시되도록 시스템에서
//  이 함수를 호출합니다.
HCURSOR CgRPCFileClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

grpc_client_::SequenceClient client(grpc::CreateChannel(TARGETIP, grpc::InsecureChannelCredentials()));

void CgRPCFileClientDlg::OnBnClickedButton1()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.

	CString dialogKey = _T("DialogA_VIDEO");

	CString lastDir = m_util.LoadLastDir(dialogKey, _T("C:\\"));
	lastDir = m_util.EnsureDirExists(lastDir, _T("C:\\"));

	char szFilter[] = "FileList (*.mkv, *.mp4, *.avi)|*.mkv;*.mp4;*.avi| All Files(*.*)|*.*||";

	CFileDialog dlg(TRUE, NULL, NULL, OFN_HIDEREADONLY, (CA2T)szFilter);

	dlg.m_ofn.lpstrInitialDir = lastDir.GetBuffer();

	if (dlg.DoModal() == IDOK)
	{
		auto str = dlg.GetPathName();

		m_video_file_name_edit.SetWindowText(str);

		m_util.SaveLastDir(dialogKey, m_util.DirFromPath(str));
	}

}


UINT CgRPCFileClientDlg::VideoPlay(LPVOID pParam)
{
	CgRPCFileClientDlg* pDlg = (CgRPCFileClientDlg*)pParam;

	CString file_nm;

	pDlg->m_video_file_name_edit.GetWindowText(file_nm);

	if (!file_nm.IsEmpty())
	{
		CString file_str;

		pDlg->m_video_file_name_edit.GetWindowText(file_str);

		if (!file_str.IsEmpty() && !file_str.IsEmpty() && !file_str.IsEmpty())
		{

			CRect Rect1;
			pDlg->m_cctv_frame.GetClientRect(Rect1);
		

			VideoCapture cap(pDlg->m_util.StringToChar(file_str));

			Mat img;
			if (cap.isOpened())
			{
				auto [w, h, fps, totalFrames] = pDlg->m_util.GetVideoInfo(cap);

				CString str;
				str.Format(_T("[비디오파일 정보]\r\n%s\r\nwidht:%d height:%d fps:%.2f total frames:%d"), file_str, w, h, fps, totalFrames);
				pDlg->m_info_edit.SetWindowText(str);

				while (pDlg->m_stop.load() == false && pDlg->m_exit.load() == false)
				{
					cap >> img;
					int frameIndex = (int)cap.get(cv::CAP_PROP_POS_FRAMES);
					if (img.empty()) break;

					auto img_copy = img.clone();
					rectangle(img_copy, pDlg->m_roi, Scalar(0, 255, 0), 5);
					pDlg->m_util.DrawImageBMP(&pDlg->m_cctv_frame, img_copy, 0, 0, (double)frameIndex / (totalFrames - 1), frameIndex);

					cv::Mat roi_img = img(pDlg->m_roi).clone();
					pDlg->m_util.DrawImageBMP(&pDlg->m_ROIcctv_frame, roi_img, 0, 0);

					
					if (pDlg->m_pause.load() == true)
					{
						WaitForSingleObject(pDlg->hPlayPause, INFINITE);
					}
					if (pDlg->m_pause.load() == true && pDlg->m_back.load() == true)
					{
						int back_index = frameIndex - 2;
						if (back_index <= 0) back_index = 0;
						cap.set(cv::CAP_PROP_POS_FRAMES, back_index);
						pDlg->m_back.store(false);
					}
					if (pDlg->m_pause.load() == true && pDlg->m_move.load() == true)
					{
						CString str;
						pDlg->m_frame_index_edit.GetWindowText(str);
						if (!str.IsEmpty())
						{
							auto move_index = _ttoi(str) - 1;
							if (move_index < 0)move_index = 0;
							if (move_index >= totalFrames) move_index = totalFrames - 1;
							cap.set(cv::CAP_PROP_POS_FRAMES, move_index);
						}
						pDlg->m_move.store(false);
					}
					Sleep(30);
				}
				cap.release();
			}
		}
	}
	pDlg->m_stop.store(false);
	pDlg->m_pause.store(false);
	pDlg->GetDlgItem(IDC_BUTTON3)->EnableWindow(true);
	pDlg->GetDlgItem(IDC_BUTTON4)->EnableWindow(false);
	pDlg->GetDlgItem(IDC_BUTTON5)->EnableWindow(false);
	pDlg->GetDlgItem(IDC_BUTTON7)->EnableWindow(false);
	pDlg->GetDlgItem(IDC_BUTTON8)->EnableWindow(false);
	pDlg->GetDlgItem(IDC_BUTTON10)->EnableWindow(false);
	pDlg->GetDlgItem(IDC_EDIT6)->EnableWindow(false);
	pDlg->hVideoPlay = nullptr;

	cv::Mat grayColor(480, 640, CV_8UC3, cv::Scalar(210, 210, 210));
	pDlg->m_util.DrawImageBMP(&pDlg->m_cctv_frame, grayColor, 0, 0);
	pDlg->m_util.DrawImageBMP(&pDlg->m_ROIcctv_frame, grayColor, 0, 0);

	return 0;
}
void CgRPCFileClientDlg::OnBnClickedOk()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	SaveCFG();

	m_exit.store(true);

	Sleep(100);

	CDialogEx::OnOK();
}


void CgRPCFileClientDlg::SaveCFG()
{
	CString str;
	USES_CONVERSION;
	FILE* fp;
	fopen_s(&fp, "cfg-new.txt", "w");
	m_roi_edit.GetWindowText(str);
	fprintf_s(fp, "%s\n", m_util.StringToChar(str));
	m_frame_range_edit.GetWindowText(str);
	fprintf_s(fp, "%s\n", m_util.StringToChar(str));
	fclose(fp);
}


void CgRPCFileClientDlg::ReadCFG()
{
	USES_CONVERSION;
	SYSTEMTIME seleted1, seleted2;
	CString str;

	FILE* fp;
	char string[3840];
	if (fopen_s(&fp, "cfg-new.txt", "r") != 0)
	{
		AfxMessageBox(_T("cfg.txt 파일이 없습니다."));
		return;
	}
	fgets(string, sizeof(string), fp);
	str = A2T(string);
	str.Replace(_T("\n"), _T(""));
	str.Replace(_T("\r"), _T(""));
	m_roi_edit.SetWindowText(str);
	fgets(string, sizeof(string), fp);
	str = A2T(string);
	str.Replace(_T("\n"), _T(""));
	str.Replace(_T("\r"), _T(""));
	m_frame_range_edit.SetWindowText(str);
	fclose(fp);
}
void CgRPCFileClientDlg::OnBnClickedButton2()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.

	CString roi_str;
	m_roi_edit.GetWindowText(roi_str);
	auto strs = m_util.SplitCString(roi_str);

	if (strs.size() >= 4)
	{
		m_roi = cv::Rect(_ttoi(strs[0]), _ttoi(strs[1]), _ttoi(strs[2]), _ttoi(strs[3]));
	}
}

void CgRPCFileClientDlg::OnBnClickedButton3() //Play
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.

	if (hVideoPlay == NULL) hVideoPlay = AfxBeginThread(VideoPlay, this);

	m_pause.store(false);
	SetEvent(hPlayPause);
	GetDlgItem(IDC_BUTTON3)->EnableWindow(false);
	GetDlgItem(IDC_BUTTON4)->EnableWindow(true);
	GetDlgItem(IDC_BUTTON5)->EnableWindow(true);
	GetDlgItem(IDC_BUTTON7)->EnableWindow(false);
	GetDlgItem(IDC_BUTTON8)->EnableWindow(false);
	GetDlgItem(IDC_BUTTON10)->EnableWindow(false);
	GetDlgItem(IDC_EDIT6)->EnableWindow(false);
}

void CgRPCFileClientDlg::OnBnClickedButton4() //Stop
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.

	m_stop.store(true);
	GetDlgItem(IDC_BUTTON3)->EnableWindow(true);
	GetDlgItem(IDC_BUTTON4)->EnableWindow(false);
	GetDlgItem(IDC_BUTTON5)->EnableWindow(false);
	GetDlgItem(IDC_BUTTON7)->EnableWindow(false);
	GetDlgItem(IDC_BUTTON8)->EnableWindow(false);
	GetDlgItem(IDC_BUTTON10)->EnableWindow(false);
	GetDlgItem(IDC_EDIT6)->EnableWindow(false);
}

void CgRPCFileClientDlg::OnBnClickedButton5() //Pause
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.

	m_pause.store(true);
	GetDlgItem(IDC_BUTTON3)->EnableWindow(true);
	GetDlgItem(IDC_BUTTON4)->EnableWindow(false);
	GetDlgItem(IDC_BUTTON5)->EnableWindow(false);
	GetDlgItem(IDC_BUTTON7)->EnableWindow(true);
	GetDlgItem(IDC_BUTTON8)->EnableWindow(true);
	GetDlgItem(IDC_BUTTON10)->EnableWindow(true);
	GetDlgItem(IDC_EDIT6)->EnableWindow(true);
}

void CgRPCFileClientDlg::OnBnClickedButton7() //Backward
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	m_back.store(true);
	SetEvent(hPlayPause);
}

void CgRPCFileClientDlg::OnBnClickedButton8() //Forward
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	SetEvent(hPlayPause);
}

void CgRPCFileClientDlg::OnBnClickedButton10() //Move to frame index
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	m_move.store(true);
	SetEvent(hPlayPause);
}


UINT CgRPCFileClientDlg::AIThread(LPVOID pParam) //제일 늦음. 모든 속도가 AI 처리 부분에 종속됨
{

	CgRPCFileClientDlg* pDlg = (CgRPCFileClientDlg*)pParam;

	CRect frame_rect;
	pDlg->m_cctv_frame.GetWindowRect(frame_rect);

	CRect frame_rect2;
	pDlg->m_ROIcctv_frame.GetWindowRect(frame_rect2);

	pybind11::scoped_interpreter guard{};

	try {

		auto exampleModule4 = pybind11::module_::import("ACR.mp_detect");
		auto func_mp_pose = exampleModule4.attr("mediapipe_pose_func");
		auto func_mp_hand = exampleModule4.attr("mediapipe_hand_func");


		while (pDlg->m_exit.load() == 0)
		{

			WaitForSingleObject(pDlg->hAIStart, INFINITE);			
	
		
			if (!pDlg->m_cap_img.empty())
			{
				cv::Mat dst_img = pDlg->m_cap_img.clone();
				if (dst_img.channels() == 4) cvtColor(dst_img, dst_img, COLOR_BGRA2BGR);
				auto result_pose_mp = func_mp_pose(pybind11::cast(dst_img));
				auto mp_result = result_pose_mp.cast<std::vector<std::vector <double>>>();

				pDlg->m_sgcp_vtSkeletonMP.clear();		

				if (mp_result.size() >= 3)
				{
					for (int k = 0; k < mp_result[0].size(); k++)
					{
						pDlg->m_sgcp_vtSkeletonMP.push_back(Point3f(mp_result[0][k], mp_result[1][k], mp_result[2][k]));						
					}
				}
			}
		
			SetEvent(pDlg->hAIFinish);
		}
	}
	catch (py::error_already_set& e) {
		std::cout << e.what() << std::endl;
	}

	pDlg->hAIThread = nullptr;

	return 0;
}


UINT CgRPCFileClientDlg::gRPCSend(LPVOID pParam) //제일 늦음. 모든 속도가 AI 처리 부분에 종속됨
{

	CgRPCFileClientDlg* pDlg = (CgRPCFileClientDlg*)pParam;

	client.SendFrames("SESSION_001");

	pDlg->hgRPCSend = nullptr;

	return 0;
}



double CgRPCFileClientDlg::GetMotionVauleWithOpticalFlow(cv::Mat prevGray, std::vector<cv::Point2f> prevPts, cv::Mat gray)
{

	std::vector<cv::Point2f> nextPts;
	std::vector<unsigned char> status;
	std::vector<float> err;

	cv::calcOpticalFlowPyrLK(
		prevGray, gray,
		prevPts, nextPts,
		status, err
	);

	double sumLen = 0.0;
	int cnt = 0;
	for (size_t i = 0; i < nextPts.size(); ++i) {
		if (!status[i]) continue;
		double dx = nextPts[i].x - prevPts[i].x;
		double dy = nextPts[i].y - prevPts[i].y;
		double len = std::sqrt(dx * dx + dy * dy);
		sumLen += len;
		cnt++;
	}

	double avgMotion = (cnt > 0) ? (sumLen / cnt) : 0.0;
	std::cout << "avg motion (LK): " << avgMotion << std::endl;

	return avgMotion;
}


double CgRPCFileClientDlg::GetMotionVauleWithOpticalFlow(cv::Mat frame)
{
	static cv::Mat prev;
	cv::Rect rect;

	if (frame.empty())
	{
		cv::Mat empty;
		prev = empty;

		return -1;
	}

	double avgMotion = 10000.0;

	if (!prev.empty()) avgMotion = GetMotionVauleWithOpticalFlow(prev, frame, rect);

	prev = frame.clone();

	return avgMotion;
}

double CgRPCFileClientDlg::GetMotionVauleWithOpticalFlow(cv::Mat prev, cv::Mat frame, cv::Rect rect)
{
	cv::Mat prevGray, gray;

	cv::cvtColor(prev, prevGray, cv::COLOR_BGR2GRAY);
	cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);

	if (!rect.empty())
	{
		prevGray = prevGray(rect);
		gray = gray(rect);
	}


	// 추적할 포인트 찾기
	std::vector<cv::Point2f> prevPts;
	cv::goodFeaturesToTrack(prevGray, prevPts, 300, 0.01, 7);



	std::vector<cv::Point2f> nextPts;
	std::vector<unsigned char> status;
	std::vector<float> err;

	cv::calcOpticalFlowPyrLK(
		prevGray, gray,
		prevPts, nextPts,
		status, err
	);

	double sumLen = 0.0;
	int cnt = 0;
	for (size_t i = 0; i < nextPts.size(); ++i) {
		if (!status[i]) continue;
		double dx = nextPts[i].x - prevPts[i].x;
		double dy = nextPts[i].y - prevPts[i].y;
		double len = std::sqrt(dx * dx + dy * dy);
		sumLen += len;
		cnt++;
	}

	double avgMotion = (cnt > 0) ? (sumLen / cnt) : 0.0;
	std::cout << "avg motion (Optical Flow): " << avgMotion << std::endl;

	return avgMotion;
}

double CgRPCFileClientDlg::GetMotionVauleWithOpticalFlow(cv::Mat prev, cv::Mat frame, cv::Rect Rrect, cv::Rect Lrect)
{
	cv::Mat prevGray, gray;
	cv::Mat RprevGray, Rgray;
	cv::Mat LprevGray, Lgray;
	double RavgMotion = 0, LavgMotion = 0;


	cv::cvtColor(prev, prevGray, cv::COLOR_BGR2GRAY);
	cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);

	if (!Rrect.empty())
	{
		cv::Rect rect;
		RprevGray = prevGray(Rrect);
		Rgray = gray(Rrect);
		RavgMotion = GetMotionVauleWithOpticalFlow(RprevGray, Rgray, rect);
	}

	if (!Lrect.empty())
	{
		cv::Rect rect;
		LprevGray = prevGray(Lrect);
		Lgray = gray(Lrect);
		LavgMotion = GetMotionVauleWithOpticalFlow(LprevGray, Lgray, rect);
	}

	double avgMotion = RavgMotion + LavgMotion;
	std::cout << "avg motion (LK): " << avgMotion << std::endl;

	return avgMotion;
}


int CgRPCFileClientDlg::GetRoughHandStatusFromMP(std::vector<Point3f> mp_pose)
{
	int status = -1; //-1:error, 0:ready, 1:right hand, 2:left hand, 3:all hands

	if (mp_pose.size() <= 0) return -1;

	auto hl = GetRefHL2DPointsMP(mp_pose);

	if (hl[1].y <= READY_LOCATION && hl[0].y <= READY_LOCATION) status = 3;
	else if (hl[1].y <= READY_LOCATION && hl[0].y > READY_LOCATION) status = 1;
	else if (hl[1].y > READY_LOCATION && hl[0].y <= READY_LOCATION) status = 2;
	else status = 0;
	return status;
}


int CgRPCFileClientDlg::GetMotionStatusFromMP(int cur_motion_status, std::vector<cv::Point3f> cur_mp)
{
	static std::vector<cv::Point3f> prev_mp;

	if (cur_motion_status == RESET_MOTION_STATUS)
	{
		prev_mp.clear();
		return RESET_MOTION_STATUS;
	}

	int motion_status = cur_motion_status;

	if (prev_mp.size() <= 0)
	{
		prev_mp = cur_mp;
		return READY_MOTION_STATUS;
	}

	auto Rdev = m_util.Distance(cv::Point2f(prev_mp[16].x - cur_mp[16].x, prev_mp[16].y - cur_mp[16].y));
	auto Ldev = m_util.Distance(cv::Point2f(prev_mp[15].x - cur_mp[15].x, prev_mp[15].y - cur_mp[15].y));

	cout << "\n\n Rdev  Ldev: " << Rdev << " " << Ldev << "\n\n";

	if (Rdev > RAPID_DISTANCE || Ldev > RAPID_DISTANCE) return RAPID_MOTION_STATUS;

	if (cur_motion_status == READY_MOTION_STATUS)
	{
		int hand_status = GetRoughHandStatusFromMP(cur_mp);
		if (hand_status > 0) motion_status = SPEAK_MOTION_STATUS;
	}
	else if (cur_motion_status == SPEAK_MOTION_STATUS)
	{
		if (prev_mp.size() > 0 && cur_mp.size() > 0)
		{
			motion_status = READY_MOTION_STATUS;
		}
	}
	prev_mp = cur_mp;
	return motion_status;
}

std::vector<Point2f> CgRPCFileClientDlg::GetRefHL2DPointsMP(std::vector <cv::Point3f> joints) //0 Left, 1 Right
{
	std::vector<Point2f> ref_hl_points;


	if (joints.size() <= 0) return ref_hl_points;

	//2D Hand Location
	std::vector<Point2f> pixels;
	for (int k = 0; k < joints.size(); k++)
	{
		Point2f pt;
		pt.x = joints[k].x * 1920.0;
		pt.y = joints[k].y * 1080.0;
		pixels.push_back(pt);
	}

	//Geometric Transformation	
	vector<Point2f> corners = { pixels[12] , pixels[11] ,pixels[0] };

	Mat trans = cv::getAffineTransform(corners, m_warpCorners);

	vector<Point2f> hl_point = { pixels[15],pixels[16] }; //Left wrist, Right wrist
	std::vector<cv::Point2f> dst;

	cv::transform(hl_point, dst, trans);

	for (auto& pt : dst)
	{
		pt.x = std::clamp(pt.x, 0.0f, 1920.0f);
		pt.y = std::clamp(pt.y, 0.0f, 1080.0f);
		double offset = 0;
		pt.y += offset;
	}

	ref_hl_points.push_back(dst[0]);
	ref_hl_points.push_back(dst[1]);

	return ref_hl_points;
}

void CgRPCFileClientDlg::OnBnClickedButton9() //gRPC Send
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	client.pDlg = this;

	if (hgRPCSend == NULL) hgRPCSend = AfxBeginThread(gRPCSend, this);

	
}
```

## File: gRPCFileClientDlg.h
```
// gRPCFileClientDlg.h: 헤더 파일
//

#include "VideoUtil-v1.1.h"

// Python/pybind11 임베딩(ACR, 문장추론 등 파이썬 모델 호출)
#include <python.h>
#include <pybind11/pybind11.h>
#include <pybind11/embed.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include "HandTurnDetector.hpp"

#pragma once

//#define TARGETIP "localhost:50051"
#define TARGETIP "192.168.0.154:50051"

// 모션 상태(정지/말하기/급격동작 등)
#define RESET_MOTION_STATUS         -1  // 상태 초기화
#define READY_MOTION_STATUS         0   // 준비 상태
#define SPEAK_MOTION_STATUS         1   // 수어 발화 상태
#define RAPID_MOTION_STATUS         2   // 급격 동작 상태

// Optical Flow-based Motion Detection 파라미터
#define OPTICALFLOW_THRESH          0.45
#define OPTICAL_FLOW_HOLD_FRAME     3   // 모션 유지 프레임 수

#define READY_LOCATION              700

#define RAPID_DISTANCE              0.1 // 급격 동작 거리 기준(정규화 단위)

// ===============================
// pybind11 <-> cv::Mat 변환용 type_caster
// ===============================
//
// Python의 numpy.ndarray <-> OpenCV cv::Mat 간 상호 변환.
// 파이썬 ACR/AI 모듈과 OpenCV C++ 코드를 자연스럽게 연결하기 위해 사용.
//
namespace pybind11 {
    namespace detail {
        template<>
        struct type_caster<cv::Mat> {
        public:
            PYBIND11_TYPE_CASTER(cv::Mat, _("numpy.ndarray"));

            // 1. numpy.ndarray -> cv::Mat 변환
            bool load(handle obj, bool) {
                array b = reinterpret_borrow<array>(obj);
                buffer_info info = b.request();

                int nh = 1;
                int nw = 1;
                int nc = 1;
                int ndims = (int)info.ndim;

                if (ndims == 2) {
                    nh = (int)info.shape[0];
                    nw = (int)info.shape[1];
                }
                else if (ndims == 3) {
                    nh = (int)info.shape[0];
                    nw = (int)info.shape[1];
                    nc = (int)info.shape[2];
                }
                else {
                    char msg[64];
                    std::sprintf(msg,
                        "Unsupported dim %d, only support 2d, or 3-d", ndims);
                    throw std::logic_error(msg);
                    return false;
                }

                int dtype;
                if (info.format == format_descriptor<unsigned char>::format()) {
                    dtype = CV_8UC(nc);
                }
                else if (info.format == format_descriptor<int>::format()) {
                    dtype = CV_32SC(nc);
                }
                else if (info.format == format_descriptor<float>::format()) {
                    dtype = CV_32FC(nc);
                }
                else {
                    throw std::logic_error(
                        "Unsupported type, only support uchar, int32, float");
                    return false;
                }

                // data 포인터를 그대로 활용하는 얕은 래핑(복사X)
                value = cv::Mat(nh, nw, dtype, info.ptr);
                return true;
            }

            // 2. cv::Mat -> numpy.ndarray 변환
            static handle cast(const cv::Mat& mat,
                return_value_policy, handle defval) {
                UNUSED(defval);

                std::string format = format_descriptor<unsigned char>::format();
                size_t elemsize = sizeof(unsigned char);

                int nw = mat.cols;
                int nh = mat.rows;
                int nc = mat.channels();
                int depth = mat.depth();
                int type = mat.type();
                int dim = (depth == type) ? 2 : 3;

                if (depth == CV_8U) {
                    format = format_descriptor<unsigned char>::format();
                    elemsize = sizeof(unsigned char);
                }
                else if (depth == CV_32S) {
                    format = format_descriptor<int>::format();
                    elemsize = sizeof(int);
                }
                else if (depth == CV_32F) {
                    format = format_descriptor<float>::format();
                    elemsize = sizeof(float);
                }
                else {
                    throw std::logic_error(
                        "Unsupport type, only support uchar, int32, float");
                }

                std::vector<size_t> bufferdim;
                std::vector<size_t> strides;

                if (dim == 2) {
                    // 단일 채널: (h, w)
                    bufferdim = { (size_t)nh, (size_t)nw };
                    strides = { elemsize * (size_t)nw, elemsize };
                }
                else if (dim == 3) {
                    // 다채널: (h, w, c)
                    bufferdim = { (size_t)nh, (size_t)nw, (size_t)nc };
                    strides = {
                        (size_t)elemsize * nw * nc,
                        (size_t)elemsize * nc,
                        (size_t)elemsize
                    };
                }
                return array(buffer_info(
                    mat.data,
                    elemsize,
                    format,
                    dim,
                    bufferdim,
                    strides)).release();
            }
        };
    }
} // end namespace pybind11::detail


namespace py = pybind11;


// CgRPCFileClientDlg 대화 상자
class CgRPCFileClientDlg : public CDialogEx
{
// 생성입니다.
public:
	CgRPCFileClientDlg(CWnd* pParent = nullptr);	// 표준 생성자입니다.


// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_GRPCFILECLIENT_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.


// 구현입니다.
protected:
	HICON m_hIcon;

	// 생성된 메시지 맵 함수
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	std::atomic<bool> m_exit;
	Rect m_roi;

	// CFG (환경설정, 최근 경로 등) 저장/로드
	void SaveCFG();
	void ReadCFG();

	//Video Player 관련
	std::atomic<bool> m_pause;
	std::atomic<bool> m_back;
	std::atomic<bool> m_move;
	std::atomic<bool> m_stop;
	HANDLE hPlayPause;
	CWinThread* hVideoPlay = nullptr;
	static UINT VideoPlay(LPVOID pParam);

	CVideoUtil m_util;

	afx_msg void OnBnClickedButton1();
	CEdit m_video_file_name_edit;
	CStatic m_cctv_frame;
	CStatic m_ROIcctv_frame;
	CEdit m_frame_index_edit;
	CEdit m_roi_edit;
	CEdit m_frame_range_edit;
	CEdit m_info_edit;
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedButton2();
	afx_msg void OnBnClickedButton3();
	afx_msg void OnBnClickedButton4();
	afx_msg void OnBnClickedButton5();
	afx_msg void OnBnClickedButton7();
	afx_msg void OnBnClickedButton8();
	afx_msg void OnBnClickedButton10();

    cv::Mat m_cap_img;
    std::vector<cv::Point3f> m_sgcp_vtSkeletonMP;

    HandTurnDetector m_Rdetector;
    HandTurnDetector m_Ldetector;

	// AI 처리 시작/종료 이벤트 + AI 스레드 핸들
	HANDLE      hAIStart;
	HANDLE      hAIFinish;
	CWinThread* hAIThread = nullptr;
	// AI 스레드 실행 함수 (영상/프레임에 대해 ACR/MP 등 수행)
	static UINT AIThread(LPVOID pParam);

    CWinThread* hgRPCSend = nullptr;
    // AI 스레드 실행 함수 (영상/프레임에 대해 ACR/MP 등 수행)
    static UINT gRPCSend(LPVOID pParam);

    // Optical Flow 기반 모션 크기 계산(여러 오버로드)
    double GetMotionVauleWithOpticalFlow(
        cv::Mat prevGray,
        std::vector<cv::Point2f> prevPts,
        cv::Mat gray);

    double GetMotionVauleWithOpticalFlow(
        cv::Mat prev,
        cv::Mat frame,
        cv::Rect rect);

    double GetMotionVauleWithOpticalFlow(
        cv::Mat prev,
        cv::Mat frame,
        cv::Rect Rrect,
        cv::Rect Lrect);

    double GetMotionVauleWithOpticalFlow(cv::Mat frame);

    int GetRoughHandStatusFromMP(std::vector<Point3f> mp_pose);
    int GetMotionStatusFromMP(int cur_motion_status, std::vector<cv::Point3f> cur_mp);
    std::vector<Point2f> GetRefHL2DPointsMP(std::vector <cv::Point3f> joints); //0 Left, 1 Right

    // 곡선/그래프/warping 등에 사용하는 삼각 코너 좌표(예시)
    vector<Point2f> m_warpCorners = {
        Point2f(720, 500),
        Point2f(1200, 500),
        Point2f(960, 400)
    };
    
    afx_msg void OnBnClickedButton9();
};
```

## File: gRPCThread_.h
```
#include "ksl_sentence_recognition.grpc.pb.h"  // 당신의 *_grpc.pb.h

#include <grpcpp/grpcpp.h>

#include <opencv2/opencv.hpp>

#include <iostream>
#include <fstream>
#include <thread>

#include "gRPCFileClientDlg.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using grpc::ClientWriter;
using vision::raw::v1::Frame;
using vision::raw::v1::SubmitResultResponse;
using vision::raw::v1::SequenceService;

#pragma once

namespace grpc_client_ {


    // UTF-8 -> std::wstring
    inline std::wstring Utf8ToWstring(const std::string& s) {
        if (s.empty()) return L"";
        int len = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0);
        std::wstring w(len, 0);
        MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), &w[0], len);
        return w;
    }

    // std::wstring -> UTF-8
    inline std::string WstringToUtf8(const std::wstring& w) {
        if (w.empty()) return "";
        int len = WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), nullptr, 0, nullptr, nullptr);
        std::string s(len, 0);
        WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), &s[0], len, nullptr, nullptr);
        return s;
    }

    // CString <-> UTF-8
    inline std::string CStringToUtf8(const CString& cs) {
#ifdef UNICODE
        return WstringToUtf8(std::wstring(cs));
#else
        // CStringA일 때는 먼저 UTF-16으로 변환 후 UTF-8로
        int lenW = MultiByteToWideChar(CP_ACP, 0, cs, -1, nullptr, 0);
        std::wstring w(lenW - 1, 0);
        MultiByteToWideChar(CP_ACP, 0, cs, -1, &w[0], lenW - 1);
        return WstringToUtf8(w);
#endif
    }

    inline CString Utf8ToCString(const std::string& s) {
#ifdef UNICODE
        return CString(Utf8ToWstring(s).c_str());
#else
        // UTF-8 -> UTF-16 -> ANSI
        std::wstring w = Utf8ToWstring(s);
        int lenA = WideCharToMultiByte(CP_ACP, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
        std::string a(lenA, 0);
        WideCharToMultiByte(CP_ACP, 0, w.c_str(), (int)w.size(), &a[0], lenA, nullptr, nullptr);
        return CString(a.c_str());
#endif
    }

    inline size_t ElemSizeFromType(int cv_type) {
        return CV_ELEM_SIZE(cv_type); // OpenCV 매크로
    }

    inline bool EncodeFrame(const cv::Mat& img,
        const std::string& session_id,
        int index, int flag, std::vector<cv::Point3f>& mpPose3D,
        Frame* out)
    {
        if (!out) return false;
        if (img.empty()) return false;

        // 연속 메모리 보장 (protobuf로 직렬화할 때는 연속 버퍼가 편함)
        cv::Mat contiguous;
        const cv::Mat& src = img.isContinuous() ? img : (contiguous = img.clone());

        const int width = src.cols;
        const int height = src.rows;
        const int type = src.type();
        const size_t elem_size = ElemSizeFromType(type);
        const size_t expected = static_cast<size_t>(width) * height * elem_size;

        // Frame 필드 세팅
        out->Clear();
        out->set_session_id(session_id);
        out->set_index(index);
        out->set_flag(flag);
        out->set_width(width);
        out->set_height(height);
        out->set_type(type);
     
        for (const auto& p : mpPose3D) {
            auto* pt = out->add_pose_points();
            pt->set_x(p.x);
            pt->set_y(p.y);
            pt->set_z(p.z);
        }

        // 데이터 복사 (protobuf는 소유권을 가져야 하므로 copy)
        out->set_data(reinterpret_cast<const char*>(src.data), expected);
        return true;
    }

    /**
     * @brief Frame -> cv::Mat
     * @details Frame.data()는 행간 padding 없이 "꽉 찬" 형태로 직렬화되어 있다고 가정
     * @return 변환된 Mat (실패 시 empty Mat)
     */
    inline cv::Mat FrameToMat(const Frame& f)
    {
        const int width = f.width();
        const int height = f.height();
        const int type = f.type();

        if (width <= 0 || height <= 0) return {};

        const size_t elem_size = ElemSizeFromType(type);
        const size_t expected = static_cast<size_t>(width) * height * elem_size;

        const std::string& blob = f.data();
        if (blob.size() != expected) {
            // 크기 불일치: 프로토콜/전송 측 정합성 문제
            return {};
        }

        // data 포인터를 참조하는 Mat를 만들고, 바로 clone() 해서 독립 데이터로 보관
        // (protobuf 문자열 수명과 무관하게 사용 가능)
        cv::Mat wrapped(height, width, type,
            const_cast<char*>(blob.data())); // 참조만
        return wrapped.clone(); // 안전하게 소유 복사
    }

    /** (선택) BGR<->RGB 변환 헬퍼
     * OpenCV 기본은 BGR. 상대 쪽이 RGB를 기대한다면 사용.
     */
    inline cv::Mat BGR2RGB(const cv::Mat& bgr) {
        cv::Mat rgb;
        if (!bgr.empty()) cv::cvtColor(bgr, rgb, cv::COLOR_BGR2RGB);
        return rgb;
    }

    inline cv::Mat RGB2BGR(const cv::Mat& rgb) {
        cv::Mat bgr;
        if (!rgb.empty()) cv::cvtColor(rgb, bgr, cv::COLOR_RGB2BGR);
        return bgr;
    }

    class SequenceClient {
    public:
        SequenceClient(std::shared_ptr<Channel> channel)
            : stub_(SequenceService::NewStub(channel)) {
        }

        CgRPCFileClientDlg* pDlg;

        void SendFrames(const std::string& session_id) {
            ClientContext context;
            SubmitResultResponse response;
            std::unique_ptr<ClientWriter<Frame>> writer(
                stub_->SendFrames(&context, &response));
          
            CString file_str, frame_range_str;

            pDlg->m_video_file_name_edit.GetWindowText(file_str);
            pDlg->m_frame_range_edit.GetWindowText(frame_range_str);

            if (!file_str.IsEmpty() && !frame_range_str.IsEmpty())
            {

                CRect Rect1;
                pDlg->m_cctv_frame.GetClientRect(Rect1);

                CRect Rect2;
                pDlg->m_ROIcctv_frame.GetClientRect(Rect2);

                auto strs2 = pDlg->m_util.SplitCString(frame_range_str);

                if (strs2.size() >= 2)
                {
                    double dt = 1.0 / 30.0;
                    int frozenflag = 0;
                    int speakstartflag = 0;
                    int RTurncnt = 0;
                    int Lturncnt = 0;
                    int Holdcnt = 0;
                    int motion_status = READY_MOTION_STATUS;

                    cv::Mat empty;
                    pDlg->GetMotionVauleWithOpticalFlow(empty);
                    pDlg->GetMotionStatusFromMP(RESET_MOTION_STATUS, {});

                    pDlg->m_Rdetector.reset();
                    pDlg->m_Ldetector.reset();               

                    VideoCapture cap(pDlg->m_util.StringToChar(file_str));

                    Mat img;
                    if (cap.isOpened())
                    {
                        auto [w, h, fps, totalFrames] = pDlg->m_util.GetVideoInfo(cap);

                        CString str;
                        str.Format(_T("[비디오파일 정보]\r\n%s\r\nwidht:%d height:%d fps:%.2f total frames:%d"), file_str, w, h, fps, totalFrames);
                        pDlg->m_info_edit.SetWindowText(str);

                        cap.set(cv::CAP_PROP_POS_FRAMES, _ttoi(strs2[0]));  // targetIndex 프레임으로 이동

                        double frame_count = cap.get(cv::CAP_PROP_FRAME_COUNT);

                        while (pDlg->m_exit.load() == false)
                        {
                            cap >> img;
                            int frameIndex = (int)cap.get(cv::CAP_PROP_POS_FRAMES);
                            if (img.empty() || frameIndex > _ttoi(strs2[1])) break;
                            auto cap_img = img.clone();
                            rectangle(cap_img, pDlg->m_roi, Scalar(0, 255, 0), 3);

                            pDlg->m_util.DrawImageBMP(&pDlg->m_cctv_frame, cap_img, 0, 0, (double)frameIndex / (totalFrames - 1), frameIndex);
                            cv::Mat roi = img(pDlg->m_roi).clone();
                            pDlg->m_util.DrawImageBMP(&pDlg->m_ROIcctv_frame, roi, 0, 0);
                            if (roi.cols >= 320)
                            {
                                cv::Mat resize_roi_img;
                                double src_x = static_cast<double>(roi.cols);
                                double src_y = static_cast<double>(roi.rows);
                                // 목표 가로 크기
                                double target_x = 320.0;
                                // 비율 계산
                                double scale = target_x / src_x;    // x축 기준 스케일
                                double dst_x_rate = scale;
                                double dst_y_rate = scale;          // y축 동일 비율 적용
                                // 이미지 리사이즈
                                cv::resize(roi, resize_roi_img, cv::Size(), dst_x_rate, dst_y_rate, cv::INTER_LINEAR);
                                pDlg->m_cap_img = resize_roi_img;
                            }
                            else pDlg->m_cap_img = roi;

                            SetEvent(pDlg->hAIStart);
                            WaitForSingleObject(pDlg->hAIFinish, INFINITE); //Wait for results		

                            if (pDlg->m_sgcp_vtSkeletonMP.size() > 0)
                            {
                                int hand_status = pDlg->GetRoughHandStatusFromMP(pDlg->m_sgcp_vtSkeletonMP); //손의 위치로 준비, 우세손, 비우세손, 양손 결정
                                motion_status = pDlg->GetMotionStatusFromMP(motion_status, pDlg->m_sgcp_vtSkeletonMP); //준비단계인지 수어 단계인지 구분

                                //             if (hand_status !=0 && (motion_status==SPEAK_MOTION_STATUS || frame_count > MIN_FRAME) )
                                if (hand_status != 0)
                                {
                                    double  avgMotion = pDlg->GetMotionVauleWithOpticalFlow(roi);

                                    bool Rhit = pDlg->m_Rdetector.update({ pDlg->m_sgcp_vtSkeletonMP[16].x, pDlg->m_sgcp_vtSkeletonMP[16].y }, dt);
                                    bool Lhit = pDlg->m_Ldetector.update({ pDlg->m_sgcp_vtSkeletonMP[15].x, pDlg->m_sgcp_vtSkeletonMP[15].y }, dt);

                                    if (Rhit)
                                    {
                                        RTurncnt++;
                                        speakstartflag = 1;
                                        //		m_cp_indexs.push_back((int)(frame_count));
                                    }
                                    else if (Lhit)
                                    {
                                        Lturncnt++;
                                        speakstartflag = 1;
                                        //		m_cp_indexs.push_back((int)(frame_count));
                                    }
                                    else if (speakstartflag && avgMotion < OPTICALFLOW_THRESH && frozenflag == 0)
                                    {
                                        Holdcnt++;                             

                                        Frame frame;
                                        EncodeFrame(roi, session_id, frameIndex, 1, pDlg->m_sgcp_vtSkeletonMP,&frame);
                                        writer->Write(frame);

                                        frozenflag = OPTICAL_FLOW_HOLD_FRAME;
                                    }
                                }
                            }
                            if (frozenflag > 0) frozenflag--;
                        }
                        cap.release();
                    }                    
                }
            }
            writer->WritesDone();
            Status status = writer->Finish();

            if (status.ok()) {
               
                std::wstring msg = Utf8ToWstring(response.message());
                CStringW cws(msg.c_str());
                CStringW str;
                str.Format(_T("[문장추론결과]\r\n%s"), cws);
                pDlg->m_info_edit.SetWindowTextW(str);              
            }
            else {
                std::cerr << "[Client] RPC failed: " << status.error_message() << "\n";
            }
        }

    private:
        std::unique_ptr<SequenceService::Stub> stub_;
    };
}
```

## File: HandTurnDetector.hpp
```
#include <opencv2\opencv.hpp>

class HandTurnDetector {
public:
	// angleDegTh : 방향이 이 각도 이상 바뀌면 전환으로 본다 (도 단위)
	// speedRatioTh : 다음 속도가 이전 속도의 이 비율보다 작으면 감속으로 본다 (예: 0.8 -> 20% 이상 감소)
	// minSpeed : 이 값보다 느리면 방향이 의미 없다고 보고 스킵
	HandTurnDetector(float angleDegTh = 30.0f,
		float speedRatioTh = 0.8f,
		float minSpeed = 1e-3f)
		: angleRadTh_(angleDegTh* static_cast<float>(CV_PI) / 180.0f),
		speedRatioTh_(speedRatioTh),
		minSpeed_(minSpeed)
	{
		reset(); // 생성 시 내부 상태 초기화
	}

	// 내부 상태를 완전히 초기화 (새 시퀀스 시작용)
	void reset()
	{
		hasPrevPos_ = false;
		hasPrevVel_ = false;
		frameIdx_ = 0;

		prevPos_ = cv::Point2f(0.0f, 0.0f);
		prevVel_ = cv::Point2f(0.0f, 0.0f);
	}

	// (옵션) 파라미터를 런타임에 변경하고 싶을 때 사용
	void setParams(float angleDegTh, float speedRatioTh, float minSpeed)
	{
		angleRadTh_ = angleDegTh * static_cast<float>(CV_PI) / 180.0f;
		speedRatioTh_ = speedRatioTh;
		minSpeed_ = minSpeed;
	}

	// 실시간으로 매 프레임 호출
	// pos : 이번 프레임 손 위치
	// dt  : 이전 프레임과 시간 차이(초). 고정 fps면 1.0/fps 로 넣어도 됨.
	// 반환값 : 이번 프레임에서 "방향 전환 + 감속"이 동시에 발생했으면 true
	bool update(const cv::Point2f& pos, double dt)
	{
		bool detected = false;

		if (!hasPrevPos_) {
			prevPos_ = pos;
			hasPrevPos_ = true;
			frameIdx_++;
			return false;
		}

		// 위치 -> 속도 (dt가 0이면 그냥 위치 차이만 보자)
		cv::Point2f vel;
		if (dt > 1e-6) {
			vel = (pos - prevPos_) * (1.0f / static_cast<float>(dt));
		}
		else {
			vel = (pos - prevPos_);
		}

		float speed = std::sqrt(vel.x * vel.x + vel.y * vel.y);

		if (hasPrevVel_) {
			float prevSpeed = std::sqrt(prevVel_.x * prevVel_.x + prevVel_.y * prevVel_.y);

			// 너무 느릴 때는 방향 의미 X
			// if (speed > minSpeed_ && prevSpeed > minSpeed_)
			if (prevSpeed > minSpeed_)
			{
				// 단위벡터
				cv::Point2f u1 = prevVel_ * (1.0f / prevSpeed);
				cv::Point2f u2 = vel * (1.0f / (speed + 1e-6f)); // 0 나누기 방지용 약간의 epsilon

				// 내적 -> 각도
				float dot = u1.x * u2.x + u1.y * u2.y;
				dot = std::max(-1.0f, std::min(1.0f, dot));
				float dtheta = std::acos(dot); // rad

				// 속력 비
				float ratio = speed / (prevSpeed + 1e-6f);

				// 두 조건을 동시에 만족해야 "유효한 코너"로 본다
				if (dtheta > angleRadTh_ && ratio < speedRatioTh_) {
					detected = true;
					// 여기서 원하는 후처리를 하면 된다 (로그, 이벤트 발생, 세그먼트 끊기 등)
					// 예: std::cout << "Turn+Slow at frame " << frameIdx_ << std::endl;
				}
			}
		}

		// 다음 프레임을 위해 저장
		prevPos_ = pos;
		prevVel_ = vel;
		hasPrevVel_ = true;
		frameIdx_++;

		return detected;
	}

private:
	float angleRadTh_;
	float speedRatioTh_;
	float minSpeed_;

	cv::Point2f prevPos_;
	cv::Point2f prevVel_;
	bool hasPrevPos_;
	bool hasPrevVel_;

	int frameIdx_; // 디버깅용
};
```

## File: ksl_sentence_recognition.grpc.pb.cc
```
// Generated by the gRPC C++ plugin.
// If you make any local change, they will be lost.
// source: ksl_sentence_recognition.proto

#include "ksl_sentence_recognition.pb.h"
#include "ksl_sentence_recognition.grpc.pb.h"

#include <functional>
#include <grpcpp/support/async_stream.h>
#include <grpcpp/support/async_unary_call.h>
#include <grpcpp/impl/channel_interface.h>
#include <grpcpp/impl/client_unary_call.h>
#include <grpcpp/support/client_callback.h>
#include <grpcpp/support/message_allocator.h>
#include <grpcpp/support/method_handler.h>
#include <grpcpp/impl/rpc_service_method.h>
#include <grpcpp/support/server_callback.h>
#include <grpcpp/impl/server_callback_handlers.h>
#include <grpcpp/server_context.h>
#include <grpcpp/impl/service_type.h>
#include <grpcpp/support/sync_stream.h>
namespace vision {
namespace raw {
namespace v1 {

static const char* SequenceService_method_names[] = {
  "/vision.raw.v1.SequenceService/SendFrames",
};

std::unique_ptr< SequenceService::Stub> SequenceService::NewStub(const std::shared_ptr< ::grpc::ChannelInterface>& channel, const ::grpc::StubOptions& options) {
  (void)options;
  std::unique_ptr< SequenceService::Stub> stub(new SequenceService::Stub(channel, options));
  return stub;
}

SequenceService::Stub::Stub(const std::shared_ptr< ::grpc::ChannelInterface>& channel, const ::grpc::StubOptions& options)
  : channel_(channel), rpcmethod_SendFrames_(SequenceService_method_names[0], options.suffix_for_stats(),::grpc::internal::RpcMethod::CLIENT_STREAMING, channel)
  {}

::grpc::ClientWriter< ::vision::raw::v1::Frame>* SequenceService::Stub::SendFramesRaw(::grpc::ClientContext* context, ::vision::raw::v1::SubmitResultResponse* response) {
  return ::grpc::internal::ClientWriterFactory< ::vision::raw::v1::Frame>::Create(channel_.get(), rpcmethod_SendFrames_, context, response);
}

void SequenceService::Stub::async::SendFrames(::grpc::ClientContext* context, ::vision::raw::v1::SubmitResultResponse* response, ::grpc::ClientWriteReactor< ::vision::raw::v1::Frame>* reactor) {
  ::grpc::internal::ClientCallbackWriterFactory< ::vision::raw::v1::Frame>::Create(stub_->channel_.get(), stub_->rpcmethod_SendFrames_, context, response, reactor);
}

::grpc::ClientAsyncWriter< ::vision::raw::v1::Frame>* SequenceService::Stub::AsyncSendFramesRaw(::grpc::ClientContext* context, ::vision::raw::v1::SubmitResultResponse* response, ::grpc::CompletionQueue* cq, void* tag) {
  return ::grpc::internal::ClientAsyncWriterFactory< ::vision::raw::v1::Frame>::Create(channel_.get(), cq, rpcmethod_SendFrames_, context, response, true, tag);
}

::grpc::ClientAsyncWriter< ::vision::raw::v1::Frame>* SequenceService::Stub::PrepareAsyncSendFramesRaw(::grpc::ClientContext* context, ::vision::raw::v1::SubmitResultResponse* response, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncWriterFactory< ::vision::raw::v1::Frame>::Create(channel_.get(), cq, rpcmethod_SendFrames_, context, response, false, nullptr);
}

SequenceService::Service::Service() {
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      SequenceService_method_names[0],
      ::grpc::internal::RpcMethod::CLIENT_STREAMING,
      new ::grpc::internal::ClientStreamingHandler< SequenceService::Service, ::vision::raw::v1::Frame, ::vision::raw::v1::SubmitResultResponse>(
          [](SequenceService::Service* service,
             ::grpc::ServerContext* ctx,
             ::grpc::ServerReader<::vision::raw::v1::Frame>* reader,
             ::vision::raw::v1::SubmitResultResponse* resp) {
               return service->SendFrames(ctx, reader, resp);
             }, this)));
}

SequenceService::Service::~Service() {
}

::grpc::Status SequenceService::Service::SendFrames(::grpc::ServerContext* context, ::grpc::ServerReader< ::vision::raw::v1::Frame>* reader, ::vision::raw::v1::SubmitResultResponse* response) {
  (void) context;
  (void) reader;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}


}  // namespace vision
}  // namespace raw
}  // namespace v1
```

## File: ksl_sentence_recognition.grpc.pb.h
```
// Generated by the gRPC C++ plugin.
// If you make any local change, they will be lost.
// source: ksl_sentence_recognition.proto
#ifndef GRPC_ksl_5fsentence_5frecognition_2eproto__INCLUDED
#define GRPC_ksl_5fsentence_5frecognition_2eproto__INCLUDED

#include "ksl_sentence_recognition.pb.h"

#include <functional>
#include <grpcpp/generic/async_generic_service.h>
#include <grpcpp/support/async_stream.h>
#include <grpcpp/support/async_unary_call.h>
#include <grpcpp/support/client_callback.h>
#include <grpcpp/client_context.h>
#include <grpcpp/completion_queue.h>
#include <grpcpp/support/message_allocator.h>
#include <grpcpp/support/method_handler.h>
#include <grpcpp/impl/proto_utils.h>
#include <grpcpp/impl/rpc_method.h>
#include <grpcpp/support/server_callback.h>
#include <grpcpp/impl/server_callback_handlers.h>
#include <grpcpp/server_context.h>
#include <grpcpp/impl/service_type.h>
#include <grpcpp/support/status.h>
#include <grpcpp/support/stub_options.h>
#include <grpcpp/support/sync_stream.h>

namespace vision {
namespace raw {
namespace v1 {

class SequenceService final {
 public:
  static constexpr char const* service_full_name() {
    return "vision.raw.v1.SequenceService";
  }
  class StubInterface {
   public:
    virtual ~StubInterface() {}
    std::unique_ptr< ::grpc::ClientWriterInterface< ::vision::raw::v1::Frame>> SendFrames(::grpc::ClientContext* context, ::vision::raw::v1::SubmitResultResponse* response) {
      return std::unique_ptr< ::grpc::ClientWriterInterface< ::vision::raw::v1::Frame>>(SendFramesRaw(context, response));
    }
    std::unique_ptr< ::grpc::ClientAsyncWriterInterface< ::vision::raw::v1::Frame>> AsyncSendFrames(::grpc::ClientContext* context, ::vision::raw::v1::SubmitResultResponse* response, ::grpc::CompletionQueue* cq, void* tag) {
      return std::unique_ptr< ::grpc::ClientAsyncWriterInterface< ::vision::raw::v1::Frame>>(AsyncSendFramesRaw(context, response, cq, tag));
    }
    std::unique_ptr< ::grpc::ClientAsyncWriterInterface< ::vision::raw::v1::Frame>> PrepareAsyncSendFrames(::grpc::ClientContext* context, ::vision::raw::v1::SubmitResultResponse* response, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncWriterInterface< ::vision::raw::v1::Frame>>(PrepareAsyncSendFramesRaw(context, response, cq));
    }
    class async_interface {
     public:
      virtual ~async_interface() {}
      virtual void SendFrames(::grpc::ClientContext* context, ::vision::raw::v1::SubmitResultResponse* response, ::grpc::ClientWriteReactor< ::vision::raw::v1::Frame>* reactor) = 0;
    };
    typedef class async_interface experimental_async_interface;
    virtual class async_interface* async() { return nullptr; }
    class async_interface* experimental_async() { return async(); }
   private:
    virtual ::grpc::ClientWriterInterface< ::vision::raw::v1::Frame>* SendFramesRaw(::grpc::ClientContext* context, ::vision::raw::v1::SubmitResultResponse* response) = 0;
    virtual ::grpc::ClientAsyncWriterInterface< ::vision::raw::v1::Frame>* AsyncSendFramesRaw(::grpc::ClientContext* context, ::vision::raw::v1::SubmitResultResponse* response, ::grpc::CompletionQueue* cq, void* tag) = 0;
    virtual ::grpc::ClientAsyncWriterInterface< ::vision::raw::v1::Frame>* PrepareAsyncSendFramesRaw(::grpc::ClientContext* context, ::vision::raw::v1::SubmitResultResponse* response, ::grpc::CompletionQueue* cq) = 0;
  };
  class Stub final : public StubInterface {
   public:
    Stub(const std::shared_ptr< ::grpc::ChannelInterface>& channel, const ::grpc::StubOptions& options = ::grpc::StubOptions());
    std::unique_ptr< ::grpc::ClientWriter< ::vision::raw::v1::Frame>> SendFrames(::grpc::ClientContext* context, ::vision::raw::v1::SubmitResultResponse* response) {
      return std::unique_ptr< ::grpc::ClientWriter< ::vision::raw::v1::Frame>>(SendFramesRaw(context, response));
    }
    std::unique_ptr< ::grpc::ClientAsyncWriter< ::vision::raw::v1::Frame>> AsyncSendFrames(::grpc::ClientContext* context, ::vision::raw::v1::SubmitResultResponse* response, ::grpc::CompletionQueue* cq, void* tag) {
      return std::unique_ptr< ::grpc::ClientAsyncWriter< ::vision::raw::v1::Frame>>(AsyncSendFramesRaw(context, response, cq, tag));
    }
    std::unique_ptr< ::grpc::ClientAsyncWriter< ::vision::raw::v1::Frame>> PrepareAsyncSendFrames(::grpc::ClientContext* context, ::vision::raw::v1::SubmitResultResponse* response, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncWriter< ::vision::raw::v1::Frame>>(PrepareAsyncSendFramesRaw(context, response, cq));
    }
    class async final :
      public StubInterface::async_interface {
     public:
      void SendFrames(::grpc::ClientContext* context, ::vision::raw::v1::SubmitResultResponse* response, ::grpc::ClientWriteReactor< ::vision::raw::v1::Frame>* reactor) override;
     private:
      friend class Stub;
      explicit async(Stub* stub): stub_(stub) { }
      Stub* stub() { return stub_; }
      Stub* stub_;
    };
    class async* async() override { return &async_stub_; }

   private:
    std::shared_ptr< ::grpc::ChannelInterface> channel_;
    class async async_stub_{this};
    ::grpc::ClientWriter< ::vision::raw::v1::Frame>* SendFramesRaw(::grpc::ClientContext* context, ::vision::raw::v1::SubmitResultResponse* response) override;
    ::grpc::ClientAsyncWriter< ::vision::raw::v1::Frame>* AsyncSendFramesRaw(::grpc::ClientContext* context, ::vision::raw::v1::SubmitResultResponse* response, ::grpc::CompletionQueue* cq, void* tag) override;
    ::grpc::ClientAsyncWriter< ::vision::raw::v1::Frame>* PrepareAsyncSendFramesRaw(::grpc::ClientContext* context, ::vision::raw::v1::SubmitResultResponse* response, ::grpc::CompletionQueue* cq) override;
    const ::grpc::internal::RpcMethod rpcmethod_SendFrames_;
  };
  static std::unique_ptr<Stub> NewStub(const std::shared_ptr< ::grpc::ChannelInterface>& channel, const ::grpc::StubOptions& options = ::grpc::StubOptions());

  class Service : public ::grpc::Service {
   public:
    Service();
    virtual ~Service();
    virtual ::grpc::Status SendFrames(::grpc::ServerContext* context, ::grpc::ServerReader< ::vision::raw::v1::Frame>* reader, ::vision::raw::v1::SubmitResultResponse* response);
  };
  template <class BaseClass>
  class WithAsyncMethod_SendFrames : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithAsyncMethod_SendFrames() {
      ::grpc::Service::MarkMethodAsync(0);
    }
    ~WithAsyncMethod_SendFrames() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status SendFrames(::grpc::ServerContext* /*context*/, ::grpc::ServerReader< ::vision::raw::v1::Frame>* /*reader*/, ::vision::raw::v1::SubmitResultResponse* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    void RequestSendFrames(::grpc::ServerContext* context, ::grpc::ServerAsyncReader< ::vision::raw::v1::SubmitResultResponse, ::vision::raw::v1::Frame>* reader, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag) {
      ::grpc::Service::RequestAsyncClientStreaming(0, context, reader, new_call_cq, notification_cq, tag);
    }
  };
  typedef WithAsyncMethod_SendFrames<Service > AsyncService;
  template <class BaseClass>
  class WithCallbackMethod_SendFrames : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithCallbackMethod_SendFrames() {
      ::grpc::Service::MarkMethodCallback(0,
          new ::grpc::internal::CallbackClientStreamingHandler< ::vision::raw::v1::Frame, ::vision::raw::v1::SubmitResultResponse>(
            [this](
                   ::grpc::CallbackServerContext* context, ::vision::raw::v1::SubmitResultResponse* response) { return this->SendFrames(context, response); }));
    }
    ~WithCallbackMethod_SendFrames() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status SendFrames(::grpc::ServerContext* /*context*/, ::grpc::ServerReader< ::vision::raw::v1::Frame>* /*reader*/, ::vision::raw::v1::SubmitResultResponse* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    virtual ::grpc::ServerReadReactor< ::vision::raw::v1::Frame>* SendFrames(
      ::grpc::CallbackServerContext* /*context*/, ::vision::raw::v1::SubmitResultResponse* /*response*/)  { return nullptr; }
  };
  typedef WithCallbackMethod_SendFrames<Service > CallbackService;
  typedef CallbackService ExperimentalCallbackService;
  template <class BaseClass>
  class WithGenericMethod_SendFrames : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithGenericMethod_SendFrames() {
      ::grpc::Service::MarkMethodGeneric(0);
    }
    ~WithGenericMethod_SendFrames() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status SendFrames(::grpc::ServerContext* /*context*/, ::grpc::ServerReader< ::vision::raw::v1::Frame>* /*reader*/, ::vision::raw::v1::SubmitResultResponse* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
  };
  template <class BaseClass>
  class WithRawMethod_SendFrames : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithRawMethod_SendFrames() {
      ::grpc::Service::MarkMethodRaw(0);
    }
    ~WithRawMethod_SendFrames() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status SendFrames(::grpc::ServerContext* /*context*/, ::grpc::ServerReader< ::vision::raw::v1::Frame>* /*reader*/, ::vision::raw::v1::SubmitResultResponse* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    void RequestSendFrames(::grpc::ServerContext* context, ::grpc::ServerAsyncReader< ::grpc::ByteBuffer, ::grpc::ByteBuffer>* reader, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag) {
      ::grpc::Service::RequestAsyncClientStreaming(0, context, reader, new_call_cq, notification_cq, tag);
    }
  };
  template <class BaseClass>
  class WithRawCallbackMethod_SendFrames : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithRawCallbackMethod_SendFrames() {
      ::grpc::Service::MarkMethodRawCallback(0,
          new ::grpc::internal::CallbackClientStreamingHandler< ::grpc::ByteBuffer, ::grpc::ByteBuffer>(
            [this](
                   ::grpc::CallbackServerContext* context, ::grpc::ByteBuffer* response) { return this->SendFrames(context, response); }));
    }
    ~WithRawCallbackMethod_SendFrames() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status SendFrames(::grpc::ServerContext* /*context*/, ::grpc::ServerReader< ::vision::raw::v1::Frame>* /*reader*/, ::vision::raw::v1::SubmitResultResponse* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    virtual ::grpc::ServerReadReactor< ::grpc::ByteBuffer>* SendFrames(
      ::grpc::CallbackServerContext* /*context*/, ::grpc::ByteBuffer* /*response*/)  { return nullptr; }
  };
  typedef Service StreamedUnaryService;
  typedef Service SplitStreamedService;
  typedef Service StreamedService;
};

}  // namespace v1
}  // namespace raw
}  // namespace vision


#endif  // GRPC_ksl_5fsentence_5frecognition_2eproto__INCLUDED
```

## File: ksl_sentence_recognition.pb.cc
```
// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: ksl_sentence_recognition.proto

#include "ksl_sentence_recognition.pb.h"

#include <algorithm>
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/extension_set.h"
#include "google/protobuf/wire_format_lite.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/generated_message_reflection.h"
#include "google/protobuf/reflection_ops.h"
#include "google/protobuf/wire_format.h"
#include "google/protobuf/generated_message_tctable_impl.h"
// @@protoc_insertion_point(includes)

// Must be included last.
#include "google/protobuf/port_def.inc"
PROTOBUF_PRAGMA_INIT_SEG
namespace _pb = ::google::protobuf;
namespace _pbi = ::google::protobuf::internal;
namespace _fl = ::google::protobuf::internal::field_layout;
namespace vision {
namespace raw {
namespace v1 {

inline constexpr SubmitResultResponse::Impl_::Impl_(
    ::_pbi::ConstantInitialized) noexcept
      : session_id_(
            &::google::protobuf::internal::fixed_address_empty_string,
            ::_pbi::ConstantInitialized()),
        message_(
            &::google::protobuf::internal::fixed_address_empty_string,
            ::_pbi::ConstantInitialized()),
        frame_count_{0},
        _cached_size_{0} {}

template <typename>
PROTOBUF_CONSTEXPR SubmitResultResponse::SubmitResultResponse(::_pbi::ConstantInitialized)
    : _impl_(::_pbi::ConstantInitialized()) {}
struct SubmitResultResponseDefaultTypeInternal {
  PROTOBUF_CONSTEXPR SubmitResultResponseDefaultTypeInternal() : _instance(::_pbi::ConstantInitialized{}) {}
  ~SubmitResultResponseDefaultTypeInternal() {}
  union {
    SubmitResultResponse _instance;
  };
};

PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT
    PROTOBUF_ATTRIBUTE_INIT_PRIORITY1 SubmitResultResponseDefaultTypeInternal _SubmitResultResponse_default_instance_;

inline constexpr Point3::Impl_::Impl_(
    ::_pbi::ConstantInitialized) noexcept
      : x_{0},
        y_{0},
        z_{0},
        _cached_size_{0} {}

template <typename>
PROTOBUF_CONSTEXPR Point3::Point3(::_pbi::ConstantInitialized)
    : _impl_(::_pbi::ConstantInitialized()) {}
struct Point3DefaultTypeInternal {
  PROTOBUF_CONSTEXPR Point3DefaultTypeInternal() : _instance(::_pbi::ConstantInitialized{}) {}
  ~Point3DefaultTypeInternal() {}
  union {
    Point3 _instance;
  };
};

PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT
    PROTOBUF_ATTRIBUTE_INIT_PRIORITY1 Point3DefaultTypeInternal _Point3_default_instance_;

inline constexpr Frame::Impl_::Impl_(
    ::_pbi::ConstantInitialized) noexcept
      : pose_points_{},
        session_id_(
            &::google::protobuf::internal::fixed_address_empty_string,
            ::_pbi::ConstantInitialized()),
        data_(
            &::google::protobuf::internal::fixed_address_empty_string,
            ::_pbi::ConstantInitialized()),
        index_{0},
        flag_{0},
        width_{0},
        height_{0},
        type_{0},
        _cached_size_{0} {}

template <typename>
PROTOBUF_CONSTEXPR Frame::Frame(::_pbi::ConstantInitialized)
    : _impl_(::_pbi::ConstantInitialized()) {}
struct FrameDefaultTypeInternal {
  PROTOBUF_CONSTEXPR FrameDefaultTypeInternal() : _instance(::_pbi::ConstantInitialized{}) {}
  ~FrameDefaultTypeInternal() {}
  union {
    Frame _instance;
  };
};

PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT
    PROTOBUF_ATTRIBUTE_INIT_PRIORITY1 FrameDefaultTypeInternal _Frame_default_instance_;
}  // namespace v1
}  // namespace raw
}  // namespace vision
static ::_pb::Metadata file_level_metadata_ksl_5fsentence_5frecognition_2eproto[3];
static constexpr const ::_pb::EnumDescriptor**
    file_level_enum_descriptors_ksl_5fsentence_5frecognition_2eproto = nullptr;
static constexpr const ::_pb::ServiceDescriptor**
    file_level_service_descriptors_ksl_5fsentence_5frecognition_2eproto = nullptr;
const ::uint32_t TableStruct_ksl_5fsentence_5frecognition_2eproto::offsets[] PROTOBUF_SECTION_VARIABLE(
    protodesc_cold) = {
    ~0u,  // no _has_bits_
    PROTOBUF_FIELD_OFFSET(::vision::raw::v1::Point3, _internal_metadata_),
    ~0u,  // no _extensions_
    ~0u,  // no _oneof_case_
    ~0u,  // no _weak_field_map_
    ~0u,  // no _inlined_string_donated_
    ~0u,  // no _split_
    ~0u,  // no sizeof(Split)
    PROTOBUF_FIELD_OFFSET(::vision::raw::v1::Point3, _impl_.x_),
    PROTOBUF_FIELD_OFFSET(::vision::raw::v1::Point3, _impl_.y_),
    PROTOBUF_FIELD_OFFSET(::vision::raw::v1::Point3, _impl_.z_),
    ~0u,  // no _has_bits_
    PROTOBUF_FIELD_OFFSET(::vision::raw::v1::Frame, _internal_metadata_),
    ~0u,  // no _extensions_
    ~0u,  // no _oneof_case_
    ~0u,  // no _weak_field_map_
    ~0u,  // no _inlined_string_donated_
    ~0u,  // no _split_
    ~0u,  // no sizeof(Split)
    PROTOBUF_FIELD_OFFSET(::vision::raw::v1::Frame, _impl_.session_id_),
    PROTOBUF_FIELD_OFFSET(::vision::raw::v1::Frame, _impl_.index_),
    PROTOBUF_FIELD_OFFSET(::vision::raw::v1::Frame, _impl_.flag_),
    PROTOBUF_FIELD_OFFSET(::vision::raw::v1::Frame, _impl_.width_),
    PROTOBUF_FIELD_OFFSET(::vision::raw::v1::Frame, _impl_.height_),
    PROTOBUF_FIELD_OFFSET(::vision::raw::v1::Frame, _impl_.type_),
    PROTOBUF_FIELD_OFFSET(::vision::raw::v1::Frame, _impl_.data_),
    PROTOBUF_FIELD_OFFSET(::vision::raw::v1::Frame, _impl_.pose_points_),
    ~0u,  // no _has_bits_
    PROTOBUF_FIELD_OFFSET(::vision::raw::v1::SubmitResultResponse, _internal_metadata_),
    ~0u,  // no _extensions_
    ~0u,  // no _oneof_case_
    ~0u,  // no _weak_field_map_
    ~0u,  // no _inlined_string_donated_
    ~0u,  // no _split_
    ~0u,  // no sizeof(Split)
    PROTOBUF_FIELD_OFFSET(::vision::raw::v1::SubmitResultResponse, _impl_.session_id_),
    PROTOBUF_FIELD_OFFSET(::vision::raw::v1::SubmitResultResponse, _impl_.frame_count_),
    PROTOBUF_FIELD_OFFSET(::vision::raw::v1::SubmitResultResponse, _impl_.message_),
};

static const ::_pbi::MigrationSchema
    schemas[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) = {
        {0, -1, -1, sizeof(::vision::raw::v1::Point3)},
        {11, -1, -1, sizeof(::vision::raw::v1::Frame)},
        {27, -1, -1, sizeof(::vision::raw::v1::SubmitResultResponse)},
};

static const ::_pb::Message* const file_default_instances[] = {
    &::vision::raw::v1::_Point3_default_instance_._instance,
    &::vision::raw::v1::_Frame_default_instance_._instance,
    &::vision::raw::v1::_SubmitResultResponse_default_instance_._instance,
};
const char descriptor_table_protodef_ksl_5fsentence_5frecognition_2eproto[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) = {
    "\n\036ksl_sentence_recognition.proto\022\rvision"
    ".raw.v1\")\n\006Point3\022\t\n\001x\030\001 \001(\002\022\t\n\001y\030\002 \001(\002\022"
    "\t\n\001z\030\003 \001(\002\"\237\001\n\005Frame\022\022\n\nsession_id\030\001 \001(\t"
    "\022\r\n\005index\030\002 \001(\005\022\014\n\004flag\030\003 \001(\005\022\r\n\005width\030\004"
    " \001(\005\022\016\n\006height\030\005 \001(\005\022\014\n\004type\030\006 \001(\005\022\014\n\004da"
    "ta\030\007 \001(\014\022*\n\013pose_points\030\010 \003(\0132\025.vision.r"
    "aw.v1.Point3\"P\n\024SubmitResultResponse\022\022\n\n"
    "session_id\030\001 \001(\t\022\023\n\013frame_count\030\002 \001(\005\022\017\n"
    "\007message\030\003 \001(\t2\\\n\017SequenceService\022I\n\nSen"
    "dFrames\022\024.vision.raw.v1.Frame\032#.vision.r"
    "aw.v1.SubmitResultResponse(\001b\006proto3"
};
static ::absl::once_flag descriptor_table_ksl_5fsentence_5frecognition_2eproto_once;
const ::_pbi::DescriptorTable descriptor_table_ksl_5fsentence_5frecognition_2eproto = {
    false,
    false,
    436,
    descriptor_table_protodef_ksl_5fsentence_5frecognition_2eproto,
    "ksl_sentence_recognition.proto",
    &descriptor_table_ksl_5fsentence_5frecognition_2eproto_once,
    nullptr,
    0,
    3,
    schemas,
    file_default_instances,
    TableStruct_ksl_5fsentence_5frecognition_2eproto::offsets,
    file_level_metadata_ksl_5fsentence_5frecognition_2eproto,
    file_level_enum_descriptors_ksl_5fsentence_5frecognition_2eproto,
    file_level_service_descriptors_ksl_5fsentence_5frecognition_2eproto,
};

// This function exists to be marked as weak.
// It can significantly speed up compilation by breaking up LLVM's SCC
// in the .pb.cc translation units. Large translation units see a
// reduction of more than 35% of walltime for optimized builds. Without
// the weak attribute all the messages in the file, including all the
// vtables and everything they use become part of the same SCC through
// a cycle like:
// GetMetadata -> descriptor table -> default instances ->
//   vtables -> GetMetadata
// By adding a weak function here we break the connection from the
// individual vtables back into the descriptor table.
PROTOBUF_ATTRIBUTE_WEAK const ::_pbi::DescriptorTable* descriptor_table_ksl_5fsentence_5frecognition_2eproto_getter() {
  return &descriptor_table_ksl_5fsentence_5frecognition_2eproto;
}
// Force running AddDescriptors() at dynamic initialization time.
PROTOBUF_ATTRIBUTE_INIT_PRIORITY2
static ::_pbi::AddDescriptorsRunner dynamic_init_dummy_ksl_5fsentence_5frecognition_2eproto(&descriptor_table_ksl_5fsentence_5frecognition_2eproto);
namespace vision {
namespace raw {
namespace v1 {
// ===================================================================

class Point3::_Internal {
 public:
};

Point3::Point3(::google::protobuf::Arena* arena)
    : ::google::protobuf::Message(arena) {
  SharedCtor(arena);
  // @@protoc_insertion_point(arena_constructor:vision.raw.v1.Point3)
}
Point3::Point3(
    ::google::protobuf::Arena* arena, const Point3& from)
    : Point3(arena) {
  MergeFrom(from);
}
inline PROTOBUF_NDEBUG_INLINE Point3::Impl_::Impl_(
    ::google::protobuf::internal::InternalVisibility visibility,
    ::google::protobuf::Arena* arena)
      : _cached_size_{0} {}

inline void Point3::SharedCtor(::_pb::Arena* arena) {
  new (&_impl_) Impl_(internal_visibility(), arena);
  ::memset(reinterpret_cast<char *>(&_impl_) +
               offsetof(Impl_, x_),
           0,
           offsetof(Impl_, z_) -
               offsetof(Impl_, x_) +
               sizeof(Impl_::z_));
}
Point3::~Point3() {
  // @@protoc_insertion_point(destructor:vision.raw.v1.Point3)
  _internal_metadata_.Delete<::google::protobuf::UnknownFieldSet>();
  SharedDtor();
}
inline void Point3::SharedDtor() {
  ABSL_DCHECK(GetArena() == nullptr);
  _impl_.~Impl_();
}

PROTOBUF_NOINLINE void Point3::Clear() {
// @@protoc_insertion_point(message_clear_start:vision.raw.v1.Point3)
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  ::uint32_t cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  ::memset(&_impl_.x_, 0, static_cast<::size_t>(
      reinterpret_cast<char*>(&_impl_.z_) -
      reinterpret_cast<char*>(&_impl_.x_)) + sizeof(_impl_.z_));
  _internal_metadata_.Clear<::google::protobuf::UnknownFieldSet>();
}

const char* Point3::_InternalParse(
    const char* ptr, ::_pbi::ParseContext* ctx) {
  ptr = ::_pbi::TcParser::ParseLoop(this, ptr, ctx, &_table_.header);
  return ptr;
}


PROTOBUF_CONSTINIT PROTOBUF_ATTRIBUTE_INIT_PRIORITY1
const ::_pbi::TcParseTable<2, 3, 0, 0, 2> Point3::_table_ = {
  {
    0,  // no _has_bits_
    0, // no _extensions_
    3, 24,  // max_field_number, fast_idx_mask
    offsetof(decltype(_table_), field_lookup_table),
    4294967288,  // skipmap
    offsetof(decltype(_table_), field_entries),
    3,  // num_field_entries
    0,  // num_aux_entries
    offsetof(decltype(_table_), field_names),  // no aux_entries
    &_Point3_default_instance_._instance,
    ::_pbi::TcParser::GenericFallback,  // fallback
  }, {{
    {::_pbi::TcParser::MiniParse, {}},
    // float x = 1;
    {::_pbi::TcParser::FastF32S1,
     {13, 63, 0, PROTOBUF_FIELD_OFFSET(Point3, _impl_.x_)}},
    // float y = 2;
    {::_pbi::TcParser::FastF32S1,
     {21, 63, 0, PROTOBUF_FIELD_OFFSET(Point3, _impl_.y_)}},
    // float z = 3;
    {::_pbi::TcParser::FastF32S1,
     {29, 63, 0, PROTOBUF_FIELD_OFFSET(Point3, _impl_.z_)}},
  }}, {{
    65535, 65535
  }}, {{
    // float x = 1;
    {PROTOBUF_FIELD_OFFSET(Point3, _impl_.x_), 0, 0,
    (0 | ::_fl::kFcSingular | ::_fl::kFloat)},
    // float y = 2;
    {PROTOBUF_FIELD_OFFSET(Point3, _impl_.y_), 0, 0,
    (0 | ::_fl::kFcSingular | ::_fl::kFloat)},
    // float z = 3;
    {PROTOBUF_FIELD_OFFSET(Point3, _impl_.z_), 0, 0,
    (0 | ::_fl::kFcSingular | ::_fl::kFloat)},
  }},
  // no aux_entries
  {{
  }},
};

::uint8_t* Point3::_InternalSerialize(
    ::uint8_t* target,
    ::google::protobuf::io::EpsCopyOutputStream* stream) const {
  // @@protoc_insertion_point(serialize_to_array_start:vision.raw.v1.Point3)
  ::uint32_t cached_has_bits = 0;
  (void)cached_has_bits;

  // float x = 1;
  static_assert(sizeof(::uint32_t) == sizeof(float),
                "Code assumes ::uint32_t and float are the same size.");
  float tmp_x = this->_internal_x();
  ::uint32_t raw_x;
  memcpy(&raw_x, &tmp_x, sizeof(tmp_x));
  if (raw_x != 0) {
    target = stream->EnsureSpace(target);
    target = ::_pbi::WireFormatLite::WriteFloatToArray(
        1, this->_internal_x(), target);
  }

  // float y = 2;
  static_assert(sizeof(::uint32_t) == sizeof(float),
                "Code assumes ::uint32_t and float are the same size.");
  float tmp_y = this->_internal_y();
  ::uint32_t raw_y;
  memcpy(&raw_y, &tmp_y, sizeof(tmp_y));
  if (raw_y != 0) {
    target = stream->EnsureSpace(target);
    target = ::_pbi::WireFormatLite::WriteFloatToArray(
        2, this->_internal_y(), target);
  }

  // float z = 3;
  static_assert(sizeof(::uint32_t) == sizeof(float),
                "Code assumes ::uint32_t and float are the same size.");
  float tmp_z = this->_internal_z();
  ::uint32_t raw_z;
  memcpy(&raw_z, &tmp_z, sizeof(tmp_z));
  if (raw_z != 0) {
    target = stream->EnsureSpace(target);
    target = ::_pbi::WireFormatLite::WriteFloatToArray(
        3, this->_internal_z(), target);
  }

  if (PROTOBUF_PREDICT_FALSE(_internal_metadata_.have_unknown_fields())) {
    target =
        ::_pbi::WireFormat::InternalSerializeUnknownFieldsToArray(
            _internal_metadata_.unknown_fields<::google::protobuf::UnknownFieldSet>(::google::protobuf::UnknownFieldSet::default_instance), target, stream);
  }
  // @@protoc_insertion_point(serialize_to_array_end:vision.raw.v1.Point3)
  return target;
}

::size_t Point3::ByteSizeLong() const {
// @@protoc_insertion_point(message_byte_size_start:vision.raw.v1.Point3)
  ::size_t total_size = 0;

  ::uint32_t cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  // float x = 1;
  static_assert(sizeof(::uint32_t) == sizeof(float),
                "Code assumes ::uint32_t and float are the same size.");
  float tmp_x = this->_internal_x();
  ::uint32_t raw_x;
  memcpy(&raw_x, &tmp_x, sizeof(tmp_x));
  if (raw_x != 0) {
    total_size += 5;
  }

  // float y = 2;
  static_assert(sizeof(::uint32_t) == sizeof(float),
                "Code assumes ::uint32_t and float are the same size.");
  float tmp_y = this->_internal_y();
  ::uint32_t raw_y;
  memcpy(&raw_y, &tmp_y, sizeof(tmp_y));
  if (raw_y != 0) {
    total_size += 5;
  }

  // float z = 3;
  static_assert(sizeof(::uint32_t) == sizeof(float),
                "Code assumes ::uint32_t and float are the same size.");
  float tmp_z = this->_internal_z();
  ::uint32_t raw_z;
  memcpy(&raw_z, &tmp_z, sizeof(tmp_z));
  if (raw_z != 0) {
    total_size += 5;
  }

  return MaybeComputeUnknownFieldsSize(total_size, &_impl_._cached_size_);
}

const ::google::protobuf::Message::ClassData Point3::_class_data_ = {
    Point3::MergeImpl,
    nullptr,  // OnDemandRegisterArenaDtor
};
const ::google::protobuf::Message::ClassData* Point3::GetClassData() const {
  return &_class_data_;
}

void Point3::MergeImpl(::google::protobuf::Message& to_msg, const ::google::protobuf::Message& from_msg) {
  auto* const _this = static_cast<Point3*>(&to_msg);
  auto& from = static_cast<const Point3&>(from_msg);
  // @@protoc_insertion_point(class_specific_merge_from_start:vision.raw.v1.Point3)
  ABSL_DCHECK_NE(&from, _this);
  ::uint32_t cached_has_bits = 0;
  (void) cached_has_bits;

  static_assert(sizeof(::uint32_t) == sizeof(float),
                "Code assumes ::uint32_t and float are the same size.");
  float tmp_x = from._internal_x();
  ::uint32_t raw_x;
  memcpy(&raw_x, &tmp_x, sizeof(tmp_x));
  if (raw_x != 0) {
    _this->_internal_set_x(from._internal_x());
  }
  static_assert(sizeof(::uint32_t) == sizeof(float),
                "Code assumes ::uint32_t and float are the same size.");
  float tmp_y = from._internal_y();
  ::uint32_t raw_y;
  memcpy(&raw_y, &tmp_y, sizeof(tmp_y));
  if (raw_y != 0) {
    _this->_internal_set_y(from._internal_y());
  }
  static_assert(sizeof(::uint32_t) == sizeof(float),
                "Code assumes ::uint32_t and float are the same size.");
  float tmp_z = from._internal_z();
  ::uint32_t raw_z;
  memcpy(&raw_z, &tmp_z, sizeof(tmp_z));
  if (raw_z != 0) {
    _this->_internal_set_z(from._internal_z());
  }
  _this->_internal_metadata_.MergeFrom<::google::protobuf::UnknownFieldSet>(from._internal_metadata_);
}

void Point3::CopyFrom(const Point3& from) {
// @@protoc_insertion_point(class_specific_copy_from_start:vision.raw.v1.Point3)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

PROTOBUF_NOINLINE bool Point3::IsInitialized() const {
  return true;
}

::_pbi::CachedSize* Point3::AccessCachedSize() const {
  return &_impl_._cached_size_;
}
void Point3::InternalSwap(Point3* PROTOBUF_RESTRICT other) {
  using std::swap;
  _internal_metadata_.InternalSwap(&other->_internal_metadata_);
  ::google::protobuf::internal::memswap<
      PROTOBUF_FIELD_OFFSET(Point3, _impl_.z_)
      + sizeof(Point3::_impl_.z_)
      - PROTOBUF_FIELD_OFFSET(Point3, _impl_.x_)>(
          reinterpret_cast<char*>(&_impl_.x_),
          reinterpret_cast<char*>(&other->_impl_.x_));
}

::google::protobuf::Metadata Point3::GetMetadata() const {
  return ::_pbi::AssignDescriptors(
      &descriptor_table_ksl_5fsentence_5frecognition_2eproto_getter, &descriptor_table_ksl_5fsentence_5frecognition_2eproto_once,
      file_level_metadata_ksl_5fsentence_5frecognition_2eproto[0]);
}
// ===================================================================

class Frame::_Internal {
 public:
};

Frame::Frame(::google::protobuf::Arena* arena)
    : ::google::protobuf::Message(arena) {
  SharedCtor(arena);
  // @@protoc_insertion_point(arena_constructor:vision.raw.v1.Frame)
}
inline PROTOBUF_NDEBUG_INLINE Frame::Impl_::Impl_(
    ::google::protobuf::internal::InternalVisibility visibility, ::google::protobuf::Arena* arena,
    const Impl_& from)
      : pose_points_{visibility, arena, from.pose_points_},
        session_id_(arena, from.session_id_),
        data_(arena, from.data_),
        _cached_size_{0} {}

Frame::Frame(
    ::google::protobuf::Arena* arena,
    const Frame& from)
    : ::google::protobuf::Message(arena) {
  Frame* const _this = this;
  (void)_this;
  _internal_metadata_.MergeFrom<::google::protobuf::UnknownFieldSet>(
      from._internal_metadata_);
  new (&_impl_) Impl_(internal_visibility(), arena, from._impl_);
  ::memcpy(reinterpret_cast<char *>(&_impl_) +
               offsetof(Impl_, index_),
           reinterpret_cast<const char *>(&from._impl_) +
               offsetof(Impl_, index_),
           offsetof(Impl_, type_) -
               offsetof(Impl_, index_) +
               sizeof(Impl_::type_));

  // @@protoc_insertion_point(copy_constructor:vision.raw.v1.Frame)
}
inline PROTOBUF_NDEBUG_INLINE Frame::Impl_::Impl_(
    ::google::protobuf::internal::InternalVisibility visibility,
    ::google::protobuf::Arena* arena)
      : pose_points_{visibility, arena},
        session_id_(arena),
        data_(arena),
        _cached_size_{0} {}

inline void Frame::SharedCtor(::_pb::Arena* arena) {
  new (&_impl_) Impl_(internal_visibility(), arena);
  ::memset(reinterpret_cast<char *>(&_impl_) +
               offsetof(Impl_, index_),
           0,
           offsetof(Impl_, type_) -
               offsetof(Impl_, index_) +
               sizeof(Impl_::type_));
}
Frame::~Frame() {
  // @@protoc_insertion_point(destructor:vision.raw.v1.Frame)
  _internal_metadata_.Delete<::google::protobuf::UnknownFieldSet>();
  SharedDtor();
}
inline void Frame::SharedDtor() {
  ABSL_DCHECK(GetArena() == nullptr);
  _impl_.session_id_.Destroy();
  _impl_.data_.Destroy();
  _impl_.~Impl_();
}

PROTOBUF_NOINLINE void Frame::Clear() {
// @@protoc_insertion_point(message_clear_start:vision.raw.v1.Frame)
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  ::uint32_t cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  _impl_.pose_points_.Clear();
  _impl_.session_id_.ClearToEmpty();
  _impl_.data_.ClearToEmpty();
  ::memset(&_impl_.index_, 0, static_cast<::size_t>(
      reinterpret_cast<char*>(&_impl_.type_) -
      reinterpret_cast<char*>(&_impl_.index_)) + sizeof(_impl_.type_));
  _internal_metadata_.Clear<::google::protobuf::UnknownFieldSet>();
}

const char* Frame::_InternalParse(
    const char* ptr, ::_pbi::ParseContext* ctx) {
  ptr = ::_pbi::TcParser::ParseLoop(this, ptr, ctx, &_table_.header);
  return ptr;
}


PROTOBUF_CONSTINIT PROTOBUF_ATTRIBUTE_INIT_PRIORITY1
const ::_pbi::TcParseTable<3, 8, 1, 46, 2> Frame::_table_ = {
  {
    0,  // no _has_bits_
    0, // no _extensions_
    8, 56,  // max_field_number, fast_idx_mask
    offsetof(decltype(_table_), field_lookup_table),
    4294967040,  // skipmap
    offsetof(decltype(_table_), field_entries),
    8,  // num_field_entries
    1,  // num_aux_entries
    offsetof(decltype(_table_), aux_entries),
    &_Frame_default_instance_._instance,
    ::_pbi::TcParser::GenericFallback,  // fallback
  }, {{
    // repeated .vision.raw.v1.Point3 pose_points = 8;
    {::_pbi::TcParser::FastMtR1,
     {66, 63, 0, PROTOBUF_FIELD_OFFSET(Frame, _impl_.pose_points_)}},
    // string session_id = 1;
    {::_pbi::TcParser::FastUS1,
     {10, 63, 0, PROTOBUF_FIELD_OFFSET(Frame, _impl_.session_id_)}},
    // int32 index = 2;
    {::_pbi::TcParser::SingularVarintNoZag1<::uint32_t, offsetof(Frame, _impl_.index_), 63>(),
     {16, 63, 0, PROTOBUF_FIELD_OFFSET(Frame, _impl_.index_)}},
    // int32 flag = 3;
    {::_pbi::TcParser::SingularVarintNoZag1<::uint32_t, offsetof(Frame, _impl_.flag_), 63>(),
     {24, 63, 0, PROTOBUF_FIELD_OFFSET(Frame, _impl_.flag_)}},
    // int32 width = 4;
    {::_pbi::TcParser::SingularVarintNoZag1<::uint32_t, offsetof(Frame, _impl_.width_), 63>(),
     {32, 63, 0, PROTOBUF_FIELD_OFFSET(Frame, _impl_.width_)}},
    // int32 height = 5;
    {::_pbi::TcParser::SingularVarintNoZag1<::uint32_t, offsetof(Frame, _impl_.height_), 63>(),
     {40, 63, 0, PROTOBUF_FIELD_OFFSET(Frame, _impl_.height_)}},
    // int32 type = 6;
    {::_pbi::TcParser::SingularVarintNoZag1<::uint32_t, offsetof(Frame, _impl_.type_), 63>(),
     {48, 63, 0, PROTOBUF_FIELD_OFFSET(Frame, _impl_.type_)}},
    // bytes data = 7;
    {::_pbi::TcParser::FastBS1,
     {58, 63, 0, PROTOBUF_FIELD_OFFSET(Frame, _impl_.data_)}},
  }}, {{
    65535, 65535
  }}, {{
    // string session_id = 1;
    {PROTOBUF_FIELD_OFFSET(Frame, _impl_.session_id_), 0, 0,
    (0 | ::_fl::kFcSingular | ::_fl::kUtf8String | ::_fl::kRepAString)},
    // int32 index = 2;
    {PROTOBUF_FIELD_OFFSET(Frame, _impl_.index_), 0, 0,
    (0 | ::_fl::kFcSingular | ::_fl::kInt32)},
    // int32 flag = 3;
    {PROTOBUF_FIELD_OFFSET(Frame, _impl_.flag_), 0, 0,
    (0 | ::_fl::kFcSingular | ::_fl::kInt32)},
    // int32 width = 4;
    {PROTOBUF_FIELD_OFFSET(Frame, _impl_.width_), 0, 0,
    (0 | ::_fl::kFcSingular | ::_fl::kInt32)},
    // int32 height = 5;
    {PROTOBUF_FIELD_OFFSET(Frame, _impl_.height_), 0, 0,
    (0 | ::_fl::kFcSingular | ::_fl::kInt32)},
    // int32 type = 6;
    {PROTOBUF_FIELD_OFFSET(Frame, _impl_.type_), 0, 0,
    (0 | ::_fl::kFcSingular | ::_fl::kInt32)},
    // bytes data = 7;
    {PROTOBUF_FIELD_OFFSET(Frame, _impl_.data_), 0, 0,
    (0 | ::_fl::kFcSingular | ::_fl::kBytes | ::_fl::kRepAString)},
    // repeated .vision.raw.v1.Point3 pose_points = 8;
    {PROTOBUF_FIELD_OFFSET(Frame, _impl_.pose_points_), 0, 0,
    (0 | ::_fl::kFcRepeated | ::_fl::kMessage | ::_fl::kTvTable)},
  }}, {{
    {::_pbi::TcParser::GetTable<::vision::raw::v1::Point3>()},
  }}, {{
    "\23\12\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
    "vision.raw.v1.Frame"
    "session_id"
  }},
};

::uint8_t* Frame::_InternalSerialize(
    ::uint8_t* target,
    ::google::protobuf::io::EpsCopyOutputStream* stream) const {
  // @@protoc_insertion_point(serialize_to_array_start:vision.raw.v1.Frame)
  ::uint32_t cached_has_bits = 0;
  (void)cached_has_bits;

  // string session_id = 1;
  if (!this->_internal_session_id().empty()) {
    const std::string& _s = this->_internal_session_id();
    ::google::protobuf::internal::WireFormatLite::VerifyUtf8String(
        _s.data(), static_cast<int>(_s.length()), ::google::protobuf::internal::WireFormatLite::SERIALIZE, "vision.raw.v1.Frame.session_id");
    target = stream->WriteStringMaybeAliased(1, _s, target);
  }

  // int32 index = 2;
  if (this->_internal_index() != 0) {
    target = ::google::protobuf::internal::WireFormatLite::
        WriteInt32ToArrayWithField<2>(
            stream, this->_internal_index(), target);
  }

  // int32 flag = 3;
  if (this->_internal_flag() != 0) {
    target = ::google::protobuf::internal::WireFormatLite::
        WriteInt32ToArrayWithField<3>(
            stream, this->_internal_flag(), target);
  }

  // int32 width = 4;
  if (this->_internal_width() != 0) {
    target = ::google::protobuf::internal::WireFormatLite::
        WriteInt32ToArrayWithField<4>(
            stream, this->_internal_width(), target);
  }

  // int32 height = 5;
  if (this->_internal_height() != 0) {
    target = ::google::protobuf::internal::WireFormatLite::
        WriteInt32ToArrayWithField<5>(
            stream, this->_internal_height(), target);
  }

  // int32 type = 6;
  if (this->_internal_type() != 0) {
    target = ::google::protobuf::internal::WireFormatLite::
        WriteInt32ToArrayWithField<6>(
            stream, this->_internal_type(), target);
  }

  // bytes data = 7;
  if (!this->_internal_data().empty()) {
    const std::string& _s = this->_internal_data();
    target = stream->WriteBytesMaybeAliased(7, _s, target);
  }

  // repeated .vision.raw.v1.Point3 pose_points = 8;
  for (unsigned i = 0,
      n = static_cast<unsigned>(this->_internal_pose_points_size()); i < n; i++) {
    const auto& repfield = this->_internal_pose_points().Get(i);
    target = ::google::protobuf::internal::WireFormatLite::
        InternalWriteMessage(8, repfield, repfield.GetCachedSize(), target, stream);
  }

  if (PROTOBUF_PREDICT_FALSE(_internal_metadata_.have_unknown_fields())) {
    target =
        ::_pbi::WireFormat::InternalSerializeUnknownFieldsToArray(
            _internal_metadata_.unknown_fields<::google::protobuf::UnknownFieldSet>(::google::protobuf::UnknownFieldSet::default_instance), target, stream);
  }
  // @@protoc_insertion_point(serialize_to_array_end:vision.raw.v1.Frame)
  return target;
}

::size_t Frame::ByteSizeLong() const {
// @@protoc_insertion_point(message_byte_size_start:vision.raw.v1.Frame)
  ::size_t total_size = 0;

  ::uint32_t cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  // repeated .vision.raw.v1.Point3 pose_points = 8;
  total_size += 1UL * this->_internal_pose_points_size();
  for (const auto& msg : this->_internal_pose_points()) {
    total_size +=
      ::google::protobuf::internal::WireFormatLite::MessageSize(msg);
  }
  // string session_id = 1;
  if (!this->_internal_session_id().empty()) {
    total_size += 1 + ::google::protobuf::internal::WireFormatLite::StringSize(
                                    this->_internal_session_id());
  }

  // bytes data = 7;
  if (!this->_internal_data().empty()) {
    total_size += 1 + ::google::protobuf::internal::WireFormatLite::BytesSize(
                                    this->_internal_data());
  }

  // int32 index = 2;
  if (this->_internal_index() != 0) {
    total_size += ::_pbi::WireFormatLite::Int32SizePlusOne(
        this->_internal_index());
  }

  // int32 flag = 3;
  if (this->_internal_flag() != 0) {
    total_size += ::_pbi::WireFormatLite::Int32SizePlusOne(
        this->_internal_flag());
  }

  // int32 width = 4;
  if (this->_internal_width() != 0) {
    total_size += ::_pbi::WireFormatLite::Int32SizePlusOne(
        this->_internal_width());
  }

  // int32 height = 5;
  if (this->_internal_height() != 0) {
    total_size += ::_pbi::WireFormatLite::Int32SizePlusOne(
        this->_internal_height());
  }

  // int32 type = 6;
  if (this->_internal_type() != 0) {
    total_size += ::_pbi::WireFormatLite::Int32SizePlusOne(
        this->_internal_type());
  }

  return MaybeComputeUnknownFieldsSize(total_size, &_impl_._cached_size_);
}

const ::google::protobuf::Message::ClassData Frame::_class_data_ = {
    Frame::MergeImpl,
    nullptr,  // OnDemandRegisterArenaDtor
};
const ::google::protobuf::Message::ClassData* Frame::GetClassData() const {
  return &_class_data_;
}

void Frame::MergeImpl(::google::protobuf::Message& to_msg, const ::google::protobuf::Message& from_msg) {
  auto* const _this = static_cast<Frame*>(&to_msg);
  auto& from = static_cast<const Frame&>(from_msg);
  // @@protoc_insertion_point(class_specific_merge_from_start:vision.raw.v1.Frame)
  ABSL_DCHECK_NE(&from, _this);
  ::uint32_t cached_has_bits = 0;
  (void) cached_has_bits;

  _this->_internal_mutable_pose_points()->MergeFrom(
      from._internal_pose_points());
  if (!from._internal_session_id().empty()) {
    _this->_internal_set_session_id(from._internal_session_id());
  }
  if (!from._internal_data().empty()) {
    _this->_internal_set_data(from._internal_data());
  }
  if (from._internal_index() != 0) {
    _this->_internal_set_index(from._internal_index());
  }
  if (from._internal_flag() != 0) {
    _this->_internal_set_flag(from._internal_flag());
  }
  if (from._internal_width() != 0) {
    _this->_internal_set_width(from._internal_width());
  }
  if (from._internal_height() != 0) {
    _this->_internal_set_height(from._internal_height());
  }
  if (from._internal_type() != 0) {
    _this->_internal_set_type(from._internal_type());
  }
  _this->_internal_metadata_.MergeFrom<::google::protobuf::UnknownFieldSet>(from._internal_metadata_);
}

void Frame::CopyFrom(const Frame& from) {
// @@protoc_insertion_point(class_specific_copy_from_start:vision.raw.v1.Frame)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

PROTOBUF_NOINLINE bool Frame::IsInitialized() const {
  return true;
}

::_pbi::CachedSize* Frame::AccessCachedSize() const {
  return &_impl_._cached_size_;
}
void Frame::InternalSwap(Frame* PROTOBUF_RESTRICT other) {
  using std::swap;
  auto* arena = GetArena();
  ABSL_DCHECK_EQ(arena, other->GetArena());
  _internal_metadata_.InternalSwap(&other->_internal_metadata_);
  _impl_.pose_points_.InternalSwap(&other->_impl_.pose_points_);
  ::_pbi::ArenaStringPtr::InternalSwap(&_impl_.session_id_, &other->_impl_.session_id_, arena);
  ::_pbi::ArenaStringPtr::InternalSwap(&_impl_.data_, &other->_impl_.data_, arena);
  ::google::protobuf::internal::memswap<
      PROTOBUF_FIELD_OFFSET(Frame, _impl_.type_)
      + sizeof(Frame::_impl_.type_)
      - PROTOBUF_FIELD_OFFSET(Frame, _impl_.index_)>(
          reinterpret_cast<char*>(&_impl_.index_),
          reinterpret_cast<char*>(&other->_impl_.index_));
}

::google::protobuf::Metadata Frame::GetMetadata() const {
  return ::_pbi::AssignDescriptors(
      &descriptor_table_ksl_5fsentence_5frecognition_2eproto_getter, &descriptor_table_ksl_5fsentence_5frecognition_2eproto_once,
      file_level_metadata_ksl_5fsentence_5frecognition_2eproto[1]);
}
// ===================================================================

class SubmitResultResponse::_Internal {
 public:
};

SubmitResultResponse::SubmitResultResponse(::google::protobuf::Arena* arena)
    : ::google::protobuf::Message(arena) {
  SharedCtor(arena);
  // @@protoc_insertion_point(arena_constructor:vision.raw.v1.SubmitResultResponse)
}
inline PROTOBUF_NDEBUG_INLINE SubmitResultResponse::Impl_::Impl_(
    ::google::protobuf::internal::InternalVisibility visibility, ::google::protobuf::Arena* arena,
    const Impl_& from)
      : session_id_(arena, from.session_id_),
        message_(arena, from.message_),
        _cached_size_{0} {}

SubmitResultResponse::SubmitResultResponse(
    ::google::protobuf::Arena* arena,
    const SubmitResultResponse& from)
    : ::google::protobuf::Message(arena) {
  SubmitResultResponse* const _this = this;
  (void)_this;
  _internal_metadata_.MergeFrom<::google::protobuf::UnknownFieldSet>(
      from._internal_metadata_);
  new (&_impl_) Impl_(internal_visibility(), arena, from._impl_);
  _impl_.frame_count_ = from._impl_.frame_count_;

  // @@protoc_insertion_point(copy_constructor:vision.raw.v1.SubmitResultResponse)
}
inline PROTOBUF_NDEBUG_INLINE SubmitResultResponse::Impl_::Impl_(
    ::google::protobuf::internal::InternalVisibility visibility,
    ::google::protobuf::Arena* arena)
      : session_id_(arena),
        message_(arena),
        _cached_size_{0} {}

inline void SubmitResultResponse::SharedCtor(::_pb::Arena* arena) {
  new (&_impl_) Impl_(internal_visibility(), arena);
  _impl_.frame_count_ = {};
}
SubmitResultResponse::~SubmitResultResponse() {
  // @@protoc_insertion_point(destructor:vision.raw.v1.SubmitResultResponse)
  _internal_metadata_.Delete<::google::protobuf::UnknownFieldSet>();
  SharedDtor();
}
inline void SubmitResultResponse::SharedDtor() {
  ABSL_DCHECK(GetArena() == nullptr);
  _impl_.session_id_.Destroy();
  _impl_.message_.Destroy();
  _impl_.~Impl_();
}

PROTOBUF_NOINLINE void SubmitResultResponse::Clear() {
// @@protoc_insertion_point(message_clear_start:vision.raw.v1.SubmitResultResponse)
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  ::uint32_t cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  _impl_.session_id_.ClearToEmpty();
  _impl_.message_.ClearToEmpty();
  _impl_.frame_count_ = 0;
  _internal_metadata_.Clear<::google::protobuf::UnknownFieldSet>();
}

const char* SubmitResultResponse::_InternalParse(
    const char* ptr, ::_pbi::ParseContext* ctx) {
  ptr = ::_pbi::TcParser::ParseLoop(this, ptr, ctx, &_table_.header);
  return ptr;
}


PROTOBUF_CONSTINIT PROTOBUF_ATTRIBUTE_INIT_PRIORITY1
const ::_pbi::TcParseTable<2, 3, 0, 60, 2> SubmitResultResponse::_table_ = {
  {
    0,  // no _has_bits_
    0, // no _extensions_
    3, 24,  // max_field_number, fast_idx_mask
    offsetof(decltype(_table_), field_lookup_table),
    4294967288,  // skipmap
    offsetof(decltype(_table_), field_entries),
    3,  // num_field_entries
    0,  // num_aux_entries
    offsetof(decltype(_table_), field_names),  // no aux_entries
    &_SubmitResultResponse_default_instance_._instance,
    ::_pbi::TcParser::GenericFallback,  // fallback
  }, {{
    {::_pbi::TcParser::MiniParse, {}},
    // string session_id = 1;
    {::_pbi::TcParser::FastUS1,
     {10, 63, 0, PROTOBUF_FIELD_OFFSET(SubmitResultResponse, _impl_.session_id_)}},
    // int32 frame_count = 2;
    {::_pbi::TcParser::SingularVarintNoZag1<::uint32_t, offsetof(SubmitResultResponse, _impl_.frame_count_), 63>(),
     {16, 63, 0, PROTOBUF_FIELD_OFFSET(SubmitResultResponse, _impl_.frame_count_)}},
    // string message = 3;
    {::_pbi::TcParser::FastUS1,
     {26, 63, 0, PROTOBUF_FIELD_OFFSET(SubmitResultResponse, _impl_.message_)}},
  }}, {{
    65535, 65535
  }}, {{
    // string session_id = 1;
    {PROTOBUF_FIELD_OFFSET(SubmitResultResponse, _impl_.session_id_), 0, 0,
    (0 | ::_fl::kFcSingular | ::_fl::kUtf8String | ::_fl::kRepAString)},
    // int32 frame_count = 2;
    {PROTOBUF_FIELD_OFFSET(SubmitResultResponse, _impl_.frame_count_), 0, 0,
    (0 | ::_fl::kFcSingular | ::_fl::kInt32)},
    // string message = 3;
    {PROTOBUF_FIELD_OFFSET(SubmitResultResponse, _impl_.message_), 0, 0,
    (0 | ::_fl::kFcSingular | ::_fl::kUtf8String | ::_fl::kRepAString)},
  }},
  // no aux_entries
  {{
    "\42\12\0\7\0\0\0\0"
    "vision.raw.v1.SubmitResultResponse"
    "session_id"
    "message"
  }},
};

::uint8_t* SubmitResultResponse::_InternalSerialize(
    ::uint8_t* target,
    ::google::protobuf::io::EpsCopyOutputStream* stream) const {
  // @@protoc_insertion_point(serialize_to_array_start:vision.raw.v1.SubmitResultResponse)
  ::uint32_t cached_has_bits = 0;
  (void)cached_has_bits;

  // string session_id = 1;
  if (!this->_internal_session_id().empty()) {
    const std::string& _s = this->_internal_session_id();
    ::google::protobuf::internal::WireFormatLite::VerifyUtf8String(
        _s.data(), static_cast<int>(_s.length()), ::google::protobuf::internal::WireFormatLite::SERIALIZE, "vision.raw.v1.SubmitResultResponse.session_id");
    target = stream->WriteStringMaybeAliased(1, _s, target);
  }

  // int32 frame_count = 2;
  if (this->_internal_frame_count() != 0) {
    target = ::google::protobuf::internal::WireFormatLite::
        WriteInt32ToArrayWithField<2>(
            stream, this->_internal_frame_count(), target);
  }

  // string message = 3;
  if (!this->_internal_message().empty()) {
    const std::string& _s = this->_internal_message();
    ::google::protobuf::internal::WireFormatLite::VerifyUtf8String(
        _s.data(), static_cast<int>(_s.length()), ::google::protobuf::internal::WireFormatLite::SERIALIZE, "vision.raw.v1.SubmitResultResponse.message");
    target = stream->WriteStringMaybeAliased(3, _s, target);
  }

  if (PROTOBUF_PREDICT_FALSE(_internal_metadata_.have_unknown_fields())) {
    target =
        ::_pbi::WireFormat::InternalSerializeUnknownFieldsToArray(
            _internal_metadata_.unknown_fields<::google::protobuf::UnknownFieldSet>(::google::protobuf::UnknownFieldSet::default_instance), target, stream);
  }
  // @@protoc_insertion_point(serialize_to_array_end:vision.raw.v1.SubmitResultResponse)
  return target;
}

::size_t SubmitResultResponse::ByteSizeLong() const {
// @@protoc_insertion_point(message_byte_size_start:vision.raw.v1.SubmitResultResponse)
  ::size_t total_size = 0;

  ::uint32_t cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  // string session_id = 1;
  if (!this->_internal_session_id().empty()) {
    total_size += 1 + ::google::protobuf::internal::WireFormatLite::StringSize(
                                    this->_internal_session_id());
  }

  // string message = 3;
  if (!this->_internal_message().empty()) {
    total_size += 1 + ::google::protobuf::internal::WireFormatLite::StringSize(
                                    this->_internal_message());
  }

  // int32 frame_count = 2;
  if (this->_internal_frame_count() != 0) {
    total_size += ::_pbi::WireFormatLite::Int32SizePlusOne(
        this->_internal_frame_count());
  }

  return MaybeComputeUnknownFieldsSize(total_size, &_impl_._cached_size_);
}

const ::google::protobuf::Message::ClassData SubmitResultResponse::_class_data_ = {
    SubmitResultResponse::MergeImpl,
    nullptr,  // OnDemandRegisterArenaDtor
};
const ::google::protobuf::Message::ClassData* SubmitResultResponse::GetClassData() const {
  return &_class_data_;
}

void SubmitResultResponse::MergeImpl(::google::protobuf::Message& to_msg, const ::google::protobuf::Message& from_msg) {
  auto* const _this = static_cast<SubmitResultResponse*>(&to_msg);
  auto& from = static_cast<const SubmitResultResponse&>(from_msg);
  // @@protoc_insertion_point(class_specific_merge_from_start:vision.raw.v1.SubmitResultResponse)
  ABSL_DCHECK_NE(&from, _this);
  ::uint32_t cached_has_bits = 0;
  (void) cached_has_bits;

  if (!from._internal_session_id().empty()) {
    _this->_internal_set_session_id(from._internal_session_id());
  }
  if (!from._internal_message().empty()) {
    _this->_internal_set_message(from._internal_message());
  }
  if (from._internal_frame_count() != 0) {
    _this->_internal_set_frame_count(from._internal_frame_count());
  }
  _this->_internal_metadata_.MergeFrom<::google::protobuf::UnknownFieldSet>(from._internal_metadata_);
}

void SubmitResultResponse::CopyFrom(const SubmitResultResponse& from) {
// @@protoc_insertion_point(class_specific_copy_from_start:vision.raw.v1.SubmitResultResponse)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

PROTOBUF_NOINLINE bool SubmitResultResponse::IsInitialized() const {
  return true;
}

::_pbi::CachedSize* SubmitResultResponse::AccessCachedSize() const {
  return &_impl_._cached_size_;
}
void SubmitResultResponse::InternalSwap(SubmitResultResponse* PROTOBUF_RESTRICT other) {
  using std::swap;
  auto* arena = GetArena();
  ABSL_DCHECK_EQ(arena, other->GetArena());
  _internal_metadata_.InternalSwap(&other->_internal_metadata_);
  ::_pbi::ArenaStringPtr::InternalSwap(&_impl_.session_id_, &other->_impl_.session_id_, arena);
  ::_pbi::ArenaStringPtr::InternalSwap(&_impl_.message_, &other->_impl_.message_, arena);
        swap(_impl_.frame_count_, other->_impl_.frame_count_);
}

::google::protobuf::Metadata SubmitResultResponse::GetMetadata() const {
  return ::_pbi::AssignDescriptors(
      &descriptor_table_ksl_5fsentence_5frecognition_2eproto_getter, &descriptor_table_ksl_5fsentence_5frecognition_2eproto_once,
      file_level_metadata_ksl_5fsentence_5frecognition_2eproto[2]);
}
// @@protoc_insertion_point(namespace_scope)
}  // namespace v1
}  // namespace raw
}  // namespace vision
namespace google {
namespace protobuf {
}  // namespace protobuf
}  // namespace google
// @@protoc_insertion_point(global_scope)
#include "google/protobuf/port_undef.inc"
```

## File: ksl_sentence_recognition.pb.h
```
// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: ksl_sentence_recognition.proto
// Protobuf C++ Version: 4.25.1

#ifndef GOOGLE_PROTOBUF_INCLUDED_ksl_5fsentence_5frecognition_2eproto_2epb_2eh
#define GOOGLE_PROTOBUF_INCLUDED_ksl_5fsentence_5frecognition_2eproto_2epb_2eh

#include <limits>
#include <string>
#include <type_traits>
#include <utility>

#include "google/protobuf/port_def.inc"
#if PROTOBUF_VERSION < 4025000
#error "This file was generated by a newer version of protoc which is"
#error "incompatible with your Protocol Buffer headers. Please update"
#error "your headers."
#endif  // PROTOBUF_VERSION

#if 4025001 < PROTOBUF_MIN_PROTOC_VERSION
#error "This file was generated by an older version of protoc which is"
#error "incompatible with your Protocol Buffer headers. Please"
#error "regenerate this file with a newer version of protoc."
#endif  // PROTOBUF_MIN_PROTOC_VERSION
#include "google/protobuf/port_undef.inc"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/arenastring.h"
#include "google/protobuf/generated_message_tctable_decl.h"
#include "google/protobuf/generated_message_util.h"
#include "google/protobuf/metadata_lite.h"
#include "google/protobuf/generated_message_reflection.h"
#include "google/protobuf/message.h"
#include "google/protobuf/repeated_field.h"  // IWYU pragma: export
#include "google/protobuf/extension_set.h"  // IWYU pragma: export
#include "google/protobuf/unknown_field_set.h"
// @@protoc_insertion_point(includes)

// Must be included last.
#include "google/protobuf/port_def.inc"

#define PROTOBUF_INTERNAL_EXPORT_ksl_5fsentence_5frecognition_2eproto

namespace google {
namespace protobuf {
namespace internal {
class AnyMetadata;
}  // namespace internal
}  // namespace protobuf
}  // namespace google

// Internal implementation detail -- do not use these members.
struct TableStruct_ksl_5fsentence_5frecognition_2eproto {
  static const ::uint32_t offsets[];
};
extern const ::google::protobuf::internal::DescriptorTable
    descriptor_table_ksl_5fsentence_5frecognition_2eproto;
namespace vision {
namespace raw {
namespace v1 {
class Frame;
struct FrameDefaultTypeInternal;
extern FrameDefaultTypeInternal _Frame_default_instance_;
class Point3;
struct Point3DefaultTypeInternal;
extern Point3DefaultTypeInternal _Point3_default_instance_;
class SubmitResultResponse;
struct SubmitResultResponseDefaultTypeInternal;
extern SubmitResultResponseDefaultTypeInternal _SubmitResultResponse_default_instance_;
}  // namespace v1
}  // namespace raw
}  // namespace vision
namespace google {
namespace protobuf {
}  // namespace protobuf
}  // namespace google

namespace vision {
namespace raw {
namespace v1 {

// ===================================================================


// -------------------------------------------------------------------

class SubmitResultResponse final :
    public ::google::protobuf::Message /* @@protoc_insertion_point(class_definition:vision.raw.v1.SubmitResultResponse) */ {
 public:
  inline SubmitResultResponse() : SubmitResultResponse(nullptr) {}
  ~SubmitResultResponse() override;
  template<typename = void>
  explicit PROTOBUF_CONSTEXPR SubmitResultResponse(::google::protobuf::internal::ConstantInitialized);

  inline SubmitResultResponse(const SubmitResultResponse& from)
      : SubmitResultResponse(nullptr, from) {}
  SubmitResultResponse(SubmitResultResponse&& from) noexcept
    : SubmitResultResponse() {
    *this = ::std::move(from);
  }

  inline SubmitResultResponse& operator=(const SubmitResultResponse& from) {
    CopyFrom(from);
    return *this;
  }
  inline SubmitResultResponse& operator=(SubmitResultResponse&& from) noexcept {
    if (this == &from) return *this;
    if (GetArena() == from.GetArena()
  #ifdef PROTOBUF_FORCE_COPY_IN_MOVE
        && GetArena() != nullptr
  #endif  // !PROTOBUF_FORCE_COPY_IN_MOVE
    ) {
      InternalSwap(&from);
    } else {
      CopyFrom(from);
    }
    return *this;
  }

  inline const ::google::protobuf::UnknownFieldSet& unknown_fields() const
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return _internal_metadata_.unknown_fields<::google::protobuf::UnknownFieldSet>(::google::protobuf::UnknownFieldSet::default_instance);
  }
  inline ::google::protobuf::UnknownFieldSet* mutable_unknown_fields()
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return _internal_metadata_.mutable_unknown_fields<::google::protobuf::UnknownFieldSet>();
  }

  static const ::google::protobuf::Descriptor* descriptor() {
    return GetDescriptor();
  }
  static const ::google::protobuf::Descriptor* GetDescriptor() {
    return default_instance().GetMetadata().descriptor;
  }
  static const ::google::protobuf::Reflection* GetReflection() {
    return default_instance().GetMetadata().reflection;
  }
  static const SubmitResultResponse& default_instance() {
    return *internal_default_instance();
  }
  static inline const SubmitResultResponse* internal_default_instance() {
    return reinterpret_cast<const SubmitResultResponse*>(
               &_SubmitResultResponse_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    2;

  friend void swap(SubmitResultResponse& a, SubmitResultResponse& b) {
    a.Swap(&b);
  }
  inline void Swap(SubmitResultResponse* other) {
    if (other == this) return;
  #ifdef PROTOBUF_FORCE_COPY_IN_SWAP
    if (GetArena() != nullptr &&
        GetArena() == other->GetArena()) {
   #else  // PROTOBUF_FORCE_COPY_IN_SWAP
    if (GetArena() == other->GetArena()) {
  #endif  // !PROTOBUF_FORCE_COPY_IN_SWAP
      InternalSwap(other);
    } else {
      ::google::protobuf::internal::GenericSwap(this, other);
    }
  }
  void UnsafeArenaSwap(SubmitResultResponse* other) {
    if (other == this) return;
    ABSL_DCHECK(GetArena() == other->GetArena());
    InternalSwap(other);
  }

  // implements Message ----------------------------------------------

  SubmitResultResponse* New(::google::protobuf::Arena* arena = nullptr) const final {
    return CreateMaybeMessage<SubmitResultResponse>(arena);
  }
  using ::google::protobuf::Message::CopyFrom;
  void CopyFrom(const SubmitResultResponse& from);
  using ::google::protobuf::Message::MergeFrom;
  void MergeFrom( const SubmitResultResponse& from) {
    SubmitResultResponse::MergeImpl(*this, from);
  }
  private:
  static void MergeImpl(::google::protobuf::Message& to_msg, const ::google::protobuf::Message& from_msg);
  public:
  PROTOBUF_ATTRIBUTE_REINITIALIZES void Clear() final;
  bool IsInitialized() const final;

  ::size_t ByteSizeLong() const final;
  const char* _InternalParse(const char* ptr, ::google::protobuf::internal::ParseContext* ctx) final;
  ::uint8_t* _InternalSerialize(
      ::uint8_t* target, ::google::protobuf::io::EpsCopyOutputStream* stream) const final;
  int GetCachedSize() const { return _impl_._cached_size_.Get(); }

  private:
  ::google::protobuf::internal::CachedSize* AccessCachedSize() const final;
  void SharedCtor(::google::protobuf::Arena* arena);
  void SharedDtor();
  void InternalSwap(SubmitResultResponse* other);

  private:
  friend class ::google::protobuf::internal::AnyMetadata;
  static ::absl::string_view FullMessageName() {
    return "vision.raw.v1.SubmitResultResponse";
  }
  protected:
  explicit SubmitResultResponse(::google::protobuf::Arena* arena);
  SubmitResultResponse(::google::protobuf::Arena* arena, const SubmitResultResponse& from);
  public:

  static const ClassData _class_data_;
  const ::google::protobuf::Message::ClassData*GetClassData() const final;

  ::google::protobuf::Metadata GetMetadata() const final;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  enum : int {
    kSessionIdFieldNumber = 1,
    kMessageFieldNumber = 3,
    kFrameCountFieldNumber = 2,
  };
  // string session_id = 1;
  void clear_session_id() ;
  const std::string& session_id() const;
  template <typename Arg_ = const std::string&, typename... Args_>
  void set_session_id(Arg_&& arg, Args_... args);
  std::string* mutable_session_id();
  PROTOBUF_NODISCARD std::string* release_session_id();
  void set_allocated_session_id(std::string* value);

  private:
  const std::string& _internal_session_id() const;
  inline PROTOBUF_ALWAYS_INLINE void _internal_set_session_id(
      const std::string& value);
  std::string* _internal_mutable_session_id();

  public:
  // string message = 3;
  void clear_message() ;
  const std::string& message() const;
  template <typename Arg_ = const std::string&, typename... Args_>
  void set_message(Arg_&& arg, Args_... args);
  std::string* mutable_message();
  PROTOBUF_NODISCARD std::string* release_message();
  void set_allocated_message(std::string* value);

  private:
  const std::string& _internal_message() const;
  inline PROTOBUF_ALWAYS_INLINE void _internal_set_message(
      const std::string& value);
  std::string* _internal_mutable_message();

  public:
  // int32 frame_count = 2;
  void clear_frame_count() ;
  ::int32_t frame_count() const;
  void set_frame_count(::int32_t value);

  private:
  ::int32_t _internal_frame_count() const;
  void _internal_set_frame_count(::int32_t value);

  public:
  // @@protoc_insertion_point(class_scope:vision.raw.v1.SubmitResultResponse)
 private:
  class _Internal;

  friend class ::google::protobuf::internal::TcParser;
  static const ::google::protobuf::internal::TcParseTable<
      2, 3, 0,
      60, 2>
      _table_;
  friend class ::google::protobuf::MessageLite;
  friend class ::google::protobuf::Arena;
  template <typename T>
  friend class ::google::protobuf::Arena::InternalHelper;
  using InternalArenaConstructable_ = void;
  using DestructorSkippable_ = void;
  struct Impl_ {

        inline explicit constexpr Impl_(
            ::google::protobuf::internal::ConstantInitialized) noexcept;
        inline explicit Impl_(::google::protobuf::internal::InternalVisibility visibility,
                              ::google::protobuf::Arena* arena);
        inline explicit Impl_(::google::protobuf::internal::InternalVisibility visibility,
                              ::google::protobuf::Arena* arena, const Impl_& from);
    ::google::protobuf::internal::ArenaStringPtr session_id_;
    ::google::protobuf::internal::ArenaStringPtr message_;
    ::int32_t frame_count_;
    mutable ::google::protobuf::internal::CachedSize _cached_size_;
    PROTOBUF_TSAN_DECLARE_MEMBER
  };
  union { Impl_ _impl_; };
  friend struct ::TableStruct_ksl_5fsentence_5frecognition_2eproto;
};// -------------------------------------------------------------------

class Point3 final :
    public ::google::protobuf::Message /* @@protoc_insertion_point(class_definition:vision.raw.v1.Point3) */ {
 public:
  inline Point3() : Point3(nullptr) {}
  ~Point3() override;
  template<typename = void>
  explicit PROTOBUF_CONSTEXPR Point3(::google::protobuf::internal::ConstantInitialized);

  inline Point3(const Point3& from)
      : Point3(nullptr, from) {}
  Point3(Point3&& from) noexcept
    : Point3() {
    *this = ::std::move(from);
  }

  inline Point3& operator=(const Point3& from) {
    CopyFrom(from);
    return *this;
  }
  inline Point3& operator=(Point3&& from) noexcept {
    if (this == &from) return *this;
    if (GetArena() == from.GetArena()
  #ifdef PROTOBUF_FORCE_COPY_IN_MOVE
        && GetArena() != nullptr
  #endif  // !PROTOBUF_FORCE_COPY_IN_MOVE
    ) {
      InternalSwap(&from);
    } else {
      CopyFrom(from);
    }
    return *this;
  }

  inline const ::google::protobuf::UnknownFieldSet& unknown_fields() const
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return _internal_metadata_.unknown_fields<::google::protobuf::UnknownFieldSet>(::google::protobuf::UnknownFieldSet::default_instance);
  }
  inline ::google::protobuf::UnknownFieldSet* mutable_unknown_fields()
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return _internal_metadata_.mutable_unknown_fields<::google::protobuf::UnknownFieldSet>();
  }

  static const ::google::protobuf::Descriptor* descriptor() {
    return GetDescriptor();
  }
  static const ::google::protobuf::Descriptor* GetDescriptor() {
    return default_instance().GetMetadata().descriptor;
  }
  static const ::google::protobuf::Reflection* GetReflection() {
    return default_instance().GetMetadata().reflection;
  }
  static const Point3& default_instance() {
    return *internal_default_instance();
  }
  static inline const Point3* internal_default_instance() {
    return reinterpret_cast<const Point3*>(
               &_Point3_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    0;

  friend void swap(Point3& a, Point3& b) {
    a.Swap(&b);
  }
  inline void Swap(Point3* other) {
    if (other == this) return;
  #ifdef PROTOBUF_FORCE_COPY_IN_SWAP
    if (GetArena() != nullptr &&
        GetArena() == other->GetArena()) {
   #else  // PROTOBUF_FORCE_COPY_IN_SWAP
    if (GetArena() == other->GetArena()) {
  #endif  // !PROTOBUF_FORCE_COPY_IN_SWAP
      InternalSwap(other);
    } else {
      ::google::protobuf::internal::GenericSwap(this, other);
    }
  }
  void UnsafeArenaSwap(Point3* other) {
    if (other == this) return;
    ABSL_DCHECK(GetArena() == other->GetArena());
    InternalSwap(other);
  }

  // implements Message ----------------------------------------------

  Point3* New(::google::protobuf::Arena* arena = nullptr) const final {
    return CreateMaybeMessage<Point3>(arena);
  }
  using ::google::protobuf::Message::CopyFrom;
  void CopyFrom(const Point3& from);
  using ::google::protobuf::Message::MergeFrom;
  void MergeFrom( const Point3& from) {
    Point3::MergeImpl(*this, from);
  }
  private:
  static void MergeImpl(::google::protobuf::Message& to_msg, const ::google::protobuf::Message& from_msg);
  public:
  PROTOBUF_ATTRIBUTE_REINITIALIZES void Clear() final;
  bool IsInitialized() const final;

  ::size_t ByteSizeLong() const final;
  const char* _InternalParse(const char* ptr, ::google::protobuf::internal::ParseContext* ctx) final;
  ::uint8_t* _InternalSerialize(
      ::uint8_t* target, ::google::protobuf::io::EpsCopyOutputStream* stream) const final;
  int GetCachedSize() const { return _impl_._cached_size_.Get(); }

  private:
  ::google::protobuf::internal::CachedSize* AccessCachedSize() const final;
  void SharedCtor(::google::protobuf::Arena* arena);
  void SharedDtor();
  void InternalSwap(Point3* other);

  private:
  friend class ::google::protobuf::internal::AnyMetadata;
  static ::absl::string_view FullMessageName() {
    return "vision.raw.v1.Point3";
  }
  protected:
  explicit Point3(::google::protobuf::Arena* arena);
  Point3(::google::protobuf::Arena* arena, const Point3& from);
  public:

  static const ClassData _class_data_;
  const ::google::protobuf::Message::ClassData*GetClassData() const final;

  ::google::protobuf::Metadata GetMetadata() const final;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  enum : int {
    kXFieldNumber = 1,
    kYFieldNumber = 2,
    kZFieldNumber = 3,
  };
  // float x = 1;
  void clear_x() ;
  float x() const;
  void set_x(float value);

  private:
  float _internal_x() const;
  void _internal_set_x(float value);

  public:
  // float y = 2;
  void clear_y() ;
  float y() const;
  void set_y(float value);

  private:
  float _internal_y() const;
  void _internal_set_y(float value);

  public:
  // float z = 3;
  void clear_z() ;
  float z() const;
  void set_z(float value);

  private:
  float _internal_z() const;
  void _internal_set_z(float value);

  public:
  // @@protoc_insertion_point(class_scope:vision.raw.v1.Point3)
 private:
  class _Internal;

  friend class ::google::protobuf::internal::TcParser;
  static const ::google::protobuf::internal::TcParseTable<
      2, 3, 0,
      0, 2>
      _table_;
  friend class ::google::protobuf::MessageLite;
  friend class ::google::protobuf::Arena;
  template <typename T>
  friend class ::google::protobuf::Arena::InternalHelper;
  using InternalArenaConstructable_ = void;
  using DestructorSkippable_ = void;
  struct Impl_ {

        inline explicit constexpr Impl_(
            ::google::protobuf::internal::ConstantInitialized) noexcept;
        inline explicit Impl_(::google::protobuf::internal::InternalVisibility visibility,
                              ::google::protobuf::Arena* arena);
        inline explicit Impl_(::google::protobuf::internal::InternalVisibility visibility,
                              ::google::protobuf::Arena* arena, const Impl_& from);
    float x_;
    float y_;
    float z_;
    mutable ::google::protobuf::internal::CachedSize _cached_size_;
    PROTOBUF_TSAN_DECLARE_MEMBER
  };
  union { Impl_ _impl_; };
  friend struct ::TableStruct_ksl_5fsentence_5frecognition_2eproto;
};// -------------------------------------------------------------------

class Frame final :
    public ::google::protobuf::Message /* @@protoc_insertion_point(class_definition:vision.raw.v1.Frame) */ {
 public:
  inline Frame() : Frame(nullptr) {}
  ~Frame() override;
  template<typename = void>
  explicit PROTOBUF_CONSTEXPR Frame(::google::protobuf::internal::ConstantInitialized);

  inline Frame(const Frame& from)
      : Frame(nullptr, from) {}
  Frame(Frame&& from) noexcept
    : Frame() {
    *this = ::std::move(from);
  }

  inline Frame& operator=(const Frame& from) {
    CopyFrom(from);
    return *this;
  }
  inline Frame& operator=(Frame&& from) noexcept {
    if (this == &from) return *this;
    if (GetArena() == from.GetArena()
  #ifdef PROTOBUF_FORCE_COPY_IN_MOVE
        && GetArena() != nullptr
  #endif  // !PROTOBUF_FORCE_COPY_IN_MOVE
    ) {
      InternalSwap(&from);
    } else {
      CopyFrom(from);
    }
    return *this;
  }

  inline const ::google::protobuf::UnknownFieldSet& unknown_fields() const
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return _internal_metadata_.unknown_fields<::google::protobuf::UnknownFieldSet>(::google::protobuf::UnknownFieldSet::default_instance);
  }
  inline ::google::protobuf::UnknownFieldSet* mutable_unknown_fields()
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return _internal_metadata_.mutable_unknown_fields<::google::protobuf::UnknownFieldSet>();
  }

  static const ::google::protobuf::Descriptor* descriptor() {
    return GetDescriptor();
  }
  static const ::google::protobuf::Descriptor* GetDescriptor() {
    return default_instance().GetMetadata().descriptor;
  }
  static const ::google::protobuf::Reflection* GetReflection() {
    return default_instance().GetMetadata().reflection;
  }
  static const Frame& default_instance() {
    return *internal_default_instance();
  }
  static inline const Frame* internal_default_instance() {
    return reinterpret_cast<const Frame*>(
               &_Frame_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    1;

  friend void swap(Frame& a, Frame& b) {
    a.Swap(&b);
  }
  inline void Swap(Frame* other) {
    if (other == this) return;
  #ifdef PROTOBUF_FORCE_COPY_IN_SWAP
    if (GetArena() != nullptr &&
        GetArena() == other->GetArena()) {
   #else  // PROTOBUF_FORCE_COPY_IN_SWAP
    if (GetArena() == other->GetArena()) {
  #endif  // !PROTOBUF_FORCE_COPY_IN_SWAP
      InternalSwap(other);
    } else {
      ::google::protobuf::internal::GenericSwap(this, other);
    }
  }
  void UnsafeArenaSwap(Frame* other) {
    if (other == this) return;
    ABSL_DCHECK(GetArena() == other->GetArena());
    InternalSwap(other);
  }

  // implements Message ----------------------------------------------

  Frame* New(::google::protobuf::Arena* arena = nullptr) const final {
    return CreateMaybeMessage<Frame>(arena);
  }
  using ::google::protobuf::Message::CopyFrom;
  void CopyFrom(const Frame& from);
  using ::google::protobuf::Message::MergeFrom;
  void MergeFrom( const Frame& from) {
    Frame::MergeImpl(*this, from);
  }
  private:
  static void MergeImpl(::google::protobuf::Message& to_msg, const ::google::protobuf::Message& from_msg);
  public:
  PROTOBUF_ATTRIBUTE_REINITIALIZES void Clear() final;
  bool IsInitialized() const final;

  ::size_t ByteSizeLong() const final;
  const char* _InternalParse(const char* ptr, ::google::protobuf::internal::ParseContext* ctx) final;
  ::uint8_t* _InternalSerialize(
      ::uint8_t* target, ::google::protobuf::io::EpsCopyOutputStream* stream) const final;
  int GetCachedSize() const { return _impl_._cached_size_.Get(); }

  private:
  ::google::protobuf::internal::CachedSize* AccessCachedSize() const final;
  void SharedCtor(::google::protobuf::Arena* arena);
  void SharedDtor();
  void InternalSwap(Frame* other);

  private:
  friend class ::google::protobuf::internal::AnyMetadata;
  static ::absl::string_view FullMessageName() {
    return "vision.raw.v1.Frame";
  }
  protected:
  explicit Frame(::google::protobuf::Arena* arena);
  Frame(::google::protobuf::Arena* arena, const Frame& from);
  public:

  static const ClassData _class_data_;
  const ::google::protobuf::Message::ClassData*GetClassData() const final;

  ::google::protobuf::Metadata GetMetadata() const final;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  enum : int {
    kPosePointsFieldNumber = 8,
    kSessionIdFieldNumber = 1,
    kDataFieldNumber = 7,
    kIndexFieldNumber = 2,
    kFlagFieldNumber = 3,
    kWidthFieldNumber = 4,
    kHeightFieldNumber = 5,
    kTypeFieldNumber = 6,
  };
  // repeated .vision.raw.v1.Point3 pose_points = 8;
  int pose_points_size() const;
  private:
  int _internal_pose_points_size() const;

  public:
  void clear_pose_points() ;
  ::vision::raw::v1::Point3* mutable_pose_points(int index);
  ::google::protobuf::RepeatedPtrField< ::vision::raw::v1::Point3 >*
      mutable_pose_points();
  private:
  const ::google::protobuf::RepeatedPtrField<::vision::raw::v1::Point3>& _internal_pose_points() const;
  ::google::protobuf::RepeatedPtrField<::vision::raw::v1::Point3>* _internal_mutable_pose_points();
  public:
  const ::vision::raw::v1::Point3& pose_points(int index) const;
  ::vision::raw::v1::Point3* add_pose_points();
  const ::google::protobuf::RepeatedPtrField< ::vision::raw::v1::Point3 >&
      pose_points() const;
  // string session_id = 1;
  void clear_session_id() ;
  const std::string& session_id() const;
  template <typename Arg_ = const std::string&, typename... Args_>
  void set_session_id(Arg_&& arg, Args_... args);
  std::string* mutable_session_id();
  PROTOBUF_NODISCARD std::string* release_session_id();
  void set_allocated_session_id(std::string* value);

  private:
  const std::string& _internal_session_id() const;
  inline PROTOBUF_ALWAYS_INLINE void _internal_set_session_id(
      const std::string& value);
  std::string* _internal_mutable_session_id();

  public:
  // bytes data = 7;
  void clear_data() ;
  const std::string& data() const;
  template <typename Arg_ = const std::string&, typename... Args_>
  void set_data(Arg_&& arg, Args_... args);
  std::string* mutable_data();
  PROTOBUF_NODISCARD std::string* release_data();
  void set_allocated_data(std::string* value);

  private:
  const std::string& _internal_data() const;
  inline PROTOBUF_ALWAYS_INLINE void _internal_set_data(
      const std::string& value);
  std::string* _internal_mutable_data();

  public:
  // int32 index = 2;
  void clear_index() ;
  ::int32_t index() const;
  void set_index(::int32_t value);

  private:
  ::int32_t _internal_index() const;
  void _internal_set_index(::int32_t value);

  public:
  // int32 flag = 3;
  void clear_flag() ;
  ::int32_t flag() const;
  void set_flag(::int32_t value);

  private:
  ::int32_t _internal_flag() const;
  void _internal_set_flag(::int32_t value);

  public:
  // int32 width = 4;
  void clear_width() ;
  ::int32_t width() const;
  void set_width(::int32_t value);

  private:
  ::int32_t _internal_width() const;
  void _internal_set_width(::int32_t value);

  public:
  // int32 height = 5;
  void clear_height() ;
  ::int32_t height() const;
  void set_height(::int32_t value);

  private:
  ::int32_t _internal_height() const;
  void _internal_set_height(::int32_t value);

  public:
  // int32 type = 6;
  void clear_type() ;
  ::int32_t type() const;
  void set_type(::int32_t value);

  private:
  ::int32_t _internal_type() const;
  void _internal_set_type(::int32_t value);

  public:
  // @@protoc_insertion_point(class_scope:vision.raw.v1.Frame)
 private:
  class _Internal;

  friend class ::google::protobuf::internal::TcParser;
  static const ::google::protobuf::internal::TcParseTable<
      3, 8, 1,
      46, 2>
      _table_;
  friend class ::google::protobuf::MessageLite;
  friend class ::google::protobuf::Arena;
  template <typename T>
  friend class ::google::protobuf::Arena::InternalHelper;
  using InternalArenaConstructable_ = void;
  using DestructorSkippable_ = void;
  struct Impl_ {

        inline explicit constexpr Impl_(
            ::google::protobuf::internal::ConstantInitialized) noexcept;
        inline explicit Impl_(::google::protobuf::internal::InternalVisibility visibility,
                              ::google::protobuf::Arena* arena);
        inline explicit Impl_(::google::protobuf::internal::InternalVisibility visibility,
                              ::google::protobuf::Arena* arena, const Impl_& from);
    ::google::protobuf::RepeatedPtrField< ::vision::raw::v1::Point3 > pose_points_;
    ::google::protobuf::internal::ArenaStringPtr session_id_;
    ::google::protobuf::internal::ArenaStringPtr data_;
    ::int32_t index_;
    ::int32_t flag_;
    ::int32_t width_;
    ::int32_t height_;
    ::int32_t type_;
    mutable ::google::protobuf::internal::CachedSize _cached_size_;
    PROTOBUF_TSAN_DECLARE_MEMBER
  };
  union { Impl_ _impl_; };
  friend struct ::TableStruct_ksl_5fsentence_5frecognition_2eproto;
};

// ===================================================================




// ===================================================================


#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif  // __GNUC__
// -------------------------------------------------------------------

// Point3

// float x = 1;
inline void Point3::clear_x() {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  _impl_.x_ = 0;
}
inline float Point3::x() const {
  // @@protoc_insertion_point(field_get:vision.raw.v1.Point3.x)
  return _internal_x();
}
inline void Point3::set_x(float value) {
  _internal_set_x(value);
  // @@protoc_insertion_point(field_set:vision.raw.v1.Point3.x)
}
inline float Point3::_internal_x() const {
  PROTOBUF_TSAN_READ(&_impl_._tsan_detect_race);
  return _impl_.x_;
}
inline void Point3::_internal_set_x(float value) {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  ;
  _impl_.x_ = value;
}

// float y = 2;
inline void Point3::clear_y() {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  _impl_.y_ = 0;
}
inline float Point3::y() const {
  // @@protoc_insertion_point(field_get:vision.raw.v1.Point3.y)
  return _internal_y();
}
inline void Point3::set_y(float value) {
  _internal_set_y(value);
  // @@protoc_insertion_point(field_set:vision.raw.v1.Point3.y)
}
inline float Point3::_internal_y() const {
  PROTOBUF_TSAN_READ(&_impl_._tsan_detect_race);
  return _impl_.y_;
}
inline void Point3::_internal_set_y(float value) {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  ;
  _impl_.y_ = value;
}

// float z = 3;
inline void Point3::clear_z() {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  _impl_.z_ = 0;
}
inline float Point3::z() const {
  // @@protoc_insertion_point(field_get:vision.raw.v1.Point3.z)
  return _internal_z();
}
inline void Point3::set_z(float value) {
  _internal_set_z(value);
  // @@protoc_insertion_point(field_set:vision.raw.v1.Point3.z)
}
inline float Point3::_internal_z() const {
  PROTOBUF_TSAN_READ(&_impl_._tsan_detect_race);
  return _impl_.z_;
}
inline void Point3::_internal_set_z(float value) {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  ;
  _impl_.z_ = value;
}

// -------------------------------------------------------------------

// Frame

// string session_id = 1;
inline void Frame::clear_session_id() {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  _impl_.session_id_.ClearToEmpty();
}
inline const std::string& Frame::session_id() const
    ABSL_ATTRIBUTE_LIFETIME_BOUND {
  // @@protoc_insertion_point(field_get:vision.raw.v1.Frame.session_id)
  return _internal_session_id();
}
template <typename Arg_, typename... Args_>
inline PROTOBUF_ALWAYS_INLINE void Frame::set_session_id(Arg_&& arg,
                                                     Args_... args) {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  ;
  _impl_.session_id_.Set(static_cast<Arg_&&>(arg), args..., GetArena());
  // @@protoc_insertion_point(field_set:vision.raw.v1.Frame.session_id)
}
inline std::string* Frame::mutable_session_id() ABSL_ATTRIBUTE_LIFETIME_BOUND {
  std::string* _s = _internal_mutable_session_id();
  // @@protoc_insertion_point(field_mutable:vision.raw.v1.Frame.session_id)
  return _s;
}
inline const std::string& Frame::_internal_session_id() const {
  PROTOBUF_TSAN_READ(&_impl_._tsan_detect_race);
  return _impl_.session_id_.Get();
}
inline void Frame::_internal_set_session_id(const std::string& value) {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  ;
  _impl_.session_id_.Set(value, GetArena());
}
inline std::string* Frame::_internal_mutable_session_id() {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  ;
  return _impl_.session_id_.Mutable( GetArena());
}
inline std::string* Frame::release_session_id() {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  // @@protoc_insertion_point(field_release:vision.raw.v1.Frame.session_id)
  return _impl_.session_id_.Release();
}
inline void Frame::set_allocated_session_id(std::string* value) {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  _impl_.session_id_.SetAllocated(value, GetArena());
  #ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
        if (_impl_.session_id_.IsDefault()) {
          _impl_.session_id_.Set("", GetArena());
        }
  #endif  // PROTOBUF_FORCE_COPY_DEFAULT_STRING
  // @@protoc_insertion_point(field_set_allocated:vision.raw.v1.Frame.session_id)
}

// int32 index = 2;
inline void Frame::clear_index() {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  _impl_.index_ = 0;
}
inline ::int32_t Frame::index() const {
  // @@protoc_insertion_point(field_get:vision.raw.v1.Frame.index)
  return _internal_index();
}
inline void Frame::set_index(::int32_t value) {
  _internal_set_index(value);
  // @@protoc_insertion_point(field_set:vision.raw.v1.Frame.index)
}
inline ::int32_t Frame::_internal_index() const {
  PROTOBUF_TSAN_READ(&_impl_._tsan_detect_race);
  return _impl_.index_;
}
inline void Frame::_internal_set_index(::int32_t value) {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  ;
  _impl_.index_ = value;
}

// int32 flag = 3;
inline void Frame::clear_flag() {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  _impl_.flag_ = 0;
}
inline ::int32_t Frame::flag() const {
  // @@protoc_insertion_point(field_get:vision.raw.v1.Frame.flag)
  return _internal_flag();
}
inline void Frame::set_flag(::int32_t value) {
  _internal_set_flag(value);
  // @@protoc_insertion_point(field_set:vision.raw.v1.Frame.flag)
}
inline ::int32_t Frame::_internal_flag() const {
  PROTOBUF_TSAN_READ(&_impl_._tsan_detect_race);
  return _impl_.flag_;
}
inline void Frame::_internal_set_flag(::int32_t value) {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  ;
  _impl_.flag_ = value;
}

// int32 width = 4;
inline void Frame::clear_width() {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  _impl_.width_ = 0;
}
inline ::int32_t Frame::width() const {
  // @@protoc_insertion_point(field_get:vision.raw.v1.Frame.width)
  return _internal_width();
}
inline void Frame::set_width(::int32_t value) {
  _internal_set_width(value);
  // @@protoc_insertion_point(field_set:vision.raw.v1.Frame.width)
}
inline ::int32_t Frame::_internal_width() const {
  PROTOBUF_TSAN_READ(&_impl_._tsan_detect_race);
  return _impl_.width_;
}
inline void Frame::_internal_set_width(::int32_t value) {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  ;
  _impl_.width_ = value;
}

// int32 height = 5;
inline void Frame::clear_height() {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  _impl_.height_ = 0;
}
inline ::int32_t Frame::height() const {
  // @@protoc_insertion_point(field_get:vision.raw.v1.Frame.height)
  return _internal_height();
}
inline void Frame::set_height(::int32_t value) {
  _internal_set_height(value);
  // @@protoc_insertion_point(field_set:vision.raw.v1.Frame.height)
}
inline ::int32_t Frame::_internal_height() const {
  PROTOBUF_TSAN_READ(&_impl_._tsan_detect_race);
  return _impl_.height_;
}
inline void Frame::_internal_set_height(::int32_t value) {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  ;
  _impl_.height_ = value;
}

// int32 type = 6;
inline void Frame::clear_type() {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  _impl_.type_ = 0;
}
inline ::int32_t Frame::type() const {
  // @@protoc_insertion_point(field_get:vision.raw.v1.Frame.type)
  return _internal_type();
}
inline void Frame::set_type(::int32_t value) {
  _internal_set_type(value);
  // @@protoc_insertion_point(field_set:vision.raw.v1.Frame.type)
}
inline ::int32_t Frame::_internal_type() const {
  PROTOBUF_TSAN_READ(&_impl_._tsan_detect_race);
  return _impl_.type_;
}
inline void Frame::_internal_set_type(::int32_t value) {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  ;
  _impl_.type_ = value;
}

// bytes data = 7;
inline void Frame::clear_data() {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  _impl_.data_.ClearToEmpty();
}
inline const std::string& Frame::data() const
    ABSL_ATTRIBUTE_LIFETIME_BOUND {
  // @@protoc_insertion_point(field_get:vision.raw.v1.Frame.data)
  return _internal_data();
}
template <typename Arg_, typename... Args_>
inline PROTOBUF_ALWAYS_INLINE void Frame::set_data(Arg_&& arg,
                                                     Args_... args) {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  ;
  _impl_.data_.SetBytes(static_cast<Arg_&&>(arg), args..., GetArena());
  // @@protoc_insertion_point(field_set:vision.raw.v1.Frame.data)
}
inline std::string* Frame::mutable_data() ABSL_ATTRIBUTE_LIFETIME_BOUND {
  std::string* _s = _internal_mutable_data();
  // @@protoc_insertion_point(field_mutable:vision.raw.v1.Frame.data)
  return _s;
}
inline const std::string& Frame::_internal_data() const {
  PROTOBUF_TSAN_READ(&_impl_._tsan_detect_race);
  return _impl_.data_.Get();
}
inline void Frame::_internal_set_data(const std::string& value) {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  ;
  _impl_.data_.Set(value, GetArena());
}
inline std::string* Frame::_internal_mutable_data() {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  ;
  return _impl_.data_.Mutable( GetArena());
}
inline std::string* Frame::release_data() {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  // @@protoc_insertion_point(field_release:vision.raw.v1.Frame.data)
  return _impl_.data_.Release();
}
inline void Frame::set_allocated_data(std::string* value) {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  _impl_.data_.SetAllocated(value, GetArena());
  #ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
        if (_impl_.data_.IsDefault()) {
          _impl_.data_.Set("", GetArena());
        }
  #endif  // PROTOBUF_FORCE_COPY_DEFAULT_STRING
  // @@protoc_insertion_point(field_set_allocated:vision.raw.v1.Frame.data)
}

// repeated .vision.raw.v1.Point3 pose_points = 8;
inline int Frame::_internal_pose_points_size() const {
  return _internal_pose_points().size();
}
inline int Frame::pose_points_size() const {
  return _internal_pose_points_size();
}
inline void Frame::clear_pose_points() {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  _impl_.pose_points_.Clear();
}
inline ::vision::raw::v1::Point3* Frame::mutable_pose_points(int index)
    ABSL_ATTRIBUTE_LIFETIME_BOUND {
  // @@protoc_insertion_point(field_mutable:vision.raw.v1.Frame.pose_points)
  return _internal_mutable_pose_points()->Mutable(index);
}
inline ::google::protobuf::RepeatedPtrField<::vision::raw::v1::Point3>* Frame::mutable_pose_points()
    ABSL_ATTRIBUTE_LIFETIME_BOUND {
  // @@protoc_insertion_point(field_mutable_list:vision.raw.v1.Frame.pose_points)
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  return _internal_mutable_pose_points();
}
inline const ::vision::raw::v1::Point3& Frame::pose_points(int index) const
    ABSL_ATTRIBUTE_LIFETIME_BOUND {
  // @@protoc_insertion_point(field_get:vision.raw.v1.Frame.pose_points)
  return _internal_pose_points().Get(index);
}
inline ::vision::raw::v1::Point3* Frame::add_pose_points() ABSL_ATTRIBUTE_LIFETIME_BOUND {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  ::vision::raw::v1::Point3* _add = _internal_mutable_pose_points()->Add();
  // @@protoc_insertion_point(field_add:vision.raw.v1.Frame.pose_points)
  return _add;
}
inline const ::google::protobuf::RepeatedPtrField<::vision::raw::v1::Point3>& Frame::pose_points() const
    ABSL_ATTRIBUTE_LIFETIME_BOUND {
  // @@protoc_insertion_point(field_list:vision.raw.v1.Frame.pose_points)
  return _internal_pose_points();
}
inline const ::google::protobuf::RepeatedPtrField<::vision::raw::v1::Point3>&
Frame::_internal_pose_points() const {
  PROTOBUF_TSAN_READ(&_impl_._tsan_detect_race);
  return _impl_.pose_points_;
}
inline ::google::protobuf::RepeatedPtrField<::vision::raw::v1::Point3>*
Frame::_internal_mutable_pose_points() {
  PROTOBUF_TSAN_READ(&_impl_._tsan_detect_race);
  return &_impl_.pose_points_;
}

// -------------------------------------------------------------------

// SubmitResultResponse

// string session_id = 1;
inline void SubmitResultResponse::clear_session_id() {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  _impl_.session_id_.ClearToEmpty();
}
inline const std::string& SubmitResultResponse::session_id() const
    ABSL_ATTRIBUTE_LIFETIME_BOUND {
  // @@protoc_insertion_point(field_get:vision.raw.v1.SubmitResultResponse.session_id)
  return _internal_session_id();
}
template <typename Arg_, typename... Args_>
inline PROTOBUF_ALWAYS_INLINE void SubmitResultResponse::set_session_id(Arg_&& arg,
                                                     Args_... args) {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  ;
  _impl_.session_id_.Set(static_cast<Arg_&&>(arg), args..., GetArena());
  // @@protoc_insertion_point(field_set:vision.raw.v1.SubmitResultResponse.session_id)
}
inline std::string* SubmitResultResponse::mutable_session_id() ABSL_ATTRIBUTE_LIFETIME_BOUND {
  std::string* _s = _internal_mutable_session_id();
  // @@protoc_insertion_point(field_mutable:vision.raw.v1.SubmitResultResponse.session_id)
  return _s;
}
inline const std::string& SubmitResultResponse::_internal_session_id() const {
  PROTOBUF_TSAN_READ(&_impl_._tsan_detect_race);
  return _impl_.session_id_.Get();
}
inline void SubmitResultResponse::_internal_set_session_id(const std::string& value) {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  ;
  _impl_.session_id_.Set(value, GetArena());
}
inline std::string* SubmitResultResponse::_internal_mutable_session_id() {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  ;
  return _impl_.session_id_.Mutable( GetArena());
}
inline std::string* SubmitResultResponse::release_session_id() {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  // @@protoc_insertion_point(field_release:vision.raw.v1.SubmitResultResponse.session_id)
  return _impl_.session_id_.Release();
}
inline void SubmitResultResponse::set_allocated_session_id(std::string* value) {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  _impl_.session_id_.SetAllocated(value, GetArena());
  #ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
        if (_impl_.session_id_.IsDefault()) {
          _impl_.session_id_.Set("", GetArena());
        }
  #endif  // PROTOBUF_FORCE_COPY_DEFAULT_STRING
  // @@protoc_insertion_point(field_set_allocated:vision.raw.v1.SubmitResultResponse.session_id)
}

// int32 frame_count = 2;
inline void SubmitResultResponse::clear_frame_count() {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  _impl_.frame_count_ = 0;
}
inline ::int32_t SubmitResultResponse::frame_count() const {
  // @@protoc_insertion_point(field_get:vision.raw.v1.SubmitResultResponse.frame_count)
  return _internal_frame_count();
}
inline void SubmitResultResponse::set_frame_count(::int32_t value) {
  _internal_set_frame_count(value);
  // @@protoc_insertion_point(field_set:vision.raw.v1.SubmitResultResponse.frame_count)
}
inline ::int32_t SubmitResultResponse::_internal_frame_count() const {
  PROTOBUF_TSAN_READ(&_impl_._tsan_detect_race);
  return _impl_.frame_count_;
}
inline void SubmitResultResponse::_internal_set_frame_count(::int32_t value) {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  ;
  _impl_.frame_count_ = value;
}

// string message = 3;
inline void SubmitResultResponse::clear_message() {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  _impl_.message_.ClearToEmpty();
}
inline const std::string& SubmitResultResponse::message() const
    ABSL_ATTRIBUTE_LIFETIME_BOUND {
  // @@protoc_insertion_point(field_get:vision.raw.v1.SubmitResultResponse.message)
  return _internal_message();
}
template <typename Arg_, typename... Args_>
inline PROTOBUF_ALWAYS_INLINE void SubmitResultResponse::set_message(Arg_&& arg,
                                                     Args_... args) {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  ;
  _impl_.message_.Set(static_cast<Arg_&&>(arg), args..., GetArena());
  // @@protoc_insertion_point(field_set:vision.raw.v1.SubmitResultResponse.message)
}
inline std::string* SubmitResultResponse::mutable_message() ABSL_ATTRIBUTE_LIFETIME_BOUND {
  std::string* _s = _internal_mutable_message();
  // @@protoc_insertion_point(field_mutable:vision.raw.v1.SubmitResultResponse.message)
  return _s;
}
inline const std::string& SubmitResultResponse::_internal_message() const {
  PROTOBUF_TSAN_READ(&_impl_._tsan_detect_race);
  return _impl_.message_.Get();
}
inline void SubmitResultResponse::_internal_set_message(const std::string& value) {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  ;
  _impl_.message_.Set(value, GetArena());
}
inline std::string* SubmitResultResponse::_internal_mutable_message() {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  ;
  return _impl_.message_.Mutable( GetArena());
}
inline std::string* SubmitResultResponse::release_message() {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  // @@protoc_insertion_point(field_release:vision.raw.v1.SubmitResultResponse.message)
  return _impl_.message_.Release();
}
inline void SubmitResultResponse::set_allocated_message(std::string* value) {
  PROTOBUF_TSAN_WRITE(&_impl_._tsan_detect_race);
  _impl_.message_.SetAllocated(value, GetArena());
  #ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
        if (_impl_.message_.IsDefault()) {
          _impl_.message_.Set("", GetArena());
        }
  #endif  // PROTOBUF_FORCE_COPY_DEFAULT_STRING
  // @@protoc_insertion_point(field_set_allocated:vision.raw.v1.SubmitResultResponse.message)
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif  // __GNUC__

// @@protoc_insertion_point(namespace_scope)
}  // namespace v1
}  // namespace raw
}  // namespace vision


// @@protoc_insertion_point(global_scope)

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_INCLUDED_ksl_5fsentence_5frecognition_2eproto_2epb_2eh
```

## File: ksl_sentence_recognition.proto
```protobuf
syntax = "proto3";
package vision.raw.v1;

service SequenceService {
  rpc SendFrames (stream Frame) returns (SubmitResultResponse);
}

message Point3 {
  float x = 1;
  float y = 2;
  float z = 3;
}

message Frame {
  string session_id = 1;
  int32 index = 2;
  int32 flag = 3; // 0: START, 1: NORMAL, 2: END
  int32 width = 4;
  int32 height = 5;
  int32 type = 6;
  bytes data = 7;
  repeated Point3 pose_points = 8;
}

message SubmitResultResponse {
  string session_id = 1;
  int32 frame_count = 2;
  string message = 3;
}
```

## File: pch.cpp
```cpp
// pch.cpp: 미리 컴파일된 헤더에 해당하는 소스 파일

#include "pch.h"

// 미리 컴파일된 헤더를 사용하는 경우 컴파일이 성공하려면 이 소스 파일이 필요합니다.
```

## File: pch.h
```
// pch.h: 미리 컴파일된 헤더 파일입니다.
// 아래 나열된 파일은 한 번만 컴파일되었으며, 향후 빌드에 대한 빌드 성능을 향상합니다.
// 코드 컴파일 및 여러 코드 검색 기능을 포함하여 IntelliSense 성능에도 영향을 미칩니다.
// 그러나 여기에 나열된 파일은 빌드 간 업데이트되는 경우 모두 다시 컴파일됩니다.
// 여기에 자주 업데이트할 파일을 추가하지 마세요. 그러면 성능이 저하됩니다.

#ifndef PCH_H
#define PCH_H


// 여기에 미리 컴파일하려는 헤더 추가
#include "framework.h"

#endif //PCH_H
```

## File: proto_data_structure.md
```markdown
**`ksl_sentence_recognition.proto` 파일에 정의된 데이터 구조 및 역할:**

1.  **`Point3` 메시지**
    *   **구조**: `float x`, `float y`, `float z` 세 개의 부동 소수점 필드를 가집니다.
    *   **역할**: 3차원 공간에서의 한 지점(예: MediaPipe로 추출된 스켈레톤 관절의 3D 좌표)을 표현하는 데 사용됩니다.

2.  **`Frame` 메시지**
    *   **구조**:
        *   `string session_id`: 현재 비디오 처리 세션을 식별하는 문자열 ID.
        *   `int32 index`: 비디오 시퀀스 내에서 현재 프레임의 순서 번호.
        *   `int32 flag`: 프레임의 특정 상태나 목적을 나타내는 정수 플래그 (예: 0=시작, 1=일반, 2=종료).
        *   `int32 width`: 프레임 이미지의 가로 픽셀 수.
        *   `int32 height`: 프레임 이미지의 세로 픽셀 수.
        *   `int32 type`: OpenCV `cv::Mat` 객체의 데이터 타입 (예: `CV_8UC3`은 8비트 unsigned char 3채널 이미지를 의미).
        *   `bytes data`: 실제 프레임 이미지의 바이너리 데이터 (압축되지 않은 픽셀 데이터).
        *   `repeated Point3 pose_points`: 이 프레임에서 감지된 여러 `Point3` 객체들의 목록. 주로 MediaPipe와 같은 AI 모델이 추출한 인체 관절의 3D 좌표를 담습니다.
    *   **역할**: 클라이언트가 gRPC 서버로 전송하는 단일 비디오 프레임에 대한 모든 관련 정보를 캡슐화합니다. 이미지 데이터 자체와 함께, 해당 프레임의 메타데이터(세션 ID, 인덱스, 플래그, 크기, 타입) 및 AI 분석 결과(포즈 포인트)를 포함하여 서버가 수어 인식을 수행하는 데 필요한 모든 컨텍스트를 제공합니다.

3.  **`SubmitResultResponse` 메시지**
    *   **구조**:
        *   `string session_id`: 요청 시 클라이언트가 보낸 세션 ID와 동일한 ID.
        *   `int32 frame_count`: 서버가 처리한 총 프레임 수.
        *   `string message`: 서버가 인식한 최종 수어 문장 텍스트 결과.
    *   **역할**: gRPC 서버가 클라이언트에게 수어 인식 처리 결과를 반환할 때 사용하는 메시지입니다. 어떤 세션에 대한 응답인지, 몇 개의 프레임이 처리되었는지, 그리고 가장 중요한 인식된 수어 문장이 무엇인지를 클라이언트에게 전달합니다.
```

## File: README.md
```markdown
# ksl
```

## File: resource.h
```
//{{NO_DEPENDENCIES}}
// Microsoft Visual C++에서 생성한 포함 파일입니다.
// gRPCFileClient.rc에서 사용되고 있습니다.
//
#define IDM_ABOUTBOX                    0x0010
#define IDD_ABOUTBOX                    100
#define IDS_ABOUTBOX                    101
#define IDD_GRPCFILECLIENT_DIALOG       102
#define IDR_MAINFRAME                   128
#define IDC_BUTTON1                     1000
#define IDC_EDIT1                       1001
#define IDC_EDIT2                       1002
#define IDC_BUTTON2                     1003
#define IDC_BUTTON3                     1004
#define IDC_BUTTON4                     1005
#define IDC_BUTTON5                     1006
#define IDC_BUTTON10                    1007
#define IDC_BUTTON7                     1008
#define IDC_BUTTON8                     1009
#define IDC_CCTV                        1010
#define IDC_ROICCTV                     1011
#define IDC_BUTTON9                     1012
#define IDC_EDIT6                       1013
#define IDC_EDIT4                       1014
#define IDC_EDIT5                       1015
#define IDC_EDIT3                       1016

// Next default values for new objects
// 
#ifdef APSTUDIO_INVOKED
#ifndef APSTUDIO_READONLY_SYMBOLS
#define _APS_NEXT_RESOURCE_VALUE        129
#define _APS_NEXT_COMMAND_VALUE         32771
#define _APS_NEXT_CONTROL_VALUE         1017
#define _APS_NEXT_SYMED_VALUE           101
#endif
#endif
```

## File: targetver.h
```
#pragma once

// SDKDDKVer.h를 포함하면 최고 수준의 가용성을 가진 Windows 플랫폼이 정의됩니다.

// 이전 Windows 플랫폼에 대해 애플리케이션을 빌드하려는 경우에는 SDKDDKVer.h를 포함하기 전에
// WinSDKVer.h를 포함하고 _WIN32_WINNT 매크로를 지원하려는 플랫폼으로 설정하십시오.

#include <SDKDDKVer.h>
```

## File: VideoUtil-v1.1.cpp
```cpp
#include "pch.h"
#include "VideoUtil-v1.1.h"

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <opencv2/opencv.hpp>
#include <algorithm>

using namespace cv;
using namespace std;
using std::tuple;



CVideoUtil::CVideoUtil()
{
}


CVideoUtil::~CVideoUtil()
{

}


BITMAPINFO* CVideoUtil::MakeBMPHeader(int width, int height)
{
	BITMAPINFO* pBmp;

	pBmp = (BITMAPINFO*)new BYTE[sizeof(BITMAPINFO)];
	pBmp->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	pBmp->bmiHeader.biWidth = width;
	pBmp->bmiHeader.biHeight = -height;
	pBmp->bmiHeader.biPlanes = 1;
	pBmp->bmiHeader.biBitCount = 24;
	pBmp->bmiHeader.biCompression = BI_RGB;
	pBmp->bmiHeader.biSizeImage = 0;
	pBmp->bmiHeader.biXPelsPerMeter = 0;
	pBmp->bmiHeader.biYPelsPerMeter = 0;
	pBmp->bmiHeader.biClrUsed = 0;
	pBmp->bmiHeader.biClrImportant = 0;

	return pBmp;
}


BITMAPINFO* CVideoUtil::MakeBMPHeader(int width, int height, int channel)
{
	BITMAPINFO* pBmp;

	pBmp = (BITMAPINFO*)new BYTE[sizeof(BITMAPINFO)];
	pBmp->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	pBmp->bmiHeader.biWidth = width;
	pBmp->bmiHeader.biHeight = -height;
	pBmp->bmiHeader.biPlanes = 1;
	pBmp->bmiHeader.biBitCount = channel * 8;
	if (channel == 1) pBmp->bmiHeader.biCompression = BI_RGB;
	else pBmp->bmiHeader.biCompression = BI_RGB;
	pBmp->bmiHeader.biSizeImage = 0;
	pBmp->bmiHeader.biXPelsPerMeter = 0;
	pBmp->bmiHeader.biYPelsPerMeter = 0;
	pBmp->bmiHeader.biClrUsed = 0;
	pBmp->bmiHeader.biClrImportant = 0;

	return pBmp;
}




vector<string> CVideoUtil::SplitString(string str)
{
	istringstream ss(str);
	string stringBuffer;
	vector<string> x;
	x.clear();
	
	while (getline(ss, stringBuffer, ' ')) {
		x.push_back(stringBuffer);		
	}
	return x;
}

int CVideoUtil::GetKoreaCharNum(CString str)
{
	int len = str.GetLength();
	int cnt = 0;
	for (int k = 0; k < len; k++)
	{
		if (str.GetAt(k) <= 0 || str.GetAt(k) < 127) cnt++;
	}
	return len - cnt;
}


void CVideoUtil::DrawImageBMPwKeepRationFast(CWnd* wnd, Mat frame, int x, int y, int width, int height)
{
	//if (frame.empty()) return;

	//int src_width = frame.cols * height / frame.rows;

	//int white_width = (width - src_width) / 2 + 1;
	//Mat white_img = Mat::zeros(height, white_width, frame.type()); //컬러 행렬 모든 픽셀 초기 색, red(OpenCV 인자는 GBR순서입니다.)

	//DrawImageBMP(wnd, white_img, x, y, white_width, height);
	//DrawImageBMP(wnd, white_img, x + width - white_width, y, white_width, height);
	//DrawImageBMP(wnd, frame, x + white_width, y, src_width, height);

	if (frame.empty()) return;	

	Mat white_img(height, width, CV_8UC3, Scalar(255, 255, 255)); //컬러 행렬 모든 픽셀 초기 색, red(OpenCV 인자는 GBR순서입니다.)
	Mat src_resize_img;
	int src_width = frame.cols* height / frame.rows;
	resize(frame, src_resize_img, Size(src_width, height));
	
	src_resize_img.copyTo(white_img(Range(0, height), Range(width/2-src_width /2, width/2 + src_width / 2)));

	DrawImageBMP(wnd, white_img, x, y, width, height);
}
//
//void CVideoUtil::DrawImageBMPwKeepRatio(CWnd* wnd, Mat frame, int x, int y, int width, int height)
//{
//	if (frame.empty()) return;
//
//	double src_ratio = (double)frame.rows / (double)frame.cols;
//	double out_ratio = (double)height / (double)width;
//
//	int blank_width;
//	if (src_ratio > out_ratio) blank_width = width - (double)height / (double)src_ratio;
//	else blank_width = 0;
//
//	if (blank_width != 0)
//	{
//		Mat white_img(height, width, CV_8UC3, Scalar(255, 255, 255)); //컬러 행렬 모든 픽셀 초기 색, red(OpenCV 인자는 GBR순서입니다.)
//		Mat re_img;
//		resize(white_img, re_img, Size(width, height));
//		CImage m_img;
//		Mat2CImage(&re_img, m_img);
//		CClientDC dc(wnd);
//		m_img.Draw(dc, x, y);
//	}
//
//	Mat re_img;
//	resize(frame, re_img, Size(width - blank_width, height));
//	CImage m_img;
//	Mat2CImage(&re_img, m_img);
//	CClientDC dc(wnd);
//	m_img.Draw(dc, x + blank_width / 2, y);
//}


void CVideoUtil::DrawImageBMP(CDC& dc, Mat frame, int x, int y, int width, int height)
{
	if (frame.empty()) return;

	// 항상 32bpp BGRA로 통일(실패/깜빡임 방지)
	Mat img;
	switch (frame.type())
	{
	case CV_8UC4: img = frame; break;
	case CV_8UC3: cvtColor(frame, img, COLOR_BGR2BGRA); break;
	case CV_8UC1: cvtColor(frame, img, COLOR_GRAY2BGRA); break;
	default: return;
	}

	// 힙 할당 대신 스택 BITMAPINFO 사용 + 상하반전 방지(음수 높이)
	BITMAPINFO bmi = {};
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = img.cols;
	bmi.bmiHeader.biHeight = -img.rows;       // top-down
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;

	dc.RealizePalette();
	::SetStretchBltMode(dc.GetSafeHdc(), COLORONCOLOR);

	::StretchDIBits(dc.GetSafeHdc(),
		x, y, width, height,
		0, 0, img.cols, img.rows,
		img.data, &bmi, DIB_RGB_COLORS, SRCCOPY);

	::GdiFlush(); // 즉시 플러시(선택)
}


void CVideoUtil::DrawImageBMP(CWnd* wnd, Mat frame, int x, int y, int width, int height)
{
	if (!wnd) return;
	CClientDC dc(wnd);
	DrawImageBMP(dc, frame, x, y, width, height); // 내부 공용 구현 호출

}

void CVideoUtil::DrawImageBMP(CWnd* wnd, Mat frame, int x, int y)
{
	auto img = frame.clone();

	if (!img.empty())
	{
		CRect frame_rect;
		wnd->GetWindowRect(frame_rect);

		DrawImageBMP(wnd, img, x, y, frame_rect.Width(), frame_rect.Height());
	}
}


void CVideoUtil::DrawImageBMP(CWnd* wnd, Mat frame, int x, int y, double time_line_percent)
{
	auto img = frame.clone();

	if (!img.empty())
	{
		CRect frame_rect;
		wnd->GetWindowRect(frame_rect);

		rectangle(img, Rect(5, img.rows-15, img.cols-10, 10), Scalar(255, 255, 255), FILLED);
		int width = (img.cols - 10) * time_line_percent;
		rectangle(img, Rect(5, img.rows - 15+2, width, 6), Scalar(0, 255, 0), FILLED);

		DrawImageBMP(wnd, img, x, y, frame_rect.Width(), frame_rect.Height());
	}	
}

void CVideoUtil::DrawImageBMP(CWnd* wnd, Mat frame, int x, int y, double time_line_percent, int frame_index)
{
	auto img = frame.clone();

	if (!img.empty())
	{
		CRect frame_rect;
		wnd->GetWindowRect(frame_rect);

		rectangle(img, Rect(5, img.rows - 15, img.cols - 10, 10), Scalar(255, 255, 255), FILLED);
		int width = (img.cols - 10) * time_line_percent;
		rectangle(img, Rect(5, img.rows - 15 + 2, width, 6), Scalar(0, 255, 0), FILLED);

		DrawImageBMP(wnd, img, x, y, frame_rect.Width(), frame_rect.Height());
		CString str;
		str.Format(_T("%d"), frame_index);	
		DrawText_(wnd, 0, frame_rect.Height()-40, 10, str, RGB(255, 255, 0), 0, CRect(0, 0, frame_rect.Width(), frame_rect.Height()));
	}
}


void CVideoUtil::DrawImageBMP(CWnd* wnd, Mat frame, int x, int y, int width, int height, CRect rec)
{
	if (frame.empty()) return;

	CClientDC dc(wnd);
	CRgn rgn;
	rgn.CreateRectRgnIndirect(rec);
	dc.SelectClipRgn(&rgn);

	if (frame.channels() != 1)
	{
		//if (frame.channels() == 3) 
		//	cvtColor(frame, frame, COLOR_BGR2BGRA);
		BITMAPINFO* pBmp = MakeBMPHeader(frame.cols, frame.rows, frame.channels());
		dc.RealizePalette();
		SetStretchBltMode(dc.GetSafeHdc(), COLORONCOLOR);
		StretchDIBits(dc.GetSafeHdc(), x, y, width, height, 0, 0, frame.cols, frame.rows, frame.data, pBmp, DIB_RGB_COLORS, SRCCOPY);
		dc.SelectClipRgn(NULL);
		delete[] pBmp;
	}
	else
	{
		BITMAPINFO* pBmp = MakeBMPHeader(frame.cols, frame.rows, frame.channels());
		Mat cimg;
		cvtColor(frame, cimg, COLOR_GRAY2RGB);
		dc.RealizePalette();
		SetStretchBltMode(dc.GetSafeHdc(), COLORONCOLOR);
		StretchDIBits(dc.GetSafeHdc(), x, y, width, height, 0, 0, cimg.cols, cimg.rows, cimg.data, pBmp, DIB_RGB_COLORS, SRCCOPY);
		dc.SelectClipRgn(NULL);
		delete[] pBmp;
	}

}

void CVideoUtil::CopyImg(Mat src, Mat& dst, int x, int  y) // partial src -> all area of dst (src>dst)
{
	Mat roi = src(Rect(x, y, dst.cols, dst.rows));
	dst.copyTo(roi);
}

void CVideoUtil::CopyImg2(Mat src, Mat& dst, int x, int  y) // all area of src -> partial dst (src<dst)
{
	if ((y + src.rows) <= dst.rows && (x + src.cols) <= dst.cols)
	{
		Mat roi = dst(Rect(x, y, src.cols, src.rows));
		src.copyTo(roi);
	}
}

void CVideoUtil::CopyImg3(Mat src, Mat& dst, int dst_x, int  dst_y, int width, int height) // all area of src -> partial dst (src<dst)
{
	if ((dst_y + height) <= dst.rows && (dst_x + width) <= dst.cols)
	{
		Mat resize_img;
		resize(src, resize_img, Size(width, height), INTER_CUBIC);

		CopyImg2(resize_img, dst, dst_x, dst_y);	
	}
}


void CVideoUtil::DrawScalarImg(CWnd* wnd, Scalar color, int x, int y, int width, int height)
{

	Mat frame(height, width, CV_8UC3, color);

	DrawImageBMP(wnd, frame, x, y, width, height);
}


void CVideoUtil::DrawImgsList(CWnd* wnd, int x_num, int y_num, vector<Mat> imgs, int ptr_index)
{

	CRect frame_rect;
	wnd->GetWindowRect(frame_rect);

	int width_num = x_num;
	int height_num = y_num;

	int width = frame_rect.Width() / width_num;
	int height = frame_rect.Height() / height_num;

	Mat white_img(frame_rect.Height(), frame_rect.Width(), CV_8UC3, Scalar(240, 240, 240)); //컬러 행렬 모든 픽셀 초기 색, red(OpenCV 인자는 GBR순서입니다.)
	

	for (int k = 0;k < imgs.size();k++)
	{
		int col = k % x_num;
		int row = k / x_num;

		if (k < x_num * y_num)
		{
			CopyImg3(imgs[k], white_img, width * col, height * row, width, height);			
		}
	}	
	DrawImageBMP(wnd, white_img, 0, 0, frame_rect.Width(), frame_rect.Height());

	if (ptr_index >= 0 && ptr_index < imgs.size())
	{
		int col = ptr_index % x_num;
		int row = ptr_index / x_num;

		DrawRect(wnd, CRect(width * col + 1, height * row + 1, width * (col + 1) - 1, height * (row + 1) - 1), RGB(255, 0, 0));
	}

	
}

void CVideoUtil::DrawImgsList(CWnd* wnd, int x_num, int y_num, vector<Mat> imgs, int ptr_index, vector<CString> labels)
{

	CRect frame_rect;
	wnd->GetWindowRect(frame_rect);

	int width_num = x_num;
	int height_num = y_num;

	int width = frame_rect.Width() / width_num;
	int height = frame_rect.Height() / height_num;

	Mat white_img(frame_rect.Height(), frame_rect.Width(), CV_8UC3, Scalar(240, 240, 240)); //컬러 행렬 모든 픽셀 초기 색, red(OpenCV 인자는 GBR순서입니다.)


	for (int k = 0; k < imgs.size(); k++)
	{
		int col = k % x_num;
		int row = k / x_num;

		if (k < x_num * y_num)
		{
			CopyImg3(imgs[k], white_img, width * col, height * row, width, height);
		}
	}
	DrawImageBMP(wnd, white_img, 0, 0, frame_rect.Width(), frame_rect.Height());

	for (int k = 0; k < labels.size(); k++)
	{
		int col = k % x_num;
		int row = k / x_num;
		DrawText_(wnd, col*width+4, row*height+4, 8, labels[k], RGB(0, 0, 0), 1, CRect(0, 0, frame_rect.Width(), frame_rect.Height()));
	}

	if (ptr_index >= 0 && ptr_index < imgs.size())
	{
		int col = ptr_index % x_num;
		int row = ptr_index / x_num;

		DrawRect(wnd, CRect(width * col + 1, height * row + 1, width * (col + 1) - 1, height * (row + 1) - 1), RGB(255, 0, 0));
	}
}

void CVideoUtil::DrawIndexList(CWnd* wnd, int x_num, int y_num, vector<int> index)
{
	CRect frame_rect;
	wnd->GetWindowRect(frame_rect);

	int width_num = x_num;
	int height_num = y_num;

	int width = frame_rect.Width() / width_num;
	int height = frame_rect.Height() / height_num;

	for (int k = 0;k < index.size();k++)
	{
		int col = k % x_num;
		int row = k / x_num;

		if (k < x_num * y_num)
		{
			CString str;
			str.Format(_T("%d"), index[k]);
			DrawText_(wnd, width * col,height * row, 80, str, RGB(255, 255, 255), 0, 0, CRect(0, 0, frame_rect.Width(), frame_rect.Height()));
		}		
	}
}


CString CVideoUtil::Vector2CString(vector<float> dat)
{
	CString str;
	for (int k=0;k< dat.size(); k++)
	{
		CString add_str;
		if( k== (dat.size()-1)) add_str.Format(_T("%.6f"), dat[k]);
		else add_str.Format(_T("%.6f "), dat[k]);
		str.Append(add_str);
	}
	return str;
}

CString CVideoUtil::Vector2CString(vector<UINT32> dat)
{
	CString str;
	for (int k = 0;k < dat.size(); k++)
	{
		CString add_str;
		if (k == (dat.size() - 1)) add_str.Format(_T("%d"), dat[k]);
		else add_str.Format(_T("%d "), dat[k]);
		str.Append(add_str);
	}
	return str;
}

CString CVideoUtil::Vector2CString(vector<Point3f> dat)
{
	CString str;
	for (int k = 0; k < dat.size(); k++)
	{
		CString add_str;


		add_str.Format(_T("%.6f "), dat[k].x);
		str.Append(add_str);
		add_str.Format(_T("%.6f "), dat[k].y);
		str.Append(add_str);

		if (k == (dat.size() - 1)) add_str.Format(_T("%.6f"), dat[k].z);
		else add_str.Format(_T("%.6f "), dat[k].z);		
		str.Append(add_str);
	}
	return str;
}


void CVideoUtil::DrawImageTransparentBMP(CWnd* wnd, Mat frame, int x, int y, int width, int height)
{
	if (frame.empty()) return;

	BITMAPINFO* pBmp = MakeBMPHeader(frame.cols, frame.rows, frame.channels());

	CClientDC dc(wnd);

	HDC hdc = dc.GetSafeHdc();

	HDC hMemDC;
	HBITMAP hImage, hOldBitmap;

	hMemDC = CreateCompatibleDC(hdc); // 메모리 DC를 만든다

	BITMAPINFOHEADER   bmih;
	memset(&bmih, 0, sizeof(BITMAPINFOHEADER));

	bmih.biWidth = frame.cols;
	bmih.biHeight = -frame.rows;
	bmih.biBitCount = 24;
	bmih.biCompression = BI_RGB;
	bmih.biSize = sizeof(BITMAPINFOHEADER);
	bmih.biPlanes = 1;

	BITMAPINFO* bmi = (BITMAPINFO*)&bmih;

	hImage = CreateDIBitmap(hdc, &bmih, CBM_INIT, frame.data, bmi, DIB_RGB_COLORS);

	hOldBitmap = (HBITMAP)SelectObject(hMemDC, hImage);

	TransparentBlt(hdc, x, y, width, height, hMemDC, 0, 0, frame.cols, frame.rows, RGB(0, 0, 0));

	SelectObject(hMemDC, hOldBitmap);
	DeleteObject(hImage); // 선택 해제된 비트맵을 제거한다
	DeleteDC(hMemDC); // 메모리 DC를 제거한다

	delete[] pBmp;
}

void CVideoUtil::DrawText_Hangul(CWnd* wnd,
	int x, int y,
	int ptSize,            // 포인트 단위 글꼴 크기
	const CString& txt,
	COLORREF textColor,
	int type,              // 0: 투명, 1: 불투명+테두리
	int mode,              // 0: 자동 줄바꿈, 1: 단일 줄
	const CRect& win_rec)  // 클리핑/최대 영역
{
	if (!wnd || !::IsWindow(wnd->GetSafeHwnd()) || txt.IsEmpty())
		return;

	CClientDC dc(wnd);
	const int saved = dc.SaveDC(); // 상태 복구를 위해 저장

	// ----- 클리핑 영역 -----
	CRgn rgn;
	rgn.CreateRectRgnIndirect(win_rec);
	dc.SelectClipRgn(&rgn);

	// ----- 글꼴 생성 (pt → 논리픽셀 변환) -----
	LOGFONT lf = {};
	lf.lfWeight = FW_BOLD;
	lf.lfCharSet = DEFAULT_CHARSET;
	// 음수 높이는 문자 높이를 의미 (포인트 → 픽셀 변환)
	int dpiY = dc.GetDeviceCaps(LOGPIXELSY);
	lf.lfHeight = -MulDiv(ptSize, dpiY, 72);

	CFont font;
	font.CreateFontIndirect(&lf);
	CFont* pOldFont = dc.SelectObject(&font);

	// ----- 색상/배경 -----
	dc.SetTextColor(textColor);
	int oldBkMode = TRANSPARENT;
	COLORREF oldBkColor = 0;
	if (type == 0) {
		oldBkMode = dc.SetBkMode(TRANSPARENT);
	}
	else {
		oldBkColor = dc.SetBkColor(RGB(255, 255, 255));
		oldBkMode = dc.SetBkMode(OPAQUE);
	}

	// ----- 서식 플래그 -----
	UINT fmt = DT_LEFT | DT_TOP | DT_NOPREFIX;
	if (mode == 0) {
		fmt |= DT_WORDBREAK;               // 자동 줄바꿈
	}
	else {
		fmt |= DT_SINGLELINE | DT_END_ELLIPSIS; // 단일줄, 길면 말줄임표
	}

	// ----- 그릴 영역 계산 -----
	// 시작점 (x,y)에서 win_rec의 우/하 경계까지 허용
	CRect rcDraw(x, y, win_rec.right, win_rec.bottom);

	// 먼저 크기 계산
	CRect rcCalc = rcDraw;
	dc.DrawText(txt, rcCalc, fmt | DT_CALCRECT);

	// 단일줄 모드일 경우, 너무 긴 텍스트라면 우측 경계를 win_rec로 제한
	if (mode == 1) {
		if (rcCalc.right > win_rec.right)
			rcCalc.right = win_rec.right;
		if (rcCalc.bottom > win_rec.bottom)
			rcCalc.bottom = y + (rcCalc.Height()); // 높이는 한 줄 높이 유지
	}
	else {
		// 워드브레이크 모드에서는 계산된 너비가 클 수 있으니, win_rec로 제한
		if (rcCalc.right > win_rec.right) rcCalc.right = win_rec.right;
		if (rcCalc.bottom > win_rec.bottom) rcCalc.bottom = win_rec.bottom;
	}

	// ----- 실제 출력 -----
	dc.DrawText(txt, rcCalc, fmt);

	// ----- 테두리(옵션) -----
	if (type == 1) {
		dc.Draw3dRect(rcCalc, ::GetSysColor(COLOR_3DLIGHT), ::GetSysColor(COLOR_3DSHADOW));
	}

	// ----- 상태 복구 -----
	dc.SetBkMode(oldBkMode);
	if (type == 1) dc.SetBkColor(oldBkColor);
	dc.SelectObject(pOldFont);
	dc.SelectClipRgn(nullptr);
	dc.RestoreDC(saved);
}


void CVideoUtil::DrawText_(CWnd* wnd, int x, int y, int size, CString txt, COLORREF color, int type, int mode, CRect win_rec) // type : 0 배경 투명, mode : 0 자동 줄바꿈
{
	
	CClientDC dc(wnd);
	CRect rec;
	
	
	CRgn rgn;
	rgn.CreateRectRgnIndirect(win_rec);	

	LOGFONT lf;
	::ZeroMemory(&lf, sizeof(lf));
	lf.lfHeight = size;	
	lf.lfWeight = DWRITE_FONT_WEIGHT_BOLD;
	CFont          NewFont;
	NewFont.CreatePointFontIndirect(&lf);
	CFont* pOldFont = dc.SelectObject(&NewFont);

	dc.SetTextColor(color);
	
	if (type == 0) dc.SetBkMode(TRANSPARENT);
	else dc.SetBkColor(RGB(255, 255, 255));


	dc.SelectClipRgn(&rgn);

	int len = txt.GetLength();
	int korea_len = GetKoreaCharNum(txt);

	len = len + korea_len;

	if (mode == 1)
	{
		if (len <= 10)
		{
			rec.left = x;
			rec.top = y;
			rec.right = size * (len)+rec.left;
			rec.bottom = size + rec.top;
			dc.DrawText(txt, rec, DT_SINGLELINE);
		}
		else
		{
			CString txt1;
			for (int idx = 0; idx < len; idx += 10)
			{
				txt1 = txt.Mid(idx, 10);
				rec.left = x;
				rec.top = y + idx / 10 * (size + 1);
				rec.right = size * (10) + rec.left;
				rec.bottom = size + 3 + rec.top;
				dc.DrawText(txt1, rec, DT_SINGLELINE);
			}
		}
	}
	else
	{
		rec.left = x;
		rec.top = y;
		rec.right = size * (len)+rec.left;
		rec.bottom = size + rec.top;
		dc.DrawText(txt, rec, DT_SINGLELINE);
	}

	if (type == 1)
	{
		rec.right = rec.left + rec.Width() / 2 - 2;
		dc.Draw3dRect(rec, ::GetSysColor(COLOR_3DLIGHT), ::GetSysColor(COLOR_3DSHADOW));
	}
	

	dc.SelectClipRgn(NULL);
	dc.SelectObject(pOldFont);
	
}

void CVideoUtil::DrawText_(CWnd* wnd, int x, int y, int size, CString txt, COLORREF color, int type, CRect win_rec)
{
	CClientDC dc(wnd);

	// --- 0) 클리핑 영역 ---
	CRgn rgn;
	rgn.CreateRectRgnIndirect(win_rec);
	dc.SelectClipRgn(&rgn);

	// --- 1) 폰트 생성: DPI 반영 (size = pt) ---
	const int dpi = dc.GetDeviceCaps(LOGPIXELSY);
	LOGFONT lf = {};
	lf.lfHeight = -MulDiv(size, dpi, 72); // 포인트 → 논리픽셀
	lf.lfWeight = FW_BOLD;                // DWRITE_* 대신 GDI 값 사용
	_tcscpy_s(lf.lfFaceName, LF_FACESIZE, _T("Segoe UI")); // 원하시는 폰트로 변경 가능
	CFont font; font.CreateFontIndirect(&lf);
	CFont* pOldFont = dc.SelectObject(&font);

	// --- 2) 텍스트 크기 계산 ---
	CRect calc = CRect(0, 0, 0, 0);
	// 사전 계산용: DT_CALCRECT는 x,y 무시 → (0,0) 기준으로 크기 산출
	dc.DrawText(txt, &calc, DT_CALCRECT | DT_SINGLELINE);

	// 패딩(여백)
	const int padX = MulDiv(size, dpi, 72) / 2;   // 글자 크기 절반 정도 가로 여백
	const int padY = MulDiv(size, dpi, 72) / 4;   // 글자 크기 1/4 세로 여백

	CRect rec;
	rec.left = x;
	rec.top = y;
	rec.right = x + calc.Width() + padX * 2;
	rec.bottom = y + calc.Height() + padY * 2;

	// --- 3) 배경/텍스트 색 세팅 ---
	dc.SetBkMode(TRANSPARENT);

	if (type == 1) {
		// color = 배경(사각형) 색
		dc.FillSolidRect(&rec, color);

		// 배경 대비 텍스트 색 자동 결정(간단한 명도 기준)
		auto luminance = [](COLORREF c) {
			double r = GetRValue(c), g = GetGValue(c), b = GetBValue(c);
			return 0.2126 * r + 0.7152 * g + 0.0722 * b; // sRGB 가중치
			};
		COLORREF textCol = (luminance(color) < 128.0) ? RGB(255, 255, 255) : RGB(0, 0, 0);
		dc.SetTextColor(textCol);

		// 테두리(옵션) 원하시면 주석 해제
		// CPen pen(PS_SOLID, 1, RGB(0,0,0));
		// CPen* oldPen = dc.SelectObject(&pen);
		// dc.SelectStockObject(NULL_BRUSH);
		// dc.Rectangle(rec);
		// dc.SelectObject(oldPen);

	}
	else {
		// type == 0 : 텍스트만, color = 텍스트 색
		dc.SetTextColor(color);
	}

	// --- 4) 텍스트 그리기 (패딩 고려해서 정렬) ---
	CRect textRc = rec;
	textRc.DeflateRect(padX, padY);
	dc.DrawText(txt, &textRc, DT_SINGLELINE | DT_LEFT | DT_VCENTER);

	// 정중앙 정렬 원하면 위 한 줄을 아래로 바꾸세요:
	// dc.DrawText(txt, &rec, DT_SINGLELINE | DT_CENTER | DT_VCENTER);

	// --- 5) 정리 ---
	dc.SelectClipRgn(NULL);
	dc.SelectObject(pOldFont);
}

vector<Point3f> CVideoUtil::DevideByN(vector<Point3f> pt, double N)
{
	vector<Point3f> ret;
	for (auto p : pt) ret.push_back(p / N);
	return ret;
}




CString CVideoUtil::GetFilePrefix(CString src)
{
	CString prefix;

	int len = src.GetLength();

	int dot_pos = src.ReverseFind('.');
	int dir_pos = src.ReverseFind('\\');
	if (dot_pos == -1 || dir_pos > dot_pos) prefix.Empty();
	else if (dir_pos == -1)
	{
		prefix = src.Mid(0, dot_pos);
	}
	else prefix = src.Mid(dir_pos + 1, dot_pos - dir_pos - 1);

	return prefix;

}

CString CVideoUtil::GetFileExt(CString src)
{
	CString ext;

	int len = src.GetLength();

	int dot_pos = src.ReverseFind('.');	
	if (dot_pos == -1 ) ext.Empty();	
	else ext = src.Mid(dot_pos + 1, len-dot_pos - 1);

	return ext;

}

vector<CString> CVideoUtil::GetAllFile(CString srcDir2)
{
	CFileFind finder;
	CString srcDir;
	CString end_srcDir2 = srcDir2.Right(1);
	if (end_srcDir2.Compare(_T("\\")) == 0) srcDir = srcDir2.Left(srcDir2.GetLength() - 1);
	else srcDir = srcDir2;

	vector<CString> ret;
	
	BOOL bWorking = finder.FindFile(srcDir + "/*.*");
	while (bWorking)
	{
		bWorking = finder.FindNextFile();
		if (finder.IsDots()) continue;
		if (finder.IsDirectory()) continue;

		CString strFileName = finder.GetFileName();
		
		CString tmp = srcDir + _T("\\") + strFileName;
		if(tmp.Find(_T("Thumbs.db"))<0) ret.push_back(tmp);	
	}
	finder.Close();	
	sort(ret.begin(), ret.end());
	return ret;
}


//
//vector<CString> CVideoUtil::GetAllFile(CString srcDir2)
//{
//	vector<CString> ret;
//
//	string path;
//	CString srcDir;
//	CString end_srcDir2 = srcDir2.Right(1);
//	if (end_srcDir2.Compare(_T("\\")) == 0) srcDir = srcDir2.Left(srcDir2.GetLength() - 1);
//	else srcDir = srcDir2;
//	srcDir.Append(_T("\\."));
//	wstring wstr = srcDir.operator LPCWSTR();
//	convert_unicode_to_utf8_string(path, wstr.c_str(), wstr.size());
//	
//	for (auto& p : fs::recursive_directory_iterator(path))
//	{
//		cout << p << endl;							//결과: root/dir1/text.txt
//		cout << p.path().filename() << endl;			//결과: "test.txt"
//		cout << p.path().filename().string() << endl;	//결과: test.txt
//		wstring unicode_2;
//		convert_utf8_to_unicode_string(unicode_2, p.path().filename().string().c_str(), p.path().filename().string().size());
//		ret.push_back(unicode_2.c_str());
//	}
//
//	return ret;
//}

vector<CString> CVideoUtil::GetAllDir(CString srcDir2) // / is attached to the end of file
{
	CFileFind finder;
	CString srcDir;
	CString end_srcDir2 = srcDir2.Right(1);
	if (end_srcDir2.Compare(_T("\\")) == 0) srcDir = srcDir2.Left(srcDir2.GetLength() - 1);
	else srcDir = srcDir2;

	vector<CString> ret;

	BOOL bWorking = finder.FindFile(srcDir + "/*.*");
	while (bWorking)
	{
		bWorking = finder.FindNextFile();
		if (finder.IsDots()) continue;
		if (finder.IsDirectory()) 
		{ 
			ret.push_back(finder.GetFilePath() + _T("\\"));
		}		
	}
	finder.Close();
	return ret;
}




double CVideoUtil::Distance(vector<Point> pts1, vector<Point> pts2)
{
	double dist = 0.0;
	if (pts1.size() != pts2.size()) return -1.0;
	else
	{
		for (int k = 0;k < pts1.size();k++)
			dist += Distance(pts1[k], pts2[k]);
	}
	return dist / pts1.size();
}

double CVideoUtil::Distance(double x1, double y1, double x2, double y2)
{
	return sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
}

double CVideoUtil::Distance(double x1, double y1, double z1, double x2, double y2, double z2)
{
	return sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2) + (z1-z2) * (z1 - z2));
}

double CVideoUtil::Distance(Point3f p1, Point3f p2)
{
	return sqrt((p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y) + (p1.z - p2.z) * (p1.z - p2.z));
}

double CVideoUtil::Distance(Point3f p1)
{
	return sqrt((p1.x ) * (p1.x ) + (p1.y ) * (p1.y ) + (p1.z ) * (p1.z ));
}

double CVideoUtil::Distance(Point2d p1, Point2d p2)
{
	return sqrt((p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y) );
}

double CVideoUtil::Distance(Point2f p1, Point2f p2)
{
	return sqrt((p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y));
}

double CVideoUtil::Distance(Point p1, Point p2)
{
	return sqrt((p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y));
}

double CVideoUtil::Distance(Point2f p1)
{
	return sqrt((p1.x ) * (p1.x ) + (p1.y ) * (p1.y ));
}

double CVideoUtil::Distance(Mat p1, Mat p2)
{
	Mat dif = p2 - p1;

	return norm(dif, NORM_L2);
}



double CVideoUtil::GetAngle(Point3f p1, Point3f p2)
{
	double angle=0;

	Point3f A = p1;
	Point3f B = p2;
	double n1 = A.x * B.x + A.y * B.y + A.z * B.z;
	double n2 = Distance(A) * Distance(B);
	double n = n1 / n2;
	if (n > 1.0)n = 1.0;
	if (n < -1.0)n = -1.0;
	angle = acos(n);

	return angle * 180 / CV_PI;
}

double CVideoUtil::GetAngle(Point2f p1, Point2f p2)
{
	// 벡터 길이
	double d1 = sqrt(p1.x * p1.x + p1.y * p1.y);
	double d2 = sqrt(p2.x * p2.x + p2.y * p2.y);

	// 제로벡터 예외 처리
	if (d1 < 1e-8 || d2 < 1e-8) {
		return 0.0;
	}

	// 내적 (dot)
	double dot = p1.x * p2.x + p1.y * p2.y;
	// 외적 크기 (cross product in 2D)
	double cross = p1.x * p2.y - p1.y * p2.x;

	// 각도 계산
	double angle = atan2(fabs(cross), dot) * 180.0 / CV_PI;
	// atan2(y, x) → 0~180° 보장됨

	return angle;
}

double CVideoUtil::GetAngle(Point2f p1)
{
	if (p1.x == 0 && p1.y == 0) return 0;
	else return atan2(p1.x, p1.y) * 180 / CV_PI; // -180~180
}



double CVideoUtil::GetAngle(Point3f p1, Point3f p2, Point3f p3)
{
	double angle;

	Point3f A = p1 - p2;
	Point3f B = p3 - p2;	

	return GetAngle(A,B);
}

Point3f CVideoUtil::MatToPoint3f(Mat m)
{
	return Point3f(m.at<double>(0, 0), m.at<double>(1, 0), m.at<double>(2, 0));
}

//int CVideoUtil::Mat2CImage(Mat* mat, CImage& img)
//{
//	if (!mat || mat->empty())
//		return -1;
//	int nBPP = mat->channels() * 8;
//	img.Create(mat->cols, mat->rows, nBPP);
//	if (nBPP == 8)
//	{
//		static RGBQUAD pRGB[256];
//		for (int i = 0; i < 256; i++)
//			pRGB[i].rgbBlue = pRGB[i].rgbGreen = pRGB[i].rgbRed = i;
//		img.SetColorTable(0, 256, pRGB);
//	}
//	uchar* psrc = mat->data;
//	uchar* pdst = (uchar*)img.GetBits();
//	int imgPitch = img.GetPitch();
//	for (int y = 0; y < mat->rows; y++)
//	{
//		memcpy(pdst, psrc, mat->cols * mat->channels());//mat->step is incorrect for those images created by roi (sub-images!)
//		psrc += mat->step;
//		pdst += imgPitch;
//	}
//
//	return 0;
//}


void CVideoUtil::DrawLine(CWnd* wnd, CPoint p1, CPoint p2, COLORREF color, int thickness)
{
	CClientDC dc(wnd);
	CPen pen;
	pen.CreatePen(PS_SOLID, thickness, color);


	CPen* oldPen = dc.SelectObject(&pen);

	dc.MoveTo(p1.x, p1.y);
	dc.LineTo(p2.x, p2.y);

	dc.SelectObject(oldPen);

	// 만약 빨간색으로 그린 후 파란색으로 그려야 한다면, 다시 새로운 펜을 만들고 그려줘야 한다.
	// 펜 굵기가 10인 파란색 실선을 그림다.
	pen.DeleteObject();

}

void CVideoUtil::DrawLine(CWnd* wnd, CPoint p1, CPoint p2, COLORREF color, int thickness, int line_type)
{
	CClientDC dc(wnd);
	CPen pen;
	pen.CreatePen(line_type, thickness, color);


	CPen* oldPen = dc.SelectObject(&pen);

	dc.MoveTo(p1.x, p1.y);
	dc.LineTo(p2.x, p2.y);

	dc.SelectObject(oldPen);

	// 만약 빨간색으로 그린 후 파란색으로 그려야 한다면, 다시 새로운 펜을 만들고 그려줘야 한다.
	// 펜 굵기가 10인 파란색 실선을 그림다.
	pen.DeleteObject();

}


void CVideoUtil::DrawRect(CWnd* wnd, CRect rect, COLORREF color)
{
	if (!wnd || !::IsWindow(wnd->GetSafeHwnd())) return;

	const int thickness = 3;

	CClientDC dc(wnd);

	// 클라이언트 영역으로 클리핑
	CRect clientRect;
	wnd->GetClientRect(&clientRect);
	dc.IntersectClipRect(&clientRect);

	// 펜 준비
	CPen pen;
	pen.CreatePen(PS_SOLID, thickness, color);
	CPen* pOldPen = dc.SelectObject(&pen);

	// 외곽선만 그릴 것이므로 속이 비는 스톡 브러시 선택 (올바른 방법)
	CBrush* pOldBrush = (CBrush*)dc.SelectStockObject(HOLLOW_BRUSH); // == NULL_BRUSH

	// 사각형 그리기
	dc.Rectangle(rect);

	// 원복
	dc.SelectObject(pOldPen);
	dc.SelectObject(pOldBrush);

	pen.DeleteObject(); // 스톡 브러시는 Delete 금지 (우린 선택만 했음)
}


// 내부 사각형만 채우기(외곽선 없음) + 클라이언트 밖으로 못 나가게
void CVideoUtil::DrawFillRect(CWnd* wnd, const CRect& rect, COLORREF color)
{
	if (!wnd) return;

	CClientDC dc(wnd);

	// 클라이언트 영역으로 클리핑
	CRect client;
	wnd->GetClientRect(&client);
	dc.IntersectClipRect(&client);

	// 클라이언트와의 교집합만 칠하고 싶으면 주석 해제
	// CRect r = rect; r.IntersectRect(rect, client);
	// dc.FillSolidRect(&r, color);

	// 클리핑이 걸렸으니 그냥 rect로 칠해도 바깥은 안 칠해짐
	dc.FillSolidRect(&rect, color);
}

// 내부 원(원형)만 채우기(외곽선 없음) + 클라이언트 밖으로 못 나가게
void CVideoUtil::DrawFillCircle(CWnd* wnd, CPoint center, int radius, COLORREF color)
{
	if (!wnd || radius <= 0) return;

	CClientDC dc(wnd);

	// 클라이언트 영역으로 클리핑
	CRect client;
	wnd->GetClientRect(&client);
	dc.IntersectClipRect(&client);

	// 외곽선(펜) 제거 → 내부만 채움
	HPEN hOldPen = (HPEN)dc.SelectObject(GetStockObject(NULL_PEN));

	// 채우기 브러시
	CBrush brush;
	brush.CreateSolidBrush(color);
	CBrush* pOldBrush = dc.SelectObject(&brush);

	// 중심/반지름 → 바운딩 사각형
	CRect r(center.x - radius, center.y - radius,
		center.x + radius, center.y + radius);

	// 원(타원) 채우기
	dc.Ellipse(&r);

	// 원복
	dc.SelectObject(pOldBrush);
	dc.SelectObject(hOldPen);
	brush.DeleteObject();
}


bool CVideoUtil::IsExistFile(string filename)
{

	if (filesystem::exists(filename)) return 1;
	return 0;

}


bool CVideoUtil::IsExistFile(CString srcDir, CString file)
{
	CString _strFile = srcDir + file;

	CFileStatus fs;


	if (CFile::GetStatus(_strFile, fs))
	{
		return 1;
	}
	else
	{
		return 0;
	}

	return 0;
}

bool CVideoUtil::IsExistIndex(vector<int> indexs, int query_insex)
{
	for (int k = 0; k < indexs.size(); k++)
	{
		if (indexs[k] == query_insex) return 1;

	}
	return 0;
}



/************************************************************************/
/* 유니코드 ↔ 멀티바이트 문자열 변환함수 구현부                             */
/************************************************************************/
//
// CString → Char
//
char* CVideoUtil::StringToChar(CString str)
{
	char* szStr = NULL;
#if defined(UNICODE) || defined(_UNICODE)
	int nLen = str.GetLength() + 1;
	TCHAR* tszTemp = NULL;
	tszTemp = new TCHAR[nLen];
	memset(tszTemp, 0x00, nLen * sizeof(TCHAR));
	_tcscpy_s(tszTemp, nLen, str);
	// Get size (실제사용되는바이트사이즈)
	int nSize = WideCharToMultiByte(CP_ACP, 0, tszTemp, -1, NULL, NULL, NULL, NULL);
	szStr = new char[nSize];
	memset(szStr, 0x00, nSize);
	WideCharToMultiByte(CP_ACP, 0, tszTemp, -1, szStr, nSize, NULL, NULL);
	if (tszTemp)
	{
		delete[] tszTemp;
		tszTemp = NULL;
	}
#else
	int nLen = str.GetLength() + 1;
	szStr = new char[nLen];
	memset(szStr, 0x00, nLen);
	strcpy(szStr, str);
#endif
	return szStr;
}

//
// CString → TCHAR
//
TCHAR* CVideoUtil::StringToTCHAR(CString str)
{
	TCHAR* tszStr = NULL;
	int nLen = str.GetLength() + 1;
	tszStr = new TCHAR[nLen];
	memset(tszStr, 0x00, nLen * sizeof(TCHAR));
	_tcscpy_s(tszStr, nLen, str);

	return tszStr;
}

//
// CString → string
//
string CVideoUtil::StringToStdString(CString str)
{
	string stdStr;
	char* szStr = StringToChar(str);
	if (szStr)
	{
		stdStr = szStr;
		delete[] szStr;
	}

	return stdStr;
}

//
// Char → CString
//
CString CVideoUtil::CharToString(char* str)
{
	CString cStr;
#if defined(UNICODE) || defined(_UNICODE)
	int nLen = strlen(str) + 1;
	TCHAR* tszTemp = NULL;
	tszTemp = new TCHAR[nLen];
	memset(tszTemp, 0x00, nLen * sizeof(TCHAR));
	MultiByteToWideChar(CP_ACP, 0, str, -1, tszTemp, nLen * sizeof(TCHAR));
	cStr.Format(_T("%s"), tszTemp);
	if (tszTemp)
	{
		delete[] tszTemp;
		tszTemp = NULL;
	}
#else
	cStr.Format("%s", str);
#endif
	return cStr;
}

//
// Char → TCHAR
//
TCHAR* CVideoUtil::CharToTCHAR(char* str)
{
	TCHAR* tszStr = NULL;
#if defined(UNICODE) || defined(_UNICODE)
	int nLen = strlen(str) + 1;
	tszStr = new TCHAR[nLen];
	memset(tszStr, 0x00, nLen * sizeof(TCHAR));
	MultiByteToWideChar(CP_ACP, 0, str, -1, tszStr, nLen * sizeof(TCHAR));
#else
	int nLen = strlen(str) + 1;
	tszStr = new TCHAR[nLen];
	memset(tszStr, 0x00, nLen * sizeof(TCHAR));
	_tcscpy(tszStr, str);
#endif
	return tszStr;
}

//
// TCHAR → CString
//
CString CVideoUtil::TCHARToString(TCHAR* str)
{
	CString cStr;
	cStr.Format(_T("%s"), str);
	return cStr;
}

//
// TCHAR → Char
//
char* CVideoUtil::TCHARToChar(TCHAR* str)
{
	char* szStr = NULL;
#if defined(UNICODE) || defined(_UNICODE)
	int nSize = WideCharToMultiByte(CP_ACP, 0, str, -1, NULL, NULL, NULL, NULL);
	szStr = new char[nSize];
	memset(szStr, 0x00, nSize);
	WideCharToMultiByte(CP_ACP, 0, str, -1, szStr, nSize, NULL, NULL);
#else
	int nLen = strlen(str) + 1;
	szStr = new char[nLen];
	memset(szStr, 0x00, nLen);
	strcpy(szStr, str);
#endif
	return szStr;
}



CString CVideoUtil::GetFileName(CString src)
{
	CString prefix;

	int len = src.GetLength();

	int dot_pos = src.GetLength();
	int dir_pos = src.ReverseFind('\\');
	if (dot_pos == -1 || dir_pos > dot_pos) prefix.Empty();
	else if (dir_pos == -1)
	{
		prefix = src.Mid(0, dot_pos);
	}
	else prefix = src.Mid(dir_pos + 1, dot_pos - dir_pos - 1);

	return prefix;

}
CString CVideoUtil::GetDirName(CString src)
{
	CString file;

	int len = src.GetLength();

	int dot_pos = len;
	int dir_pos = src.ReverseFind('\\');
	if (dot_pos == -1 || dir_pos == -1 || dir_pos > dot_pos) file.Empty();
	else file = src.Mid(0, dir_pos);

	return file;

}


vector<CString> CVideoUtil::SplitCString(CString str)
{

	vector<CString> ret;
	CString   resToken;
	int curPos = 0;

	if (str.IsEmpty()) return ret;

	resToken = str.Tokenize(_T(" "), curPos);
	ret.push_back(resToken);
	while (resToken != "")
	{
		resToken = str.Tokenize(_T(" "), curPos);
		if (!resToken.IsEmpty()) ret.push_back(resToken);
	};
	if (ret.size() > 0)
	{
		ret[ret.size() - 1].Remove('\n');
	}
	
	return ret;
}


vector<CString> CVideoUtil::SplitCString(CString str, CString space)
{
	vector<CString> ret;

	// 구분자 집합이 비어 있으면 전체를 하나로
	if (space.IsEmpty()) {
		ret.push_back(str);
		return ret;
	}

	CString cur; // 현재 토큰
	for (int i = 0; i < str.GetLength(); ++i) {
		const TCHAR ch = str[i];

		// space에 포함된 문자는 모두 구분자로 취급
		if (space.Find(ch) != -1) {
			// cur이 빈 문자열이면 _T("")가 push_back 됩니다.
			ret.push_back(cur);
			cur.Empty();
		}
		else {
			cur.AppendChar(ch);
		}
	}

	// 마지막 토큰(비어 있을 수도 있음) 추가
	ret.push_back(cur);

	return ret;
}


//
// convert_ansi_to_unicode_string.
//

DWORD CVideoUtil::convert_ansi_to_unicode_string(
	__out wstring& unicode,
	__in const char* ansi,
	__in const size_t ansi_size
) {

	DWORD error = 0;

	do {

		if ((nullptr == ansi) || (0 == ansi_size)) {
			error = ERROR_INVALID_PARAMETER;
			break;
		}

		unicode.clear();

		//
		// getting required cch.
		//

		int required_cch = ::MultiByteToWideChar(
			CP_ACP,
			0,
			ansi, static_cast<int>(ansi_size),
			nullptr, 0
		);

		if (0 == required_cch) {
			error = ::GetLastError();
			break;
		}

		unicode.resize(required_cch);

		//
		// convert.
		//

		if (0 == ::MultiByteToWideChar(
			CP_ACP,
			0,
			ansi, static_cast<int>(ansi_size),
			const_cast<wchar_t*>(unicode.c_str()), static_cast<int>(unicode.size())
		)) {
			error = ::GetLastError();
			break;
		}

	} while (false);

	return error;
}

//
// convert_unicode_to_ansi_string.
//

DWORD CVideoUtil::convert_unicode_to_ansi_string(
	__out string& ansi,
	__in const wchar_t* unicode,
	__in const size_t unicode_size
) {

	DWORD error = 0;

	do {

		if ((nullptr == unicode) || (0 == unicode_size)) {
			error = ERROR_INVALID_PARAMETER;
			break;
		}

		ansi.clear();

		//
		// getting required cch.
		//

		int required_cch = ::WideCharToMultiByte(
			CP_ACP,
			0,
			unicode, static_cast<int>(unicode_size),
			nullptr, 0,
			nullptr, nullptr
		);

		if (0 == required_cch) {
			error = ::GetLastError();
			break;
		}

		//
		// allocate.
		//

		ansi.resize(required_cch);

		//
		// convert.
		//

		if (0 == ::WideCharToMultiByte(
			CP_ACP,
			0,
			unicode, static_cast<int>(unicode_size),
			const_cast<char*>(ansi.c_str()), static_cast<int>(ansi.size()),
			nullptr, nullptr
		)) {
			error = ::GetLastError();
			break;
		}

	} while (false);

	return error;
}

//
// convert_unicode_to_utf8_string
//

DWORD CVideoUtil::convert_unicode_to_utf8_string(
	__out string& utf8,
	__in const wchar_t* unicode,
	__in const size_t unicode_size
) {

	DWORD error = 0;

	do {

		if ((nullptr == unicode) || (0 == unicode_size)) {
			error = ERROR_INVALID_PARAMETER;
			break;
		}

		utf8.clear();

		//
		// getting required cch.
		//

		int required_cch = ::WideCharToMultiByte(
			CP_UTF8,
			WC_ERR_INVALID_CHARS,
			unicode, static_cast<int>(unicode_size),
			nullptr, 0,
			nullptr, nullptr
		);

		if (0 == required_cch) {
			error = ::GetLastError();
			break;
		}

		//
		// allocate.
		//

		utf8.resize(required_cch);

		//
		// convert.
		//

		if (0 == ::WideCharToMultiByte(
			CP_UTF8,
			WC_ERR_INVALID_CHARS,
			unicode, static_cast<int>(unicode_size),
			const_cast<char*>(utf8.c_str()), static_cast<int>(utf8.size()),
			nullptr, nullptr
		)) {
			error = ::GetLastError();
			break;
		}

	} while (false);

	return error;
}

//
// convert_utf8_to_unicode_string
//


vector<CString> CVideoUtil::SPlitCString(CString str)
{
	vector<CString>	vecText;
	CString strText(_T(""));
	//1. AfxExtractSubString 사용
	int nP = 0;
	while (FALSE != AfxExtractSubString(strText, str, nP++, _T(' ')))
	{
		vecText.push_back(strText);
	}
	return vecText;
}

vector<CString> CVideoUtil::SPlitBracket(CString str)
{
	CString vecText1=str;
	vector<CString>	vecText2;
	vector<int>ptr1, ptr2;
	
	for (int k = 0; k < str.GetLength(); k++)
	{
		if (str.Mid(k, 1).Compare(_T("("))==0) ptr1.push_back(k);
		else if(str.Mid(k, 1).Compare(_T(")")) == 0) ptr2.push_back(k);
	}

	for (int k = 0; k < ptr1.size(); k++)
	{
		if (k < ptr2.size())
		{
			vecText1.Replace(str.Mid(ptr1[k], ptr2[k] - ptr1[k] + 1), _T(""));
			vecText2.push_back(str.Mid(ptr1[k] + 1, ptr2[k] - ptr1[k] - 1));
		}
	}
	vecText2.insert(vecText2.begin(), vecText1);

	return vecText2;

}



void CVideoUtil::DrawFillRect_(CWnd* wnd, int x, int y, int width, int height, COLORREF clr)
{
	CClientDC dc(wnd);
	CRect r;

	CPen pen;
	CBrush brush, * pOldBrush;

	dc.SetBkMode(TRANSPARENT);
	//	brush.CreateSolidBrush(RGB(0, 255, 0));
	brush.CreateSolidBrush(clr);

	pOldBrush = dc.SelectObject(&brush);

	r.top = y;
	r.bottom = r.top + height;
	r.left = x;
	r.right = x + width;
	dc.FillRect(&r, &brush);

	dc.SelectObject(pOldBrush);
	brush.DeleteObject();

}



int CVideoUtil::PlayVideoFile(CWnd* wnd, CString file)
{
	CRect Rect;
	wnd->GetClientRect(Rect);

	VideoCapture cap(StringToChar(file));

	Mat img;
	if (cap.isOpened())
	{
		while (1)
		{
			cap >> img;
			if (img.empty()) break;
			DrawImageBMP(wnd, img, 0, 0, Rect.Width(), Rect.Height());
			Sleep(6);
		}
		cap.release();
	}	
	return 1;
}



int CVideoUtil::PlayVideoFile(CWnd* wnd, CString file, CString txt)
{
	CRect Rect;
	wnd->GetClientRect(Rect);

	VideoCapture cap(StringToChar(file));

	Mat img;
	if (cap.isOpened())
	{
		while (1)
		{
			cap >> img;
			if (img.empty()) break;
			DrawImageBMP(wnd, img.clone(), 0, 0, Rect.Width(), Rect.Height());
			DrawText_(wnd, 0, 0, 100, txt, RGB(255, 255, 255), 0, 0, CRect(0, 0, Rect.Width(), Rect.Height()));
			Sleep(10);
		}
		cap.release();
	}
	return 1;
}




int CVideoUtil::PlayImgs(CWnd* wnd,  CString txt, vector<Mat> imgs)
{
	CRect Rect;
	wnd->GetClientRect(Rect);
	
	for(auto img :imgs )
	{	
		DrawImageBMP(wnd, img, 0, 0, Rect.Width(), Rect.Height());
		DrawText_(wnd, 10, 10, 100, txt, RGB(255, 255, 255), 0, 0, CRect(0, 0, Rect.Width(), Rect.Height()));
			
		Sleep(30);			
	}		
}



Mat CVideoUtil::GetImageFromVideowithframeIndex(CString video_file, int index)
{
	// VideoCapture 열기
	VideoCapture cap(StringToChar(video_file));
	if (!cap.isOpened()) {
		cerr << "비디오 파일 열기 실패: " << video_file << endl;
		return Mat();
	}

	// 전체 프레임 개수 확인
	int total_frames = static_cast<int>(cap.get(CAP_PROP_FRAME_COUNT));
	if (index < 0 || index >= total_frames) {
		cerr << "잘못된 프레임 인덱스: " << index << " (총 프레임: " << total_frames << ")\n";
		return Mat();
	}

	// 원하는 프레임으로 이동
	cap.set(CAP_PROP_POS_FRAMES, index);

	// 프레임 읽기
	Mat frame;
	if (!cap.read(frame)) {
		cerr << "프레임 읽기 실패 (index=" << index << ")\n";
		return Mat();
	}

	return frame.clone(); // 안전하게 반환


}








void CVideoUtil::DrawXYChart(CWnd* wnd, vector<vector<float>>dat)
{
	float min_val = FLT_MAX;
	float max_val = FLT_MIN;

	for (int k = 0; k < dat.size(); k++)
	{
		auto min_val_ = *min_element(dat[k].begin(), dat[k].end());
		auto max_val_ = *max_element(dat[k].begin(), dat[k].end());
		if (min_val_ < min_val) min_val = min_val_;
		if (max_val_ > max_val) max_val = max_val_;
	}

	CRect rect;
	wnd->GetClientRect(rect);

	Mat bg_img(rect.Height(), rect.Width(), CV_8UC3, Scalar(0, 0, 0));

	char str[50];

	int limit_y = bg_img.rows * 0.8;
	int base_y = bg_img.rows * 0.2 / 2;
	int x_step = round((double)bg_img.cols / (dat[0].size() + 1));


	Scalar fixed_color[10] = { Scalar(0, 255, 0), Scalar(0, 255, 255), Scalar(255, 255, 0), Scalar(255, 255, 100),
								 Scalar(0, 255, 255),Scalar(255, 0, 255), Scalar(255, 255, 255),
								 Scalar(100, 255, 100),Scalar(255, 100, 100),Scalar(100, 100, 255)
	};

	double y_step = (max_val - min_val) / 10;
	for (double m = (int)min_val; m <= max_val; m += y_step)
	{
		int x1 = x_step;
		int x2 = x_step * (dat[0].size() - 1);
		int y = base_y + limit_y * (max_val - m) / (max_val - min_val);
		line(bg_img, Point(x1, y), Point(x2, y), Scalar(80, 80, 80), 1, 8, 0);
		sprintf(str, "%.2f", m);
		putText(bg_img, str, Point(x2, y), FONT_HERSHEY_DUPLEX, 0.3, Scalar(0, 255, 255), 1, 1);
	}

	for (int m = 0; m < dat[0].size(); m += (dat[0].size() / 20))
	{
		int x = x_step * (m + 1);
		int y = base_y + limit_y * (max_val - dat[0][m]) / (max_val - min_val);

		line(bg_img, Point(x, base_y), Point(x, base_y + limit_y), Scalar(80, 80, 80), 1, 8, 0);
	}


	for (int k = 0; k < dat.size(); k++)
	{
		for (int m = 0; m < dat[k].size(); m++)
		{
			int x = x_step * (m + 1);
			int y = base_y + limit_y * (max_val - dat[k][m]) / (max_val - min_val);

			rectangle(bg_img, Point(x - 1, y - 1), Point(x + 1, y + 1), fixed_color[k % 10], FILLED);
			if (m != (dat[k].size() - 1))
			{
				int x2 = x_step * (m + 2);
				int y2 = base_y + limit_y * (max_val - dat[k][m + 1]) / (max_val - min_val);
				line(bg_img, Point(x, y), Point(x2, y2), fixed_color[k % 10], 1);
			}

		}
	}

	for (int k = 0; k < dat.size(); k++)
	{
		rectangle(bg_img, Point(1 + k * 5, 1), Point(1 + (k + 1) * 5, 6), fixed_color[k % 10], FILLED);
	}

	DrawImageBMP(wnd, bg_img, 0, 0, rect.Width(), rect.Height());
}

int CVideoUtil::GetNearIndex(vector<int> data, int query)
{
	if (data.empty()) return -1; // 빈 벡터 예외 처리

	auto it = min_element(data.begin(), data.end(),
		[query](int a, int b) {
			return abs(a - query) < abs(b - query);
		});

	return distance(data.begin(), it);

}

int CVideoUtil::GetInsertIndex(vector<int> data, int query)
{
	if (data.empty()) return -1; // 빈 벡터 예외 처리

	auto it = lower_bound(data.begin(), data.end(), query);

	return static_cast<int>(it - data.begin());
}


vector<int> CVideoUtil::DrawXYChart(CWnd* wnd, vector<vector<float>>dat, vector<vector<int>> index, Mat &bg_img_, int cur_index)
{

	float min_val = FLT_MAX;
	float max_val = FLT_MIN;

	vector<int> x_steps;

	for (int k = 0; k < dat.size(); k++)
	{
		if (dat[k].size() > 0)
		{
			auto min_val_ = *min_element(dat[k].begin(), dat[k].end());
			auto max_val_ = *max_element(dat[k].begin(), dat[k].end());
			if (min_val_ < min_val) min_val = min_val_;
			if (max_val_ > max_val) max_val = max_val_;
		}
		else
			return x_steps;
	}

	CRect rect;
	wnd->GetClientRect(rect);

	Mat bg_img(rect.Height(), rect.Width(), CV_8UC3, Scalar(0, 0, 0));

	char str[50];

	int limit_y = bg_img.rows * 0.8;
	int base_y = bg_img.rows * 0.2 / 2;
	int text_mrg = 50;
	double x_step = (double)(bg_img.cols- text_mrg) / (double)(dat[0].size());

	

	Scalar fixed_color[10] = { Scalar(0, 255, 0), Scalar(0, 0, 255), Scalar(255, 0, 0), Scalar(255, 255, 0),
								 Scalar(0, 255, 255),Scalar(255, 0, 255), Scalar(255, 255, 255),
								 Scalar(100, 255, 100),Scalar(255, 100, 100),Scalar(100, 100, 255)
	};
	
	int last_m = 0;
//	for (int m = 0; m < (dat[0].size() + (dat[0].size() % 20) ); m += (dat[0].size() / 20))
	for (int m = 0; m < (dat[0].size() ); m += (dat[0].size() / 20))
	{
		int x = round(x_step * (m + 1));
		int y = base_y + limit_y * (max_val - dat[0][m]) / (max_val - min_val);
		line(bg_img, Point(x, base_y), Point(x, base_y + limit_y), Scalar(80, 80, 80), 1, 8, 0);

		sprintf(str, "%d", m);
		putText(bg_img, str, Point(x, base_y + limit_y +15), FONT_HERSHEY_DUPLEX, 0.4, Scalar(0, 255, 255), 1, 1);
		last_m = m;
	}
	

	double y_step = (max_val - min_val) / 10;	
	for (double m = (int)min_val; m <= max_val; m += y_step)
	{
		int x1 = round(x_step);
		int x2 = bg_img.cols - text_mrg;
		int y = base_y + limit_y * (max_val - m) / (max_val - min_val);
		line(bg_img, Point(x1, y), Point(x2, y), Scalar(80, 80, 80), 1, 8, 0);
		sprintf(str, "%.4f", m);
		putText(bg_img, str, Point(x2+5, y), FONT_HERSHEY_DUPLEX, 0.4, Scalar(0, 255, 255), 1, 1);
	}
	line(bg_img, Point(x_step, base_y), Point(bg_img.cols - text_mrg, base_y), Scalar(80, 80, 80), 1, 8, 0);

	for (int k = 0; k < dat.size(); k++)
	{
		for (int m = 0; m < dat[k].size(); m++)
		{
			int x = round(x_step * (m + 1));
			int y = base_y + limit_y * (max_val - dat[k][m]) / (max_val - min_val);

			rectangle(bg_img, Point(x - 1, y - 1), Point(x + 1, y + 1), fixed_color[k % 10], FILLED);
			if (m != (dat[k].size() - 1))
			{
				int x2 = round(x_step * (m + 2));
				int y2 = base_y + limit_y * (max_val - dat[k][m + 1]) / (max_val - min_val);
				line(bg_img, Point(x, y), Point(x2, y2), fixed_color[k % 10], 1);
			}

			if(k==0) x_steps.push_back(x);

		}
	}

	for (int k = 0; k < dat.size(); k++)
	{
		rectangle(bg_img, Point(1 + k * 5, 1), Point(1 + (k + 1) * 5, 6), fixed_color[k % 10], FILLED);
	}

	for (int k = 0; k < index.size(); k++)
	{
		rectangle(bg_img, Point(1 + k * 5+100, 1), Point(1 + (k + 1) * 5+100, 6), fixed_color[ (k+dat.size()) % 10], FILLED);
	}

	for (int k = 0; k < index.size(); k++)
	{
		for (int m = 0; m < index[k].size(); m++)
		{
			int x = round(x_step * (index[k][m]+1));
			int y1 = base_y ;
			int y2 = base_y + limit_y ;

			line(bg_img, Point(x, y1), Point(x, y2), fixed_color[(k + dat.size()) % 10], 1);

			int target_x_text_num = 10;

			int skip_num = index[k].size() / target_x_text_num;

			if (skip_num == 0 || (m % skip_num)==0)
			{
				CString str;
				str.Format(_T("%d"), index[k][m]);
				putText(bg_img, StringToChar(str), Point(x, y1), FONT_HERSHEY_DUPLEX, 0.4, Scalar(255, 255, 255), 1, 1);
			}
		}
	}

	DrawImageBMP(wnd, bg_img, 0, 0, rect.Width(), rect.Height());

	bg_img_ = bg_img;

	return x_steps;
}




void CVideoUtil::CreateFolder(CString csPath)
{
	// UpdateData(TRUE);
	// csPath = m_csTopFolderName + csPath;

	CString csPrefix(_T("")), csToken(_T(""));
	int nStart = 0, nEnd;
	while ((nEnd = csPath.Find('/', nStart)) >= 0)
	{
		CString csToken = csPath.Mid(nStart, nEnd - nStart);
		CreateDirectory(csPrefix + csToken, NULL);

		csPrefix += csToken;
		csPrefix += _T("/");
		nStart = nEnd + 1;
	}
	csToken = csPath.Mid(nStart);
	CreateDirectory(csPrefix + csToken, NULL);
}

Mat  CVideoUtil::DrawHist(int w, int h, vector<float> data, float* channel_rangle, int number_bins)
{
	float step = (channel_rangle[1] - channel_rangle[0]) / number_bins;
	int* num_bin = new int[number_bins];
	for (int k = 0; k < number_bins; k++)num_bin[k] = 0;

	int max_num_bin = 0;
	for (int k = 0; k < data.size(); k++)
	{
		int bin_idx = (data[k] - channel_rangle[0]) / step;
		if (bin_idx >= 0 && bin_idx < number_bins)
		{
			num_bin[bin_idx]++;
			if (num_bin[bin_idx] > max_num_bin) max_num_bin = num_bin[bin_idx];
		}
	}
	
	int hist_w = w;
	int hist_h = h;
	int bin_w = cvRound((double)hist_w / number_bins);

	Mat hist_img(hist_h, hist_w, CV_8UC3, Scalar::all(0));
	

	for (int i = 0; i < number_bins; i++) {
		line(hist_img, Point(bin_w * (i ), hist_h), Point(bin_w * (i), hist_h - cvRound(hist_h*(double)num_bin[i])/ max_num_bin), Scalar(0, 255, 0), 1, 8, 0);
	}	

	delete[] num_bin;
	return hist_img;
}



void CVideoUtil::calcNormal(double v0[3], double v1[3], double v2[3], double out[3])
{
	double V1[3], V2[3];
	static const int x = 0;
	static const int y = 1;
	static const int z = 2;

	// Calculate two vectors from the three points
	V1[x] = v0[x] - v1[x];
	V1[y] = v0[y] - v1[y];
	V1[z] = v0[z] - v1[z];

	V2[x] = v1[x] - v2[x];
	V2[y] = v1[y] - v2[y];
	V2[z] = v1[z] - v2[z];

	// Take the cross product of the two vectors to get
	// the normal vector which will be stored in out[]
	out[x] = V1[y] * V2[z] - V1[z] * V2[y];
	out[y] = V1[z] * V2[x] - V1[x] * V2[z];
	out[z] = V1[x] * V2[y] - V1[y] * V2[x];

	// Normalize the vector (shorten length to one)
	double nor = sqrt(out[0] * out[0] + out[1] * out[1] + out[2] * out[2]);
	out[0] = out[0] / nor;
	out[1] = out[1] / nor;
	out[2] = out[2] / nor;
}

void CVideoUtil::calcNormal(Point3f v0, Point3f v1, Point3f v2, Point3f &out)
{
	double V1[3], V2[3];
	static const int x = 0;
	static const int y = 1;
	static const int z = 2;

	// Calculate two vectors from the three points
	V1[x] = v0.x - v1.x;
	V1[y] = v0.y - v1.y;
	V1[z] = v0.z - v1.z;

	V2[x] = v1.x - v2.x;
	V2[y] = v1.y - v2.y;
	V2[z] = v1.z - v2.z;

	// Take the cross product of the two vectors to get
	// the normal vector which will be stored in out[]
	out.x = V1[y] * V2[z] - V1[z] * V2[y];
	out.y = V1[z] * V2[x] - V1[x] * V2[z];
	out.z = V1[x] * V2[y] - V1[y] * V2[x];

	// Normalize the vector (shorten length to one)
	double nor = sqrt(out.x * out.x + out.y * out.y + out.z * out.z);
	out.x = out.x / nor;
	out.y = out.y / nor;
	out.z = out.z / nor;
}

double CVideoUtil::GetAngle(Point3f p11, Point3f p12, Point3f p21, Point3f p22)
{

	double angle;

	Point3f A = p11 - p12;
	Point3f B = p21 - p22;	

	return GetAngle(A,B);

}


void CVideoUtil::DrawResetImage(CWnd* wnd)
{
	CRect rect;
	wnd->GetWindowRect(rect);

	Mat white_img(rect.Height(), rect.Width(), CV_8UC3, Scalar(240, 240, 240)); //컬러 행렬 모든 픽셀 초기 색, red(OpenCV 인자는 GBR순서입니다.)
	DrawImageBMP(wnd, white_img, 0, 0, rect.Width(), rect.Height());
//	DrawText_(wnd, rect.Width() * 3 / 10, rect.Height() / 2 - 50, 200, _T("Preocessing..."), RGB(210, 210, 210), 0, 0, CRect(0, 0, rect.Width(), rect.Height()));
}



int CVideoUtil::IsExist(int index, vector<int>data)
{
	for (int k = 0; k < data.size(); k++)
	{
		if (index == data[k]) return 1;
	}
	return 0;
}

int CVideoUtil::IsExist(int index, vector<UINT32>data)
{
	for (int k = 0; k < data.size(); k++)
	{
		if (index == (int)data[k]) return 1;
	}
	return 0;
}




bool CVideoUtil::IsExistDir(CString dir)
{
	if (GetFileAttributes(dir) != 0xFFFFFFFF)
	{
		return true;
	}
	else return false;
}

// 특정 바이트 크기(n)마다 개행(\n) 추가하는 함수
string CVideoUtil::InsertLineFeed_(string& input, int n)
{
	string result;
	int byteCount = 0; // 바이트 카운터

	for (size_t i = 0; i < input.length(); i++) {
		unsigned char ch = input[i]; // 현재 문자

		// 한글 (멀티바이트, 2바이트 문자)
		if (ch & 0x80) { // 첫 바이트가 1xxx xxxx 형태이면 한글
			if (i + 1 < input.length()) { // 한글은 2바이트이므로 한 글자 더 추가
				result += input.substr(i, 2);
				byteCount += 2;
				i++; // 한글이므로 다음 문자로 이동
			}
		}
		else { // 영문, 숫자, 특수문자 (1바이트)
			result += ch;
			byteCount += 1;
		}

		// 지정된 바이트 수(n) 초과 시 개행 추가
		if (byteCount >= n) {
			result += '\r';
			result += '\n';
			byteCount = 0;
		}
	}

	return result;
}


Rect CVideoUtil::GetBox(vector <Point2i> pts, double scale, Mat img) // margin은 rect의 스케일
{
	// pts가 비어있으면 빈 rect 반환
	if (pts.empty())
		return Rect();

	// pts의 모든 점에서 최소/최대 좌표 계산
	int minX = pts[0].x, minY = pts[0].y;
	int maxX = pts[0].x, maxY = pts[0].y;
	for (size_t i = 1; i < pts.size(); i++)
	{
		minX = min(minX, pts[i].x);
		minY = min(minY, pts[i].y);
		maxX = max(maxX, pts[i].x);
		maxY = max(maxY, pts[i].y);
	}

	// 최소 bounding box의 가로, 세로 길이 계산
	int width = maxX - minX;
	int height = maxY - minY;

	// 정사각형을 위해 두 길이 중 큰 값을 사용
	int maxDim = max(width, height);

	// scale을 적용한 정사각형의 사이즈
	int squareSize = static_cast<int>(maxDim * scale);

	// 만약 확대된 정사각형이 이미지 크기보다 크다면 이미지 크기로 제한
	if (squareSize > img.cols)
		squareSize = img.cols;
	if (squareSize > img.rows)
		squareSize = img.rows;

	// 원래 bounding box의 중심 계산
	int centerX = (minX + maxX) / 2;
	int centerY = (minY + maxY) / 2;

	// 중심을 기준으로 정사각형의 좌상단 좌표 계산
	int x = centerX - squareSize / 2;
	int y = centerY - squareSize / 2;

	// 이미지 범위 내에 위치하도록 보정
	if (x < 0)
		x = 0;
	if (y < 0)
		y = 0;
	if (x + squareSize > img.cols)
		x = img.cols - squareSize;
	if (y + squareSize > img.rows)
		y = img.rows - squareSize;

	return Rect(x, y, squareSize, squareSize);
}

Rect CVideoUtil::getRectWidth(Rect rect, int width, Mat img)
{
	auto center_pt = getRectCenter(rect);
	Rect r_rect = Rect(center_pt.x - width / 2, center_pt.y - width / 2, width, width);
	r_rect = r_rect & Rect(0, 0, img.cols, img.rows);

	return r_rect;
}

Point CVideoUtil::getRectCenter(const Rect& rect)
{
	return Point(rect.x + rect.width / 2, rect.y + rect.height / 2);
}


RECT CVideoUtil::CvRectToRECT(const Rect& cvRect) {
	RECT rc;
	rc.left = cvRect.x;
	rc.top = cvRect.y;
	rc.right = cvRect.x + cvRect.width;
	rc.bottom = cvRect.y + cvRect.height;
	return rc;
}


double CVideoUtil::getRotationAngle(const Mat& R1, const Mat& R2)
{
	Mat R_diff = R1.t() * R2;
	Mat rvec;
	Rodrigues(R_diff, rvec);
	return norm(rvec);  // radians
}

double CVideoUtil::getRotationAngle(vector<float> dat1, vector<float> dat2)
{
	if (dat1.size() != 9 || dat2.size() != 9) return -CV_PI;


	Mat R1 = (Mat_<float>(3, 3) <<
		dat1[0], dat1[3], dat1[6],
		dat1[1], dat1[4], dat1[7],
		dat1[2], dat1[5], dat1[8]);
	Mat R2 = (Mat_<float>(3, 3) <<
		dat2[0], dat2[3], dat2[6],
		dat2[1], dat2[4], dat2[7],
		dat2[2], dat2[5], dat2[8]);

	Mat R_diff = R1.t() * R2;
	Mat rvec;
	Rodrigues(R_diff, rvec);
	return norm(rvec);  // radians
}

// 비디오 정보 가져오기 함수
tuple<int, int, double, int> CVideoUtil::GetVideoInfo(VideoCapture cap)
{	
	if (!cap.isOpened()) {
		// 실패 시 -1 반환
		return make_tuple(-1, -1, -1.0, -1);
	}

	int width = static_cast<int>(cap.get(CAP_PROP_FRAME_WIDTH));
	int height = static_cast<int>(cap.get(CAP_PROP_FRAME_HEIGHT));
	double fps = cap.get(CAP_PROP_FPS);
	int frame_count = static_cast<int>(cap.get(CAP_PROP_FRAME_COUNT));

	return make_tuple(width, height, fps, frame_count);
}

CString CVideoUtil::LoadLastDir(LPCTSTR dialogKey, LPCTSTR defDir = _T("C:\\")) {
	CString section = _T("Recent");
	CString key; key.Format(_T("LastDir_%s"), dialogKey);
	return AfxGetApp()->GetProfileString(section, key, defDir);
}

void CVideoUtil::SaveLastDir(LPCTSTR dialogKey, const CString& dir) {
	if (dir.IsEmpty()) return;
	CString section = _T("Recent");
	CString key; key.Format(_T("LastDir_%s"), dialogKey);
	AfxGetApp()->WriteProfileString(section, key, dir);
}

CString CVideoUtil::DirFromPath(const CString& path) {
	int pos = path.ReverseFind(_T('\\'));
	return (pos >= 0) ? path.Left(pos) : path;
}

// 존재 확인 후 대체 기본값으로 폴백
CString CVideoUtil::EnsureDirExists(const CString& dir, LPCTSTR fallback = _T("C:\\")) {
	DWORD attr = GetFileAttributes(dir);
	if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY))
		return dir;
	return fallback;
}

void CVideoUtil::DrawRactangles(Mat& img, vector<Point> pts, Scalar s, int width) //scalar B G R order
{
	for (auto pt : pts)
		rectangle(img, Rect(pt.x - width / 2, pt.y - width / 2, width, width), s, FILLED);
}

void CVideoUtil::DrawLines(Mat& img, vector<Point> pts, Scalar s, int len)
{
	for (auto pt : pts)
	{
		line(img, pt + Point(-len / 2, -len / 2), pt + Point(len / 2, len / 2), s, 1, 8, 0);
		line(img, pt + Point(len / 2, -len / 2), pt + Point(-len / 2, len / 2), s, 1, 8, 0);
	}
}



Rect CVideoUtil::ConvRect(Rect2f norm_rect, int img_w, int img_h)
{
	Rect pixel_rect(
		static_cast<int>(norm_rect.x * img_w),
		static_cast<int>(norm_rect.y * img_h),
		static_cast<int>(norm_rect.width * img_w),
		static_cast<int>(norm_rect.height * img_h)
	);
	return pixel_rect;
}

Rect2f CVideoUtil::ConvRect2f(Rect pixel_rect, int img_w, int img_h)
{
	Rect2f norm_rect;
	norm_rect.x = static_cast<float>(pixel_rect.x) / img_w;
	norm_rect.y = static_cast<float>(pixel_rect.y) / img_h;
	norm_rect.width = static_cast<float>(pixel_rect.width) / img_w;
	norm_rect.height = static_cast<float>(pixel_rect.height) / img_h;

	return norm_rect;
}

CRect CVideoUtil::ConvCRect(Rect rc)
{
	CRect rect;
	rect.left = rc.x;
	rect.right = rc.x + rc.width;
	rect.top = rc.y;
	rect.bottom = rc.y + rc.height;

	return rect;
}
Rect CVideoUtil::ConvRect(CRect rc)
{
	return Rect(rc.left, rc.top, rc.Width(), rc.Height());
}

size_t CVideoUtil::argmax_index_nan_safe(const vector<double>& v) {
	if (v.empty()) throw runtime_error("argmax on empty vector");
	auto it = max_element(v.begin(), v.end(),
		[](double a, double b) {
			if (isnan(a)) return true;   // a가 NaN이면 b가 우선(= a<b 취급)
			if (isnan(b)) return false;  // b가 NaN이면 a가 우선
			return a < b;
		});
	return static_cast<size_t>(distance(v.begin(), it));
}

vector<UINT32> CVideoUtil::removeDuplicates(vector<UINT32> vec) {
	sort(vec.begin(), vec.end());
	vec.erase(unique(vec.begin(), vec.end()), vec.end());
	return vec; // 정렬 + 중복제거된 벡터 반환
}

CString CVideoUtil::FindMatchingFileName(vector<CString> file_nms, vector<CString> conditions)
{
	for (auto file : file_nms)
	{
		int flag = 0;
		for (auto condition : conditions)
		{
			if (file.Find(condition) < 0)
			{
				flag = 1;
				break;
			}
		}
		if (flag == 0) return file;
	}
	return _T("");
}


// data : 원본 데이터
// N : 유지할 그룹 개수
// return : 제거되는 인덱스들
vector<size_t> CVideoUtil::removeExtraGroups(const vector<UINT32>& data, size_t N)
{
	vector<size_t> removedIndices;

	if (data.empty() || N == 0) {
		for (size_t i = 0; i < data.size(); ++i)
			removedIndices.push_back(i);
		return removedIndices;
	}

	// 1️⃣ 연속된 동일값 그룹 탐색
	struct Group { size_t start, end; uint32_t value; };
	vector<Group> groups;
	size_t start = 0;

	for (size_t i = 1; i <= data.size(); ++i) {
		if (i == data.size() || data[i] != data[i - 1]) {
			groups.push_back({ start, i - 1, data[start] });
			start = i;
		}
	}

	// 2️⃣ 각 그룹 내부에서 N개 이하만 균등 샘플링
	for (const auto& g : groups) {
		size_t len = g.end - g.start + 1;

		if (len <= N)
			continue; // N개 이하 → 제거 없음

		// 균등 간격 샘플 인덱스 계산
		vector<size_t> keepIdx;
		keepIdx.reserve(N);
		double step = static_cast<double>(len - 1) / static_cast<double>(N - 1);

		for (size_t i = 0; i < N; ++i) {
			size_t idx = g.start + static_cast<size_t>(llround(i * step));
			keepIdx.push_back(idx);
		}

		// 나머지 인덱스 제거 리스트에 추가
		for (size_t i = g.start; i <= g.end; ++i) {
			if (find(keepIdx.begin(), keepIdx.end(), i) == keepIdx.end())
				removedIndices.push_back(i);
		}
	}

	return removedIndices;
}

bool CVideoUtil::IsExist(size_t kk, const vector<size_t>& remove_index)
{
	return find(remove_index.begin(), remove_index.end(), kk) != remove_index.end();
}
```

## File: VideoUtil-v1.1.h
```
#pragma once
#include "afxdialogex.h"


#include <opencv2\opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <cmath>
#include <filesystem>

#include <iostream>
#include <numeric>
#include <vector>
#include <math.h>
#include <algorithm>

#include <fstream>

#include <cstdlib>
#include <ctime>
#include <limits>
#include <tuple>

using namespace cv;
using namespace std;


class CVideoUtil :
	public CDialogEx
{
public:
	CVideoUtil();
	~CVideoUtil();

	//화면출력 관련툴
	void DrawImageBMP(CDC& dc, Mat frame, int x, int y, int width, int height);
	void DrawImageBMP(CWnd* wnd, Mat frame, int x, int y, int width, int height);
	void DrawImageBMP(CWnd* wnd, Mat frame, int x, int y, double time_line_percent);
	void DrawImageBMP(CWnd* wnd, Mat frame, int x, int y, double time_line_percent, int frame_index);
	void DrawImageBMP(CWnd* wnd, Mat frame, int x, int y);
	void DrawImageBMP(CWnd* wnd, Mat frame, int x, int y, int width, int height, CRect rec);
//	void DrawImageBMPwKeepRatio(CWnd* wnd, Mat frame, int x, int y, int width, int height);
	void DrawImageBMPwKeepRationFast(CWnd* wnd, Mat frame, int x, int y, int width, int height);
	void DrawImageTransparentBMP(CWnd* wnd, Mat frame, int x, int y, int width, int height);
	void DrawLine(CWnd* wnd, CPoint p1, CPoint p2, COLORREF color, int thickness);
	void DrawLine(CWnd* wnd, CPoint p1, CPoint p2, COLORREF color, int thickness, int line_type);	
	void DrawLines(Mat& img, vector<Point> pts, Scalar s, int len);
	void DrawRect(CWnd* wnd, CRect rect, COLORREF color);
	void DrawFillRect(CWnd* wnd, const CRect& rect, COLORREF color);
	void DrawFillRect_(CWnd* wnd, int x, int y, int width, int height, COLORREF clr);
	void DrawFillCircle(CWnd* wnd, CPoint center, int radius, COLORREF color);
	void DrawScalarImg(CWnd* wnd, Scalar color, int x, int y, int width, int height);
	void DrawImgsList(CWnd* wnd, int x_num, int y_num, vector<Mat> imgs, int ptr_index);
	void DrawImgsList(CWnd* wnd, int x_num, int y_num, vector<Mat> imgs, int ptr_index, vector<CString> labels);
	void DrawIndexList(CWnd* wnd, int x_num, int y_num, vector<int> index);
	void DrawRactangles(Mat& img, vector<Point> pts, Scalar s, int width);	
	void DrawResetImage(CWnd* wnd);
	Mat  DrawHist(int w, int h, vector<float> data, float* channel_rangle, int number_bins);
	void DrawText_(CWnd* wnd, int x, int y, int size, CString txt, COLORREF color, int type, int mode, CRect win_rec);
	void DrawText_(CWnd* wnd, int x, int y, int size, CString txt, COLORREF color, int type, CRect win_rec); // type : 0 배경 투명
	void DrawText_Hangul(CWnd* wnd,
		int x, int y,
		int ptSize,            // 포인트 단위 글꼴 크기
		const CString& txt,
		COLORREF textColor,
		int type,              // 0: 투명, 1: 불투명+테두리
		int mode,              // 0: 자동 줄바꿈, 1: 단일 줄
		const CRect& win_rec); // 클리핑/최대 영역	
	void DrawXYChart(CWnd* wnd, vector<vector<float>>dat);
	vector<int>  DrawXYChart(CWnd* wnd, vector<vector<float>>dat, vector<vector<int>> index, Mat& bg_img_, int cur_index);	
	int PlayVideoFile(CWnd* wnd, CString file);
	int PlayVideoFile(CWnd* wnd, CString file, CString txt);
	int PlayImgs(CWnd* wnd, CString txt, vector<Mat> imgs);

	//영상처리툴 모음
	void CopyImg(Mat src, Mat& dst, int x, int y);
	void CopyImg2(Mat src, Mat& dst, int x, int y);
	void CopyImg3(Mat src, Mat& dst, int x, int  y, int width, int height); // all area of src -> partial dst (src<dst)
	Mat GetImageFromVideowithframeIndex(CString video_file, int index);
	tuple<int, int, double, int> GetVideoInfo(VideoCapture cap);
	tuple<int, int, double, int> GetVideoInfo(const string& filename);
	Rect getRectWidth(Rect rect, int width, Mat img);
	Point getRectCenter(const Rect& rect);
	Rect GetBox(vector <Point2i> pts, double scale, Mat img); // margin은 rect의 스케일	

	//CString 관련툴
	CString Vector2CString(vector<float> dat);
	CString Vector2CString(vector<Point3f> dat);
	CString Vector2CString(vector<UINT32> dat);
	vector<CString> SplitCString(CString str);
	vector<CString> SplitCString(CString str, CString space);
	vector<string> SplitString(string str);
	vector<CString> SPlitCString(CString str);
	vector<CString> SPlitBracket(CString str); //() 사이로 분리된 단어 리턴

	//File처리 관련툴
	string InsertLineFeed_(string& input, int n);
	void CreateFolder(CString csPath);
	vector<CString> GetAllFile(CString srcDir);
	vector<CString> GetAllDir(CString srcDir);
	CString GetFilePrefix(CString dirName);	
	CString GetFileExt(CString src);
	CString GetDirName(CString src);
	CString GetFileName(CString dirName);
	bool IsExistFile(CString srcDir, CString file);
	bool IsExistFile(string filename);
	bool IsExistDir(CString dir);
	
	CString FindMatchingFileName(vector<CString> file_nms, vector<CString> conditions); //conditions의 단어가 포함되는 파일 리스트 얻기

	//파일선택 다이얼로그 이전값 기억을 위한 함수
	CString LoadLastDir(LPCTSTR dialogKey, LPCTSTR defDir);
	void SaveLastDir(LPCTSTR dialogKey, const CString& dir);
	CString DirFromPath(const CString& path);
	// 존재 확인 후 대체 기본값으로 폴백
	CString EnsureDirExists(const CString& dir, LPCTSTR fallback);

	//Math tools
	double GetAngle(Point3f p11, Point3f p12, Point3f p21, Point3f p22);
	double GetAngle(Point3f p1, Point3f p2, Point3f p3);
	double GetAngle(Point3f p1, Point3f p2);
	double GetAngle(Point2f p1, Point2f p2);
	double GetAngle(Point2f p1);

	double Distance(vector<Point> pts1, vector<Point> pts2);
	double Distance(double x1, double y1, double x2, double y2);
	double Distance(double x1, double y1, double z1, double x2, double y2, double z2);
	double Distance(Point3f p1, Point3f p2);
	double Distance(Point3f p1);
	double Distance(Point2d p1, Point2d p2);
	double Distance(Point2f p1, Point2f p2);
	double Distance(Point p1, Point p2);
	double Distance(Point2f p1);
	double Distance(Mat p1, Mat p2);
	void calcNormal(Point3f v0, Point3f v1, Point3f v2, Point3f& out);
	void calcNormal(double v0[3], double v1[3], double v2[3], double out[3]);
	double getRotationAngle(const Mat& R1, const Mat& R2);
	double getRotationAngle(vector<float> dat1, vector<float> dat2);

	//Type conversion
	Point3f MatToPoint3f(Mat m);	
	RECT CvRectToRECT(const Rect& cvRect);
	Rect ConvRect(Rect2f norm_rect, int img_w, int img_h);
	Rect2f ConvRect2f(Rect pixel_rect, int img_w, int img_h);
	CRect ConvCRect(Rect rect);
	Rect ConvRect(CRect rect);

	//기타 툴 모음
	vector<UINT32> removeDuplicates(vector<UINT32> vec); //벡터 정렬 및 중복된 원소 제거
	vector<size_t> removeExtraGroups(const vector<UINT32>& data, size_t N); //그룹네에 N을 초과하는 그룹에서 초과 원소를 균등 제거
	size_t argmax_index_nan_safe(const vector<double>& v); //벡터가 비어있거나 값이 무한대일때 예외처리
	vector<Point3f> DevideByN(vector<Point3f> pt, double N);
	bool IsExistIndex(vector<int> indexs, int query_index);
	int IsExist(int index, vector<int>data);
	int IsExist(int index, vector<UINT32>data);
	bool IsExist(size_t kk, const vector<size_t>& remove_index);
	int GetNearIndex(vector<int> data, int query);
	int GetInsertIndex(vector<int> data, int query);	

	//한글코드관련 툴
	static char* StringToChar(CString str);              // CString → Char
	static TCHAR* StringToTCHAR(CString str);            // CString → TCHAR
	static string StringToStdString(CString str);   // CString → string
	static CString CharToString(char* str);              // Char → CString
	static TCHAR* CharToTCHAR(char* str);                // Char → TCHAR
	static CString TCHARToString(TCHAR* str);            // TCHAR → CString
	static char* TCHARToChar(TCHAR* str);                // TCHAR → Char

/************************************************************************/
/* 유니코드 ↔ 멀티바이트 문자열 변환함수 정의부                             */
/************************************************************************/
	DWORD convert_unicode_to_utf8_string(
		__out string& utf8,
		__in const wchar_t* unicode,
		__in const size_t unicode_size
	);

	DWORD convert_unicode_to_ansi_string(
		__out string& ansi,
		__in const wchar_t* unicode,
		__in const size_t unicode_size
	);

	DWORD CVideoUtil::convert_ansi_to_unicode_string(
		__out wstring& unicode,
		__in const char* ansi,
		__in const size_t ansi_size
	);

	private:
	BITMAPINFO* MakeBMPHeader(int width, int height);
	BITMAPINFO* MakeBMPHeader(int width, int height, int channel);

	//int Mat2CImage(Mat* mat, CImage& img);
	int GetKoreaCharNum(CString str);
};
```
