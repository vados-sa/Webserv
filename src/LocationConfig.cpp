#include "LocationConfig.hpp"

LocationConfig::LocationConfig() : autoindex(false)
{
    allowed_methods.push_back("GET");
    allowed_methods.push_back("POST");
    allowed_methods.push_back("DELETE");
}