#pragma once
#include <fstream>
#include <json.hpp>
#include <string>
#include <vector>

using json = nlohmann::json;

// Statistics for a single run
struct RunStats {
  int roomsCleared = 0;
  int totalRooms = 0;
  int enemiesKilled = 0;
  int damageDone = 0;
  int damageTaken = 0;
  int abilitiesCollected = 0;
  float timePlayed = 0.0f; // seconds
  bool bossDefeated = false;
  std::string characterUsed;

  json toJson() const {
    return {{"rooms_cleared", roomsCleared},
            {"total_rooms", totalRooms},
            {"enemies_killed", enemiesKilled},
            {"damage_done", damageDone},
            {"damage_taken", damageTaken},
            {"abilities_collected", abilitiesCollected},
            {"time_played", timePlayed},
            {"boss_defeated", bossDefeated},
            {"character", characterUsed}};
  }
};

// Persistent unlock / progression data
struct MetaProgress {
  int totalRuns = 0;
  int totalDeaths = 0;
  int totalVictories = 0;
  int totalEnemiesKilled = 0;
  int bestRoom = 0;
  float bestTime = 0.0f; // Fastest victory time (0 = never won)

  // Unlocks
  bool unlockedRogue = false;
  bool unlockedKnight = false;

  // Lore fragments found
  std::vector<std::string> loreFound;

  json toJson() const {
    json j = {{"total_runs", totalRuns},
              {"total_deaths", totalDeaths},
              {"total_victories", totalVictories},
              {"total_enemies_killed", totalEnemiesKilled},
              {"best_room", bestRoom},
              {"best_time", bestTime},
              {"unlocked_rogue", unlockedRogue},
              {"unlocked_knight", unlockedKnight},
              {"lore_found", loreFound}};
    return j;
  }

  void fromJson(const json &j) {
    totalRuns = j.value("total_runs", 0);
    totalDeaths = j.value("total_deaths", 0);
    totalVictories = j.value("total_victories", 0);
    totalEnemiesKilled = j.value("total_enemies_killed", 0);
    bestRoom = j.value("best_room", 0);
    bestTime = j.value("best_time", 0.0f);
    unlockedRogue = j.value("unlocked_rogue", false);
    unlockedKnight = j.value("unlocked_knight", false);
    if (j.contains("lore_found"))
      for (auto &l : j["lore_found"])
        loreFound.push_back(l.get<std::string>());
  }
};

class SaveManager {
public:
  static constexpr const char *SAVE_PATH = "save/progress.json";

  // Load progression data
  bool load() {
    std::ifstream file(SAVE_PATH);
    if (!file.is_open())
      return false;

    json data;
    file >> data;
    progress.fromJson(data);
    return true;
  }

  // Save progression data
  void save() const {
    // Ensure save directory exists
    std::ofstream file(SAVE_PATH);
    if (file.is_open())
      file << progress.toJson().dump(4);
  }

  // Record end-of-run stats and update progression
  void recordRun(const RunStats &stats) {
    progress.totalRuns++;

    if (stats.bossDefeated) {
      progress.totalVictories++;
      if (progress.bestTime <= 0.0f || stats.timePlayed < progress.bestTime)
        progress.bestTime = stats.timePlayed;
    } else {
      progress.totalDeaths++;
    }

    progress.totalEnemiesKilled += stats.enemiesKilled;

    if (stats.roomsCleared > progress.bestRoom)
      progress.bestRoom = stats.roomsCleared;

    // Unlock conditions
    if (progress.totalRuns >= 1)
      progress.unlockedRogue = true;
    if (progress.totalVictories >= 1)
      progress.unlockedKnight = true;

    save();
  }

  // Add a lore fragment if not already found
  void addLore(const std::string &loreId) {
    for (auto &l : progress.loreFound)
      if (l == loreId)
        return;
    progress.loreFound.push_back(loreId);
    save();
  }

  MetaProgress &getProgress() { return progress; }
  const MetaProgress &getProgress() const { return progress; }
  RunStats &getCurrentRun() { return currentRun; }
  void resetCurrentRun() { currentRun = RunStats{}; }

private:
  MetaProgress progress;
  RunStats currentRun;
};
