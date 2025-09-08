#pragma once
#include <string>

enum LogLevel { INFO, ACCESS, ERROR };
void logs(LogLevel level, const std::string &msg);