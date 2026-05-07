#pragma once
// JSON loading with clear error messages.
//
// Replaces silent fallbacks like:
//   std::ifstream f(path); if (f.is_open()) f >> data;
// with explicit logging of:
//   - File not found
//   - Parse errors (with line/column from nlohmann::json::parse_error)
//   - Missing required keys (via require<T>())
//
// Usage:
//   nlohmann::json data;
//   if (!JsonLoader::load("assets/data/foo.json", data)) {
//     // error already logged; either return or use defaults
//   }

#include <fstream>
#include <json.hpp>
#include <string>
#include "raylib.h"

namespace JsonLoader {

// Load and parse a JSON file. Returns true on success. On failure, logs a
// detailed error to TraceLog (LOG_WARNING) and leaves `out` empty.
inline bool load(const std::string &path, nlohmann::json &out) {
  std::ifstream file(path);
  if (!file.is_open()) {
    TraceLog(LOG_WARNING, "JSON: file not found — %s", path.c_str());
    return false;
  }
  try {
    file >> out;
    return true;
  } catch (const nlohmann::json::parse_error &e) {
    TraceLog(LOG_WARNING, "JSON: parse error in %s — byte %zu: %s",
             path.c_str(), (size_t)e.byte, e.what());
    out = nlohmann::json{}; // ensure empty
    return false;
  } catch (const std::exception &e) {
    TraceLog(LOG_WARNING, "JSON: unknown error in %s — %s", path.c_str(), e.what());
    out = nlohmann::json{};
    return false;
  }
}

// Read a required field — logs warning if missing or wrong type, returns fallback.
template <typename T>
inline T require(const nlohmann::json &j, const std::string &key,
                 const std::string &source, const T &fallback) {
  auto it = j.find(key);
  if (it == j.end()) {
    TraceLog(LOG_WARNING, "JSON: missing required field '%s' in %s — using default",
             key.c_str(), source.c_str());
    return fallback;
  }
  try {
    return it->get<T>();
  } catch (const nlohmann::json::type_error &e) {
    TraceLog(LOG_WARNING, "JSON: wrong type for '%s' in %s — %s",
             key.c_str(), source.c_str(), e.what());
    return fallback;
  }
}

} // namespace JsonLoader
