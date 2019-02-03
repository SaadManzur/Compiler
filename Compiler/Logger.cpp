#define _CRT_SECURE_NO_WARNINGS

#include "Logger.h"

void Logger::PrintLog(ofstream &out, string message)
{
	time_t now = time(0);
	string t(ctime(&now));

	out << "[" << t.substr(0, t.length()-1) << "] " << message << endl;
}

Logger::Logger() : info("info.log", ofstream::app), warn("warning.log", ofstream::app), err("error.log", ofstream::app)
{
	
}

void Logger::Inform(string information)
{
	PrintLog(info, information);
}

void Logger::Warn(string warning)
{
	PrintLog(warn, warning);
}

void Logger::Error(string error)
{
	PrintLog(err, error);
}
