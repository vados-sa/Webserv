#pragma once
#include <string>

enum LogLevel { INFO, ERROR };
void logs(LogLevel level, const std::string &msg);