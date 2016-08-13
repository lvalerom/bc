/*
Copyright 2016 Luis Valero Martin

This file is part of BC.
BC is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
any later version.
BC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with Through the galaxy.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "log.h"

#include <boost/thread.hpp>

#include <signal.h>
#include <csignal>

#include <cstdarg>
//#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <map>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>

#ifdef _WIN32
#include <Windows.h>
#endif

namespace Log {

static std::string id_header_fmt("%llu (Thread id: %s) [%7.7s] ");
static std::string id_header("(%s): ");
static std::string header_fmt("%llu (Thread id: %s) [%7.7s]: ");

typedef int(*formatter)(const char* fmt, ...);
static formatter printf_f = printf;

typedef int(*vformatter)(const char* fmt, va_list args);
static vformatter vprintf_f = vprintf;

// Mutex
static boost::mutex level_mutex;
static boost::mutex fatal_mutex;

static boost::mutex write_mutex;

static boost::mutex signal_init_mutex;
static boost::mutex win32_signal_init_mutex;

// Configuration
static level::ELogLevel default_level;

static bool fatal_exit = false;

static bool id_configuration = true;

static std::time_t start_time(std::time(0));

static enum timestamp_fmt {
	SECONDS,
	MILLISECONDS,
	TIME
} timestamp_;

static timestamp_fmt timestamp = SECONDS;

// Class level map
typedef std::map<const std::string, level::ELogLevel> classlevel_map;
static classlevel_map cl_map;

// ---------------

static level::ELogLevel string_to_level(const std::string & str)
{
	if (str == "TRACE") return (level::TRACE);
	else if (str == "DEBUG") return (level::DEBUG);
	else if (str == "INFO") return (level::INFO);
	else if (str == "WARNING") return (level::WARNING);
	else if (str == "ERROR") return (level::_ERROR);
	else if (str == "FATAL") return (level::FATAL);
	return (static_cast<level::ELogLevel> (5));
	//TODO catch exception
	//return (static_cast<level::ELogLevel> (std::stoi(str)));
}

static const char* level_to_string(level::ELogLevel lvl)
{
	if (lvl >= level::FATAL) return ("FATAL");
	else if (lvl >= level::_ERROR) return ("ERROR");
	else if (lvl >= level::WARNING) return ("WARNING");
	else if (lvl >= level::INFO) return ("INFO");
	else if (lvl >= level::DEBUG) return ("DEBUG");
	return ("TRACE");
}

static void set_up_level(const std::string &id, const std::string &lvl,
	classlevel_map& map, level::ELogLevel &conf)
{
	if (id == "default"){
		conf = string_to_level(lvl);
	}
	else {
		classlevel_map::iterator it(map.find(id));
		if (it != map.end()){
			it->second = string_to_level(lvl);
		}
		else {
			map.insert(std::make_pair(id, string_to_level(lvl)));
		}
	}
}

static level::ELogLevel get_default_level()
{
	boost::lock_guard<boost::mutex> guard(level_mutex);
	return default_level;
}

static void set_default_level(level::ELogLevel lev)
{
	boost::lock_guard<boost::mutex> guard(level_mutex);
	default_level = lev;
}

void signal_handler(int signal)
{
	fprintf(stderr, "Signal %i received\n", signal);
	fprintf(stderr, "Flushing sdtout...\n");
	boost::lock_guard<boost::mutex> guard(write_mutex);
	fflush(stdout);
	throw std::runtime_error("Signal received!");
}

typedef void(*signal_handler_ptr)(int);

static void start_signal_handler()
{
	static bool init = false;
	if (!init){
		boost::lock_guard<boost::mutex> guard(signal_init_mutex);
		if (!init){
			signal(SIGINT, signal_handler);
			signal(SIGABRT, signal_handler);
			signal(SIGFPE, signal_handler);
			signal(SIGILL, signal_handler);
			signal(SIGSEGV, signal_handler);
			signal(SIGTERM, signal_handler);
			init = true;
		}
	}
}
#ifdef _WIN32
BOOL win32_handler(DWORD fdwCtrlType)
{
	fprintf(stderr, "Windows signal received\n");
	fprintf(stderr, "Flushing sdtout...\n");
	boost::lock_guard<boost::mutex> guard(write_mutex);
	fflush(stdout);
	throw std::runtime_error("Windows signal received!");
}

static void start_win32_handler()
{
	static bool init = false;
	if (!init){
		boost::lock_guard<boost::mutex> guard(signal_init_mutex);
		if (!init){

			init = SetConsoleCtrlHandler((PHANDLER_ROUTINE)win32_handler, TRUE);
		}
	}
}
#endif

void set_up(level::ELogLevel lvl,const char* file, const char* conf)
{
	using std::string;
	using std::ifstream;
	using std::stringstream;

	level::ELogLevel default_lev = lvl;
	if (conf){
		ifstream in(conf);
		if (!in){
			fprintf(stderr, "Error while loading the file %s\n", conf);
			return;
		}
		while (in && !in.eof()){
			char start = in.peek();
			if (start == '#' || start == '\n'){
				string comment; getline(in, comment);
			}
			else if (start == '$'){
				string time_fmt;
				getline(in, time_fmt);
				if (time_fmt != ""){
					if (time_fmt == "$seconds"){
						timestamp = SECONDS;
					}
					else if (time_fmt == "$mseconds"){
						timestamp = MILLISECONDS;
					}
					else if (time_fmt == "$time"){
						timestamp = TIME;
					}
					else {
						fprintf(stderr, "Error invalid timestamp format\n");
					}
				}
			}
			else if (start == '&'){
				string sexit;
				getline(in, sexit);
				if (sexit != ""){
					if (sexit == "&exit=true"){
						boost::lock_guard<boost::mutex> guard(fatal_mutex);
						fatal_exit = true;
					}
					else if (sexit == "&exit=false"){
						boost::lock_guard<boost::mutex> guard(fatal_mutex);
						fatal_exit = false;
					}
					else{
						fprintf(stderr, "Error invalid on_fatal_exit format\n");
					}
				}
			}
			else if (start == '%'){
				string id_fmt;
				getline(in, id_fmt);
				if (id_fmt != ""){
					if (id_fmt == "%id=true"){
						id_configuration = true;
					}
					else if (id_fmt == "%id=false"){
						id_configuration = false;
					}
					else{
						fprintf(stderr, "Error invalid id_configuration format\n");
					}
				}
			}
			else if(id_configuration){
				string id, lv;
				getline(in, id, '=');
				if (id != ""){
					getline(in, lv);
					if (lv != ""){
						set_up_level(id, lv, cl_map, default_lev);
					}
					else {
						fprintf(stderr, "Error no level in the line\n");
					}
				}
				else{
					fprintf(stderr, "Error invalid id format\n");
				}
			}
			else{
				string line; getline(in, line);
			}
		}
	}
	
	set_default_level(default_lev);
	start_signal_handler();
#ifdef _WIN32
	start_win32_handler();
#endif
	if (file){
		freopen(file, "w", stdout);
	}
}

static bool find_parent_level(const std::string &id, level::ELogLevel &l)
{
	std::size_t pos(id.find(':'));
	if (pos != std::string::npos){
		classlevel_map::const_iterator it(cl_map.find(std::string(id, 0, pos)));
		if (it != cl_map.end()){
			l = it->second;
			return true;
		}
	}
	return false;
}

static bool check_level(level::ELogLevel lev, std::string id)
{
	level::ELogLevel l(get_default_level());
	if (id != ""){
		classlevel_map::const_iterator it(cl_map.find(id));
		if (it != cl_map.end()){
			return (lev >= it->second);
		}
		else if (find_parent_level(id, l)){
			return (lev >= l);
		}
	}
	return (lev >= l);
}

static std::string get_id(va_list args)
{
	return (va_arg(args,char*));
}

static unsigned long long get_timestamp()
{
	if (timestamp == SECONDS){
		return static_cast<unsigned long long> (std::time(0) - start_time);
	}
	else if (timestamp == MILLISECONDS){
		return static_cast<unsigned long long> (std::clock() / (CLOCKS_PER_SEC / 1000));
	}
	else if (timestamp == TIME){
		return static_cast<unsigned long long> (std::time(0));
	}
	return static_cast<unsigned long long> (std::time(0) - start_time);
}

static std::string get_thread_id()
{
	boost::thread::id id(boost::this_thread::get_id());
	static std::vector<boost::thread::id> thread_vector;

	std::vector<boost::thread::id>::iterator it
		(std::find(thread_vector.begin(), thread_vector.end(), id));
	if (it == thread_vector.end()){
		thread_vector.push_back(id);
	}
	std::stringstream ss;
	ss << id;
	return (ss.str());
}

static void vlog(level::ELogLevel lvl, const char* fmt, va_list args)
{
	//FIXME
	boost::lock_guard<boost::mutex> guard (write_mutex);
	
	std::string id;
	if ((id_configuration) && (check_level(lvl, id = get_id(args)))){
		std::string format(id_header);
		format.append(fmt);
		format.append("\n");
		printf_f(id_header_fmt.c_str(), get_timestamp(), get_thread_id().c_str(), level_to_string(lvl));
		vprintf_f(format.c_str(), args);

		if (fatal_exit && lvl >= level::FATAL){
			fflush(stdout);
			throw std::runtime_error("Fatal error!");
		}

		return;
	} 
	else if((!id_configuration) && (lvl >= get_default_level())){
		printf_f(header_fmt.c_str(), get_timestamp(), get_thread_id().c_str(), level_to_string(lvl));
		std::string format(fmt);
		format.append("\n");
		vprintf_f(format.c_str(), args);

		if (fatal_exit && lvl >= level::FATAL){
			fflush(stdout);
			throw std::runtime_error("Fatal error!");
		}
	}
}

void trace(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	vlog(level::TRACE, format, args);
	va_end(args);
}

void debug(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	vlog(level::DEBUG, format, args);
	va_end(args);
}

void info(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	vlog(level::INFO, format, args);
	va_end(args);
}

void warning(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	vlog(level::WARNING, format, args);
	va_end(args);
}

void error(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	vlog(level::_ERROR, format, args);
	va_end(args);
}

void fatal(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	vlog(level::FATAL, format, args);
	va_end(args);
}

}
