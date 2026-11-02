Import("env")

import os


source_path = os.path.join(
    env.subst("$PROJECT_DIR"),
    ".pio",
    "libdeps",
    env.subst("$PIOENV"),
    "Adafruit BusIO",
    "Adafruit_I2CDevice.cpp",
)
if not os.path.isfile(source_path):
    raise RuntimeError("Adafruit BusIO source is missing: " + source_path)

with open(source_path, "r") as source_file:
    contents = source_file.read()

incompatible = "_wire->write(0); // forces a write request instead of a read"
compatible = "_wire->write((uint8_t)0); // forces a write request instead of a read"
if incompatible in contents:
    contents = contents.replace(incompatible, compatible)
    with open(source_path, "w") as source_file:
        source_file.write(contents)
    print("Applied Apollo3 Wire overload compatibility fix to Adafruit BusIO")
elif compatible not in contents:
    raise RuntimeError("Unrecognized Adafruit BusIO detected() implementation")
