#!/usr/bin/env sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p build
cc -std=c11 -Wall -Wextra -Werror -pedantic -Isrc src/app.c src/pak.c tests/app_test.c -o build/app-test
./build/app-test
cc -std=c11 -Wall -Wextra -Werror -pedantic -Isrc src/app.c src/pak.c src/tone.c tests/utilities_test.c -lm -o build/utilities-test
./build/utilities-test
cc -std=c11 -Wall -Wextra -Werror -pedantic -Isrc src/app.c src/pak.c tests/pak_test.c -o build/pak-test
./build/pak-test
cc -std=c11 -Wall -Wextra -Werror -pedantic -Isrc src/app.c src/pak.c src/pak_write.c tests/pak_write_test.c -o build/pak-write-test
./build/pak-write-test
cc -std=c11 -Wall -Wextra -Werror -pedantic -Isrc src/app.c src/pak.c src/ui.c tools/preview.c -lm -o build/preview
for state in menu test disconnected all switch rumble rumble-missing audio pak pak-details pak-empty pak-error pak-reading pak-last pak-invalid pak-format; do
    ./build/preview "$state" > "build/preview-$state.svg"
done
