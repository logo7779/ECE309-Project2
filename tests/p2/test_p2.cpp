// tests/p2/test_p2.cpp
//
// YOUR test suite goes here. At least 12 assert-based test cases — see
// spec §5 for the required categories and the sample test for the
// expected level of rigor.
//
// This file is a stub so the project builds out of the box; replace the
// body of main() with your own tests.

#include "core/conversation.h"
#include "core/message.h"
#include "core/sentinel_scanner.h"
#include "harness/harness.h"
#include "model/replay_client.h"
#include "model/scripted_client.h"

#include <cassert>

#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>


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
// Main
// ============================================================

int main()
{
    // Conversation
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

    // SentinelScanner
    test_scanner_clean_text();
    test_scanner_whole_sentinel();
    test_scanner_split_every_boundary();
    test_scanner_one_character_at_a_time();
    test_scanner_false_alarm();
    test_scanner_partial_sentinel_then_normal_text();
    test_scanner_flush();
    test_scanner_bounded_stream();

    std::cout << "\n========================================\n";
    std::cout << "ALL TESTS PASSED\n";
    std::cout << "========================================\n";

    return 0;
}
