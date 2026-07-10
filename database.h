#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace hki {

class NameNormalizer {
public:
    NameNormalizer() = default;
    explicit NameNormalizer(const std::filesystem::path& dataDir);

    std::string normalize(const std::string& knotName) const;

private:
    std::unordered_map<std::string, std::string> name1ToName2_;
    std::unordered_map<std::string, bool> amphichiral_;
    std::unordered_map<std::string, bool> needMirror_;

    bool isAmphichiralPrime(const std::string& rawPrimeName) const;
    std::string regularizePrime(const std::string& knotName) const;
    std::string simplifyPrime(const std::string& knotName) const;
};

using InvariantMap = std::unordered_map<std::string, std::vector<std::string>>;

InvariantMap loadInvariantMap(const std::filesystem::path& file, const NameNormalizer& normalizer);
std::vector<std::string> lookupInvariant(const InvariantMap& map, const std::string& invariant);

} // namespace hki
