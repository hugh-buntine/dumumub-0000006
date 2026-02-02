# Quick test/dev
./run.sh

# Build for DAW testing
./build-plugins.sh

# Clean rebuild (if needed)
rm -rf build
cmake -B build -DCMAKE_BUILD_TYPE=Debug
./run.sh

# Create new component
./create-component.sh MyComponentName