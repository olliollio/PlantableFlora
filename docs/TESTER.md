# PlantableFlora – Tester Guide

Thanks for helping test. This page tells you what the mod does and what to look at.

---

## 1. What the mod does

You can plant trees and bushes. Use them to hide your base, grow cover, or as a renewable source of sticks and logs.

- Get a **seed**, dig a **planting mound**, plant the seed. It grows by itself, also while you are offline.
- Bushes can be cut (they grow back) or dug up. Trees can be felled with an axe (logs, sticks, sometimes a stump) and never grow back.
- Planted trees and bushes should look exactly like wild ones.
- You can't plant inside another player's base.

The test server uses **short grow times**, so you don't have to wait hours. Ask the server owner for the current values.

---

## 2. How to plant

### 2.1 Get a seed

- **Loot:** farms and villages.
- **Wild bush:** cut a wild rose, hazel, hawthorn, blackthorn or elder bush with a knife or axe. Sometimes you get a seed, the bush stays.
- **Wild tree:** look at a wild tree with empty hands, "Search for seeds". Each tree only once per player. Tall forest trees give a "(forest)" seed.
- **Fruit trees:** "Search for fruit" on a wild apple, pear or plum tree. Then craft knife + fruit → "Get seeds".

If the server owner is an admin with VPP, seeds can also be spawned: `PF_Seed_Spruce`, `PF_Seed_Hazel` and so on.

### 2.2 Dig a planting mound

You need a **shovel, field shovel or farming hoe**.

Shovel in hands, look at the ground, "Dig planting hole".

If the spot isn't allowed, the prompt tells you why (road too close, house too close, …).

### 2.3 Water (required) and fertilize (optional)

- Garden lime or compost in hands → look at the mound → "Fertilize".
- Bottle, canteen or pot with at least 500 ml water → "Water".
- Without water the seed can't be planted ("Needs water first"). Fertilizer is optional.
- Rain waters the hole too: after about 10 minutes of rain it counts as watered.
- Each makes the plant grow faster: garden lime more than plant material. The mound changes its look.

### 2.4 Plant

Seed in hands, placement preview. It snaps to the mound. "Plant". The mound is used up and a sapling appears.

An empty vanilla garden plot works too, instead of a mound.

### 2.5 After planting

| What | How |
|---|---|
| Cut a bush | Knife/axe/machete → "Harvest Long Wooden Stick". Grows back after a while. |
| Fell a tree | Axe → "Harvest Wooden Log" (45 s) |
| Dig up a bush, sapling or stump | Shovel in hands → "Dig up" (20 s) |
| Fill in an unused mound | Shovel in hands → "Fill in planting hole" |

---

## 3. What to test

Most of this is built but **not tested yet**. Each section says what should happen. If something else happens, that's a finding.

### 3.1 Planting basics

1. Plant one bush and one tree. Watch them grow (saplings → full-grown).
2. Do they stand at the right height? Not floating, not sunk into the ground. Compare with wild trees of the same kind next to them.
3. Plant on a slope. Does the tree look right there?
4. Plant several kinds of trees. Note any that look wrong (floating, sunk, odd size).

### 3.2 Fertilizer and water (untested)

1. Dig a mound. Fertilize it with garden lime. Did the lime get used up? Does the mound look different?
2. Water it with 500 ml or more. Was the water used up? Does the mound look wet?
3. Try with less than 500 ml water: the action should not show.
4. Try fertilizing/watering twice: the second time the action should not show.
5. Plant into an unwatered mound: should be refused, "Needs water first". After watering it should work. Same with a vanilla garden plot (water one slot).
6. Dig a hole and leave it unwatered while it rains (an admin can set rain). After about 10 minutes of rain the hole should look wet and accept a seed.
7. Plant the same seed into three watered mounds at the same time: one with garden lime, one with plant material, one without fertilizer. Lime should grow fastest, then plant material, then the plain one.

### 3.3 Where you can plant

- Next to a road (closer than ~5 m): should be refused, "Too close to a road".
- Right next to a house, wall, car or player: "Too close to …".
- Two planting holes closer than ~1.5 m, or a hole closer than ~1.5 m to a planted tree: the "Dig planting hole" prompt should not show.
- A tree on a vanilla garden plot closer than ~1.5 m to another planted tree: "Too close to another tree". (Planting holes already keep that distance, so planting into a hole never says this.)
- On rock, sand or concrete: the preview stays red.
- Inside another player's base (60 m around their flag): should not work, **without** telling you why.
- In your own base: should work.

### 3.4 Strict mode (only if the server owner turns it on)

With `RequireOwnFlag` on:

1. In the open world, far from any flag: digging a mound should be refused, "Needs your own territory flag nearby".
2. Build a flag pole (or any base part near a flag): now planting within 60 m of it should work.
3. Near someone else's flag: still refused (no reason shown).
4. A flag pole spawned by an admin counts as foreign until you build something near it.

### 3.5 Limits

The prompt shows your counts, e.g. "Dig planting hole (Trees 3/10, Bushes 7/10)".

1. Plant more trees than your limit. Planting should still work, with a warning.
2. When the new tree is fully grown, your **oldest** tree should disappear.
3. In one base, plant more than the base limit (all players together): "Base full".

### 3.6 Does it look planted?

Planted trees and bushes must not be recognizable. Look for anything that gives them away:

- A name when you look at it, an entry in the inventory "vicinity" list.
- Different prompts than on wild trees.
- Missing sounds, missing slowdown when walking through a bush.

### 3.7 Persistence

1. Plant, log out, log back in: plants still there, still growing.
2. Server restart: same.
3. Walk 200+ m away and come back: plants still there.

---

## 4. How to report

For each finding, send:

- What you did and what you saw, in a few words.
- Where (map position or nearby town) and which seed/plant.
- What you expected instead.

Useful extras:

- A **screenshot**, best with a wild tree of the same kind next to it for height problems.
- Your script log if something broke: `%LocalAppData%\DayZ\`, the newest `script_*.log`.

Things we especially want to know:

1. Is planting too much effort, too little, or about right?
2. Are the grow times (on a real server: 1 h bush, 12 h tree) fun?
3. Can you find a way to grief other players with trees?
4. Any tree that looks wrong or gives itself away as planted.
