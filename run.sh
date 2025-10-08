#!/bin/bash
# Build and run standalone plugin

echo "🔨 Building standalone plugin..."
cmake --build build --config Debug --target dumumub-0000006_Standalone -- -j8

if [ $? -eq 0 ]; then
    echo "✅ Build successful! Launching standalone app..."
    open "build/dumumub-0000006_artefacts/Debug/Standalone/dumumub-0000006.app"
else
    echo "❌ Build failed!"
    exit 1
fi
