// tests/p2/test_p2.cpp
//
#include "core/conversation.h"
#include "core/sentinel_scanner.h"
#include "harness/harness.h"
#include "model/replay_client.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>

// ============================================================
// Small test helpers
// ============================================================

class TestInput : public InputSource {
public:
    explicit TestInput(std::vector<std::string> lines)
        : lines_(std::move(lines)) {}

    std::string read_line() override {
        if (index_ >= lines_.size()) {
            eof_ = true;
            return "";
        }

        return lines_[index_++];
    }

    bool is_eof() const override {
        return eof_;
    }

private:
    std::vector<std::string> lines_;
    std::size_t index_ = 0;
    bool eof_ = false;
};


class TestOutput : public OutputSink {
public:
    void write(std::string_view text) override {
        output += text;
    }

    std::string output;
};


class TestModel : public ModelClient {
public:
    explicit TestModel(std::string response)
        : response_(std::move(response)) {}

    void generate(const Conversation&, TokenSink& sink) override {
        sink.on_chunk(response_);
        sink.on_complete();
    }

private:
    std::string response_;
};


// ============================================================
// 1. Empty Conversation
// ============================================================

void test_empty_conversation()
{
    Conversation conv;

    assert(conv.size() == 0);
    assert(conv.begin() == conv.end());

    std::cout << "PASS: empty conversation\n";
}


// ============================================================
// 2. System Message
// ============================================================

void test_system_message()
{
    Conversation conv;

    conv.append(Message(Role::System, "Be helpful."));
    conv.append(Message(Role::User, "hello"));

    assert(conv.size() == 2);
    assert(conv.at(0).role() == Role::System);
    assert(conv.at(0).content() == "Be helpful.");
    assert(conv.at(1).role() == Role::User);

    std::cout << "PASS: system message\n";
}


// ============================================================
// 3. Copy Constructor
// ============================================================

void test_copy_constructor()
{
    Conversation original;

    original.append(Message(Role::User, "hello"));
    original.append(Message(Role::Assistant, "hi"));

    Conversation copy(original);

    assert(copy.size() == original.size());
    assert(copy.begin() != original.begin());

    assert(copy.at(0).content() == "hello");
    assert(copy.at(1).content() == "hi");

    original.append(Message(Role::User, "third"));

    assert(original.size() == 3);
    assert(copy.size() == 2);

    std::cout << "PASS: copy constructor\n";
}


// ============================================================
// 4. Move Constructor
// ============================================================

void test_move_constructor()
{
    Conversation original;

    original.append(Message(Role::User, "hello"));
    original.append(Message(Role::Assistant, "response"));

    const Message* original_data = original.begin();

    Conversation moved(std::move(original));

    assert(moved.begin() == original_data);
    assert(moved.size() == 2);
    assert(moved.at(0).content() == "hello");
    assert(moved.at(1).content() == "response");

    assert(original.size() == 0);
    assert(original.begin() == original.end());

    std::cout << "PASS: move constructor\n";
}


// ============================================================
// 5. Append and Access
// ============================================================

void test_append_and_access()
{
    Conversation conv;

    conv.append(Message(Role::User, "hello"));
    conv.append(Message(Role::Assistant, "hi"));

    assert(conv.size() == 2);
    assert(conv.at(0).role() == Role::User);
    assert(conv.at(0).content() == "hello");
    assert(conv.at(1).role() == Role::Assistant);
    assert(conv.at(1).content() == "hi");

    std::cout << "PASS: append and access\n";
}


// ============================================================
// 6. Scanner Clean Text
// ============================================================

void test_scanner_clean_text()
{
    const std::string sentinel = "<|end_conversation|>";

    SentinelScanner scanner(sentinel);

    auto result = scanner.feed("Hello, how are you?");

    assert(!result.sentinel_found);

    auto final = scanner.flush();

    assert(!final.sentinel_found);
    assert(final.safe_text == "Hello, how are you?");

    std::cout << "PASS: scanner clean text\n";
}


// ============================================================
// 7. Scanner Split Sentinel
// ============================================================

void test_scanner_split_sentinel()
{
    const std::string sentinel = "<|end_conversation|>";

    SentinelScanner scanner(sentinel);

    auto first = scanner.feed("Goodbye.<|end_");
    auto second = scanner.feed("conversation|>");

    assert(!first.sentinel_found);
    assert(second.sentinel_found);

    assert(first.safe_text + second.safe_text == "Goodbye.");

    std::cout << "PASS: scanner split sentinel\n";
}


// ============================================================
// 8. Scanner False Alarm
// ============================================================

void test_scanner_false_alarm()
{
    const std::string sentinel = "<|end_conversation|>";

    SentinelScanner scanner(sentinel);

    auto result = scanner.feed("Hello <|end_world|> goodbye.");

    assert(!result.sentinel_found);

    auto final = scanner.flush();

    assert(!final.sentinel_found);
    assert(result.safe_text + final.safe_text
           == "Hello <|end_world|> goodbye.");

    std::cout << "PASS: scanner false alarm\n";
}


// ============================================================
// 9. Harness Turn Limit
// ============================================================

void test_harness_turn_limit()
{
    auto model = std::make_unique<TestModel>("hello");

    HarnessConfig cfg;
    cfg.max_turns = 2;

    Harness harness(std::move(model), cfg);

    TestInput input({"first", "second", "third"});
    TestOutput output;

    StopReason reason = harness.run(input, output);

    assert(reason.kind == StopReason::Kind::TurnLimit);
    assert(harness.conversation().size() == 4);

    assert(harness.conversation().at(0).content() == "first");
    assert(harness.conversation().at(1).content() == "hello");
    assert(harness.conversation().at(2).content() == "second");
    assert(harness.conversation().at(3).content() == "hello");

    std::cout << "PASS: harness turn limit\n";
}


// ============================================================
// 10. Harness User Exit / EOF
// ============================================================

void test_harness_user_exit()
{
    auto model = std::make_unique<TestModel>("should not happen");

    HarnessConfig cfg;
    cfg.max_turns = 5;

    Harness harness(std::move(model), cfg);

    // Empty input causes TestInput to reach EOF.
    TestInput input({});
    TestOutput output;

    StopReason reason = harness.run(input, output);

    assert(reason.kind == StopReason::Kind::UserExit);
    assert(harness.conversation().size() == 0);
    assert(output.output.find("should not happen")
           == std::string::npos);

    std::cout << "PASS: harness user exit\n";
}


// ============================================================
// 11. Harness Sentinel Halt
// ============================================================

void test_harness_sentinel_halt()
{
    const std::string sentinel = "<|end_conversation|>";

    auto model = std::make_unique<TestModel>(
        "Hello there!" + sentinel + "discarded"
    );

    HarnessConfig cfg;
    cfg.max_turns = 5;

    Harness harness(std::move(model), cfg);

    TestInput input({"hello"});
    TestOutput output;

    StopReason reason = harness.run(input, output);

    assert(reason.kind == StopReason::Kind::Sentinel);

    assert(harness.conversation().size() == 2);

    assert(harness.conversation().at(1).content()
           == "Hello there!" + sentinel);

    assert(output.output.find(sentinel) == std::string::npos);
    assert(output.output.find("discarded") == std::string::npos);
    assert(output.output.find("Hello there!") != std::string::npos);

    std::cout << "PASS: harness sentinel halt\n";
}


// ============================================================
// 12. Replay Transcript
// ============================================================

void test_replay_transcript()
{
    const std::string path = "test_round_trip.transcript";

    {
        std::ofstream file(path);

        assert(file.is_open());

        file << "role: system\n";
        file << "Be concise.\n";
        file << "---\n";

        file << "role: user\n";
        file << "hello\n";
        file << "---\n";

        file << "role: assistant\n";
        file << "Hi there!\n";
        file << "---\n";
    }

    ReplayModelClient replay(path);

    assert(replay.system_message() == "Be concise.");

    Conversation conv;
    conv.append(Message(Role::System, "Be concise."));
    conv.append(Message(Role::User, "hello"));

    Message reply = replay.generate(conv);

    assert(reply.role() == Role::Assistant);
    assert(reply.content() == "Hi there!");

    std::remove(path.c_str());

    std::cout << "PASS: replay transcript\n";
}


// ============================================================
// Main
// ============================================================

int main()
{
    test_empty_conversation();
    test_system_message();
    test_copy_constructor();
    test_move_constructor();
    test_append_and_access();

    test_scanner_clean_text();
    test_scanner_split_sentinel();
    test_scanner_false_alarm();

    test_harness_turn_limit();
    test_harness_user_exit();
    test_harness_sentinel_halt();

    test_replay_transcript();

    std::cout << "\nAll tests passed!\n";

    return 0;
}
