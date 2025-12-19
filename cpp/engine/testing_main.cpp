#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "ai.hpp"
#include "event_logging.hpp"
#include "world_data.hpp"

int main(int argc, char** argv) {
    using namespace pyrisk;

    std::uint32_t seed = 42;
    if (argc > 1) {
        seed = static_cast<std::uint32_t>(std::strtoul(argv[1], nullptr, 10));
    }

    World world;
    world.load(kAreas, kConnectionData);

    std::vector<std::string> names = {"ALPHA", "BRAVO"};
    std::vector<GameDriver::AiFactory> factories;
    factories.push_back([](Player& p, Game& g) { return std::make_unique<DeterministicAI>(p, g); });
    factories.push_back([](Player& p, Game& g) { return std::make_unique<DeterministicAI>(p, g); });

    auto logger = [](const Event& event) { std::cout << event_to_json(event) << std::endl; };

    GameDriver driver(std::move(world), names, factories, /*deal=*/false, logger, seed);
    driver.play();
    return 0;
}
