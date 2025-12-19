#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace pyrisk {

struct AreaDefinition {
    int value{};
    std::vector<std::string> territories;
};

inline const std::string kConnectionData = R"CONNECT(
Alaska--Northwest Territories--Alberta--Alaska
Alberta--Ontario--Greenland--Northwest Territories
Greenland--Quebec--Ontario--Eastern United States--Quebec
Alberta--Western United States--Ontario--Northwest Territories
Western United States--Eastern United States--Mexico--Western United States

Venezuala--Peru--Argentina--Brazil
Peru--Brazil--Venezuala

North Africa--Egypt--East Africa--North Africa
North Africa--Congo--East Africa--South Africa--Congo
East Africa--Madagascar--South Africa

Indonesia--Western Australia--Eastern Australia--New Guinea--Indonesia
Western Australia--New Guinea

Iceland--Great Britain--Western Europe--Southern Europe--Northern Europe--Western Europe
Northern Europe--Great Britain--Scandanavia--Northern Europe--Ukraine--Scandanavia--Iceland
Southern Europe--Ukraine

Middle East--India--South East Asia--China--Mongolia--Japan--Kamchatka--Yakutsk--Irkutsk--Kamchatka--Mongolia--Irkutsk
Yakutsk--Siberia--Irkutsk
China--Siberia--Mongolia
Siberia--Ural--China--Afghanistan--Ural
Middle East--Afghanistan--India--China

Mexico--Venezuala
Brazil--North Africa
Western Europe--North Africa--Southern Europe--Egypt--Middle East--East Africa
Southern Europe--Middle East--Ukraine--Afghanistan--Ural
Ukraine--Ural
Greenland--Iceland
Alaska--Kamchatka
South East Asia--Indonesia
)CONNECT";

inline const std::unordered_map<std::string, AreaDefinition> kAreas = {
    {"North America", {5, {"Alaska", "Northwest Territories", "Greenland", "Alberta", "Ontario", "Quebec", "Western United States", "Eastern United States", "Mexico"}}},
    {"South America", {2, {"Venezuala", "Brazil", "Peru", "Argentina"}}},
    {"Africa", {3, {"North Africa", "Egypt", "East Africa", "Congo", "South Africa", "Madagascar"}}},
    {"Europe", {5, {"Iceland", "Great Britain", "Scandanavia", "Ukraine", "Northern Europe", "Western Europe", "Southern Europe"}}},
    {"Asia", {7, {"Middle East", "Afghanistan", "India", "South East Asia", "China", "Mongolia", "Japan", "Kamchatka", "Irkutsk", "Yakutsk", "Siberia", "Ural"}}},
    {"Australia", {2, {"Indonesia", "New Guinea", "Eastern Australia", "Western Australia"}}},
};

}  // namespace pyrisk
