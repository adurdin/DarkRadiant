#include "LogStream.h"

#include "itextstream.h"
#include "COutRedirector.h"
#include "StringLogDevice.h"

namespace applog {

LogStream::LogStream(ELogLevel logLevel) :
    std::ostream(new LogStreamBuf(logLevel))
{}

LogStream::~LogStream()
{
    LogStreamBuf* buf = static_cast<LogStreamBuf*>(rdbuf());

    if (buf != nullptr)
    {
        delete buf;
    }
}

std::ostream& getGlobalOutputStream() {
	static applog::LogStream _stream(SYS_STANDARD);
	return _stream;
}

std::ostream& getGlobalErrorStream() {
	static applog::LogStream _stream(SYS_ERROR);
	return _stream;
}

std::ostream& getGlobalWarningStream() {
	static applog::LogStream _stream(SYS_WARNING);
	return _stream;
}

void LogStream::InitialiseStreams()
{
	GlobalOutputStream().setStream(getGlobalOutputStream());
	GlobalWarningStream().setStream(getGlobalWarningStream());
	GlobalErrorStream().setStream(getGlobalErrorStream());

#if !defined(POSIX) || !defined(_DEBUG)
	// Redirect std::cout to the log, except on Linux debug builds where
    // logging to the console is more useful
	COutRedirector::init();
#endif

	// Instantiate a temporary buffer, which copies the log until the
	// GTK-based console is ready. The buffer's contents will then be copied over
	StringLogDevice::InstancePtr() = StringLogDevicePtr(new StringLogDevice);
}

void LogStream::ShutdownStreams()
{
#if !defined(POSIX) || !defined(_DEBUG)
	// Stop redirecting std::cout
	COutRedirector::destroy();
#endif
}

} // namespace applog
