#include <sflog.hpp>

#include <unistd.h>
#include <posixfio_tl.hpp>

#include <iostream>
#include <memory>



using posixfio::FileView;
using posixfio::ArrayOutputBuffer;



using LoggerSink = ArrayOutputBuffer<>;
using Logger = sflog::Logger<LoggerSink*>;

Logger logger;



struct CopyableInt {
	int i;
	CopyableInt(int i): i(i) { }
	CopyableInt(const CopyableInt& cp): i(cp.i) { logger.info("Copied int {}", i); }
	CopyableInt(CopyableInt&&) = default;
};

int format_as(const CopyableInt& ci) { return ci.i; }



int main() {
	using namespace std::string_view_literals;
	LoggerSink stdoutbb = ArrayOutputBuffer<>(STDOUT_FILENO);
	auto       stdoutb  = &stdoutbb;
	sflog::formatTo(*stdoutb,         "Hello, {}!\n", "posixfio buffer"); stdoutb->flush();
	sflog::formatTo(stdoutb->file(),  "Hello, {}!\n", "posixfio fileview");
	sflog::formatTo(&std::cout,       "Hello, {}!\n", "std::cout*");
	sflog::formatTo(std::cout,        "Hello, {}!\n", "std::cout"); std::cout.flush();

	logger = Logger(
		stdoutb,
		sflog::Level::eAll,
		sflog::AnsiSgr::eYes,
		"["sv, "Skengine "sv, ""sv, "]: "sv );
	int        i  = 2;
	const int& ir = i;
	std::string str = "1234";
	logger.trace   ("    Trace log" "     {} {}={} {} {} {}.", 1, i, ir, i+1, ir+2, str);
	logger.debug   ("    Debug log" "     {} {}={} {} {} {}.", 1, i, ir, i+1, ir+2, str);
	logger.info    ("     Info log""      {} {}={} {} {} {}.", 1, i, ir, i+1, ir+2, str);
	logger.warn    ("     Warn log""      {} {}={} {} {} {}.", 1, i, ir, i+1, ir+2, str);
	logger.error   ("    Error log" "     {} {}={} {} {} {}.", 1, i, ir, i+1, ir+2, str);
	logger.critical(" Critical log"    "  {} {}={} {} {} {}.", 1, i, ir, i+1, ir+2, str);
	CopyableInt ci = 255;
	logger.info("Copyable int {}", ci);

	return EXIT_SUCCESS;
}
