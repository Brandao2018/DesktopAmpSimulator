#pragma once

#include <chrono>
#include <ctime>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>

// Simple thread-safe logger writing to stderr and a log file next to the
// executable's working directory. NOT for use on the audio thread (it locks
// and allocates) — audio-thread diagnostics go through atomics instead.

namespace ampsim
{
    class Logger
    {
    public:
        enum class Level { Info, Warning, Error };

        static Logger& instance()
        {
            static Logger logger;
            return logger;
        }

        void log(Level level, const std::string& message)
        {
            std::lock_guard<std::mutex> lock(mutex_);

            const std::string line = timestamp() + " [" + levelName(level) + "] " + message;

            std::cerr << line << std::endl;
            if (file_.is_open())
                file_ << line << std::endl;
        }

        static void info(const std::string& msg)    { instance().log(Level::Info, msg); }
        static void warning(const std::string& msg) { instance().log(Level::Warning, msg); }
        static void error(const std::string& msg)   { instance().log(Level::Error, msg); }

    private:
        Logger()
        {
            file_.open("DesktopAmpSimulator.log", std::ios::app);
        }

        static std::string timestamp()
        {
            const auto now = std::chrono::system_clock::now();
            const std::time_t t = std::chrono::system_clock::to_time_t(now);
            char buf[32] = { 0 };
        #if defined(_WIN32)
            std::tm tmBuf {};
            localtime_s(&tmBuf, &t);
            std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tmBuf);
        #else
            std::tm tmBuf {};
            localtime_r(&t, &tmBuf);
            std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tmBuf);
        #endif
            return buf;
        }

        static const char* levelName(Level level)
        {
            switch (level)
            {
                case Level::Info:    return "INFO";
                case Level::Warning: return "WARN";
                case Level::Error:   return "ERROR";
            }
            return "?";
        }

        std::mutex mutex_;
        std::ofstream file_;
    };
}
