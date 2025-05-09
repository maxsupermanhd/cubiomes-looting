#include <stdio.h>

#include "lib/mc_loot.h"

static void print_loot(LootTableContext* ctx)
{
	for (int i = 0; i < ctx->generated_item_count; i++)
	{
		ItemStack* item_stack = &(ctx->generated_items[i]);
		printf("%s x %d\n", get_item_name(ctx, item_stack->item), item_stack->count);

		for (int j = 0; j < item_stack->enchantment_count; j++)
		{
			EnchantInstance* ench = &(item_stack->enchantments[j]);
			printf("    %s %d\n", get_enchantment_name(ench->enchantment), ench->level);
		}
	}
}

#include "cubiomes/finders.h"
#include "cubiomes/biomes.h"
#include "cubiomes/rng.h"

uint64_t getLootSeed(uint64_t worldSeed, int cx, int cz, uint64_t index, uint64_t step) {
	Xoroshiro chunkRand;

	// setPopulationSeed
	xSetSeed(&chunkRand, worldSeed);
	uint64_t l = xNextLongJ(&chunkRand) | 1ULL;
	uint64_t m = xNextLongJ(&chunkRand) | 1ULL;
	uint64_t populationSeed = ((cx*16) * l + (cz*16) * m) ^ worldSeed;
	xSetSeed(&chunkRand, populationSeed);

	// setDecoratorSeed
	xSetSeed(&chunkRand, populationSeed + index + 10000 * step);

	xNextIntJ(&chunkRand, 3);

	return xNextLongJ(&chunkRand);
}

int main() {
	LootTableContext ctx;

	FILE* file = fopen("tables/desert_pyramid.json", "r");
	int ret = init_loot_table_file(file, &ctx, (enum MCVersion)MC_1_21);
	fclose(file);
	if (ret) {
		printf("loot table did not init %d\n", ret);
		abort();
	}


// [System] [CHAT] Seed: [-3438928525885583395]
// [System] [CHAT] The nearest minecraft:desert_pyramid is at [-3008, ~, -4880] (5731 blocks away)
// [System] [CHAT] -2998, 61, -4868 has the following block data: {LootTable: "minecraft:chests/desert_pyramid", x: -2998, y: 61, z: -4868, id: "minecraft:chest", LootTableSeed: 5607812447116310305L}
// [System] [CHAT] -3000, 61, -4870 has the following block data: {LootTable: "minecraft:chests/desert_pyramid", x: -3000, y: 61, z: -4870, id: "minecraft:chest", LootTableSeed: -4740816410992747849L}
// [System] [CHAT] -2998, 61, -4872 has the following block data: {LootTable: "minecraft:chests/desert_pyramid", x: -2998, y: 61, z: -4872, id: "minecraft:chest", LootTableSeed: -7348923201375380957L}
// [System] [CHAT] -2996, 61, -4870 has the following block data: {LootTable: "minecraft:chests/desert_pyramid", x: -2996, y: 61, z: -4870, id: "minecraft:chest", LootTableSeed: 1426284063932910835L}


	int64_t worldSeed = 3438928525885583395;
	int chestChunkX = -3008;
	int chestChunkZ = -4880;

	// salts.txt step index *
	uint64_t step = 4;
	uint64_t index = 1;

	uint64_t lootSeedShouldBe[] = {
		5607812447116310305L,
		-4740816410992747849L,
		-7348923201375380957L,
		1426284063932910835L
	};

	uint64_t gotLootSeed = getLootSeed(worldSeed, chestChunkX, chestChunkZ, index, step);
	printf("got       loot seed %ld\n", gotLootSeed);
	printf("should be loot seed %ld\n", lootSeedShouldBe[0]);
	printf("should be loot seed %ld\n", lootSeedShouldBe[1]);
	printf("should be loot seed %ld\n", lootSeedShouldBe[2]);
	printf("should be loot seed %ld\n", lootSeedShouldBe[3]);

	set_loot_seed(&ctx, gotLootSeed);
	generate_loot(&ctx);
	print_loot(&ctx);
	free_loot_table(&ctx);
	return 0;
}
