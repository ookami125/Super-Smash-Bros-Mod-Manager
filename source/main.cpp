#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string>
#include <vector>
#include <algorithm>
#include <set>

#include <switch.h>

#include "fs.h"
#include "Mod.h"


void PrintMod(const Mod& mod)
{
	printf("Mod name: %s\n", mod.name.c_str());

	for(auto fighterPair : mod.fighters)
	{
		auto fighter = fighterPair.second;
		printf("\t%s (%d):\n", fighter.name.c_str(), fighter.id);
		for(auto costumePair : fighter.costumes)
		{
			auto costume = costumePair.second;
			printf("\t\t%d:\n", costume.id);
			for(auto costumeFile : costume.files)
			{
				printf("\t\t\t%s\n", costumeFile.c_str());
			}
		}
		for(auto fighterFile : fighter.files)
		{
			printf("\t\t%s\n", fighterFile.c_str());
		}
	}

	for(auto modFile : mod.files)
	{
		printf("\t%s\n", modFile.c_str());
	}

	consoleUpdate(NULL);
}

void waitForInput()
{
	while(appletMainLoop())
	{
		hidScanInput();
		u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);
		if(kDown & KEY_PLUS)
			break;
	}
}

Mod* LoadMod(std::string name)
{
	Mod* mod = new Mod();
	bool res = InitMod(*mod, name);
	if(res)
		return mod;
	else
	{
		printf("Failed to load mod\n");
		consoleUpdate(NULL);
		delete mod;
		waitForInput();
		return nullptr;
	}
}

int main(int argc, char* argv[])
{
	consoleInit(NULL);

	if(appletGetAppletType() != 0)
	{
		printf("This app cannot be run in applet mode.\n");
		consoleUpdate(NULL);
		waitForInput();
		return 0;
	}

	std::vector<std::string> modNames;
	{
		DIR* dir = opendir(MODS_INPUT_FOLDER.c_str());
		if(dir==NULL)
		{
			printf("Failed to open mods folder.\n");
			consoleUpdate(NULL);
			waitForInput();
			return 0;
		}
		else
		{
			struct dirent* ent;
			while ((ent = readdir(dir)))
			{
				if(ent->d_type == DT_DIR)
				{
					if(ent->d_name[0] != '.')
					{
						modNames.push_back(ent->d_name);
					}
				}
			}
			closedir(dir);
		}
	}
	std::sort(modNames.begin(), modNames.end());

	printf("Scanning mods folder...\n");
	consoleUpdate(NULL);
	std::vector<Mod*> modList;
	
	for (std::string modName : modNames)
	{
		printf("Loading mod: %s\n", modName.c_str());
		consoleUpdate(NULL);
		Mod* mod = LoadMod(modName);
		modList.push_back(mod);
	}
	
	CostumeArray filledCostumeCount;
	for(size_t i=0; i<filledCostumeCount.size(); i++)
		filledCostumeCount[i] = 0;
	
	printf("Deleting old files...\n");
	consoleUpdate(NULL);
	deleteFolder(MODS_FOLDER + "SSBUMM/");

	printf("Copying new files...\n");
	consoleUpdate(NULL);
	for(Mod* mod : modList)
	{
		printf("Mod: %s\n", mod->name.c_str());
		consoleUpdate(NULL);
		for(auto& fighterPair : mod->fighters)
		{
			auto& fighter = fighterPair.second;
			
			for(auto& costumePair : fighter.costumes)
			{
				auto& costume = costumePair.second;
				auto costumeCount = ++filledCostumeCount[fighter.id];
				auto err = costume.Reslot(MODS_FOLDER + "SSBUMM/", 7 + costumeCount);
				if(err.IsError())
				{
					printf("%s", err.GetError().c_str());
					continue;
				}
			}
		}
	}

	for(size_t i=0; i<filledCostumeCount.size(); i++)
	{
		printf("{%ld, %d}", i, filledCostumeCount[i]);
		consoleUpdate(NULL);
	}
	printf("\n");
	consoleUpdate(NULL);

	printf("Generating XML...\n");
	consoleUpdate(NULL);
	//Generate XML
	{
		std::string xmlPath = MODS_FOLDER + "SSBUMM/ui/param/database/ui_chara_db.prcxml";
		std::string xml = generateUICharaXML(filledCostumeCount);

		_mkdir(xmlPath.substr(0, xmlPath.find_last_of('/') + 1), true);
		FILE* file = fopen(xmlPath.c_str(), "w");
		if(file != nullptr)
		{
			fwrite(xml.c_str(), 1, xml.size(), file);
			fclose(file);
		}
	}

	printf("Generating JSON...\n");
	consoleUpdate(NULL);
	{
		ErrorOr<Mod*> errMod = MergeMods(modList);
		if(errMod.IsError())
		{
			printf("Failed to Merge Mods: %s\n", errMod.GetError().c_str());
			return 1;
		}
		Mod* mod = errMod.Get();
		mod->name = "SSBUMM";
		mod->path = MODS_FOLDER + mod->name + "/";
		PrintMod(*mod);

		std::string jsonPath = mod->path + "config.json";
		std::string json = generateJson(*mod);
		FILE* file = fopen(jsonPath.c_str(), "w");
		if(file != nullptr)
		{
			fwrite(json.c_str(), 1, json.size(), file);
			fclose(file);
		}

		mod->files.insert("config.json");
	}

	printf("Done! Press + to exit\n");
	consoleUpdate(NULL);
	waitForInput();

	consoleExit(NULL);
	return 0;
}