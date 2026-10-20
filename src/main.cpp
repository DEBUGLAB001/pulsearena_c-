#include "httplib.h"
#include "json.hpp"
#include "service/tournament.hpp"
#include "service/results.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace {

std::string readFileContent(const std::string& path) {
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    return {};
  }
  std::ostringstream ss;
  ss << file.rdbuf();
  return ss.str();
}

std::string computeVersion() {
  std::uintmax_t stamp = 0;
  const std::vector<std::string> paths = {
    "public/index.html",
    "public/js/app.js",
    "public/css/style.css",
    "include/tournament.hpp",
    "src/main.cpp",
    "src/service/tournament.cpp",
    "src/service/standings.cpp",
    "src/service/schedule.cpp",
    "src/service/results.cpp",
    "src/service/bracket.cpp"
  };

  for (const auto& path : paths) {
    std::error_code ec;
    auto val = fs::last_write_time(path, ec);
    if (!ec) {
      stamp += static_cast<std::uintmax_t>(val.time_since_epoch().count());
    }
  }

  return std::to_string(stamp);
}

} // namespace

int main() {
  httplib::Server svr;
  std::mutex tournament_mutex;
  auto tournament = pulsearena::sampleTournament();

  // 1. Mount public directory for CSS/JS/HTML assets
  svr.set_mount_point("/", "./public");

  // 2. Health & Status APIs
  auto health_handler = [](const httplib::Request&, httplib::Response& res) {
    res.set_content(R"({"status":"ok"})", "application/json");
  };
  svr.Get("/api/health", health_handler);
  svr.Get(R"((?:.*)/api/health)", health_handler);

  // 3. Application State Endpoint (Polled by public/js/app.js)
  auto state_handler = [&](const httplib::Request&, httplib::Response& res) {
    std::lock_guard<std::mutex> lock(tournament_mutex);
    res.set_content(pulsearena::stateJson(tournament), "application/json");
  };
  svr.Get("/api/state", state_handler);
  svr.Get(R"((?:.*)/api/state)", state_handler);

  // Version polling endpoint
  auto version_handler = [](const httplib::Request&, httplib::Response& res) {
    json v = {{"version", computeVersion()}};
    res.set_content(v.dump(), "application/json");
  };
  svr.Get("/api/version", version_handler);
  svr.Get(R"((?:.*)/api/version)", version_handler);

  // Result submission endpoint
  auto result_handler = [&](const httplib::Request& req, httplib::Response& res) {
    std::lock_guard<std::mutex> lock(tournament_mutex);
    std::string matchId;
    std::string winner;

    if (req.has_param("matchId")) {
      matchId = req.get_param_value("matchId");
    }
    if (req.has_param("winner")) {
      winner = req.get_param_value("winner");
    }

    if (matchId.empty() && !req.body.empty()) {
      try {
        auto parsed = json::parse(req.body);
        if (parsed.contains("matchId")) {
          matchId = parsed["matchId"].get<std::string>();
        }
        if (parsed.contains("winner")) {
          winner = parsed["winner"].get<std::string>();
        }
      } catch (...) {}
    }

    bool ok = pulsearena::submitResult(tournament, matchId, winner);
    if (ok) {
      res.status = 200;
      res.set_content(R"({"ok":true})", "application/json");
    } else {
      res.status = 400;
      res.set_content(R"({"ok":false,"error":"Unknown match or winner"})", "application/json");
    }
  };
  svr.Post("/api/result", result_handler);
  svr.Post(R"((?:.*)/api/result)", result_handler);

  // Reset endpoint
  auto reset_handler = [&](const httplib::Request&, httplib::Response& res) {
    std::lock_guard<std::mutex> lock(tournament_mutex);
    tournament = pulsearena::sampleTournament();
    res.set_content(R"({"ok":true})", "application/json");
  };
  svr.Post("/api/reset", reset_handler);
  svr.Post(R"((?:.*)/api/reset)", reset_handler);

  // Proxy-aware fallback routes for static assets
  svr.Get(R"((?:.*)/css/style\.css)", [](const httplib::Request&, httplib::Response& res) {
    std::string content = readFileContent("public/css/style.css");
    res.set_content(content, "text/css");
  });

  svr.Get(R"((?:.*)/js/app\.js)", [](const httplib::Request&, httplib::Response& res) {
    std::string content = readFileContent("public/js/app.js");
    res.set_content(content, "application/javascript");
  });

  svr.Get(R"((?:.*)/preview/?)", [](const httplib::Request&, httplib::Response& res) {
    std::string content = readFileContent("public/index.html");
    res.set_content(content, "text/html; charset=utf-8");
  });

  // 4. Resolve port and BIND TO 0.0.0.0
  int port = 8080;
  const char* env_port = std::getenv("PORT");
  if (env_port) {
    port = std::atoi(env_port);
  }

  std::cout << "[Server] Listening on 0.0.0.0:" << port << std::endl;
  svr.listen("0.0.0.0", port);
  return 0;
}

