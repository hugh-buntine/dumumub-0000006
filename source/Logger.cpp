#include "Logger.h"

Logger& Logger::getInstance()
{
    static Logger instance;
    return instance;
}

void Logger::initialize(const juce::String& logFileName, const juce::String& welcomeMessage)
{
    if (isInitialized)
    {
        logWarning("Logger already initialized. Skipping re-initialization.");
        return;
    }
    
    // Use a fixed path to the project logs directory
    // This works for both Standalone and AU/VST3 plugins
    auto logsDir = juce::File("/Users/hughbuntine/Desktop/DUMUMUB/DUMUMUB PLUGINS/dumumub-0000006/logs");
    
    // Create the directory if it doesn't exist
    if (!logsDir.exists())
        logsDir.createDirectory();
    
    // Create the log file
    logFile = logsDir.getChildFile(logFileName);
    
    // Delete the old log file to start fresh each time
    if (logFile.existsAsFile())
        logFile.deleteFile();
    
    // Create the FileLogger
    fileLogger = std::make_unique<juce::FileLogger>(logFile, welcomeMessage);
    
    isInitialized = true;
    
    // Log initialization
    if (fileLogger != nullptr)
    {
        fileLogger->logMessage("===========================================");
        fileLogger->logMessage("Logger initialized");
        fileLogger->logMessage("Log file: " + logFile.getFullPathName());
        fileLogger->logMessage("Timestamp: " + juce::Time::getCurrentTime().toString(true, true));
        fileLogger->logMessage("===========================================");
    }
}

void Logger::logMessage(const juce::String& message)
{
    if (fileLogger != nullptr && loggingEnabled)
    {
        fileLogger->logMessage(message);
    }
}

void Logger::logInfo(const juce::String& message)
{
    if (fileLogger != nullptr && loggingEnabled)
    {
        fileLogger->logMessage("[INFO] " + message);
    }
}

void Logger::logWarning(const juce::String& message)
{
    if (fileLogger != nullptr && loggingEnabled)
    {
        fileLogger->logMessage("[WARNING] " + message);
    }
}

void Logger::logError(const juce::String& message)
{
    if (fileLogger != nullptr && loggingEnabled)
    {
        fileLogger->logMessage("[ERROR] " + message);
    }
}

juce::String Logger::getLogFilePath() const
{
    return logFile.getFullPathName();
}

void Logger::setLoggingEnabled(bool enabled)
{
    loggingEnabled = enabled;
    if (fileLogger != nullptr)
    {
        if (enabled)
            fileLogger->logMessage("[INFO] Logging enabled");
        else
            fileLogger->logMessage("[INFO] Logging disabled");
    }
}

void Logger::shutdown()
{
    if (fileLogger != nullptr && isInitialized)
    {
        fileLogger->logMessage("===========================================");
        fileLogger->logMessage("Logger shutting down");
        fileLogger->logMessage("Timestamp: " + juce::Time::getCurrentTime().toString(true, true));
        fileLogger->logMessage("===========================================");
        fileLogger.reset();
        isInitialized = false;
    }
}

Logger::~Logger()
{
    shutdown();
}
