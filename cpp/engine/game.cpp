#include "game.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace pyrisk {

Player::Player(std::string name) : name(std::move(name)) {}

Territory::Territory(std::string name_in, Area* area_in)
    : name(std::move(name_in)), area(area_in) {}

bool Territory::border() const {
    return std::any_of(connect.begin(), connect.end(), [&](Territory* t) {
        return t->owner && t->owner != owner;
    });
}

bool Territory::area_owned() const {
    return owner != nullptr && area->owner() == owner;
}

bool Territory::area_border() const {
    return std::any_of(connect.begin(), connect.end(), [&](Territory* t) { return t->area != area; });
}

std::vector<Territory*> Territory::adjacent(std::optional<bool> friendly,
                                            std::optional<bool> thisarea) const {
    std::vector<Territory*> out;
    for (auto* t : connect) {
        if (friendly.has_value() && friendly.value() != (t->owner == owner)) {
            continue;
        }
        if (thisarea.has_value() && thisarea.value() != (t->area == area)) {
            continue;
        }
        out.push_back(t);
    }
    return out;
}

int Territory::adjacent_forces(std::optional<bool> friendly, std::optional<bool> thisarea) const {
    int total = 0;
    for (auto* t : adjacent(friendly, thisarea)) {
        total += t->forces;
    }
    return total;
}

Area::Area(std::string name_in, int value_in) : name(std::move(name_in)), value(value_in) {}

Player* Area::owner() const {
    Player* candidate = nullptr;
    for (auto* t : territories) {
        if (candidate == nullptr) {
            candidate = t->owner;
        } else if (t->owner != candidate) {
            return nullptr;
        }
    }
    return candidate;
}

int Area::forces() const {
    int total = 0;
    for (auto* t : territories) {
        total += t->forces;
    }
    return total;
}

std::unordered_set<Area*> Area::adjacent() const {
    std::unordered_set<Area*> adj;
    for (auto* t : territories) {
        for (auto* other : t->connect) {
            if (other->area != this) {
                adj.insert(other->area);
            }
        }
    }
    return adj;
}

Territory* World::territory(const std::string& t) {
    auto it = territories.find(t);
    if (it != territories.end()) {
        return it->second.get();
    }
    return nullptr;
}

Area* World::area(const std::string& a) {
    auto it = areas.find(a);
    if (it != areas.end()) {
        return it->second.get();
    }
    return nullptr;
}

void World::load(const std::unordered_map<std::string, AreaDefinition>& area_defs,
                 const std::string& connections) {
    static const std::vector<char> ords = {'\\', '/', '-', '|', '+'};

    for (const auto& [name, def] : area_defs) {
        auto area_ptr = std::make_unique<Area>(name, def.value);
        Area* area_raw = area_ptr.get();
        areas.emplace(name, std::move(area_ptr));
        for (const auto& territory_name : def.territories) {
            auto territory_ptr = std::make_unique<Territory>(territory_name, area_raw);
            area_raw->territories.insert(territory_ptr.get());
            territories.emplace(territory_name, std::move(territory_ptr));
        }
    }

    std::istringstream input(connections);
    std::string line;
    while (std::getline(input, line)) {
        if (line.find_first_not_of(" \t\r") == std::string::npos) {
            continue;
        }
        std::vector<std::string> joins;
        size_t start_pos = 0;
        while (start_pos < line.size()) {
            size_t delim = line.find("--", start_pos);
            std::string token;
            if (delim == std::string::npos) {
                token = line.substr(start_pos);
                start_pos = line.size();
            } else {
                token = line.substr(start_pos, delim - start_pos);
                start_pos = delim + 2;
            }
            size_t start = token.find_first_not_of(" \t");
            size_t endpos = token.find_last_not_of(" \t");
            if (start == std::string::npos) {
                joins.emplace_back("");
            } else {
                joins.emplace_back(token.substr(start, endpos - start + 1));
            }
        }

        for (size_t i = 0; i + 1 < joins.size(); ++i) {
            auto* t0 = territory(joins[i]);
            auto* t1 = territory(joins[i + 1]);
            if (!t0 || !t1) {
                throw std::runtime_error("Unknown territory in connection line: " + line);
            }
            t0->connect.insert(t1);
            t1->connect.insert(t0);
        }
    }

    for (auto& [_, territory_ptr] : territories) {
        auto* t = territory_ptr.get();
        std::vector<char> avail = ords;
        for (auto* c : t->connect) {
            avail.erase(std::remove(avail.begin(), avail.end(), c->ord), avail.end());
        }
        if (avail.empty()) {
            throw std::runtime_error("No available ord symbol for territory");
        }
        t->ord = avail.back();
    }
}

namespace {
constexpr std::uint32_t kMatrixA = 0x9908B0DFU;
constexpr std::uint32_t kUpperMask = 0x80000000U;
constexpr std::uint32_t kLowerMask = 0x7FFFFFFFU;
}

PythonicRNG::PythonicRNG(Mode mode) : mode_(mode) {
    seed(static_cast<std::uint32_t>(std::random_device{}()));
}
PythonicRNG::PythonicRNG(std::uint32_t seed_value, Mode mode) : mode_(mode) { seed(seed_value); }

void PythonicRNG::init_by_array(const std::vector<std::uint32_t>& key) {
    state_[0] = 19650218UL;
    for (std::size_t i = 1; i < kN; ++i) {
        state_[i] =
            1812433253UL * (state_[i - 1] ^ (state_[i - 1] >> 30)) + static_cast<std::uint32_t>(i);
    }

    std::size_t i = 1;
    std::size_t j = 0;
    std::size_t key_length = key.size();
    std::size_t k = kN > key_length ? kN : key_length;
    for (; k > 0; --k) {
        state_[i] = (state_[i] ^ ((state_[i - 1] ^ (state_[i - 1] >> 30)) * 1664525UL)) + key[j] +
                    static_cast<std::uint32_t>(j);
        state_[i] &= 0xFFFFFFFFUL;
        ++i;
        ++j;
        if (i >= kN) {
            state_[0] = state_[kN - 1];
            i = 1;
        }
        if (j >= key_length) {
            j = 0;
        }
    }
    for (k = kN - 1; k > 0; --k) {
        state_[i] = (state_[i] ^ ((state_[i - 1] ^ (state_[i - 1] >> 30)) * 1566083941UL)) -
                    static_cast<std::uint32_t>(i);
        state_[i] &= 0xFFFFFFFFUL;
        ++i;
        if (i >= kN) {
            state_[0] = state_[kN - 1];
            i = 1;
        }
    }
    state_[0] = 0x80000000UL;
    index_ = kN;
}

void PythonicRNG::twist() {
    for (std::size_t i = 0; i < kN; ++i) {
        std::uint32_t y = (state_[i] & kUpperMask) | (state_[(i + 1) % kN] & kLowerMask);
        state_[i] = state_[(i + kM) % kN] ^ (y >> 1) ^ ((y & 0x1U) ? kMatrixA : 0x0U);
    }
    index_ = 0;
}

std::uint32_t PythonicRNG::extract() {
    if (index_ >= kN) {
        twist();
    }
    std::uint32_t y = state_[index_++];
    y ^= (y >> 11);
    y ^= (y << 7) & 0x9D2C5680UL;
    y ^= (y << 15) & 0xEFC60000UL;
    y ^= (y >> 18);
    return y;
}

void PythonicRNG::seed(std::uint32_t seed_value) {
    last_seed_ = seed_value;
    if (mode_ == Mode::StdMT) {
        std_engine_.seed(seed_value);
    } else {
        init_by_array({seed_value});
    }
}
void PythonicRNG::set_mode(Mode mode) {
    mode_ = mode;
    seed(last_seed_);
}

int PythonicRNG::randint(int low, int high_inclusive) {
    if (high_inclusive < low) {
        throw std::invalid_argument("high must be >= low");
    }
    if (mode_ == Mode::StdMT) {
        std::uniform_int_distribution<int> dist(low, high_inclusive);
        return dist(std_engine_);
    }
    return randbelow(high_inclusive - low + 1) + low;
}

std::uint32_t PythonicRNG::randbits(int k) {
    if (k <= 0 || k > 32) {
        throw std::invalid_argument("k must be between 1 and 32");
    }
    if (mode_ == Mode::StdMT) {
        if (k == 32) {
            return std_engine_();
        }
        std::uint32_t mask = (static_cast<std::uint64_t>(1) << k) - 1;
        return std_engine_() & mask;
    } else {
        std::uint64_t accum = 0;
        int bits = 0;
        while (bits < k) {
            accum = (accum << 32) | extract();
            bits += 32;
        }
        return static_cast<std::uint32_t>(accum >> (bits - k));
    }
}

int PythonicRNG::randbelow(int n) {
    if (n <= 0) {
        throw std::invalid_argument("n must be positive");
    }
    if (n == 1) {
        return 0;
    }
    if (mode_ == Mode::StdMT) {
        std::uniform_int_distribution<int> dist(0, n - 1);
        return dist(std_engine_);
    } else {
        int k = 0;
        for (int temp = n; temp > 0; temp >>= 1) {
            ++k;
        }
        while (true) {
            std::uint32_t r = randbits(k);
            if (r < static_cast<std::uint32_t>(n)) {
                return static_cast<int>(r);
            }
        }
    }
}

double PythonicRNG::random() {
    if (mode_ == Mode::StdMT) {
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        return dist(std_engine_);
    } else {
        std::uint32_t a = extract() >> 5;
        std::uint32_t b = extract() >> 6;
        return (a * 67108864.0 + b) / 9007199254740992.0;  // 2**53
    }
}

Game::Game(World world_in, std::vector<Player> players_in, EventLogger logger,
           std::optional<std::uint32_t> seed)
    : world(std::move(world_in)), players(std::move(players_in)), logger_(std::move(logger)) {
    if (seed.has_value()) {
        rng_.seed(seed.value());
    }
}

Player* Game::find_player(const std::string& name) {
    auto it = std::find_if(players.begin(), players.end(), [&](const Player& p) { return p.name == name; });
    return it != players.end() ? &(*it) : nullptr;
}

Territory* Game::find_territory(const std::string& name) { return world.territory(name); }

int Game::territory_count(const Player& player) const {
    int count = 0;
    for (const auto& [_, t] : world.territories) {
        if (t->owner == &player) {
            ++count;
        }
    }
    return count;
}

int Game::reinforcement_count(const Player& player) const {
    int base = std::max(territory_count(player) / 3, 3);
    int area_bonus = 0;
    for (const auto& [_, area] : world.areas) {
        if (area->owner() == &player) {
            area_bonus += area->value;
        }
    }
    return base + area_bonus;
}

bool Game::claim(const std::string& player_name, const std::string& territory_name, int forces) {
    auto* player = find_player(player_name);
    auto* territory_ptr = find_territory(territory_name);
    if (!player || !territory_ptr) {
        return false;
    }
    if (territory_ptr->owner && territory_ptr->owner != player) {
        return false;
    }
    territory_ptr->owner = player;
    territory_ptr->forces += forces;
    emit("claim", {player->name, territory_ptr->name, forces});
    return true;
}

bool Game::reinforce(const std::string& player_name, const std::string& territory_name, int forces) {
    auto* player = find_player(player_name);
    auto* territory_ptr = find_territory(territory_name);
    if (!player || !territory_ptr || territory_ptr->owner != player || forces < 0) {
        return false;
    }
    territory_ptr->forces += forces;
    emit("reinforce", {player->name, territory_ptr->name, forces});
    return true;
}

bool Game::validate_move(const Territory& src, const Territory& dst, int forces) const {
    return src.owner == dst.owner && forces >= 0 && forces < src.forces;
}

bool Game::move(const std::string& player_name, const std::string& src_name,
                const std::string& target_name, int forces) {
    auto* player = find_player(player_name);
    auto* src = find_territory(src_name);
    auto* dst = find_territory(target_name);
    if (!player || !src || !dst || src->owner != player || dst->owner != player) {
        return false;
    }
    if (!validate_move(*src, *dst, forces)) {
        return false;
    }
    src->forces -= forces;
    dst->forces += forces;
    emit("move", {player->name, src->name, dst->name, forces});
    return true;
}

bool Game::resolve_combat(const std::string& src_name, const std::string& target_name,
                          const std::function<bool(int, int)>& attack_decider,
                          const std::function<int(int)>& move_decider) {
    Territory* src = find_territory(src_name);
    Territory* dst = find_territory(target_name);
    if (!src || !dst || src->owner == nullptr || src->owner == dst->owner ||
        src->connect.count(dst) == 0) {
        return false;
    }

    int initial_atk = src->forces;
    int initial_def = dst->forces;
    int n_atk = initial_atk;
    int n_def = initial_def;
    auto should_attack = attack_decider ? attack_decider : [](int, int) { return true; };
    auto decide_move = move_decider ? move_decider : [](int a) { return a - 1; };

    while (n_atk > 1 && n_def > 0 && should_attack(n_atk, n_def)) {
        int atk_dice = std::min(n_atk - 1, 3);
        int def_dice = std::min(n_def, 2);
        std::vector<int> atk_roll;
        std::vector<int> def_roll;
        for (int i = 0; i < atk_dice; ++i) {
            atk_roll.push_back(rng_.randint(1, 6));
        }
        for (int i = 0; i < def_dice; ++i) {
            def_roll.push_back(rng_.randint(1, 6));
        }
        std::sort(atk_roll.begin(), atk_roll.end(), std::greater<int>());
        std::sort(def_roll.begin(), def_roll.end(), std::greater<int>());
        for (size_t i = 0; i < std::min(atk_roll.size(), def_roll.size()); ++i) {
            if (atk_roll[i] > def_roll[i]) {
                --n_def;
            } else {
                --n_atk;
            }
        }
    }

    if (n_def == 0) {
        int move = decide_move(n_atk);
        int min_move = std::min(n_atk - 1, 3);
        int max_move = n_atk - 1;
        move = std::clamp(move, min_move, max_move);
        src->forces = n_atk - move;
        dst->forces = move;
        Player* previous_owner = dst->owner;
        dst->owner = src->owner;
        emit("conquer",
             {src->owner->name, previous_owner ? previous_owner->name : std::string(), src->name,
              dst->name, std::make_pair(initial_atk, initial_def),
              std::make_pair(src->forces, dst->forces)});
        return true;
    }

    src->forces = n_atk;
    dst->forces = n_def;
    emit("defeat", {src->owner->name, dst->owner ? dst->owner->name : std::string(), src->name,
                    dst->name, std::make_pair(initial_atk, initial_def),
                    std::make_pair(src->forces, dst->forces)});
    return false;
}

void Game::set_logger(EventLogger logger) { logger_ = std::move(logger); }

void Game::reseed(std::uint32_t seed) { rng_.seed(seed); }

PythonicRNG& Game::rng() { return rng_; }

void Game::victory(const std::string& player_name) { emit("victory", {player_name}); }

void Game::emit(const std::string& name, std::vector<EventValue> args) {
    if (logger_) {
        logger_({name, std::move(args)});
    }
}

}  // namespace pyrisk
