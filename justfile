# Build the ROM with the pinned libdragon toolchain.
build:
    ./tools/build-rom.sh -j4

# Build, then copy onto the connected SummerCart64's SD card.
# Power off the N64 first so it releases its SD-card lock.
deploy: build
    sc64deployer sd upload n64-util.z64 /CUSTOM/n64-util.z64

# Run portable input-state checks and generate layout previews.
check:
    ./tools/check.sh

# Remove generated ROM and build artifacts.
clean:
    ./tools/build-rom.sh clean
