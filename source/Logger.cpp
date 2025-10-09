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
    
    // Get the current executable's directory and go up to find the project root
    auto executableFile = juce::File::getSpecialLocation(juce::File::currentExecutableFile);
    auto projectDir = executableFile.getParentDirectory() // MacOS folder
                        .getParentDirectory()              // Contents folder
                        .getParentDirectory()              // .app folder
                        .getParentDirectory()              // Standalone folder
                        .getParentDirectory()              // Debug/Release folder
                        .getParentDirectory()              // dumumub-0000006_artefacts folder
                        .getParentDirectory()              // build folder
                        .getParentDirectory();             // project root
    
    // Create logs directory in project root
    auto logsDir = projectDir.getChildFile("logs");
    
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
    if (fileLogger != nullptr)
    {
        fileLogger->logMessage(message);
    }
}

void Logger::logInfo(const juce::String& message)
{
    if (fileLogger != nullptr)
    {
        fileLogger->logMessage("[INFO] " + message);
    }
}

void Logger::logWarning(const juce::String& message)
{
    if (fileLogger != nullptr)
    {
        fileLogger->logMessage("[WARNING] " + message);
    }
}

void Logger::logError(const juce::String& message)
{
    if (fileLogger != nullptr)
    {
        fileLogger->logMessage("[ERROR] " + message);
    }
}

juce::String Logger::getLogFilePath() const
{
    return logFile.getFullPathName();
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
