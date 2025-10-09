# Logger Documentation

## Overview

The dumumub-0000006 plugin now includes a file-based logging system using JUCE's `FileLogger` class. This logger writes messages to a text file for debugging and monitoring purposes.

## Log File Location

The log file is automatically created in the project's `logs` directory:

```
<project-root>/logs/dumumub-0000006.log
```

For example:
```
/Users/hughbuntine/Desktop/DUMUMUB/DUMUMUB PLUGINS/dumumub-0000006/logs/dumumub-0000006.log
```

The `logs` directory will be created automatically if it doesn't exist.

**Note:** The log file is cleared on each app startup, giving you a fresh log for each session.

## Usage

### Basic Logging

Include the Logger header in your source files:

```cpp
#include "Logger.h"
```

### Logging Methods

The Logger provides several methods for different log levels:

```cpp
// Log a general message
LOG_MESSAGE("This is a general message");

// Log an informational message
LOG_INFO("Plugin initialized successfully");

// Log a warning
LOG_WARNING("Buffer size is larger than expected");

// Log an error
LOG_ERROR("Failed to load preset file");
```

### Using the Logger Singleton Directly

If you prefer not to use the macros, you can access the Logger singleton directly:

```cpp
Logger::getInstance().logMessage("Direct message");
Logger::getInstance().logInfo("Direct info message");
Logger::getInstance().logWarning("Direct warning message");
Logger::getInstance().logError("Direct error message");
```

### Get Log File Path

To get the full path to the log file:

```cpp
juce::String logPath = Logger::getInstance().getLogFilePath();
```

## Initialization

The logger is automatically initialized when the `PluginProcessor` is constructed. You don't need to manually initialize it.

However, if you want to initialize it manually in another context:

```cpp
Logger::getInstance().initialize("custom-log-name.log", "Custom Welcome Message");
```

## Log Format

Log entries are automatically timestamped by JUCE's `FileLogger`. Each log level adds a prefix to help identify message types:

```
[INFO] Plugin initialized
[WARNING] Buffer size exceeds recommended limit
[ERROR] Failed to allocate memory
```

## Examples

### Example 1: Logging in Constructor

```cpp
MyClass::MyClass()
{
    LOG_INFO("MyClass constructed");
}
```

### Example 2: Logging with Variables

```cpp
void processAudio(int numSamples)
{
    LOG_INFO("Processing " + juce::String(numSamples) + " samples");
    
    if (numSamples > maxSamples)
    {
        LOG_WARNING("Sample count exceeds maximum: " + juce::String(numSamples));
    }
}
```

### Example 3: Error Handling

```cpp
bool loadFile(const juce::File& file)
{
    if (!file.existsAsFile())
    {
        LOG_ERROR("File does not exist: " + file.getFullPathName());
        return false;
    }
    
    LOG_INFO("Successfully loaded file: " + file.getFileName());
    return true;
}
```

## Performance Considerations

- The logger writes to disk synchronously, so frequent logging in the audio thread should be avoided
- For audio processing, consider logging only in `prepareToPlay`, `releaseResources`, or error conditions
- Use sparingly in the `processBlock` method to avoid performance issues

## Thread Safety

JUCE's `FileLogger` is thread-safe, so you can safely log from multiple threads including the audio thread. However, excessive logging from the audio thread may cause performance issues.

## Cleanup

The logger automatically shuts down and closes the log file when the plugin is destroyed. You don't need to manually clean up.

If you need to manually shut down the logger:

```cpp
Logger::getInstance().shutdown();
```

## Current Integration

The logger is currently integrated into:

- **PluginProcessor**: Logs construction, destruction, `prepareToPlay`, and editor creation
- **Canvas**: Ready for logging (include added)

You can add logging to any other components by including `Logger.h` and using the logging macros.
