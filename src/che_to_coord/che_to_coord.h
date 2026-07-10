#pragma once

#include <cstdint>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

namespace cki::che_to_coord {

struct Point3 {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

struct OrderedPoint {
    std::int64_t atom_id = 0;
    Point3 position;
};

struct ParseOptions {
    bool prefer_atom_one_start = true;
};

class ParseError : public std::runtime_error {
public:
    explicit ParseError(const std::string& message) : std::runtime_error(message) {}
};

using CoordinateLoop = std::vector<OrderedPoint>;

CoordinateLoop parseCoordinateLoopText(const std::string& text,
                                       const ParseOptions& options = ParseOptions());
CoordinateLoop readCoordinateLoopFile(const std::filesystem::path& path,
                                      const ParseOptions& options = ParseOptions());

std::vector<Point3> positionsOnly(const CoordinateLoop& loop);
std::string formatCoordinateLoop(const CoordinateLoop& loop);

}  // namespace cki::che_to_coord
