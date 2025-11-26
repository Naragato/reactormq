# ReactorMQ

One codebase, three shapes. ReactorMQ is a modern C++ MQTT client that can be built as:

- a standalone library with CMake,
- a UE5 module with UBT, and
- an O3DE Gem.

## Status (please read first)

ReactorMQ is not ready for real use yet. The state machine and the reactor core are untested. Expect the surface to shift, and expect it to differ from MQTTIFY until the internal model settles into something cleaner and more honest.

Where this began (credit where it’s due):

This project started as **Mqttify** for Unreal Engine:  
https://github.com/Naragato/Mqttify

MQTTIFY’s original goals were simple and practical:
- A UE-native MQTT client with a small C++ API and Blueprint nodes.
- A non-blocking, event-driven usage style that behaves well with the game tick.
- Protocol and socket details tucked away behind a surface that feels natural in UE.
- Sensible defaults; explicit control when it matters.
- Reliability first: keepalive, ping, reconnection, backoff that behaves.
- Topic and payload handling that respects UTF-8 and size rules.
- Built-in support for QoS 0/1/2.
- Minimal dependencies and packaging that feels at home in Unreal.
- Support for `ws://`, `wss://`, `mqtt://`, and `mqtts://`.

ReactorMQ carries those ideas forward but widens the scope: a single codebase that can serve standalone C++, UE5, and O3DE without forking or drifting.

### A note on the UE5/O3DE socket scaffolding

To get early structure in place, the first UE5 and O3DE socket layers were produced with LLM assistance:

- UE5: a direct port of the MQTTIFY socket code.
- O3DE: a first attempt at an adapter over O3DE’s socket APIs.

They are untested at this stage. Their role is mainly to define boundaries, compile-time toggles, and directory layout.

## Why one codebase?

Because maintaining three almost-identical clients is a reliable way to encourage bugs. ReactorMQ centralizes everything that should be shared-packet handling, validation, cross-platform I/O-into a single, well-tested core. Each runtime gets only a thin adapter:

- **Standalone C++**: a light, normal library for tools, servers, and desktop apps.
- **Unreal Engine 5**: a UBT module (and later, a plugin) providing a UE-style API. No socket juggling.
- **O3DE**: a Gem that fits naturally into O3DE’s event and lifecycle model.

## Compatibility at a glance

Runtime shapes:

- **Standalone C++ (CMake)**
    - Target compilers: MSVC 2022, Clang 15+, GCC 11+ (active testing ongoing)
    - Library type: static by default, shared optional

- **Unreal Engine 5 (UBT)**
    - Target: UE5.x

- **Open 3D Engine (Gem)**
    - Target: O3DE 23.x / 24.x

## Operating systems (intent)

- Windows family (including Xbox) - primary development platform today
- Linux (x64) - untested
- macOS (x64/arm64) - untested
- PlayStation - untested (will need a dev kit)
- Nintendo Switch - untested (will need a dev kit)

The aim is broad support; each row will be marked officially once it is verified.

## Repository tour

- `include/reactormq/...` - public headers for the core
- `src/mqtt/...` - MQTT packets, properties, wire protocol
- `src/serialize/...` - serialization helpers
- `src/socket/native/...` - standalone socket implementation
- `src/socket/ue5/...` - UE5 adapters + tests
- `src/socket/o3de/...` - O3DE adapter
- `tests/...` - GoogleTest unit and integration tests for the core

## Testing

Two testing layers exist: GoogleTest for the portable core, and Unreal Automation Tests for UE5. (O3DE tests will be added once the Gem stabilizes.)

### GoogleTest (standalone/core)

Tests live under `tests/unit` and `tests/integration`.

Build and run:
```

mkdir build && cmake -S . -B build -DREACTORMQ_BUILD_TESTS=ON
cmake --build build --config Debug
ctest --test-dir build -C Debug -V

```

### Unreal Automation Tests (UE5)

Tests live beside the UE5 adapters (`src/socket/ue5/tests`). These are early smoke tests and will expand.

To run:
```

<UE5Editor> <YourProject>.uproject -ExecCmds="Automation RunTests ReactorMQ; Quit" -unattended -nop4 -nopause

```

Names will settle as the UE5 API stabilizes.

## O3DE

The Gem exists and is evolving. Test scaffolding will come once the reactor core is online.

## Building

### Standalone (CMake)

```

mkdir build
cmake -S . -B build -DREACTORMQ_BUILD_TESTS=ON
cmake --build build --config Release

````

### UE5 (UBT)

> **Status:** Experimental / WIP. The UE5 integration is further along than O3DE but still **untested end-to-end**.

Treat the `unreal_engine` folder in this repo as a module you can add to your UE5 project:

1. Symlink the `unreal_engine/` folder from this repo into your project’s `Plugin` tree (or add it as an external module path).
2. Add `ReactorMQ` as a dependency in your module’s `.Build.cs`.
3. Run the debug helper script from the repo root to wire native sources into the UE layout:
   - On Windows (PowerShell / cmd): `scripts\setup-unreal-debug.bat`
   - On Unix-like systems: `scripts/setup-unreal-debug.sh`
4. Regenerate project files and build in the editor.

Expect breaks: the public surface is still moving, and the UE-side socket layer has not been validated beyond compilation.

### O3DE (Gem)

> **Status:** Early scaffolding / WIP. The O3DE integration is **less mature and also untested**.

The O3DE shape is currently a skeletal Gem layout, suitable for experimentation only:

1. Point your O3DE engine/project to this repo so it can see the `o3de` folder as a Gem.
2. Enable the Gem in your project settings.
3. Optionally, for local iteration against the latest core sources, run from the repo root:
   - On Windows (PowerShell / cmd): `scripts\setup-o3de-debug.bat`
   - On Unix-like systems: `scripts/setup-o3de-debug.sh`

APIs, folder layout, and activation steps may change; treat this as a preview rather than a supported integration.

## Threading and Callback Marshalling

ReactorMQ gives you a choice: let callbacks run directly on the reactor thread (the default), or provide an executor that hands them off to another thread or system. Engines and custom thread pools can use this to ensure callbacks land where they belong.

### Default behavior (no marshalling)

```cpp
#include <reactormq/mqtt/client.h>
#include <reactormq/mqtt/connection_settings_builder.h>
#include <mqtt/client/client_factory.h>

using namespace reactormq::mqtt;

auto settings = ConnectionSettingsBuilder()
    .setHost("broker.example.com")
    .setPort(1883)
    .build();

auto client = reactormq::mqtt::client::createClient(settings);

client->onMessage().add([](const Message& msg) {
    std::cout << "Topic: " << msg.getTopic() << std::endl;
});

while (running)
{
    client->tick();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
}
````

### Unreal Engine 5 integration

```cpp
#include <reactormq/mqtt/client.h>
#include <reactormq/mqtt/connection_settings_builder.h>
#include <mqtt/client/client_factory.h>
#include <Async/Async.h>

using namespace reactormq::mqtt;

auto settings = ConnectionSettingsBuilder()
    .setHost("broker.example.com")
    .setPort(1883)
    .setCallbackExecutor([](std::function<void()> callback) {
        AsyncTask(ENamedThreads::GameThread, [cb = std::move(callback)]()
        {
            cb();
        });
    })
    .build();

auto client = reactormq::mqtt::client::createClient(settings);

client->onMessage().add([this](const Message& msg)
{
    UE_LOG(LogTemp, Log, TEXT("Received: %s"), *FString(msg.getTopic().c_str()));

    if (AActor* actor = GetWorld()->SpawnActor<AMyActor>())
    {
        actor->ProcessMessage(msg);
    }
});

void Tick(float DeltaTime) override
{
    client->tick();
}
```

### Custom thread pool

```cpp
// ... (unchanged code block; wording preserved as provided)
```

## Using `reactormq::mqtt::Message`

The `Message` type represents an MQTT application message: immutable topic, payload, retain flag, QoS, and a UTC timestamp.

Header:

```
#include <reactormq/mqtt/message.h>
```

Constructors:

* Move topic and payload:

```cpp
Message::Payload payload{ 'h','e','l','l','o' };
Message m1("sensors/temp", std::move(payload), false, QualityOfService::AtLeastOnce);
```

* Copy topic and payload:

```cpp
const std::string topic = "alerts/door";
const Message::Payload data{ 1,2,3 };
Message m2(topic, data, true, QualityOfService::AtMostOnce);
```

* Move topic and copy from span:

```cpp
const std::string text = "OK";
auto* bytes = reinterpret_cast<const std::uint8_t*>(text.data());
std::span<const std::uint8_t> view(bytes, text.size());
Message m3("status", view, false, QualityOfService::ExactlyOnce);
```

Accessors:

```cpp
auto when = m1.getTimestampUtc();
auto& t = m1.getTopic();
auto viewBytes = m1.getPayloadView();
bool retain = m1.shouldRetain();
auto qos = m1.getQualityOfService();
```

Publishing:

```cpp
auto settings = ConnectionSettingsBuilder("127.0.0.1")
    .setPort(1883)
    .setProtocol(ConnectionProtocol::Tcp)
    .setCredentialsProvider(std::make_shared<NoCredsProvider>())
    .build();

auto client = reactormq::mqtt::client::createClient(settings);
auto fut = client->connectAsync(true);

// ... tick until ready ...

Message msg("reactormq/demo", Message::Payload{ 'o','k' }, false, QualityOfService::AtLeastOnce);
auto pub = client->publishAsync(std::move(msg));
```

### Key points

* **Callbacks run directly** on the reactor thread unless you say otherwise.
* **Thread-pool agnostic**: any executor that accepts a `std::function<void()>` works.
* **Lifetime rules matter**: whatever the executor captures must outlive the client.

## Relationship to MQTTIFY

The project began as MQTTIFY (also written by me), which served UE5 only.
ReactorMQ carries those same goals-simple surface, predictable behavior, reliable delivery-but generalizes them for standalone C++, UE5, and O3DE, without maintaining three separate implementations.

## Roadmap (high-level)

1. Establish a stable socket abstraction (native + UE5 + O3DE)
2. Lock down the public headers under `include/reactormq`
3. Stabilize the UE5 surface and introduce Blueprint nodes
4. Wire up O3DE events and publish a first Gem release

## Caveats and honesty

This is early work. The shape is still changing. Tests will fail, then improve. If you are choosing technology for production today, ReactorMQ isn’t ready. If you want a cross-engine MQTT client tomorrow, and you’re willing to help it take form, that is exactly the purpose of this repository.

# MQTT

### MQTT Packet Types and Acknowledgments

| MQTT Packet | MQTT Version | QoS 0 Expected Acknowledgment | QoS 1 Expected Acknowledgment | QoS 2 Expected Acknowledgment |
|-------------|--------------|-------------------------------|-------------------------------|-------------------------------|
| CONNECT     | 3.1.1 & 5    | CONNACK                       | CONNACK                       | CONNACK                       |
| CONNACK     | 3.1.1 & 5    | N/A                           | N/A                           | N/A                           |
| PUBLISH     | 3.1.1 & 5    | None                          | PUBACK                        | PUBREC                        |
| PUBACK      | 3.1.1 & 5    | N/A                           | None                          | N/A                           |
| PUBREC      | 3.1.1 & 5    | N/A                           | N/A                           | PUBREL                        |
| PUBREL      | 3.1.1 & 5    | N/A                           | N/A                           | PUBCOMP                       |
| PUBCOMP     | 3.1.1 & 5    | N/A                           | N/A                           | None                          |
| SUBSCRIBE   | 3.1.1 & 5    | SUBACK                        | SUBACK                        | SUBACK                        |
| SUBACK      | 3.1.1 & 5    | N/A                           | N/A                           | N/A                           |
| UNSUBSCRIBE | 3.1.1 & 5    | N/A                           | N/A                           | N/A                           |
| UNSUBACK    | 3.1.1 & 5    | UNSUBACK                      | UNSUBACK                      | UNSUBACK                      |
| PINGREQ     | 3.1.1 & 5    | PINGRESP                      | PINGRESP                      | PINGRESP                      |
| PINGRESP    | 3.1.1 & 5    | N/A                           | N/A                           | N/A                           |
| DISCONNECT  | 3.1.1 & 5    | None                          | None                          | None                          |
| AUTH        | 5            | None                          | None                          | None                          |

### Quality of Service (QoS)

MQTT offers three QoS levels:

* **QoS 0 (At most once)** - fire-and-forget.
* **QoS 1 (At least once)** - confirmation required.
* **QoS 2 (Exactly once)** - a careful four-step handshake.

#### QoS 2 Publish Handshake

| Step | Sender        | Packet  | Receiver      | Description                                           |
|------|---------------|---------|---------------|-------------------------------------------------------|
| 1    | Publisher     | PUBLISH | Broker        | Publisher sends the message with QoS 2 to the broker. |
| 2    | Broker        | PUBREC  | Publisher     | Broker acknowledges receipt with PUBREC.              |
| 3    | Publisher     | PUBREL  | Broker        | Publisher replies with PUBREL.                        |
| 4    | Broker        | PUBCOMP | Publisher     | Broker confirms with PUBCOMP; handshake complete.     |
| 5    | Broker        | PUBLISH | Subscriber(s) | Broker forwards the message to subscribers.           |
| 6    | Subscriber(s) | PUBREC  | Broker        | Subscribers acknowledge with PUBREC.                  |
| 7    | Broker        | PUBREL  | Subscriber(s) | Broker responds with PUBREL.                          |
| 8    | Subscriber(s) | PUBCOMP | Broker        | Subscribers confirm with PUBCOMP.                     |

For deeper detail:

* MQTT 5.0
* MQTT 3.1.1

## License

Licensed under the Mozilla Public License 2.0 (MPL-2.0). see the [LICENSE](LICENSE) file for details.

## Contributing

Issues and PRs are welcome-especially around platform quirks, engine lifecycles, and tests. Clear and focused changes are appreciated.

---

Developed by (https://github.com/naragato | ETH: 0xf7f210e645859B13edde6b99BECACf89fbEF80bA | BTC:  33FWb8ddqJMmWdrbQmDcLND3aNa8G3hiH7). For any issues, please report them [here](https://github.com/Naragato/reactormq/issues).
