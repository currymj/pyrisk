#pragma once

#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

#include "world_data.hpp"

namespace pyrisk {

class Player {
public:
    explicit Player(std::string name);

    std::string name;
    int color{0};
    char ord{0};
};

class Area;

class Territory {
public:
    Territory(std::string name, Area* area);

    bool border() const;
    bool area_owned() const;
    bool area_border() const;
    std::vector<Territory*> adjacent(std::optional<bool> friendly = std::nullopt,
                                     std::optional<bool> thisarea = std::nullopt) const;
    int adjacent_forces(std::optional<bool> friendly = std::nullopt,
                        std::optional<bool> thisarea = std::nullopt) const;

    std::string name;
    Area* area;
    Player* owner{nullptr};
    int forces{0};
    std::unordered_set<Territory*> connect;
    char ord{0};
};

class Area {
public:
    Area(std::string name, int value);

    Player* owner() const;
    int forces() const;
    std::unordered_set<Area*> adjacent() const;

    std::string name;
    int value{0};
    std::unordered_set<Territory*> territories;
};

class World {
public:
    World() = default;

    Territory* territory(const std::string& t);
    Area* area(const std::string& a);
    void load(const std::unordered_map<std::string, AreaDefinition>& areas,
              const std::string& connections);

    std::unordered_map<std::string, std::unique_ptr<Territory>> territories;
    std::unordered_map<std::string, std::unique_ptr<Area>> areas;
};

using EventValue = std::variant<std::string, int, std::pair<int, int>>;
struct Event {
    std::string name;
    std::vector<EventValue> args;
};

using EventLogger = std::function<void(const Event&)>;

class PythonicRNG {
public:
    PythonicRNG();
    explicit PythonicRNG(std::uint32_t seed);

    void seed(std::uint32_t seed);
    int randint(int low, int high_inclusive);
    std::uint32_t randbits(int k);
    int randbelow(int n);

    template <typename Iterator>
    void shuffle(Iterator first, Iterator last);

private:
    std::mt19937 engine_;
};

template <typename Iterator>
void PythonicRNG::shuffle(Iterator first, Iterator last) {
    auto distance = static_cast<int>(std::distance(first, last));
    for (int i = distance - 1; i > 0; --i) {
        int j = randbelow(i + 1);
        std::swap(*(first + i), *(first + j));
    }
}

class Game {
public:
    Game(World world, std::vector<Player> players,
         EventLogger logger = {}, std::optional<std::uint32_t> seed = std::nullopt);

    Player* find_player(const std::string& name);
    Territory* find_territory(const std::string& name);

    int territory_count(const Player& player) const;
    int reinforcement_count(const Player& player) const;

    bool claim(const std::string& player_name, const std::string& territory_name, int forces = 1);
    bool reinforce(const std::string& player_name, const std::string& territory_name, int forces);
    bool validate_move(const Territory& src, const Territory& dst, int forces) const;
    bool move(const std::string& player_name, const std::string& src_name,
              const std::string& target_name, int forces);
    void victory(const std::string& player_name);

    bool resolve_combat(const std::string& src_name, const std::string& target_name,
                        const std::function<bool(int, int)>& attack_decider = {},
                        const std::function<int(int)>& move_decider = {});

    void set_logger(EventLogger logger);
    void reseed(std::uint32_t seed);
    PythonicRNG& rng();

    World world;
    std::vector<Player> players;

private:
    void emit(const std::string& name, std::vector<EventValue> args);

    EventLogger logger_;
    PythonicRNG rng_;
};

}  // namespace pyrisk
