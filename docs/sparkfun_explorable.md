# SparkFun LoRa Thing Plus - expLoRaBLE

This is an **unofficial, experimental USB Companion port** for the SparkFun
LoRa Thing Plus - expLoRaBLE. It is based on upstream MeshCore firmware
`companion-v1.18.0` and targets the board's Apollo3 MCU, SX1262 radio, and
CH340 USB-serial bridge. It does not provide Bluetooth Companion support.

The port is for development and testing, not a released or officially
supported MeshCore target. The successful tests listed here are from recorded
project runs; the build and hardware tests were not repeated while preparing
these publication notes.

## Hardware and board configuration

- SparkFun LoRa Thing Plus - expLoRaBLE
- Apollo3 MCU; SX1262 radio; CH340 USB-serial bridge
- Radio uses SPI1 with NSS 36, DIO1 40, reset 44, and BUSY 39
- USB Companion serial: 115200 baud, 8N1, no flow control
- The RTS solder jumper was visually inspected and found open

The board adapter initializes its I2C bus, exposes the SparkFun model name,
and supplies the SX1262 wiring to the RadioLib wrapper. Apollo3 settings and
filesystem integration are confined to the new board variant and
`APOLLO3_PLATFORM` build path. The current build settings are 915.0 MHz,
250 kHz bandwidth, SF10, coding-rate setting 5, and 20 dBm transmit power.
These are the values reported in the recorded runtime test, not a universal
regional preset or evidence of regulatory compliance.

## Port changes relative to upstream

The source snapshot matches upstream tag `companion-v1.18.0` (commit
`7d6d255104eadbfccd85ceae4ccbc14133946f7e`). The upstream history and
`license.txt` are retained. The port changes are:

- Added `platformio.apollo3.ini`, `scripts/apollo3_compat.py`,
  `src/helpers/apollo3/Apollo3FileSystem.h`, and
  `variants/sparkfun_explorable/{SparkFunExplorableBoard.cpp,SparkFunExplorableBoard.h,target.cpp,target.h}`.
- Added Apollo3 filesystem setup in `examples/companion_radio/main.cpp`,
  Apollo3 file modes and capacity reporting in
  `examples/companion_radio/DataStore.cpp` and
  `src/helpers/IdentityStore.cpp`, and Apollo3 filesystem selection in
  `src/helpers/IdentityStore.h`.
- Changed `examples/companion_radio/MyMesh.cpp` and `MyMesh.h` and
  `examples/companion_radio/main.cpp` to use source-relative MeshCore includes
  for the PlatformIO target. In `MyMesh::loop()`, serial handling now runs
  before `BaseChatMesh::loop()`. This is a candidate behavior change shared by
  Companion builds; the test record does not isolate it as the cause of the
  earlier startup-handshake failures.
- Changed shared helper includes to source-relative paths in
  `src/Mesh.h`, `src/helpers/AbstractBridge.h`,
  `src/helpers/AdvertDataHelpers.h`, `src/helpers/ArduinoHelpers.h`,
  `src/helpers/AutoDiscoverRTCClock.h`, `src/helpers/BaseChatMesh.h`,
  `src/helpers/ChannelDetails.h`, `src/helpers/ClientACL.h`,
  `src/helpers/CommonCLI.h`, `src/helpers/ContactInfo.h`,
  `src/helpers/SimpleMeshTables.h`, `src/helpers/StatsFormatHelper.h`,
  `src/helpers/radiolib/RadioLibWrappers.h`,
  `src/helpers/sensors/EnvironmentSensorManager.h`, and
  `src/helpers/sensors/LocationProvider.h`.
- Changed Crypto include paths in `src/Packet.cpp`, `src/Utils.cpp`,
  `src/helpers/RegionMap.cpp`, and `src/helpers/TransportKeyStore.cpp`.
  The `RegionMap.cpp` output rewrite preserves the existing text format while
  avoiding a `printf` call unsupported by the Apollo3 stream implementation.

The helper changes above are shared source files, not a separate fork of their
behavior. Apollo3-specific file operations are guarded by
`APOLLO3_PLATFORM`; the stream-format rewrite emits the same text on existing
targets. The target build below validates the Apollo3 path, but no full
cross-board test matrix is claimed.

## Dependencies and reproducible build preparation

Use PlatformIO Core **6.2.0**. The Apollo3 platform and framework are pinned to
the revisions used for the recorded build; direct PlatformIO library
dependencies are version-pinned in `platformio.apollo3.ini`.

| Component | Revision used |
|---|---|
| `platform-apollo3blue` | `a4872991c2746c7e1d775cc2bfc5bdf3da0beb89` |
| SparkFun `framework-arduinoapollo3` | `771ce8fe14b54425775bb48c777709bf28eb9100` (2.2.0) |
| RadioLib | `6d8934836678d8894e3d556550475b37dce3e2b6` (7.7.1) |
| Crypto | 0.4.0 |
| Adafruit BusIO | 1.17.4 |
| RTClib | 2.1.4 |
| Melopero RV3028 | 1.2.0 |
| CayenneLPP | 1.6.1 |
| base64 | 1.4.0 |
| ArduinoJson | 7.4.3 |
| Apollo3 ARM toolchain | 1.90201.191206 |

`SPI` and `Wire` come from the Arduino framework. PlatformIO downloads the
pinned platform, framework, and library packages into its package storage;
the build does not require hand-copied variant files or a pre-existing
project `.pio` cache. The SparkFun board definition comes from the pinned
Apollo3 platform package. The corresponding `LoRa_THING_PLUS_expLoRaBLE`
variant, including its Mbed support files, comes from the pinned Arduino
Apollo3 framework package. PlatformIO creates object files and other build
outputs under `.pio`; those generated files are not project source and are
not to be committed.

Before each build, `scripts/apollo3_compat.py` checks the freshly resolved
Adafruit BusIO source and applies the required Apollo3 `uint8_t` overload
cast to its I2C probe call. The edit is confined to PlatformIO's downloaded
library copy, is safe to repeat, and fails the build if the expected source is
missing or has an unrecognized implementation. This makes the compatibility
adjustment explicit rather than relying on a previously patched `.pio` copy.

## Build and upload

From the project root, build the USB Companion environment:

```sh
pio run -c platformio.apollo3.ini -e SparkFun_expLoRaBLE_companion_usb
```

To upload, connect the board and use its SVL bootloader. Replace `COM6` with
the board's serial port on your system:

```sh
pio run -c platformio.apollo3.ini -e SparkFun_expLoRaBLE_companion_usb -t upload --upload-port COM6
```

On Linux, use the actual device path, for example `/dev/ttyUSB0`. Follow the
board/bootloader instructions if the SVL uploader asks for bootloader mode.
No firmware image is included in this source project.

Recorded build result with PlatformIO Core 6.2.0:

- RAM: 125,584 / 393,216 bytes
- Flash: 318,132 / 843,776 bytes
- The configured upload protocol is `svl`.

The installed Apollo3 framework reserves the final 128 KiB of internal flash
for LittleFS and keeps the final EEPROM-emulation page reserved. The image
limit in this environment is 843,776 bytes.

## USB Companion verification

The USB connection uses the binary MeshCore Companion protocol; it is not a
text CLI or KISS modem. The stock client opens the serial port at 115200 baud
and sends `CMD_APP_START` (`0x01`). A successful startup returns a framed
`SELF_INFO` response (`0x05`). The normal client handshake succeeded without
an added delay or monkey patch in a recorded Raspberry Pi test.

Recorded verification:

- A direct serial test after flashing received a complete `SELF_INFO` on
  three consecutive immediate opens, in approximately 0.10-0.11 seconds per
  open. An earlier five-open sample had one incomplete response, so this is
  not enough to establish reliable repeated-open behavior.
- The unmodified MeshCore Python client connected on its first attempt without
  an added delay or monkey patch.
- Runtime `SELF_INFO` reported 915.0 MHz, 250 kHz bandwidth, SF10, coding-rate
  setting 5, and 20 dBm transmit power.
- MeshPoint v0.7.9 identified firmware v1.18.0 as SparkFun expLoRaBLE,
  reported `MT=True MC=True`, and logged `MeshCore advert sent: command_ok`.
  `command_ok` confirms command acceptance, not reception by another node.

Earlier failing tests and a candidate firmware loop-order change were not
isolated from the separate MeshPoint serial-source configuration conflict.
An early raw test also observed a lone `fe` byte whose cause remains unknown.
Do not treat either observation as proof of a reset or of the root cause of
the initial handshake failure.

## Raspberry Pi and MeshPoint

Use a stable serial path when available, such as a `/dev/serial/by-id/` link,
and make sure only one process owns the USB serial port. A recorded CH340
device appeared as `/dev/ttyUSB0`. Linux device names are case-sensitive.

The following is a **partial example** to merge into the existing MeshPoint
configuration; it is not a replacement for the entire configuration:

```yaml
capture:
  sources:
    - concentrator
    - meshcore_usb
  concentrator_spi_device: /dev/spidev0.0
  meshcore_usb:
    serial_port: /dev/ttyUSB0
    baud_rate: 115200
    auto_detect: false
```

Do not also configure a Meshtastic serial source on the same `/dev/ttyUSB0`
device. One recorded MeshPoint failure was resolved by removing that duplicate
serial assignment; it was a separate configuration issue from the firmware
handshake behavior.

## Troubleshooting

- **Port opens but no Companion response:** confirm the selected device is the
  CH340 adapter, the baud rate is 115200, and no other application owns the
  port. Check that the client sends the Companion `CMD_APP_START` packet; a
  LoRa frequency change cannot fix a serial framing or port-selection issue.
  The successful stock-client test needed no added startup delay.
- **MeshPoint reports `MC=False` or fails after opening the port:** check for
  a competing serial source in MeshPoint's configuration, restart MeshPoint
  after editing the config, and review its serial logs.
- **Upload does not start:** use the configured SVL protocol, the correct
  serial port, and the board's documented bootloader procedure.
- **No messages from other LoRa nodes:** verify compatible regional
  configuration and antenna/radio setup. Another node's traffic or an
  accepted advertisement command does not prove that this board has exchanged
  MeshCore packets over the air.

## Known limitations and remaining validation

- No over-the-air exchange with another MeshCore node has been verified.
- Repeated cold starts, repeated stock-client opens as an acceptance series,
  and long-term reliability remain unverified.
- MeshPoint's successful application integration was recorded, but repeated
  MeshPoint starts and its normal DTR/recovery behavior have not been
  independently validated here.
- The effect of moving Companion serial handling before radio/mesh processing
  was not isolated as the cause of the initial startup issue.
- The cause of the early lone `fe` byte is unknown.
- This is a USB-only Companion port; Bluetooth Companion support is not
  implemented or tested.
- The tested radio settings are not a regional preset or a regulatory
  compliance determination.

Before treating the port as reliable, repeat the stock-client connection over
multiple opens without delays or monkey patches, test a separately recorded
cold power-up, exercise MeshPoint through restart/recovery, and verify
two-way over-the-air MeshCore traffic with another node.
