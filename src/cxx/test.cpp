#include <sflog.hpp>

#include "timer.tpp"

#include <unistd.h>
#include <posixfio_tl.hpp>

#include <iostream>
#include <memory>



using posixfio::FileView;
using posixfio::ArrayOutputBuffer;



namespace sflog {

	std::string dummyString;
	struct DummySink { };
	struct DummySinkPtr { constexpr auto& operator*() noexcept { static DummySink r; return r; } };

	template <typename... Args>
	void formatTo(DummySink&, fmt::format_string<Args...> fmtStr, Args... args) {
		fmt::format_to(std::back_inserter(dummyString), fmtStr, std::forward<Args>(args)...);
	}

	void flush(DummySink&) { dummyString.clear(); }

}



int main() {
	using namespace std::string_view_literals;
	auto stdoutbb = ArrayOutputBuffer<>(STDOUT_FILENO);
	auto stdoutb = &stdoutbb;
	sflog::formatTo(*stdoutb,         "Hello, {}!\n", "posixfio buffer"); stdoutb->flush();
	sflog::formatTo(stdoutb->file(),  "Hello, {}!\n", "posixfio fileview");
	sflog::formatTo(&std::cout,       "Hello, {}!\n", "std::cout*");
	sflog::formatTo(std::cout,        "Hello, {}!\n", "std::cout"); std::cout.flush();

	sflog::Logger<decltype(stdoutb)> logger = { };
	int        i  = 2;
	const int& ir = i;
	logger.setPrefix("["sv, "Skengine "sv, ""sv, "]: "sv);
	logger.l_level = sflog::Level::eAll;
	logger.l_sink = stdoutb;
	logger.l_sgr = true;
	logger.trace   ("   Trace log {} {} {}.", 1, i, ir);
	logger.debug   ("   Debug log {} {} {}.", 1, i, ir);
	logger.info    ("    Info log {} {} {}.", 1, i, ir);
	logger.warn    ("    Warn log {} {} {}.", 1, i, ir);
	logger.error   ("   Error log {} {} {}.", 1, i, ir);
	logger.critical("Critical log {} {} {}.", 1, i, ir);

	return EXIT_SUCCESS;
}
