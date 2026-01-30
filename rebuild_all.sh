set -e # Exit on error

echo "=== 0. Cleaning Build Directory ==="
rm -rf build

echo "=== 1. Configuring Project (CMake) ==="
# Ensure we are generating for Xcode or Ninja depending on what's available, 
# but usually just `cmake -B build` is enough to set up default generator.
/Applications/CMake.app/Contents/bin/cmake -B build -G "Xcode"

echo "=== 2. Compiling Source Code (Release Mode) ==="
# This is the critical step that was missing!
/Applications/CMake.app/Contents/bin/cmake --build build --config Release --parallel 4

echo "=== 3. Packaging Installer ==="
# Now that binaries are fresh, package them.
./build_installer.sh

echo "=== SUCCESS ==="
echo "New installer created at: Installer/output/Howling Wolves Installer.pkg"
echo "Please run this installer now."
