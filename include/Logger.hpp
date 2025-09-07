#pragma once

#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <ctime>


enum LogLevel { INFO, ACCESS, ERROR };
void logs(LogLevel level, const std::string &msg);