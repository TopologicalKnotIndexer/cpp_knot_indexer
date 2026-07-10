#pragma once

#include <array>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace cki::link_pd_code {

struct Point3 {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

struct Crossing {
    std::array<int, 4> labels{};
};

using Component = std::vector<Point3>;
using Link = std::vector<Component>;

struct Options {
    int max_attempts = 256;
    double epsilon = 1e-10;
    bool prefer_min_crossings = true;
    bool encode_isolated_components = false;
    std::optional<Point3> direction;
};

class ProjectionError : public std::runtime_error {
public:
    explicit ProjectionError(const std::string& message) : std::runtime_error(message) {}
};

using PDCode = std::vector<Crossing>;

Link parseLinkCoordinateText(const std::string& text);
Link readLinkCoordinateFile(const std::filesystem::path& path);

PDCode computePDCode(const Link& link, const Options& options = Options());
PDCode computePDCode(const std::vector<Point3>& points, const Options& options = Options());
bool validatePDCode(const PDCode& pd_code);
std::string formatPDCode(const PDCode& pd_code);

}  // namespace cki::link_pd_code
