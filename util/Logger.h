#ifndef _Logger_h_
#define _Logger_h_

#include <boost/log/sources/severity_channel_logger.hpp>
#include <boost/log/expressions/keyword.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/signals2/signal.hpp>

#ifdef FREEORION_WIN32
// Note: The is a workaround for Visual C++ non-conformant pre-processor
// handling of empty macro arguments.
// https://msdn.microsoft.com/en-us/library/hh567368.aspx
// https://blogs.msdn.microsoft.com/vcblog/2017/03/07/c-standards-conformance-from-microsoft/
#include <boost/preprocessor/control/if.hpp>
#include <boost/preprocessor/facilities/is_empty_variadic.hpp>
#endif

#include <string>
#include <unordered_map>

#include "Export.h"


/** \file
    \brief The logging system consists of named loggers with levels.

    The system is a thin wrapper around boost::log.

    The logging system is composed of a sink which writes to the log files and
    sources, called loggers, which collect the log information while the
    application is running.

    Logs collected by a loggers are filtered by logger name and log threshold level.

    Loggers names are arbitrary. For example "combat" is the name of the combat system's logger.

    Each application has a single unnamed or default logger.  The unnamed logger is given an
    identifier for display purposes when the output file is initialized in InitLoggingSystem().

    Both the sinks and the sources use debug levels (trace, debug, info, warn and error) as
    thresholds to filter which log records are generated at the sources and which log records are
    consumed by the sinks. Logs that are filtered out at either the sink or the source are not
    generated by the source.

    The intended uses of the levels are:
    error - used for "major" unrecoverable errors which will affect game play.  Error level issues
            need to be fixed.  Error level will probably not be turned off unless they are flooding
            the logs.
            Examples are: the game is about to crash, a string is missing from the stringtable, etc.
    warn  - used for "minor", recoverable errors that will not affect game play but do indicate
            a problem.
            For example a missing id that can be ignored, an extra item in a container.
          - used to indicate non critcal degraded system state to the player.  Warnings may be
            unavoidable due to system issues outside of the game's control.
            For example warning that the sound system is unavailable.
    info  - used to report normal game state and progress.  This should be the default level of
            logging.  The level of detail and the volume should be low.  The number of log entries
            should be low enough to not require truncating the log even for a complete game.  The
            information should be of interest to all devs and interested players and of a nature
            that people unfamiliar with the code can follow.
            For example the unnamed logger reporting that turn X has started.
            Another example the network logger reporting that the network connected/disconnected.
    debug - used for low-level implementation or calculation details.  For a named logger this
            level will probably only be turned on by devs working on that section of code.
            This will be detailed and perhaps voluminous.  Debug messages may require familiarity
            with the code to understand.
            For example reporting that the network disconnected due to a client initiated shutdown
            with a linger time of 30 ms before closing.
    trace - used for the most detailed logging.  Trace should probably only be used with a named
            logger, since the extreme detail will only be of interest to a developer working on a
            specific section of code and without the additional filtering of the name the log
            output will be flooded.  Setting all logger's thresholds to trace may generate
            unreasonably large log files.
            For example the network logger might log every message received/sent as it is
            added to or removed from its internal queues.

    The loggers are thread safe and safe to use during static initialization.

    Usage:

    Generating Logs:

    The default or unnamed logger works as an output stream, like std::cout as follows:

    ErrorLogger() << "Put any streamable output here.  It will only be
                  << "computed at the error threshold or greater.";
    WarnLogger()  << "Put any streamable output here.  It will only be
                  << "computed at the warn threshold or greater.";
    InfoLogger()  << "Put any streamable output here.  It will only be
                  << "computed at the info threshold or greater.";
    DebugLogger() << "Put any streamable output here.  It will only be
                  << "computed at the debug threshold or greater.";
    TraceLogger() << "Put any streamable output here.  It will only be
                  << "computed at the trace threshold or greater.";

    The named loggers are declared with:

    DeclareThreadSafeLogger(name_of_logger);

    A good place for the declaration is an anonymous namespace.  It is
    idempotent, so multiple calls won't change the logging system state.  It is
    not cost free, so don't add it before every logging call.


    And used with:

    ErrorLogger(name_of_logger) << "any streamable output, only computed "
                                << "at the error threshold or greater "
                                << "for the 'name_of_logger' logger.";
    WarnLogger(name_of_logger)  << "streamable output";
    InfoLogger(name_of_logger)  << "streamable output";
    DebugLogger(name_of_logger) << "streamable output";
    TraceLogger(name_of_logger) << "streamable output";

    <Error/Warn/Info/Debug/Trace>Logger() is a macro that gates the
    generation of the ""streamable output" following the stream
    operator. If the "streamable output" is a function call it is only
    executed if that given source is above its threshold and it has a sink
    to consume the generated log.


    Setting Logger Thresholds:

    The following functions are used to set logger thresholds to determine
    if a log record is generated by the source and output by the sink.  The
    functions are used to provide functionality for setting logger
    thresholds with the config files using OptionsDB in
    LoggerWithOptionsDB.<h/cpp>, through the UI in the OptionsWnd.<h/cpp>
    and from the command line in each application's main() function.

    Setting logger thresholds is an implementation detail not needed by
    most coders.  Most coders just declare a logger and generate logs, leaving
    the determination of thresholds to users.

    Set the logger threshold with:

    SetLoggerThreshold(name_of_logger, a_threshold);

    Force the threshold for all loggers to be the same with:

    OverrideAllLoggerThresholds(a_threshold);

*/

// The logging levels.
enum class LogLevel {trace, debug, info, warn, error, min = trace, max = error};

constexpr LogLevel default_log_level_threshold = LogLevel::debug;

FO_COMMON_API std::string to_string(const LogLevel level);
FO_COMMON_API LogLevel to_LogLevel(const std::string& name);

std::unordered_map<std::string, LogLevel> ValidNameToLogLevel();

// Prefix \p name to create a global logger name less likely to collide.
#ifndef FREEORION_WIN32

#define FO_GLOBAL_LOGGER_NAME(name) fo_logger_global_##name

#else

// Note: The is a workaround for Visual C++ non-conformant pre-processor
// handling of empty macro arguments.
// https://msdn.microsoft.com/en-us/library/hh567368.aspx
// https://blogs.msdn.microsoft.com/vcblog/2017/03/07/c-standards-conformance-from-microsoft/
#define FO_GLOBAL_LOGGER_NAME_NO_ARG() fo_logger_global_
#define FO_GLOBAL_LOGGER_NAME_ONE_ARG(name) fo_logger_global_##name
#define FO_GLOBAL_LOGGER_NAME(...)                                      \
    BOOST_PP_IF(BOOST_PP_IS_EMPTY(__VA_ARGS__),                         \
                FO_GLOBAL_LOGGER_NAME_NO_ARG(),                         \
                FO_GLOBAL_LOGGER_NAME_ONE_ARG(__VA_ARGS__))

#endif

/** Initializes the logging system. Log to the \p log_file.  If \p log_file already exists it will
 * be deleted. \p unnamed_logger_identifier is the name used in the log file to identify logs from
 * the singular unnamed logger for this executable.  Logs from the named loggers are identified by
 * their own name.*/
FO_COMMON_API void InitLoggingSystem(const std::string& log_file, const std::string& unnamed_logger_identifier);

/** Shutdown the file sink.  This should be called near the end of main() before the start of
    static de-initialization.

    The file sink may not be safe to use during static deinitialization, because of the
    following bug:

    http://www.boost.org/doc/libs/1_64_0/libs/log/doc/html/log/rationale/why_crash_on_term.html
    https://svn.boost.org/trac/boost/ticket/8642
    https://svn.boost.org/trac/boost/ticket/9119

    When either ticket is fixed the ShutdownLoggingSystemFileSink() function can be removed.
*/
FO_COMMON_API void ShutdownLoggingSystemFileSink();

/** Sets all logger thresholds to \p threshold permanently. If \p threshold is boost::none then
    remove the ovverride and allow subsequent SetLoggerThreshold() to work as normal. */
FO_COMMON_API void OverrideAllLoggersThresholds(const boost::optional<LogLevel>& threshold);

FO_COMMON_API const std::string& DefaultExecLoggerName();

/** A type for loggers (sources) that allows for severity and a logger name (channel in
    boost parlance) and supports multithreading.*/
using NamedThreadedLogger = boost::log::sources::severity_channel_logger_mt<
    LogLevel,     ///< the type of the severity level
    std::string   ///< the channel name of the logger
    >;

// Setup file sink, formatting, and \p name channel filter for \p logger.
FO_COMMON_API void ConfigureLogger(NamedThreadedLogger& logger, const std::string& name);

// Place in source file to create the previously defined global logger \p name
#define DeclareThreadSafeLogger(name)                                   \
    BOOST_LOG_INLINE_GLOBAL_LOGGER_INIT(                                \
        FO_GLOBAL_LOGGER_NAME(name), NamedThreadedLogger)               \
    {                                                                   \
        auto lg = NamedThreadedLogger(                                  \
            (boost::log::keywords::severity = LogLevel::debug),         \
            (boost::log::keywords::channel = #name));                   \
        ConfigureLogger(lg, #name);                                     \
        return lg;                                                      \
    }

// Signal that logger \p name has been created
using LoggerCreatedSignalType = boost::signals2::signal<void (const std::string logger)>;
FO_COMMON_API extern LoggerCreatedSignalType LoggerCreatedSignal;

// Return all loggers created since app start.  Used to provide the UI a complete list of global
// loggers intialized during static initialization.
FO_COMMON_API std::vector<std::string> CreatedLoggersNames();

// Create the default logger
#ifndef FREEORION_WIN32

DeclareThreadSafeLogger();

#else

// Note: The is a workaround for Visual C++ non-conformant pre-processor
// handling of empty macro arguments.
// https://msdn.microsoft.com/en-us/library/hh567368.aspx
// https://blogs.msdn.microsoft.com/vcblog/2017/03/07/c-standards-conformance-from-microsoft/
BOOST_LOG_INLINE_GLOBAL_LOGGER_INIT(                                    \
    fo_logger_global_, NamedThreadedLogger)                             \
{                                                                       \
    auto lg = NamedThreadedLogger(                                      \
        (boost::log::keywords::severity = LogLevel::debug),             \
        (boost::log::keywords::channel = ""));                          \
    ConfigureLogger(lg, "");                                            \
    return lg;                                                          \
}

#endif

BOOST_LOG_ATTRIBUTE_KEYWORD(log_severity, "Severity", LogLevel);
BOOST_LOG_ATTRIBUTE_KEYWORD(log_channel, "Channel", std::string)
BOOST_LOG_ATTRIBUTE_KEYWORD(log_src_filename, "SrcFilename", std::string);
BOOST_LOG_ATTRIBUTE_KEYWORD(log_src_linenum, "SrcLinenum", int);

#define __BASE_FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)

#ifndef FREEORION_WIN32

#define FO_LOGGER(name, lvl)                                            \
    BOOST_LOG_STREAM_WITH_PARAMS(                                       \
        FO_GLOBAL_LOGGER_NAME(name)::get(),                             \
        (boost::log::keywords::severity = lvl))                         \
    << boost::log::add_value("SrcFilename", __BASE_FILENAME__)          \
    << boost::log::add_value("SrcLinenum", __LINE__)

#define TraceLogger(name) FO_LOGGER(name, LogLevel::trace)

#define DebugLogger(name) FO_LOGGER(name, LogLevel::debug)

#define InfoLogger(name) FO_LOGGER(name, LogLevel::info)

#define WarnLogger(name) FO_LOGGER(name, LogLevel::warn)

#define ErrorLogger(name) FO_LOGGER(name, LogLevel::error)


#else


// Note: The is a workaround for Visual C++ non-conformant pre-processor
// handling of empty macro arguments.
// https://msdn.microsoft.com/en-us/library/hh567368.aspx
// https://blogs.msdn.microsoft.com/vcblog/2017/03/07/c-standards-conformance-from-microsoft/
#define FO_LOGGER_PRESTITCHED(lvl, logger)                              \
    BOOST_LOG_STREAM_WITH_PARAMS(                                       \
        logger::get(),                                                  \
        (boost::log::keywords::severity = lvl))                         \
    << boost::log::add_value("SrcFilename", __BASE_FILENAME__)          \
    << boost::log::add_value("SrcLinenum", __LINE__)

#define TraceLogger(...) FO_LOGGER_PRESTITCHED(LogLevel::trace, fo_logger_global_##__VA_ARGS__)
#define DebugLogger(...) FO_LOGGER_PRESTITCHED(LogLevel::debug, fo_logger_global_##__VA_ARGS__)
#define InfoLogger(...)  FO_LOGGER_PRESTITCHED(LogLevel::info,  fo_logger_global_##__VA_ARGS__)
#define WarnLogger(...)  FO_LOGGER_PRESTITCHED(LogLevel::warn,  fo_logger_global_##__VA_ARGS__)
#define ErrorLogger(...) FO_LOGGER_PRESTITCHED(LogLevel::error, fo_logger_global_##__VA_ARGS__)


#endif

/** Sets the \p threshold of \p source.  \p source == "" is the default logger.*/
FO_COMMON_API void SetLoggerThreshold(const std::string& source, LogLevel threshold);

/** Apply \p configure_front_end to a new FileSinkFrontEnd for \p channel_name.  During static
    initialization if the backend does not yet exist, then \p configure_front_end will be
    stored until the backend is created.*/
using LoggerTextFileSinkFrontend = boost::log::sinks::synchronous_sink<boost::log::sinks::text_file_backend>;
using LoggerFileSinkFrontEndConfigurer = std::function<void(LoggerTextFileSinkFrontend& sink_frontend)>;
FO_COMMON_API void ApplyConfigurationToFileSinkFrontEnd(
    const std::string& channel_name,
    const LoggerFileSinkFrontEndConfigurer& configure_front_end);

extern int g_indent;

/** A function that returns the correct amount of spacing for the current
  * indentation level during a dump. */
std::string DumpIndent();

#endif // _Logger_h_
