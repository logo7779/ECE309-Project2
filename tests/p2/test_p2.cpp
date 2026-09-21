// tests/p2/test_p2.cpp
//
// YOUR test suite goes here. At least 12 assert-based test cases — see
// spec §5 for the required categories and the sample test for the
// expected level of rigor.
//
// This file is a stub so the project builds out of the box; replace the
// body of main() with your own tests.

#include "core/conversation.h"
#include "core/sentinel_scanner.h"
#include "harness/harness.h"
#include "model/replay_client.h"
#include "model/scripted_client.h"

#include <cassert>

#include <fstream>
#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

// ============================================================
// Test helper classes
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


class EOFInput : public InputSource {
public:
    std::string read_line() override {
        return "";
    }

    bool is_eof() const override {
        return true;
    }
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
// Conversation Tests
// ============================================================

void test_empty_conversation()
{
    Conversation conv;

    assert(conv.size() == 0);
    assert(conv.begin() == conv.end());

    std::cout << "PASS: empty conversation\n";
}


void test_conversation_append_and_access()
{
    Conversation conv;

    conv.append(Message(Role::User, "hello"));
    conv.append(Message(Role::Assistant, "hi there"));
    conv.append(Message(Role::User, "how are you?"));

    assert(conv.size() == 3);

    assert(conv.at(0).role() == Role::User);
    assert(conv.at(0).content() == "hello");

    assert(conv.at(1).role() == Role::Assistant);
    assert(conv.at(1).content() == "hi there");

    assert(conv.at(2).role() == Role::User);
    assert(conv.at(2).content() == "how are you?");

    std::cout << "PASS: conversation append and access\n";
}


void test_system_message_ordering()
{
    Conversation conv;

    conv.append(Message(Role::System, "You are a helpful assistant."));
    conv.append(Message(Role::User, "hello"));
    conv.append(Message(Role::Assistant, "Hi!"));

    assert(conv.size() == 3);

    assert(conv.at(0).role() == Role::System);
    assert(conv.at(0).content() == "You are a helpful assistant.");

    assert(conv.at(1).role() == Role::User);
    assert(conv.at(2).role() == Role::Assistant);

    std::cout << "PASS: system message ordering\n";
}


void test_conversation_out_of_bounds()
{
    Conversation conv;

    conv.append(Message(Role::User, "hello"));

    bool threw = false;

    try
    {
        conv.at(1);
    }
    catch (const std::out_of_range&)
    {
        threw = true;
    }

    assert(threw);

    std::cout << "PASS: out-of-bounds access throws\n";
}


void test_copy_constructor_deep_copy()
{
    Conversation original;

    original.append(Message(Role::User, "hello"));
    original.append(Message(Role::Assistant, "world"));

    Conversation copy(original);

    assert(copy.size() == original.size());

    // The two Conversations must own different backing arrays.
    assert(copy.begin() != original.begin());

    // Contents must be identical.
    assert(copy.at(0).role() == original.at(0).role());
    assert(copy.at(0).content() == original.at(0).content());

    assert(copy.at(1).role() == original.at(1).role());
    assert(copy.at(1).content() == original.at(1).content());

    // Changing the original must not affect the copy.
    original.at(0); // Access is valid.

    original.append(Message(Role::User, "third"));

    assert(original.size() == 3);
    assert(copy.size() == 2);

    std::cout << "PASS: copy constructor deep copy\n";
}


void test_copy_assignment_deep_copy()
{
    Conversation original;

    original.append(Message(Role::System, "system"));
    original.append(Message(Role::User, "hello"));
    original.append(Message(Role::Assistant, "response"));

    Conversation copy;

    copy.append(Message(Role::User, "old data"));

    copy = original;

    assert(copy.size() == original.size());

    // Different backing arrays.
    assert(copy.begin() != original.begin());

    for (std::size_t i = 0; i < original.size(); ++i)
    {
        assert(copy.at(i).role() == original.at(i).role());
        assert(copy.at(i).content() == original.at(i).content());
    }

    // Self-assignment should also be safe.
    copy = copy;

    assert(copy.size() == 3);
    assert(copy.at(0).content() == "system");

    std::cout << "PASS: copy assignment deep copy\n";
}


void test_move_constructor()
{
    Conversation original;

    original.append(Message(Role::User, "hello"));
    original.append(Message(Role::Assistant, "response"));

    const Message* original_data = original.begin();

    Conversation moved(std::move(original));

    // Move must steal the original backing pointer.
    assert(moved.begin() == original_data);

    // The moved-to object owns the messages.
    assert(moved.size() == 2);
    assert(moved.at(0).content() == "hello");
    assert(moved.at(1).content() == "response");

    // Moved-from object must be valid and empty.
    assert(original.size() == 0);
    assert(original.begin() == original.end());

    std::cout << "PASS: move constructor\n";
}


void test_move_assignment()
{
    Conversation source;

    source.append(Message(Role::User, "source"));
    source.append(Message(Role::Assistant, "response"));

    const Message* source_data = source.begin();

    Conversation destination;

    destination.append(Message(Role::User, "old"));
    destination.append(Message(Role::User, "data"));

    destination = std::move(source);

    // Destination should now own source's original buffer.
    assert(destination.begin() == source_data);

    assert(destination.size() == 2);
    assert(destination.at(0).content() == "source");
    assert(destination.at(1).content() == "response");

    // Source must be empty.
    assert(source.size() == 0);
    assert(source.begin() == source.end());

    std::cout << "PASS: move assignment\n";
}


void test_conversation_growth()
{
    Conversation conv;

    // More than enough messages to force several reallocations:
    //
    // capacity sequence with doubling:
    // 0 -> 1 -> 2 -> 4 -> 8 -> 16 -> 32 -> 64 -> ...
    //
    // We don't directly inspect private capacity_, but we verify
    // that all elements survive the reallocations correctly.

    constexpr std::size_t count = 100;

    for (std::size_t i = 0; i < count; ++i)
    {
        conv.append(
            Message(Role::User, "message " + std::to_string(i))
        );

        assert(conv.size() == i + 1);
    }

    for (std::size_t i = 0; i < count; ++i)
    {
        assert(conv.at(i).role() == Role::User);
        assert(conv.at(i).content() ==
               "message " + std::to_string(i));
    }

    std::cout << "PASS: conversation growth\n";
}


void test_conversation_iteration()
{
    Conversation conv;

    conv.append(Message(Role::User, "one"));
    conv.append(Message(Role::Assistant, "two"));
    conv.append(Message(Role::User, "three"));

    std::size_t count = 0;

    for (const Message& message : conv)
    {
        if (count == 0)
        {
            assert(message.content() == "one");
        }
        else if (count == 1)
        {
            assert(message.content() == "two");
        }
        else if (count == 2)
        {
            assert(message.content() == "three");
        }

        ++count;
    }

    assert(count == 3);

    std::cout << "PASS: conversation iteration\n";
}


// ============================================================
// SentinelScanner Tests
// ============================================================

void test_scanner_clean_text()
{
    const std::string sentinel = "<|end_conversation|>";

    SentinelScanner scanner(sentinel);

    auto out = scanner.feed("Hello, how are you?");

    assert(!out.sentinel_found);

    auto final = scanner.flush();

    assert(!final.sentinel_found);

    const std::string result =
        out.safe_text + final.safe_text;

    assert(result == "Hello, how are you?");

    std::cout << "PASS: scanner clean text\n";
}


void test_scanner_whole_sentinel()
{
    const std::string sentinel = "<|end_conversation|>";

    SentinelScanner scanner(sentinel);

    auto out = scanner.feed(
        "Goodbye." + sentinel
    );

    assert(out.sentinel_found);
    assert(out.safe_text == "Goodbye.");

    std::cout << "PASS: scanner whole sentinel\n";
}


void test_scanner_split_every_boundary()
{
    const std::string sentinel = "<|end_conversation|>";
    const std::string text = "Goodbye." + sentinel;

    for (std::size_t split = 0; split <= text.size(); ++split)
    {
        SentinelScanner scanner(sentinel);

        auto out1 = scanner.feed(
            std::string_view(text.data(), split)
        );

        auto out2 = scanner.feed(
            std::string_view(
                text.data() + split,
                text.size() - split
            )
        );

        assert(
            out1.sentinel_found ||
            out2.sentinel_found
        );

        assert(
            out1.safe_text + out2.safe_text
            == "Goodbye."
        );
    }

    std::cout << "PASS: scanner split at every boundary\n";
}


void test_scanner_one_character_at_a_time()
{
    const std::string sentinel = "<|end_conversation|>";
    const std::string text = "Goodbye." + sentinel;

    SentinelScanner scanner(sentinel);

    std::string safe_text;
    bool found = false;

    for (char c : text)
    {
        auto out = scanner.feed(
            std::string_view(&c, 1)
        );

        safe_text += out.safe_text;

        if (out.sentinel_found)
        {
            found = true;
            break;
        }
    }

    assert(found);
    assert(safe_text == "Goodbye.");

    std::cout << "PASS: scanner one character at a time\n";
}


void test_scanner_false_alarm()
{
    const std::string sentinel = "<|end_conversation|>";

    SentinelScanner scanner(sentinel);

    const std::string text =
        "Hello <|end_world|> goodbye.";

    auto out = scanner.feed(text);

    assert(!out.sentinel_found);

    auto final = scanner.flush();

    assert(!final.sentinel_found);

    assert(out.safe_text + final.safe_text == text);

    std::cout << "PASS: scanner false alarm\n";
}


void test_scanner_partial_sentinel_then_normal_text()
{
    const std::string sentinel = "<|end_conversation|>";

    SentinelScanner scanner(sentinel);

    auto out = scanner.feed("Hello <|end_");

    assert(!out.sentinel_found);

    auto out2 = scanner.feed("world>");

    assert(!out2.sentinel_found);

    auto final = scanner.flush();

    assert(!final.sentinel_found);

    assert(
        out.safe_text +
        out2.safe_text +
        final.safe_text
        == "Hello <|end_world>"
    );

    std::cout << "PASS: scanner partial sentinel\n";
}


void test_scanner_flush()
{
    const std::string sentinel = "<|end_conversation|>";

    SentinelScanner scanner(sentinel);

    auto out = scanner.feed("Hello <|end_");

    assert(!out.sentinel_found);

    auto final = scanner.flush();

    assert(!final.sentinel_found);

    assert(
        out.safe_text + final.safe_text
        == "Hello <|end_"
    );

    std::cout << "PASS: scanner flush\n";
}


void test_scanner_bounded_stream()
{
    const std::string sentinel = "<|end_conversation|>";

    SentinelScanner scanner(sentinel);

    // 4 MB, matching the grading rubric.
    constexpr std::size_t stream_size = 4 * 1024 * 1024;

    std::string safe_text;
    safe_text.reserve(stream_size);

    for (std::size_t i = 0; i < stream_size; ++i)
    {
        char c = 'A';

        auto out = scanner.feed(
            std::string_view(&c, 1)
        );

        assert(!out.sentinel_found);

        /*
         * Because the scanner holds back at most
         * sentinel.size() - 1 characters, once enough
         * characters have arrived, each one-character
         * feed should be able to release one character.
         *
         * This also verifies that the scanner isn't
         * accumulating the entire stream in pending_.
         */
        assert(out.safe_text.size() <= 1);

        safe_text += out.safe_text;
    }

    auto final = scanner.flush();

    assert(!final.sentinel_found);

    safe_text += final.safe_text;

    assert(safe_text.size() == stream_size);

    assert(
        safe_text == std::string(stream_size, 'A')
    );

    std::cout << "PASS: scanner bounded 4 MB stream\n";
}

// ============================================================
// Harness Tests
// ============================================================

void test_harness_turn_limit() {
    auto model = std::make_unique<TestModel>("hello");

    HarnessConfig cfg;
    cfg.max_turns = 2;

    Harness harness(std::move(model), cfg);

    TestInput input({
        "first",
        "second",
        "third"
    });

    TestOutput output;

    StopReason reason = harness.run(input, output);

    assert(reason.kind == StopReason::Kind::TurnLimit);

    // Exactly two turns should occur.
    assert(harness.conversation().size() == 4);

    assert(harness.conversation().at(0).role() == Role::User);
    assert(harness.conversation().at(0).content() == "first");

    assert(harness.conversation().at(1).role() == Role::Assistant);
    assert(harness.conversation().at(1).content() == "hello");

    assert(harness.conversation().at(2).role() == Role::User);
    assert(harness.conversation().at(2).content() == "second");

    assert(harness.conversation().at(3).role() == Role::Assistant);
    assert(harness.conversation().at(3).content() == "hello");
}


void test_harness_sentinel_halt() {
    const std::string sentinel = "<|end_conversation|>";

    auto model = std::make_unique<TestModel>(
        "Hello there!" +
        sentinel +
        "THIS SHOULD BE DISCARDED"
    );

    HarnessConfig cfg;
    cfg.max_turns = 20;

    Harness harness(std::move(model), cfg);

    TestInput input({
        "hello"
    });

    TestOutput output;

    StopReason reason = harness.run(input, output);

    assert(reason.kind == StopReason::Kind::Sentinel);

    // Only one turn should occur.
    assert(harness.conversation().size() == 2);

    assert(harness.conversation().at(0).role() == Role::User);
    assert(harness.conversation().at(0).content() == "hello");

    assert(harness.conversation().at(1).role() == Role::Assistant);

    // The sentinel must be stored in the conversation.
    assert(
        harness.conversation().at(1).content()
        == "Hello there!" + sentinel
    );

    // The sentinel must NOT be printed.
    assert(output.output.find(sentinel) == std::string::npos);

    // Text after the sentinel must be discarded.
    assert(
        output.output.find("THIS SHOULD BE DISCARDED")
        == std::string::npos
    );

    // Normal assistant text must be printed.
    assert(
        output.output.find("Hello there!")
        != std::string::npos
    );
}


void test_harness_system_message() {
    auto model = std::make_unique<TestModel>("response");

    HarnessConfig cfg;
    cfg.max_turns = 1;
    cfg.system_message = "Be concise.";

    Harness harness(std::move(model), cfg);

    TestInput input({
        "hello"
    });

    TestOutput output;

    StopReason reason = harness.run(input, output);

    assert(reason.kind == StopReason::Kind::TurnLimit);

    assert(harness.conversation().size() == 3);

    assert(harness.conversation().at(0).role() == Role::System);
    assert(harness.conversation().at(0).content() == "Be concise.");

    assert(harness.conversation().at(1).role() == Role::User);
    assert(harness.conversation().at(1).content() == "hello");

    assert(harness.conversation().at(2).role() == Role::Assistant);
    assert(harness.conversation().at(2).content() == "response");
}


void test_harness_user_exit() {
    auto model = std::make_unique<TestModel>("should not happen");

    HarnessConfig cfg;
    cfg.max_turns = 5;

    Harness harness(std::move(model), cfg);

    EOFInput input;
    TestOutput output;

    StopReason reason = harness.run(input, output);

    assert(reason.kind == StopReason::Kind::UserExit);

    // No conversation messages should have been created.
    assert(harness.conversation().size() == 0);

    // Model should never have generated anything.
    assert(
        output.output.find("should not happen")
        == std::string::npos
    );
}

void test_replay_transcript_round_trip() {
    const std::string transcript_path = "test_round_trip.transcript";

    {
        std::ofstream file(transcript_path);

        assert(file.is_open());

        file << "role: system\n";
        file << "\n";
        file << "Be concise.\n";
        file << "\n";
        file << "---\n";
        file << "\n";
        file << "role: user\n";
        file << "\n";
        file << "hello\n";
        file << "\n";
        file << "---\n";
        file << "\n";
        file << "role: assistant\n";
        file << "\n";
        file << "Hi there!\n";
        file << "\n";
        file << "---\n";
        file << "\n";
        file << "role: user\n";
        file << "\n";
        file << "How are you?\n";
        file << "\n";
        file << "---\n";
        file << "\n";
        file << "role: assistant\n";
        file << "\n";
        file << "I am here to help.\n";
        file << "\n";
        file << "---\n";
    }

    ReplayModelClient replay(transcript_path);

    // The leading system block must be recovered.
    assert(replay.system_message() == "Be concise.");

    Conversation conv;

    conv.append(Message(Role::System, "Be concise."));
    conv.append(Message(Role::User, "hello"));

    Message first_reply = replay.generate(conv);

    assert(first_reply.role() == Role::Assistant);
    assert(first_reply.content() == "Hi there!");

    conv.append(first_reply);
    conv.append(Message(Role::User, "How are you?"));

    Message second_reply = replay.generate(conv);

    assert(second_reply.role() == Role::Assistant);
    assert(second_reply.content() == "I am here to help.");

    conv.append(second_reply);

    // The replayed conversation should contain the same
    // system/user/assistant sequence represented by the transcript.
    assert(conv.size() == 5);

    assert(conv.at(0).role() == Role::System);
    assert(conv.at(0).content() == "Be concise.");

    assert(conv.at(1).role() == Role::User);
    assert(conv.at(1).content() == "hello");

    assert(conv.at(2).role() == Role::Assistant);
    assert(conv.at(2).content() == "Hi there!");

    assert(conv.at(3).role() == Role::User);
    assert(conv.at(3).content() == "How are you?");

    assert(conv.at(4).role() == Role::Assistant);
    assert(conv.at(4).content() == "I am here to help.");

    // Clean up the temporary transcript.
    std::remove(transcript_path.c_str());
}

// ============================================================
// Main
// ============================================================

int main() {
    // Conversation tests
    test_empty_conversation();
    test_conversation_append_and_access();
    test_system_message_ordering();
    test_conversation_out_of_bounds();
    test_copy_constructor_deep_copy();
    test_copy_assignment_deep_copy();
    test_move_constructor();
    test_move_assignment();
    test_conversation_growth();
    test_conversation_iteration();

    // SentinelScanner tests
    test_scanner_clean_text();
    test_scanner_whole_sentinel();
    test_scanner_split_every_boundary();
    test_scanner_one_character_at_a_time();
    test_scanner_false_alarm();
    test_scanner_partial_sentinel_then_normal_text();
    test_scanner_flush();
    test_scanner_bounded_stream();

    // Harness tests
    test_harness_turn_limit();
    test_harness_sentinel_halt();
    test_harness_system_message();
    test_harness_user_exit();
    test_replay_transcript_round_trip();

    std::cout << "All tests passed!\n";
    return 0;
}
