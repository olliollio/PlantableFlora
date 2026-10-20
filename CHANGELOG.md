# Changelog

Format follows [Keep a Changelog](https://keepachangelog.com/). The version is `version` in `release/mod.cpp`.
Config changes: new keys are added to an existing `config.json` automatically, existing values stay.

## [0.1.0] - unreleased

First Workshop test build.

### Added
- Planting: dig a planting hole (shovel), water it (required by default), optionally fertilize it (garden lime, plant material: faster growth), plant a seed. Empty vanilla garden plots work too.
- Own seedling model on a soil heap as the first stage of every tree and bush; it keeps the hole's look (limed, compost, wet).
- Trees: seedling → young tree → full-grown, fell with an axe (logs, sticks, stump where vanilla has one). Bushes: seedling → young → full-grown, cut (sticks, grows back) or dig up.
- Species: Chernarus / Livonia: spruce, oak, pine, beech, birch, larch, ash, walnut, hornbeam, white poplar, willow, rowan, acacia, apple, pear, plum (+ forest variants of spruce, oak, pine, beech, birch, larch, acacia); rose, hazel, hawthorn, blackthorn, elder, dwarf birch. Badlands: fig, Turkish pine, olive, date palm, poplar, ash, plum; oleander, fig bush, prickly pear, cycas, Syrian rue.
- Seeds from loot (one loot file per map), wild tree search (0-3), wild bush cut (0-3), fruit ("Search for fruit", then "Get seeds" with a knife, 0-3). Grown planted trees give seeds and fruit like wild ones.
- Seeds only grow on their own map ("This seed won't grow here"); seed names hidden by default (`ShowSeedSpecies`).
- Placement rules: fertile ground, distance to roads and buildings, tree spacing, no planting in other players' bases or on stashes (silent), in caves or admin no-plant zones; optional strict mode `RequireOwnFlag`.
- Limits: per player outside the own bases (rolling: the oldest disappears when a new one is grown), per player inside the own bases, per base (all members), per server.
- Decay like base building: plants within an active flag radius live, others disappear after `PlantLifetimeDays`.
- Admin tools: `stats.txt`, `WipeAllPlants`, `NoPlantZones`; messages as notification popups.
