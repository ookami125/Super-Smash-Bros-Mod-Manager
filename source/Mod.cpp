#include <algorithm>
#include "Mod.h"
#include "fs.h"
#include "str.h"
#include <fstream>

#include <switch.h>

const std::string FIGHTER_FOLDER_PATH = std::string("fighter/");

const std::vector<std::string> characterList = { "random", "mario", "donkey",
"link", "samus", "samusd", "yoshi", "kirby", "fox", "pikachu", "luigi", "ness",
"captain", "purin", "peach", "daisy", "koopa", "ice_climber", "sheik", "zelda",
"mariod", "pichu", "falco", "marth", "lucina", "younglink", "ganon", "mewtwo",
"chrom", "roy", "gamewatch", "metaknight", "pit", "pitb", "szerosuit", "wario",
"snake", "ike", "ptrainer", "pzenigame", "pfushigisou", "plizardon", "diddy",
"lucas", "sonic", "dedede", "pikmin", "lucario", "robot", "toonlink", "wolf",
"murabito", "rockman", "wiifit", "rosetta", "littlemac", "gekkouga",
"palutena", "pacman", "reflet", "shulk", "koopajr", "duckhunt", "ryu", "ken",
"cloud", "kamui", "bayonetta", "richter", "inkling", "ridley", "krool",
"simon", "shizue", "gaogaen", "miiall", "miifighter", "miiswordsman",
"miigunner", "generalall", "generalmii", "masterhand", "crazyhand",
"mastercrazy", "galleom", "lioleus", "marx", "marx2", "dracula", "dracula2",
"ganonboss", "kiila", "darz", "koopag", "kiila_darz", "koopa_koopag",
"ganon_ganonboss", "mewtwo_masterhand", "sandbag", "miienemyf", "miienemys",
"miienemyg", "narrator", "ui_chara_murabito_spirits_aw", "smashbros", "packun",
"jack", "brave", "buddy", "dolly", "master", "tantan", "pickel", "edge",
"element", "eflame", "elight", "eflame_only", "elight_only", "demon", "trail"
};

static std::set<std::string> known_files;
static json dirs_data;
static json::array_t file_array;

int GetFighterID(std::string name)
{
	return std::find(characterList.begin(), characterList.end(), name) - characterList.begin();
}

bool isInt(char c)
{
	return c >= '0' && c <= '9';
}

bool isCostumeID(std::string name)
{
	if(name[0] != 'c') return false;
	if(name.size() == 3 && isInt(name[1]) && isInt(name[2])) return true;
	if(name.size() == 4 && isInt(name[1]) && isInt(name[2]) && isInt(name[3])) return true;
	return false;
}

bool InitMod(Mod &mod, std::string name)
{
	const std::string rootFolder = MODS_INPUT_FOLDER + name + "/";

	mod.path = rootFolder;
	mod.name = name;
	auto files = GetAllFilesRecursively(rootFolder);
	for(auto file : files)
	{
		file = getRelativePath(rootFolder, file);
		auto path = file;
		if(startsWith(file, FIGHTER_FOLDER_PATH))
		{
			file = file.substr(FIGHTER_FOLDER_PATH.size());
			std::string fighterName = file.substr(0, file.find("/"));
			auto& fighter = mod.GetFighter(fighterName);

			file = file.substr(file.find("/") + 1);
			
			std::string parentFolder = file.substr(0, file.rfind("/"));
			parentFolder = parentFolder.substr(parentFolder.rfind("/") + 1);

			if(isCostumeID(parentFolder))
			{
				std::string costumeName = parentFolder.substr(1, 2);
				int costumeID = std::stoi(costumeName);
				auto& costume = fighter.GetCostume(costumeID);
				costume.files.insert(path);
			}
			else
			{
				fighter.files.insert(path);
			}
		}
		else if(startsWith(file, "ui/replace/chara/chara_") && endsWith(file, ".bntx"))
		{
			file = file.substr(std::string("ui/replace/chara/chara_X/chara_X_").size());
			auto fighterName = file.substr(0, file.find("_"));

			auto& fighter = mod.GetFighter(fighterName);
			file = file.substr(file.find("_") + 1);

			std::string costumeName = file.substr(0, file.find("."));
			int costumeID = std::stoi(costumeName);
			auto& costume = fighter.GetCostume(costumeID);
			
			costume.files.insert(path);
		}
		else if(startsWith(file, "sound/bank/fighter_voice/vc_") && ( endsWith(file, ".nus3audio") || endsWith(file, ".nus3bank")))
		{
			file = file.substr(std::string("sound/bank/fighter_voice/vc_").size());
			auto fighterName = file.substr(0, file.find("_"));
			auto& fighter = mod.GetFighter(fighterName);
			file = file.substr(file.find("_") + 2);

			std::string costumeName = file.substr(0, file.find("."));
			int costumeID = std::stoi(costumeName);
			auto& costume = fighter.GetCostume(costumeID);
			costume.files.insert(path);
		}
		else
		{
			mod.files.insert(path);
		}
	}

	if(known_files.size() == 0)
	{
		printf("Loading hashes...\n");
		consoleUpdate(NULL);

		std::ifstream f("Hashes_all.txt");
		std::string line;
		while(std::getline(f, line))
		{
			known_files.insert(line);
		}
		printf("Done loading hashes\n%d hashes loaded\n", (int)known_files.size());
		consoleUpdate(NULL);

		printf("Loading trimmed files...\n");
		consoleUpdate(NULL);

		std::ifstream trimmed_files("dir_info_with_files_trimmed.json");
		auto res = json::parse(trimmed_files);
		//printf("%s\n", res.dump().c_str());
		dirs_data = res["dirs"];
		file_array = res["file_array"].get<json::array_t>();
		
		printf("Done loading trimmed files\n");
		consoleUpdate(NULL);
	}

	return true;
}

std::string generateUICharaXML(CostumeArray counts)
{
	const std::string XMLSTART = "<?xml version=\"1.0\" encoding=\"UTF-16\"?>\n<struct>\n  <list hash=\"db_root\">\n";
	const std::string XMLEND = "  </list>\n</struct>"; 

	std::string contents = "";

	for(size_t i=0; i<counts.size(); i++)
	{
		if(counts[i] == 0)
		{
			contents += "    <hash40 index=\"" + std::to_string(i) + "\">dummy</hash40>\n";
		}
		else
		{
			uint8_t highestCostumeID = 8 + counts[i];
			contents += "    <struct index=\"" + std::to_string(i) + "\"><byte hash=\"color_num\">" + std::to_string(highestCostumeID) + "</byte></struct>\n";
		}
	}

	return XMLSTART + contents + XMLEND;
}

std::string CostumeID(uint8_t id)
{
	std::string costumeID = std::to_string(id);
	if(costumeID.size() == 1)
		costumeID = "0" + costumeID;
	return "c" + costumeID;
}

std::string vec2str(std::vector<std::string> vec)
{
	std::string res = "";
	for(auto str : vec)
	{
		res += str + ", ";
	}
	return res;
}

void add_missing_files(
	ConfigJson& resulting_config,
	std::vector<std::string> reslotted_files,
	std::string fighter_name,
	std::string target_alt,
	bool is_new_slot = false
) {
	//appendToFile("log.txt", "add_missing_files([" + vec2str(reslotted_files) + "]" + fighter_name + ", " + target_alt + ")\n");
	std::string new_dir_info = "fighter/" + fighter_name + "/" + target_alt;
	if(resulting_config.newDirFiles.find(new_dir_info) == resulting_config.newDirFiles.end())
	{
		resulting_config.newDirFiles.insert(std::make_pair(new_dir_info, std::set<std::string>()));
	}

	for(auto file : reslotted_files)
	{
		if(!is_new_slot && file.find("effect") != std::string::npos)
			continue;
		if(known_files.find(file) == known_files.end())
		{
			auto& newDirInfo = resulting_config.newDirFiles[new_dir_info];
			if(newDirInfo.find(file) == newDirInfo.end())
				resulting_config.newDirFiles[new_dir_info].insert(file);
		}
	}
}

std::vector<std::string> split(std::string str, char c)
{
	std::vector<std::string> result;
	std::string current = "";
	for(size_t i=0; i<str.size(); i++)
	{
		if(str[i] == c)
		{
			result.push_back(current);
			current = "";
		}
		else
		{
			current += str[i];
		}
	}
	result.push_back(current);
	return result;
}

void waitForInput();

void add_new_slot(
	ConfigJson& resulting_config,
	std::string dir_info,
	std::string source_slot,
	std::string new_slot,
	std::string share_slot
) {
	//appendToFile("log.txt", "add_new_slot(" + dir_info + ", " + source_slot + ", " + new_slot + ", " + share_slot + ")\n");
	std::vector<std::string> folders = split(dir_info, '/');
	json target_dir = dirs_data;

	for(auto folder : folders)
	{
		target_dir = target_dir["directories"][folder];
	}

	if(target_dir["directories"].find(source_slot) != target_dir["directories"].end())
	{
		json source_slot_dir = target_dir["directories"][source_slot];
		std::string source_slot_path = dir_info + "/" + source_slot;
		std::string new_slot_dir_path = dir_info + "/" + new_slot;
		json share_slot_dir = target_dir["directories"][share_slot];
		std::string share_slot_path = dir_info + "/" + share_slot;

		if(resulting_config.newDirInfos.find(new_slot_dir_path) == resulting_config.newDirInfos.end())
		{
			resulting_config.newDirInfos.insert(new_slot_dir_path);
		}

		auto share_slot_dir_files = share_slot_dir["files"];
		
		addFilesToDirInfo(resulting_config, new_slot_dir_path, share_slot_dir_files, new_slot);
		addSharedFiles(resulting_config, share_slot_dir_files, source_slot, new_slot, share_slot);
		for(auto dirPair : source_slot_dir["directories"].get<json::object_t>())
		{
			std::string dir = dirPair.first;
			std::string source_slot_base = source_slot_path + "/" + dir;
			std::string new_slot_base = new_slot_dir_path + "/" + dir;
			std::string share_slot_base = share_slot_path + "/" + dir;
			resulting_config.newDirInfosBase[new_slot_base] = share_slot_base;
		}
	}
	
	for(auto dirPair : target_dir["directories"].get<json::object_t>())
	{
		std::string dir = dirPair.first;
		json target_obj = target_dir["directories"][dir];
		if(target_obj["directories"].find(source_slot) != target_obj["directories"].end())
		{
			std::string dir_str = dir;
			json source_slot_dir = target_obj["directories"][source_slot];
			std::string source_slot_path = dir_info + "/" + dir_str + "/" + source_slot;
			std::string new_slot_dir_path = dir_info + "/" + dir_str + "/" + new_slot;
			json share_slot_dir = target_obj["directories"][share_slot];
			std::string share_slot_path = dir_info + "/" + dir_str + "/" + share_slot;

			if(resulting_config.newDirInfos.find(new_slot_dir_path) == resulting_config.newDirInfos.end())
			{
				resulting_config.newDirInfos.insert(new_slot_dir_path);
			}

			addFilesToDirInfo(resulting_config, new_slot_dir_path, share_slot_dir["files"], new_slot);
			addSharedFiles(resulting_config, share_slot_dir["files"], source_slot, new_slot, share_slot);

			for(auto child_dirPair : source_slot_dir["directories"].get<json::object_t>())
			{
				std::string child_dir = child_dirPair.first;
				std::string source_slot_base = source_slot_path + "/" + child_dir;
				std::string new_slot_base = new_slot_dir_path + "/" + child_dir;
				std::string share_slot_base = share_slot_path + "/" + child_dir;
				resulting_config.newDirInfosBase[new_slot_base] = share_slot_base;
			}
		}
	}
}

void addFilesToDirInfo(
	ConfigJson& resulting_config,
	std::string dir_info,
	json files,
	std::string target_color
) {
	//appendToFile("log.txt", "addFilesToDirInfo(" + dir_info + ", " + target_color + ")\n");
	if(resulting_config.newDirFiles.find(dir_info) == resulting_config.newDirFiles.end())
	{
		resulting_config.newDirFiles[dir_info] = std::set<std::string>();
	}

	for(auto file : files)
	{
		std::string file_path = file_array[file];
		if(file_path.find("0x") == 0)
		{
			continue;
		}
		std::string new_file_path = file_path;
		new_file_path.replace(new_file_path.find("c0"), 3, target_color);
		if(resulting_config.newDirFiles[dir_info].find(new_file_path) != resulting_config.newDirFiles[dir_info].end())
		{
			continue;
		}
		resulting_config.newDirFiles[dir_info].insert(new_file_path);
	}
}

std::set<std::string> existing_files;

void addSharedFiles(
	ConfigJson& resulting_config,
	json src_files,
	std::string source_color,
	std::string target_color,
	std::string share_slot
) {
	//appendToFile("log.txt", "addSharedFiles(" + source_color + ", " + target_color + ", " + share_slot + ")\n");
	std::set<std::string> used_files;

	for(auto file : src_files)
	{
		std::string file_path = file_array[file];
		//appendToFile("log.txt", "for: " + file_path + "\t");
		if(file_path.find("0x") == 0)
		{
			//appendToFile("log.txt", "continue (0x)\n");
			continue;
		}

		std::string temp_file_path = file_path;
		//temp_file_path = temp_file_path.replace(file_path.find("c0"), 3, source_color);
		//appendToFile("log.txt", "\n\ttemp_file_path: " + temp_file_path + "\t");
		if(used_files.find(temp_file_path) != used_files.end())
		{
			//appendToFile("log.txt", "continue (used)\n");
			continue;
		}

		used_files.insert(file_path);

		/*
		new_file_path = re.sub(r"c0[0-9]", target_color, file_path, 1)
		if new_file_path in existing_files:
			continue
		*/
		std::string new_file_path = file_path;
		new_file_path.replace(new_file_path.find("c0"), 3, target_color);
		//appendToFile("log.txt", "\n\tnew_file_path: " + new_file_path + "\t");
		if(existing_files.find(new_file_path) != existing_files.end())
		{
			//appendToFile("log.txt", "continue (existing file)\n");
			continue;
		}

		std::unordered_map<std::string, std::set<std::string>>* share_to = &resulting_config.shareToVanilla;
		if(file_path.find("motion/") != std::string::npos || file_path.find("camera/") != std::string::npos)
		{
			//appendToFile("log.txt", "added shareToAdded\n");
			share_to = &resulting_config.shareToAdded;
		}
		else
		{
			//appendToFile("log.txt", "added shareToVanilla\n");
		}

		if(share_to->find(file_path) == share_to->end())
			(*share_to)[file_path] = std::set<std::string>();

		if((*share_to)[file_path].find(new_file_path) == (*share_to)[file_path].end())
			(*share_to)[file_path].insert(new_file_path);
	}
}

std::string generateJson(Mod &mod)
{
	//json j;
	//j["new-dir-infos"] = json::array();
	//j["new-dir-infos-base"] = json::object();
	//j["share-to-vanilla"] = json::object();
	//j["share-to-added"] = json::object();
	//j.newDirFiles = json::object();
	
	ConfigJson j;

	std::vector<std::string> allCostumeFiles;

	for(auto fighterPair : mod.fighters)
	{
		auto fighter = fighterPair.second;
		for(auto costumePair : fighter.costumes)
		{
			auto costume = costumePair.second;
			allCostumeFiles.clear();
			for(auto file : costume.files)
			{
				if(file.find("ui/replace/chara") == std::string::npos && file.find("ui/replace_patch/chara") == std::string::npos)
					allCostumeFiles.push_back(file);
			}
			
			existing_files.clear();
			//appendToFile("log.txt", "existing_files: \n");
			for(auto file : allCostumeFiles)
			{
				existing_files.insert(file);
				//appendToFile("log.txt", "\t" + file + "\n");
			}
			add_new_slot(j, "fighter/"+fighter.name, CostumeID(costume.refId), CostumeID(costume.id), CostumeID(costume.refId));
			add_missing_files(j, allCostumeFiles, fighter.name, CostumeID(costume.id), true);
		}
	}
	
	//Convert ConfigJson to nohlmann::json
	json j2;
	j2["new-dir-infos"] = json::array();
	j2["new-dir-infos-base"] = json::object();
	j2["share-to-vanilla"] = json::object();
	j2["share-to-added"] = json::object();
	j2["new-dir-files"] = json::object();

	for(auto dir_info : j.newDirInfos)
	{
		j2["new-dir-infos"].push_back(dir_info);
	}

	for(auto dir_info_base : j.newDirInfosBase)
	{
		j2["new-dir-infos-base"][dir_info_base.first] = dir_info_base.second;
	}

	for(auto share_to_vanilla : j.shareToVanilla)
	{
		j2["share-to-vanilla"][share_to_vanilla.first] = json::array();
		for(auto file : share_to_vanilla.second)
		{
			j2["share-to-vanilla"][share_to_vanilla.first].push_back(file);
		}
	}

	for(auto share_to_added : j.shareToAdded)
	{
		j2["share-to-added"][share_to_added.first] = json::array();
		for(auto file : share_to_added.second)
		{
			j2["share-to-added"][share_to_added.first].push_back(file);
		}
	}

	for(auto new_dir_file : j.newDirFiles)
	{
		j2["new-dir-files"][new_dir_file.first] = json::array();
		for(auto file : new_dir_file.second)
		{
			j2["new-dir-files"][new_dir_file.first].push_back(file);
		}
	}

	auto result = j2.dump(4, ' ');


	//auto result = j.dump( 4, ' ' );
	//printf("JSON: %s\n", result.c_str());
	
	return result;
}

ErrorOr<Mod*> MergeMods(std::vector<Mod*> mods)
{
	Mod* finalMod = new Mod();
	finalMod->name = "merged";
	finalMod->files = {};
	finalMod->fighters = {};
	for(auto mod : mods)
	{
		for(auto file : mod->files)
		{
			finalMod->files.insert(file);
		}
		for(auto fighterPair : mod->fighters)
		{
			auto fighter = fighterPair.second;
			auto &finalFighter = finalMod->GetFighter(fighter.name);
			for(auto costumePair : fighter.costumes)
			{
				auto costume = costumePair.second;
				auto &finalCostume = finalFighter.GetCostume(costume.id);
				printf("costume %d files %ld\n", costume.id, costume.files.size());
				for(auto file : costume.files)
				{
					finalCostume.files.insert(file);
				}
			}
		}
	}

    return ErrorOr<Mod*>(finalMod);
}

Costume &Fighter::GetCostume(uint8_t id)
{
	if(costumes.find(id) == costumes.end())
	{
		uint8_t refId = id % 8;
		return costumes[id] = Costume{
			.parent = this,
			.id = id,
			.refId = refId,
			.files = {}
		};
	}
	return costumes[id];
}

Fighter &Mod::GetFighter(std::string name)
{
	if(fighters.find(name) == fighters.end())
	{
		return fighters[name] = Fighter{
			.parent = this,
			.id = GetFighterID(name),
			.name = name,
			.costumes = {},
			.files = {}
		};
	}
	return fighters[name];
}

ErrorOr<void> CopyFiles(std::string root, std::string newRoot, std::vector<std::pair<std::string, std::string>> files)
{
	for(auto file : files)
	{
		//verify destination directory exists
		auto err = _mkdir(newRoot + file.second.substr(0, file.second.find_last_of("/")), true);
		if(err.IsError())
			return err;
		err = copyFile(root + file.first, newRoot + file.second);
		if(err.IsError())
		{
			//print files that failed to copy
			printf("[!] %s -> %s\n", (root + file.first).c_str(), (newRoot + file.second).c_str());
			return err;
		}
	}
	return ErrorOr<void>();
}

ErrorOr<void> Costume::Reslot(std::string new_path, uint8_t newSlot)
{
	if(!hasRef) {
		hasRef = true;
		refId = id;
	}

	std::string costumeID = std::to_string(id);
	if(costumeID.size() == 1)
		costumeID = "0" + costumeID;
	auto fullCostumeID = "c" + costumeID;

	std::string newCostumeID = std::to_string(newSlot);
	if(newCostumeID.size() == 1)
		newCostumeID = "0" + newCostumeID;
	auto newFullCostumeID = "c" + newCostumeID;

	printf("Reslotting costume %s to %s\n", fullCostumeID.c_str(), newFullCostumeID.c_str());
	consoleUpdate(NULL);

	std::vector<std::pair<std::string,std::string>> newFiles;
	std::set<std::string> brokenFiles;

	for(auto file : files)
	{
		auto path = file;
		
		size_t fullOffset = file.find(fullCostumeID);
		size_t partOffset = file.rfind(costumeID);
		
		if(fullOffset != std::string::npos)
		{
			file = file.substr(0, fullOffset) + newFullCostumeID + file.substr(fullOffset + fullCostumeID.size());
			newFiles.push_back(std::make_pair(path, file));
		}
		else if(file.rfind(costumeID) != std::string::npos)
		{
			file = file.substr(0, partOffset) + newCostumeID + file.substr(partOffset + costumeID.size());
			newFiles.push_back(std::make_pair(path, file));
		}
		else
		{
			brokenFiles.insert(file);
		}
	}

	if(brokenFiles.size() > 0)
	{
		auto err = vformat("Error: Costume %d has broken files:\n", id);
		for(auto file : brokenFiles)
		{
			err = vformat("%s  %s\n", err, file.c_str());
		}
		return ErrorOr<void>(err);
	}

	id = newSlot;
	
	auto fighter = this->parent;
	auto mod = fighter->parent;
	std::string modPath = mod->path;
	
	files.clear();

	for(auto newFile : newFiles)
	{
		files.insert(newFile.second);
	}

	return CopyFiles(modPath, new_path, newFiles);
}