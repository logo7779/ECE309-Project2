# ECE 309 Project 2

## Overview

This project implements a small C++ mini-harness for interacting with a model client. The project is designed to demonstrate object-oriented programming, dynamic containers, message handling, streamed model responses, sentinel detection, and automated testing.

The harness accepts user messages, stores conversation history, sends messages to a model client, and processes the model's response. The project also includes different model-client implementations for scripted responses and replaying a previously recorded conversation.

The project was developed using C++17.

---

## Project Structure

The project is organized into several components:

```text
ECE309-project2/
├── CMakeLists.txt
├── README.md
│
├── include/
│   ├── core/
│   │   ├── conversation.h
│   │   ├── message.h
│   │   └── sentinel_scanner.h
│   │
│   ├── harness/
│   │   └── harness.h
│   │
│   └── model/
│       ├── model_client.h
│       ├── replay_client.h
│       └── scripted_client.h
│
├── src/
│   ├── conversation.cpp
│   ├── harness.cpp
│   ├── message.cpp
│   ├── model_client.cpp
│   ├── replay_client.cpp
│   ├── scripted_client.cpp
│   ├── sentinel_scanner.cpp
│   └── main.cpp
│
├── tests/
│   └── p2/
│       └── test_p2.cpp
│
├── scripts/
│   └── greeting.script
│
└── docs/
    └── design-log-p2.md
```

### Main Components

#### `Message`

The `Message` class represents one message in a conversation. Each message contains a role and its text content.

The supported roles include:

* System
* User
* Assistant

The class provides access to the message role and content.

#### `Conversation`

The `Conversation` class stores a sequence of messages.

It supports:

* Creating an empty conversation
* Appending messages
* Accessing messages by index
* Reporting the number of messages
* Iterating through messages
* Copy construction
* Copy assignment
* Move construction
* Move assignment
* Growing as additional messages are added

The conversation container is used by the harness to maintain the history of a conversation.

#### `SentinelScanner`

The `SentinelScanner` processes model output that may arrive in pieces.

The scanner looks for a specified sentinel string:

```text
<|end_conversation|>
```

When the sentinel is found, the scanner reports that it has been detected and prevents the sentinel itself from being treated as normal response text.

The scanner also handles cases where the sentinel is split across multiple input chunks. Text that cannot yet be safely classified is temporarily buffered until enough information is available.

The scanner can also be flushed at the end of a response.

#### `ModelClient`

`ModelClient` defines the interface used by the harness to communicate with a model.

The project does not communicate with an actual online language model. Instead, model-client implementations are used to provide predictable local behavior for testing.

#### `ScriptedClient`

The `ScriptedClient` provides predetermined model responses.

This makes it possible to test the harness without depending on an external model or network connection.

A sample script is provided in:

```text
scripts/greeting.script
```

#### `ReplayClient`

The `ReplayClient` reads a previously recorded conversation transcript.

This provides a repeatable way to replay system, user, and assistant messages.

#### `Harness`

The `Harness` coordinates the other components.

It is responsible for:

1. Maintaining the conversation.
2. Receiving user input.
3. Sending the conversation to the model client.
4. Processing the model's streamed response.
5. Passing model output through the sentinel scanner.
6. Detecting when the conversation should stop.
7. Enforcing the maximum number of turns.

---

## Building the Project

This project uses CMake and requires a C++17-compatible compiler.

From the project directory, create a build directory and configure the project:

```bash
cmake -S . -B build
```

Then build the project:

```bash
cmake --build build
```

The build creates the following executables:

```text
build/miniharness
build/test_p2
```

The `build/` directory contains generated build files and is intentionally excluded from Git using `.gitignore`.

---

## Running the Program

After building the project, the main program can be run with:

```bash
./build/miniharness
```

The harness then accepts user input and processes the conversation using the configured model client.

---

## Running the Tests

The Project 2 test suite can be run with:

```bash
./build/test_p2
```

The tests use C++ `assert` statements to verify the required behavior of the project components.

The test suite covers the required categories from the Project 2 specification, including:

* Conversation construction
* Conversation message insertion
* System messages
* Bounds checking
* Copy construction
* Copy assignment
* Move construction
* Move assignment
* Conversation growth
* Conversation iteration
* Sentinel scanning
* Sentinel detection across streamed chunks
* Sentinel false-alarm handling
* Partial sentinel handling
* Scanner flushing
* Harness turn limits
* Sentinel-based conversation termination
* System message handling
* User exit behavior
* Replay transcript behavior

The project is built with AddressSanitizer and UndefinedBehaviorSanitizer enabled during development to help identify memory errors and undefined behavior.

---

## Testing Approach

The test suite is designed around the behavior of the public interfaces rather than the internal implementation details.

Each test uses assertions to verify an expected result.

For example, a conversation test verifies that appending a message increases the conversation size and that the stored message can be retrieved correctly.

The sentinel scanner tests also account for streamed output. A sentinel may arrive as one complete string or may be split between multiple chunks. The tests verify that both situations are handled correctly.

The harness tests verify that the harness can:

* Stop after the configured maximum number of turns.
* Stop when the sentinel is encountered.
* Preserve the system message.
* Handle a user exit request.

---

## Error and Edge-Case Handling

Several edge cases are explicitly handled by the implementation.

### Conversation Bounds

Accessing a conversation outside its valid range is handled rather than silently accessing invalid memory.

### Empty Conversations

A newly created conversation contains no messages and reports the correct size.

### Conversation Growth

The conversation container can grow as messages are added rather than having a fixed maximum number of messages.

### Sentinel Split Across Chunks

The sentinel scanner keeps enough trailing text buffered to detect a sentinel that may be split across multiple model-output chunks.

For example:

```text
<|end_con
versation|>
```

can still be recognized as the sentinel when the two pieces arrive separately.

### Partial Output

Text that cannot yet be classified as safe output is temporarily stored by the scanner and can be returned when the scanner is flushed.

### Maximum Turns

The harness stops after reaching its configured maximum number of turns.

---

## Development and Design

The project was developed incrementally by implementing and testing individual components before integrating them into the harness.

The major development stages were:

1. Implement the message representation.
2. Implement the conversation container.
3. Implement sentinel scanning.
4. Implement model-client behavior.
5. Implement the harness.
6. Add automated tests.
7. Build and run the complete project.
8. Use sanitizers to check for memory and undefined-behavior problems.
9. Fix integration and testing issues discovered during development.

The design attempts to keep each component responsible for one primary task. The conversation stores messages, the scanner processes streamed output, model clients provide model behavior, and the harness coordinates the overall interaction.

Additional design decisions and development notes are documented in:

```text
docs/design-log-p2.md
```

---

## Dependencies

The project uses standard C++ functionality and C++17 language features.

The project does not require:

* An external language-model API
* Network access
* A JSON library
* Threads
* A real online model

The model behavior used for testing is local and deterministic.

---
