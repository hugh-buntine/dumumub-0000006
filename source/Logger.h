#pragma once

#include <juce_core/juce_core.h>
#include <memory>

/**
 * @brief Singleton logger class that writes to a text file
 * 
 * This class provides a simple interface for logging messages to a file.
 * The log file is created in the user's application data directory.
 */
class Logger
{
public:
    /**
     * @brief Get the singleton instance of the logger
     * @return Reference to the Logger instance
     */
    static Logger& getInstance();
    
    /**
     * @brief Initialize the logger with a specific log file name
     * @param logFileName The name of the log file (without path)
     * @param welcomeMessage Optional welcome message to write when logger is initialized
     */
    void initialize(const juce::String& logFileName = "dumumub-0000006.log",
                   const juce::String& welcomeMessage = "");
    
    /**
     * @brief Log a message to the file
     * @param message The message to log
     */
    void logMessage(const juce::String& message);
    
    /**
     * @brief Log an informational message
     * @param message The message to log
     */
    void logInfo(const juce::String& message);
    
    /**
     * @brief Log a warning message
     * @param message The message to log
     */
    void logWarning(const juce::String& message);
    
    /**
     * @brief Log an error message
     * @param message The message to log
     */
    void logError(const juce::String& message);
    
    /**
     * @brief Get the path to the log file
     * @return The full path to the log file
     */
    juce::String getLogFilePath() const;
    
    /**
     * @brief Shutdown the logger and close the file
     */
    void shutdown();
    
    // Delete copy constructor and assignment operator
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
private:
    Logger() = default;
    ~Logger();
    
    std::unique_ptr<juce::FileLogger> fileLogger;
    juce::File logFile;
    bool isInitialized = false;
};

// Convenience macros for logging
#define LOG_MESSAGE(msg) Logger::getInstance().logMessage(msg)
#define LOG_INFO(msg) Logger::getInstance().logInfo(msg)
#define LOG_WARNING(msg) Logger::getInstance().logWarning(msg)
#define LOG_ERROR(msg) Logger::getInstance().logError(msg)
