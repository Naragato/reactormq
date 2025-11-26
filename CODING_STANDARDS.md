# C++ CODING STANDARDS

Based on the referenced section, adapted for general C++.

## Goals

- Maximize clarity and consistency.
- Encode scope/role when helpful; do not encode types (no Hungarian).
- Prefer descriptive, searchable names.

## Case and word breaks

- Variables: `lowerCamelCase`
- Treat acronyms as words: `httpServer`, `xmlReader` (not `HTTPServer`).
- Types: `PascalCase` 

## Scope markers (prefixes)

- Member data: `m_fooBar`
- Static class data: `s_fooBar`
- Globals: avoid; if unavoidable, `g_fooBar`

## Constants

- Prefer `constexpr`/`const` over macros.
- Name format: `kPascalCase` (e.g., `kDefaultTimeoutMs`, `kPi`).

## Booleans

- Start with verbs/adjectives: `isReady`, `hasItems`, `canRetry`, `shouldFlush`.

## Pointers and references

- No `p`/`ptr`/`ref` prefixes or suffixes.
- Name by ownership/meaning if helpful: `owner`, `target`, `outBuffer`.

## Containers and ranges

- Plural for collections: `users`, `points`, `taskIds`.
- For maps, reflect key/value: `usersById`, `countByCategory`.

## Indices, iterators, temps

- Short temps OK in tiny scopes: `i`, `j`, `it`.
- Otherwise be descriptive: `rowIndex`, `userIt`.

## Units and conventions

- Put units in names: `timeoutMs`, `sizeBytes`, `angleDeg`.
- Be consistent across the codebase.

## Integer and size types

- Prefer fixed-width integer types from <cstdint> for stored or serialized values and wire/data formats: `std::int32_t`, `std::uint16_t`, etc.
  - Do not use the ambiguous built-in types `short`, `long`, or `long long` in project code. These vary by platform.
  - `int`/`unsigned` are acceptable for local loop counters or APIs that are inherently int-based, but prefer fixed-width or `size_t` when representing data sizes or protocol fields.
- Use `size_t` for sizes and indexes of containers and buffers.
  - For pointer/iterator differences, use `std::ptrdiff_t`.
  - When converting between signed and unsigned (or different widths), do explicit, checked conversions at boundaries.
- Time durations should use `<chrono>` types (e.g., `std::chrono::milliseconds`) instead of raw integers with units, when practical.
- External API boundaries: When an operating system or third‑party API mandates a specific built‑in integer type (e.g., `unsigned long` for OpenSSL options), keep that usage localized to the boundary layer, document it with a comment, and convert to fixed‑width or domain types within our code.

Good:

```cpp
#include <cstdint>
#include <chrono>
#include <vector>

struct Header
{
    std::uint16_t packetId;     // wire format: exactly 16 bits
    std::uint32_t payloadSize;  // wire format: exactly 32 bits
};

std::vector<int> values;
for (size_t i = 0; i < values.size(); ++i)
{
    // index with size_t
}

void setKeepAlive(std::chrono::seconds interval)
{
    m_keepAlive = interval;
}
```

Bad:

```cpp
struct Header
{
    unsigned short packetId;   // ambiguous width; prefer std::uint16_t
    unsigned long payloadSize; // ambiguous; prefer std::uint32_t or size_t if an in-memory size
};

for (long long i = 0; i < count; ++i) // avoid long long; prefer size_t for sizes/indices
{
    // ...
}

void setKeepAliveMs(int timeoutMs) // prefer chrono types
{
    // ...
}
```

## Function parameters

- Use `lowerCamelCase`.
- Avoid leading underscores; use a trailing underscore only to resolve shadowing if needed.

## Avoid

- Type-encoding prefixes (`strName`, `iCount`, etc.).
- Double underscores or names starting with underscore + capital (reserved by C++).
- Cryptic abbreviations. Prefer `count` over `cnt`, `message` over `msg` unless ubiquitous.

## Examples

Good:

```cpp
class Session {
public:
    void setTimeoutMs(int timeoutMs);
    bool isActive() const;

private:
    int m_timeoutMs = 0;
    static int s_maxSessions;
};

constexpr int kDefaultPort = 8080;

std::vector<UserId> userIds;
std::unordered_map<UserId, User> usersById;

for (size_t i = 0; i < userIds.size(); ++i) {
    // ...
}
```

Bad:

```cpp
int mTimeout;          // not using agreed member prefix
int iCount;            // encodes type
bool flag;             // not descriptive
int timeout;           // missing units
std::vector<User> list;// not plural or specific
```

## Braces and control statements

- Brace style: BSD (Allman).
  - Opening brace goes on its own line, aligned with the statement/declaration it belongs to.
  - Closing brace goes on its own line, aligned with the start of the construct.
  - else / else if start on their own line (no '} else {').
- Always use braces for the bodies of all control statements, even if the body is a single statement.
  - Applies to: if/else/else if, for (incl. range-for), while, do/while, and switch bodies.
- Functions, classes/structs, and namespaces also use BSD (Allman) braces.

Good:

```cpp
if (isReady)
{
    doWork();
}
else if (shouldRetry)
{
    retry();
}
else
{
    logFailure();
}

for (int i = 0; i < count; ++i)
{
    process(i);
}

while (hasMore())
{
    step();
}

do
{
    step();
}
while (shouldContinue());

switch (state)
{
case State::Idle:
{
    onIdle();
    break;
}
case State::Busy:
{
    onBusy();
    break;
}
default:
{
    onUnknown();
    break;
}
}

class Session
{
public:
    void run();
};
```

Bad (not BSD braces or missing braces):

```cpp
if (isReady) {
    doWork();
} else
    retry(); // missing braces

for (auto& x : items) process(x); // missing braces

while (ok) { step(); }
```

Notes:

- Prefer a comment over an empty body. If you must use an empty body, still use braces:

```cpp
if (!enabled)
{
    // no-op
}
```

## Function and method naming

- Names must describe what the function returns or does.
  - If a function returns a value, the name should make that value clear.
  - For commands (functions that change state or cause effects), use imperative verb-based names.

### Boolean-returning functions

- Prefix with is/has/can/should (pick the most accurate):
  - isConnected(), hasPendingMessages(), canRetry(), shouldReconnect().
- Prefer affirmative names; avoid negatives like isNotReady(). Use !isReady() at call site instead.
- Avoid generic adjectives without a verb: ready() â†’ isReady().

### Value-returning query functions (no side effects)

- Use get... when retrieving/returning data without changing state: getClientId(), getRetryCount().
- Use find... when the result may not exist; return optional/pointer/nullable: findUserById(...).
  - For boolean success + out param, prefer try...: tryParseTopic(str, outTopic).
- Use compute/calculate/estimate when doing non-trivial work that derives a value: calculateBackoffMs().
- Do not use get... for operations that perform I/O, block, or mutate state; prefer load/fetch/query if they contact external systems: loadConfigFromDisk(), fetchRetainedMessage().

### Commands (state-changing, side effects)

- Use clear, imperative verbs: connect(), disconnect(), start(), stop(), enable(), disable(), addSubscription(), removeSubscription(), applySettings().
- Setters should be setX(...), doing straightforward assignment/validation only: setRetryCount(int count).
  - If additional work happens (I/O, expensive recompute), choose a more specific verb: updateRetryPolicy(...), reloadConfig().
- Avoid get... for commands (no hidden mutations in a get... function).

### Consistency and symmetry

- Maintain pairs consistently:
  - getX()/setX(), isX()/setX() or enableX()/disableX(), addX()/removeX().
- Prefer domain terms used elsewhere in the project and MQTT spec.

### Good examples

```cpp
class Client
{
public:
    bool isConnected() const;
    bool hasUnsentPackets() const;

    const FString& getClientId() const;
    int getRetryCount() const;
    TOptional<User> findUserById(UserId id) const;
    bool tryParseTopic(const FString& text, FTopic& outTopic) const;

    void connect();
    void disconnect();
    void setRetryCount(int count);
    void applySettings(const Settings& s);
};
```

### Bad examples (and preferred alternatives)

```cpp
class Client
{
public:
    bool connected() const;          // Prefer: isConnected()
    bool notReady() const;           // Prefer: isReady() and negate at call site

    FString clientId() const;        // Prefer: getClientId()
    bool getAndClearFlag();          // Side effect in getter; Prefer: consumeFlag() or clearFlag()

    void getConnection();            // Command named as getter; Prefer: connect()
    void process();                   // Ambiguous; Prefer a specific verb: flushOutgoing(), pollIncoming()
};
```