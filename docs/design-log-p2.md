# Design Log — Project 2

# ECE 309 Project 2 — Design Log

## Overview

The main design challenges in Project 2 were implementing a growable `Conversation` container without using `std::vector`, safely managing its dynamically allocated memory, and processing streamed model output without allowing the sentinel scanner's memory usage to grow with the size of the complete response.

## Conversation Growth Strategy

I implemented `Conversation` as a dynamically allocated array of `Message` objects. The container maintains three values: `data_`, `size_`, and `capacity_`. When the array is full, the capacity is doubled. Starting from an empty container, the capacities therefore grow approximately as:

```text
1, 2, 4, 8, 16, 32, ...
```

Doubling was chosen because it provides amortized `O(1)` insertion while still keeping the implementation simple.

Suppose `n` messages are appended. Most appends do not require reallocation and therefore take constant time. A reallocation occurs when the current capacity is reached. The numbers of existing elements moved during reallocations are approximately:

```text
1 + 2 + 4 + 8 + ... + 2^k
```

where `2^k` is the largest capacity below `n`. This geometric sum is less than `2n`, so the total amount of copying or moving caused by all reallocations is `O(n)`. The `n` ordinary insertions also contribute `O(n)` total work. Therefore, the total work for `n` appends is `O(n)`, giving an amortized cost of `O(1)` per append.

A fixed-size growth strategy such as increasing the capacity by one would require moving approximately `1 + 2 + ... + n` elements over `n` insertions, resulting in `O(n^2)` total work. The doubling strategy avoids this behavior.

## Rule of Five and Memory Ownership

`Conversation` owns its dynamically allocated `Message` array, so its copy and move behavior must be explicitly defined.

The destructor releases the owned array. The copy constructor and copy-assignment operator perform deep copies: they allocate their own storage and copy the messages from the source conversation. This means two copied conversations have independent buffers and can be destroyed or modified independently.

The move constructor and move-assignment operator instead transfer ownership of the existing buffer. The destination receives the source's `data_`, `size_`, and `capacity_`, while the source is reset to an empty state. This avoids unnecessary per-element copying.

The move operations are marked `noexcept`, since transferring the pointer and resetting the source does not require an allocation. The implementation also handles self-assignment in the copy and move assignment operations.

The tests specifically check copy and move behavior so that shallow copies or incorrect ownership transfers can be detected. AddressSanitizer was also used during development to help identify invalid memory operations and double frees.

## Sentinel Scanner and Bounded Memory

The model response can arrive in arbitrary chunks, so the sentinel cannot be assumed to occur entirely inside one chunk. The scanner uses a `pending_` string to temporarily hold trailing characters that could still become part of:

```text
<|end_conversation|>
```

If the sentinel has length `S`, the scanner keeps at most `S - 1` characters pending. After combining the pending characters with the newest chunk, if the sentinel is not found, every character except the final `S - 1` characters is guaranteed not to be the beginning of a future sentinel. Those safe characters can therefore be returned immediately, while only the final `S - 1` characters are retained.

Thus, at every point:

```text
|pending_| <= S - 1
```

regardless of how many total characters have been received. This gives the scanner bounded working memory with respect to the input stream instead of storing the entire response.

The implementation was tested with split sentinels, one-character-at-a-time input, false partial matches, and a large adversarial stream.

## Hindsight

One thing I might change is how the SentinelScanner searches for the sentinel. The current implementation uses std::string::find(), which is simple and easy to understand. A more advanced string-search algorithm could potentially be faster for larger inputs, but the sentinel is short and the pending buffer is already limited in size. Because of this, I decided that the simpler approach was appropriate for this project.
