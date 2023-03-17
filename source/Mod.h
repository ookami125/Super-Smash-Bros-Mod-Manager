#include <string>
#include <set>
#include <unordered_map>
#include "json.hpp"
using json = nlohmann::json;
#include "errorOr.h"
//#include "bencode.hpp"

const std::string MODS_INPUT_FOLDER = "sdmc:/ultimate/mod_packs/";
const std::string MODS_FOLDER = "sdmc:/ultimate/mods/";

struct Mod;
struct Fighter;
struct Costume;

struct Costume
{
    Fighter* parent = nullptr;
    uint8_t id;
    bool hasRef = false;
    uint8_t refId;
    std::set<std::string> files;

    ErrorOr<void> Reslot(std::string new_path, uint8_t newSlot);
};

typedef std::array<uint8_t, 121> CostumeArray;

struct Fighter
{
    Mod* parent = nullptr;
    int id;
    std::string name;
    std::unordered_map<uint8_t, Costume> costumes;
    std::set<std::string> files;

    Costume& GetCostume(uint8_t id);
};

struct Mod
{
    std::string path;
    std::string name;
    std::unordered_map<std::string, Fighter> fighters;
    std::set<std::string> files;

    Fighter& GetFighter(std::string name);
};

struct ConfigJson
{
	std::unordered_map<std::string, std::set<std::string>> newDirFiles;
	std::set<std::string> newDirInfos;
	std::unordered_map<std::string, std::string> newDirInfosBase;
	std::unordered_map<std::string, std::set<std::string>> shareToAdded;
	std::unordered_map<std::string, std::set<std::string>> shareToVanilla;
};

bool InitMod(Mod& mod, std::string name);
std::string generateUICharaXML(CostumeArray counts);
std::string generateJson(Mod &mod);

ErrorOr<Mod*> MergeMods(std::vector<Mod*> mods);

void add_new_slot( ConfigJson& resulting_config, std::string dir_info, std::string source_slot, std::string new_slot, std::string share_slot);

void addFilesToDirInfo( ConfigJson& resulting_config, std::string dir_info, json files, std::string target_color);

void addSharedFiles( ConfigJson& resulting_config, json src_files, std::string source_color, std::string target_color, std::string share_slot );