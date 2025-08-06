#include "LocationConfig.hpp"

LocationConfig::LocationConfig() : autoindex(false)
{
    allowed_methods.push_back("GET");
    allowed_methods.push_back("POST");
    allowed_methods.push_back("DELETE");
}

// std::ostream& operator<<(std::ostream& os, LocationConfig& obj) {
//      os << "Location Configuration:\n";
//      (void) obj;
//     // os << "  Root: " << obj.getRoot() << "\n";

//     // // os << "  Index Files: ";
//     // // const std::vector<std::string> &indexFiles = obj.getIndexFiles();
//     // // for (size_t i = 0; i < indexFiles.size(); ++i) {
//     // //     os << indexFiles[i];
//     // //     if (i != indexFiles.size() - 1)
//     // //         os << ", ";
//     // // }
//     // // os << "\n";

//     // // os << "  Allowed Methods: ";
//     // // const std::vector<std::string> &methods = obj.getAllowedMethods();
//     // // for (size_t i = 0; i < methods.size(); ++i) {
//     // //     os << methods[i];
//     // //     if (i != methods.size() - 1)
//     // //         os << ", ";
//     // // }
//     // // os << "\n";

//     // os << "  Upload Dir: " << obj.getUploadDir() << "\n";
//     // os << "  Autoindex: " << (obj.getAutoindex() ? "true" : "false") << "\n";
//     return os;
// }
