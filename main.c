#include <stdio.h>

#include "cjson/cJSON.h"

#include "lib/logging.h"
#include "lib/mc_loot.h"

#include "cubiomes/finders.h"
#include "cubiomes/biomes.h"
#include "cubiomes/rng.h"

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

static char* get_file(const char* path) {
	FILE* file = fopen(path, "r");
	if (file == NULL)
		return NULL;

	int file_size = 0;
	for (char c = getc(file); c != EOF; c = getc(file))
		file_size++;

	fseek(file, 0, SEEK_SET);		// go back to the beginning of the file

	// allocate memory for file contents
	char* file_content = (char*)malloc(file_size + 1); // 1
	if (file_content == NULL) {
		return NULL;
	}

	// read the file content
	fread(file_content, 1, file_size, file);
	file_content[file_size] = '\0';
	fclose(file);
	return file_content;
}

uint64_t getLootSeed(uint64_t worldSeed, int cx, int cz, uint64_t index, uint64_t step, int chestIndex) {
	Xoroshiro chunkRand;

	// setPopulationSeed
	xSetSeed(&chunkRand, worldSeed);
	uint64_t l = xNextLongJ(&chunkRand) | 1ULL;
	uint64_t m = xNextLongJ(&chunkRand) | 1ULL;
	uint64_t populationSeed = ((cx*16) * l + (cz*16) * m) ^ worldSeed;
	xSetSeed(&chunkRand, populationSeed);

	// setDecoratorSeed
	xSetSeed(&chunkRand, populationSeed + index + 10000 * step);

	for (int i = 0; i < chestIndex; i++) {
		xNextLongJ(&chunkRand);
	}

	xNextIntJ(&chunkRand, 3);

	return xNextLongJ(&chunkRand);
}

int64_t spSeed = 0;
int64_t spSearchX0 = 0;
int64_t spSearchZ0 = 0;
int64_t spSearchX1 = 0;
int64_t spSearchZ1 = 0;
const char* spLootTable = NULL;
int64_t spStep = 0;
int64_t spIndex = 0;
int64_t spChestCount = 0;
enum StructureType spStructure = Desert_Pyramid;

void getConfig() {
	const char* fileContent = get_file("config.json");
	if (!fileContent) {
		LOG_ERROR("failed to read config.json");
		abort();
	}

	cJSON* cfg = cJSON_Parse(fileContent);

	spLootTable = cJSON_GetStringValue(cJSON_GetObjectItem(cfg, "lootTable"));
	spStructure = cJSON_GetNumberValue(cJSON_GetObjectItem(cfg, "structureID"));
	spStep = cJSON_GetNumberValue(cJSON_GetObjectItem(cfg, "structureStep"));
	spIndex = cJSON_GetNumberValue(cJSON_GetObjectItem(cfg, "structureIndex"));
	spChestCount = cJSON_GetNumberValue(cJSON_GetObjectItem(cfg, "chestCount"));
	spSeed = atoll(cJSON_GetStringValue(cJSON_GetObjectItem(cfg, "seed")));
	spSearchX0 = atoll(cJSON_GetStringValue(cJSON_GetObjectItem(cfg, "searchX0")));
	spSearchZ0 = atoll(cJSON_GetStringValue(cJSON_GetObjectItem(cfg, "searchZ0")));
	spSearchX1 = atoll(cJSON_GetStringValue(cJSON_GetObjectItem(cfg, "searchX1")));
	spSearchZ1 = atoll(cJSON_GetStringValue(cJSON_GetObjectItem(cfg, "searchZ1")));

}

int main() {
	getConfig();

	LootTableContext ctx;
	FILE* file = fopen(spLootTable, "r");
	int ret = init_loot_table_file(file, &ctx, MC_1_21_3);
	fclose(file);
	if (ret) {
		printf("loot table did not init %d\n", ret);
		abort();
	}
	printf("Loot table %s\n", spLootTable);
	printf("Seed %lld\n", spSeed);

	Generator g;
	setupGenerator(&g, MC_1_21_3, 0);
	applySeed(&g, DIM_OVERWORLD, spSeed);

	StructureConfig structConf;
	getStructureConfig(spStructure, MC_1_21_3, &structConf);

	int sx0 = floordiv(spSearchX0, structConf.regionSize * 16);
	int sz0 = floordiv(spSearchZ0, structConf.regionSize * 16);
	int sx1 = floordiv((spSearchX1-1), structConf.regionSize * 16);
	int sz1 = floordiv((spSearchZ1-1), structConf.regionSize * 16);


	printf("Search space from %d:%d to %d:%d structure regions\n", sx0, sz0, sx1, sz1);

	for (int rx = sx0; rx < sx1; rx++) {
		for (int rz = sz0; rz < sz1; rz++) {
			Pos p;
			int ok = getStructurePos(structConf.structType, MC_1_21_3, spSeed, rx, rz, &p);
			if (!ok) {
				continue;
			}
			if (!(p.x >= spSearchX0 && p.x < spSearchX1 && p.z >= spSearchZ0 && p.z < spSearchZ1)) {
				continue;
			}
			int id = isViableStructurePos(structConf.structType, &g, p.x, p.z, 0);
			if (!id) {
				continue;
			}
			printf("Found structure at %d %d\n", p.x, p.z);
			if (!isViableStructureTerrain(structConf.structType, &g, p.x, p.z)) {
				continue;
			}

			printf("Found structure at %ld %ld with following loot:\n", p.x, p.z);
			for (int chestIndex = 0; chestIndex < spChestCount; chestIndex++) {
				uint64_t lootSeed = getLootSeed(spSeed, p.x>>4, p.z>>4, spIndex, spStep, chestIndex);
				printf("Chest %d %ld\n", chestIndex, lootSeed);
				set_loot_seed(&ctx, lootSeed);
				generate_loot(&ctx);
				print_loot(&ctx);
			}
		}
	}

	free_loot_table(&ctx);
	return 0;
}
