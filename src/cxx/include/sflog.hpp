#pragma once

#include <fmt/format.h>

#include <string>
#include <memory>
#include <ostream>
#include <iterator>
#include <concepts>
#include <ranges>

#ifdef SFLOG_ENABLE_POSIXFIO
	#ifdef SFLOG_NO_POSIXFIO
		#undef SFLOG_ENABLE_POSIXFIO
		#define SFLOG_ENABLE_POSIXFIO_RESETMACRO_
	#else
		#include <posixfio_tl.hpp>
	#endif
#endif



namespace sflog {

	using level_e = unsigned;
	enum class Level : level_e {
		eAll      = 1,
		eTrace    = 1,
		eDebug    = 2,
		eInfo     = 3,
		eWarn     = 4,
		eError    = 5,
		eCritical = 6,
		eOff      = 7
	};
	constexpr Level operator|(Level l, Level r) noexcept { return Level(level_e(l) | level_e(r)); }
	constexpr bool operator<(Level l, Level r) noexcept { return level_e(l) > level_e(r); }

	template <Level level>
	concept RealLevel = (Level::eTrace <= level) && (level <= Level::eCritical);


	constexpr char ansiCsi[] = { 033, 0133 };
	template <Level level> constexpr char levelAnsiSgr[5];
	template <> constexpr char levelAnsiSgr<Level::eTrace   >[5] = { ansiCsi[0], ansiCsi[1], 071, 060, 0155 };
	template <> constexpr char levelAnsiSgr<Level::eDebug   >[5] = { ansiCsi[0], ansiCsi[1], 063, 066, 0155 };
	template <> constexpr char levelAnsiSgr<Level::eInfo    >[5] = { ansiCsi[0], ansiCsi[1], 063, 063, 0155 };
	template <> constexpr char levelAnsiSgr<Level::eWarn    >[5] = { ansiCsi[0], ansiCsi[1], 063, 065, 0155 };
	template <> constexpr char levelAnsiSgr<Level::eError   >[5] = { ansiCsi[0], ansiCsi[1], 063, 061, 0155 };
	template <> constexpr char levelAnsiSgr<Level::eCritical>[5] = { ansiCsi[0], ansiCsi[1], 071, 061, 0155 };
	template <Level level> constexpr std::string_view levelAnsiSgrView = std::string_view(levelAnsiSgr<level>, levelAnsiSgr<level> + std::size(levelAnsiSgr<level>));
	constexpr char ansiResetSgr[] = { ansiCsi[0], ansiCsi[1], 0155 };
	constexpr auto ansiResetSgrView = std::string_view(ansiResetSgr, ansiResetSgr + std::size(ansiResetSgr));

	template <Level level> requires RealLevel<level> constexpr std::string_view levelStr;
	template <> constexpr std::string_view levelStr<Level::eTrace>     = "Trace";
	template <> constexpr std::string_view levelStr<Level::eDebug>     = "Debug";
	template <> constexpr std::string_view levelStr<Level::eInfo>      = "Info";
	template <> constexpr std::string_view levelStr<Level::eWarn>      = "Warn";
	template <> constexpr std::string_view levelStr<Level::eError>     = "Error";
	template <> constexpr std::string_view levelStr<Level::eCritical>  = "Critical";


	#ifdef SFLOG_ENABLE_POSIXFIO

		template <typename T>
		concept PosixfioOutputBufferType = requires(T t, const T ct, const void* cvp, size_t s) {
			{ t.write(cvp, s) } -> std::same_as<posixfio::ssize_t>;
			{ t.writeAll(cvp, s) } -> std::same_as<posixfio::ssize_t>;
			{ t.flush() } -> std::same_as<void>;
			{ ct.file() } -> std::same_as<const posixfio::FileView>;
		};

		template <typename T>
		concept PosixfioOutputFileType = (! PosixfioOutputBufferType<T>) && requires(T t, const void* cvp, size_t s) {
			{ t.write(cvp, s) } -> std::same_as<posixfio::ssize_t>;
			{ posixfio::writeAll(t, cvp, s) } -> std::same_as<posixfio::ssize_t>;
		};

		template <typename T>
		concept PosixfioWriteableType = PosixfioOutputBufferType<T> || PosixfioOutputFileType<T>;

		template <PosixfioWriteableType PosixfioWriteable>
		struct PosixfioInserter {
			PosixfioInserter& operator=(char c) noexcept { file.write(&c, 1); return *this; }
			PosixfioInserter& operator*()       noexcept { return *this; }
			PosixfioInserter& operator++()      noexcept { return *this; }
			PosixfioInserter& operator++(int)   noexcept { return *this; }
			PosixfioWriteable& file;
		};

		template <typename... Args>
		void formatTo(posixfio::FileView file, fmt::format_string<Args...> fmtStr, Args... args) {
			PosixfioInserter<posixfio::FileView> ins = { file };
			fmt::format_to(ins, fmtStr, std::forward<Args>(args)...);
		}

		template <PosixfioOutputBufferType BufferType, typename... Args>
		void formatTo(BufferType& buffer, fmt::format_string<Args...> fmtStr, Args... args) {
			PosixfioInserter<BufferType> ins = { buffer };
			fmt::format_to(ins, fmtStr, std::forward<Args>(args)...);
		}

		inline void flush(posixfio::FileView) { }

		template <PosixfioOutputBufferType BufferType, typename... Args>
		void flush(BufferType& buffer) { buffer.flush(); }

	#endif


	template <typename T>
	concept StdOstreamPtrType = requires(T t) {
		typename std::remove_pointer_t<T>::char_type;
		(*t) << "cstr" << std::string("str") << 1 << std::endl;
		(*t).write("cstr", 4) << std::endl;
		(*t).flush();
	};

	template <typename T>
	concept StdOstreamType = StdOstreamPtrType<T*> && ! StdOstreamPtrType<T>;

	template <StdOstreamType StdOstream, typename... Args>
	void formatTo(StdOstream& ostr, fmt::format_string<Args...> fmtStr, Args... args) {
		auto ins = std::ostream_iterator<typename StdOstream::char_type>(ostr);
		fmt::format_to(ins, fmtStr, std::forward<Args>(args)...);
	}

	template <StdOstreamPtrType StdOstreamPtr, typename... Args>
	void formatTo(StdOstreamPtr ostr, fmt::format_string<Args...> fmtStr, Args... args) {
		formatTo(*ostr, fmtStr, std::forward<Args>(args)...);
	}

	template <StdOstreamType StdOstream> void flush(StdOstream& ostr) { ostr.flush(); }

	template <StdOstreamPtrType StdOstreamPtr> void flush(StdOstreamPtr ostr) { (*ostr).flush(); }


	template <typename T>
	concept SinkType = requires(T t) {
		formatTo(t, "format_string {}", 1);
		flush(t);
	};

	template <typename T>
	concept SinkPtrType = requires(T t) { requires SinkType<decltype(*t)>; };

	template <typename T, typename U>
	concept StringLike =
		std::ranges::sized_range<T> &&
		std::ranges::input_range<T> &&
		requires(T t) { { * std::ranges::begin(t) } -> std::convertible_to<U>; };


	template <SinkPtrType SinkPtr>
	class Logger {
	public:
		template <Level level, typename... Args> requires RealLevel<level>
		void logRaw(fmt::format_string<Args...> fmtStr, Args... args) {
			auto& ref = *l_sink;
			auto pfx0 = std::string_view(l_prefix.begin(),                   l_prefix.begin() + l_prefixSegm[0]);
			auto pfx1 = std::string_view(l_prefix.begin() + l_prefixSegm[2], l_prefix.end()                    );
			formatTo(ref, "{}{}{}", pfx0, levelStr<level>, pfx1);
			formatTo(ref, fmtStr, std::forward<Args>(args)...);
			formatTo(ref, "\n");
		}

		template <Level level, typename... Args> requires RealLevel<level>
		void logFormatted(fmt::format_string<Args...> fmtStr, Args... args) {
			auto& ref = *l_sink;
			auto pfx0 = std::string_view(l_prefix.begin(),                   l_prefix.begin() + l_prefixSegm[0]);
			auto pfx1 = std::string_view(l_prefix.begin() + l_prefixSegm[0], l_prefix.begin() + l_prefixSegm[1]);
			auto pfx2 = std::string_view(l_prefix.begin() + l_prefixSegm[1], l_prefix.begin() + l_prefixSegm[2]);
			auto pfx3 = std::string_view(l_prefix.begin() + l_prefixSegm[2], l_prefix.end()                    );
			formatTo(ref, "{}{}{}{}{}{}{}", pfx0, levelAnsiSgrView<level>, pfx1, levelStr<level>, pfx2, ansiResetSgrView, pfx3);
			formatTo(ref, fmtStr, std::forward<Args>(args)...);
			formatTo(ref, "\n");
		}

		template <Level level, typename... Args> requires RealLevel<level>
		void log(fmt::format_string<Args...> fmtStr, Args... args) {
			// Optimize for level > eDebug
			#define LOG_ if(l_sgr) logFormatted<level, Args...>(fmtStr, std::forward<Args>(args)...); else logRaw<level, Args...>(fmtStr, std::forward<Args>(args)...);
			if constexpr (level <= Level::eDebug) { if(level >= l_level) [[unlikely]] { LOG_ }; }
			if constexpr (level >  Level::eDebug) { if(level >= l_level) [[  likely]] { LOG_ }; }
			#undef LOG_
		}

		#define LOG_ALIAS_SIG_(UC_, LC_, REST_) template <typename... Args> void LC_##REST_(fmt::format_string<Args...> fmtStr, Args... args)
		#define LOG_ALIAS_BODY_(UC_, LC_, REST_) log<Level::e##UC_##REST_, Args...>(fmtStr, std::forward<Args>(args)...);
		#define LOG_ALIAS_(UC_, LC_, REST_) LOG_ALIAS_SIG_(UC_, LC_, REST_) { LOG_ALIAS_BODY_(UC_, LC_, REST_) }
		LOG_ALIAS_(T,t,race)
		LOG_ALIAS_(D,d,ebug)
		LOG_ALIAS_(I,i,nfo)
		LOG_ALIAS_(W,w,arn)
		LOG_ALIAS_(E,e,rror)
		LOG_ALIAS_(C,c,ritical)
		#undef LOG_ALIAS_
		#undef LOG_ALIAS_BODY_
		#undef LOG_ALIAS_SIG_

		template <
			StringLike<char> Str0,
			StringLike<char> Str1,
			StringLike<char> Str2,
			StringLike<char> Str3 >
		void setPrefix(const Str0& beforeLevelColor, const Str1& beforeLevel, const Str2& afterLevel, const Str3& afterLevelColor) {
			using std::ranges::size;
			using std::ranges::begin;
			using std::ranges::end;
			l_prefix.reserve(size(beforeLevelColor) + size(beforeLevel) + size(afterLevel) + size(afterLevelColor));
			l_prefix.append(begin(beforeLevelColor), end(beforeLevelColor)); l_prefixSegm[0] = l_prefix.size();
			l_prefix.append(begin(beforeLevel     ), end(beforeLevel     )); l_prefixSegm[1] = l_prefix.size();
			l_prefix.append(begin(afterLevel      ), end(afterLevel      )); l_prefixSegm[2] = l_prefix.size();
			l_prefix.append(begin(afterLevelColor ), end(afterLevelColor ));
		}

		template <StringLike<char> Str0, StringLike<char> Str1>
		void setPrefix(Str0& beforeLevelColor, Str1& afterLevelColor) {
			using namespace std::string_view_literals;
			setPrefix(beforeLevelColor, ""sv, ""sv, afterLevelColor);
		}

	private:
		std::string l_prefix;
		size_t      l_prefixSegm[3];
		SinkPtr     l_sink;
		Level       l_level;
		bool        l_sgr;
	};

}



#ifdef SFLOG_ENABLE_POSIXFIO_RESETMACRO_
	// SFLOG_ENABLE_POSIXFIO's existence should be consistent before AND after this header's inclusion
	#undef SFLOG_ENABLE_POSIXFIO_RESETMACRO_
	#define SFLOG_ENABLE_POSIXFIO
#endif
