#pragma once
#ifndef _LOGGER_H_
#define _LOGGER_H_

#include <fstream>
#include <ctime>
#include <sstream>
using namespace std;
#endif

class Logger
{
private:

	ofstream info;
	ofstream warn;
	ofstream err;

	void PrintLog(ofstream &out, string message);

public:

	Logger();

	void Inform(string information);
	void Warn(string warning);
	void Error(string error);
};

