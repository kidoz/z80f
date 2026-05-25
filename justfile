set shell := ["bash", "-cu"]

build_dir := "build"

# Default: print available recipes.
default:
    @just --list

# Configure the Meson build directory. Idempotent.
setup:
    meson setup {{build_dir}} --reconfigure -Dcatch2:tests=false

# Configure once if missing; otherwise no-op.
setup-if-missing:
    @if [ ! -d "{{build_dir}}" ]; then meson setup {{build_dir}} -Dcatch2:tests=false; fi

# Build everything.
build: setup-if-missing
    meson compile -C {{build_dir}}

# Run the test suite.
test: build
    meson test -C {{build_dir}} --print-errorlogs

# Run the host integration examples.
run-spectrum: build
    {{build_dir}}/zx-spectrum-host-example

run-megadrive: build
    {{build_dir}}/megadrive-sound-z80-host-example

# Clean the build directory.
clean:
    rm -rf {{build_dir}}

# Format every C++ source / header in-place.
fmt:
    @echo "Running clang-format..."
    @find include src tests examples -type f \( -name '*.cpp' -o -name '*.hpp' -o -name '*.h' \) \
        -print0 | xargs -0 clang-format -i

# Check formatting without modifying files. Exits non-zero if anything would change.
fmt-check:
    @echo "Checking clang-format..."
    @find include src tests examples -type f \( -name '*.cpp' -o -name '*.hpp' -o -name '*.h' \) \
        -print0 | xargs -0 clang-format --dry-run --Werror

# macOS needs an explicit sysroot so homebrew clang-tidy finds Apple's libc++.
sdkroot := if `uname` == "Darwin" { `xcrun --show-sdk-path` } else { "" }
tidy_extra_args := if `uname` == "Darwin" { "--extra-arg=-isysroot --extra-arg=" + sdkroot } else { "" }

# Run clang-tidy against the project sources using compile_commands.json.
tidy: setup-if-missing
    @echo "Running clang-tidy..."
    @find src tests examples -type f -name '*.cpp' \
        -print0 | xargs -0 -n1 clang-tidy {{tidy_extra_args}} -p {{build_dir}}

# Apply clang-tidy fixes where it can suggest them.
tidy-fix: setup-if-missing
    @find src tests examples -type f -name '*.cpp' \
        -print0 | xargs -0 -n1 clang-tidy {{tidy_extra_args}} -p {{build_dir}} --fix

# Run formatting + linting + tests in one shot.
check: fmt-check tidy test

# Install the library to the default Meson prefix (usually /usr/local).
install: build
    meson install -C {{build_dir}}
