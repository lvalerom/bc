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
#ifdef _DEBUG

#include <sstream>
#include <time.h>

#define _VISUALLEAKDETECTOR

#ifdef _VISUALLEAKDETECTOR
//#include <vld.h>
#endif

#if (!defined _VISUALLEAKDETECTOR) && (defined _LEAKDETECTOR) && (defined _WIN32)
#include <crtdbg.h>
#include <assert.h>
#endif 

#endif

#include <csignal>
#include <string>

#include <common/log/log.h>


#if (defined _DEBUG) || !(defined _WIN32)
int main(int argc, char **argv)
{
#if (!defined _VISUALLEAKDETECTOR) && (defined _LEAKDETECTOR)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetBreakAlloc(-1);
#endif
#else
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
int WINAPI
WinMain(HINSTANCE zhInstance, HINSTANCE prevInstance, LPSTR lpCmdLine, int nCmdShow)
{
#endif

	log_setup(Log::level::ELogLevel::TRACE, "log.log");

	log_trace("log_main\n");

	return 0;
}