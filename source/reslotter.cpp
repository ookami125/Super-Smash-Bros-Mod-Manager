// #Original code by BluJay <https://github.com/blu-dev> and Jozz <https://github.com/jozz024/ssbu-skin-reslotter>
// #Modified by Coolsonickirby to get it to work with dir addition
// import os
// import shutil
// import sys
// import json
// import re

#include <stdarg.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <set>
#include <utility>
#include <regex>
#include <switch.h>

#include "ghc/filesystem.hpp"
namespace fs = ghc::filesystem;
#include "json.hpp"
using json = nlohmann::json;

// 	global dirs_data
json dirs_data;
// 	global file_array
json::array_t file_array;
// 	global existing_files
std::vector<std::string> existing_files;
// 	global existing_config
json existing_config;
// 	global resulting_config
json resulting_config;
// 	global fighter_files
std::vector<std::string> fighter_files;
//  global known_files
std::vector<std::string> known_files;

std::error_code ec;

void log(const char* fmt, ...)
{
    // initialize use of the variable argument array
    va_list vaArgs;
    va_start(vaArgs, fmt);

    // reliably acquire the size
    // from a copy of the variable argument array
    // and a functionally reliable call to mock the formatting
    va_list vaArgsCopy;
    va_copy(vaArgsCopy, vaArgs);
    const int iLen = std::vsnprintf(NULL, 0, fmt, vaArgsCopy);
    va_end(vaArgsCopy);

    // return a formatted string without risking memory mismanagement
    // and without assuming any compiler or platform specific behavior
    std::vector<char> zc(iLen + 1);
    std::vsnprintf(zc.data(), zc.size(), fmt, vaArgs);
    va_end(vaArgs);
	printf(zc.data());
	consoleUpdate(NULL);
}

//reimplmenting this since fs::copy_file is not implemented
bool copyFile(fs::path from, fs::path to)
{
	std::ifstream src(from, std::ios::binary);
	std::ofstream dst(to, std::ios::binary);
	dst << src.rdbuf();
	return src && dst;
}

bool find(json::array_t j, std::string value)
{
	for (auto& e : j)
	{
		if (e == value)
			return true;
	}
	return false;
}

bool find(json::object_t j, std::string value)
{
	for (auto& e : j)
	{
		if (e.first == value)
			return true;
	}
	return false;
}

bool find(json j, std::string value)
{
	if (j.is_array())
		return find(j.get<json::array_t>(), value);
	else if (j.is_object())
		return find(j.get<json::object_t>(), value);
	return false;
}

// def usage():
//     print("usage: python reslotter.py <mod_directory> <hashes_file> <fighter_name> <current_alt> <target_alt> <share_slot> <out_directory>")
//     sys.exit(2)

void usage(std::string name)
{
	log("usage: %s <mod_directory> <hashes_file> <fighter_name> <current_alt> <target_alt> <share_slot> <out_directory>\n", name.c_str());
}

// def makeDirsFromFile(path):
// 	dirName = os.path.dirname(path)
// 	try:
// 		os.makedirs(dirName)
// 	except:
// 		pass

void makeDirsFromFile(std::string path)
{
	//log("makeDirsFromFile(%s)\n", path.c_str());

	fs::path dirName = fs::path(path);
	if(dirName.has_extension())
		dirName = dirName.parent_path();
	if(!fs::create_directories(dirName, ec))
	{
		if(fs::exists(dirName, ec))
			return;
		log("Failed to create directory %s\n", dirName.string().c_str());
	}
}

std::string replace(std::string str, const std::string& from, const std::string& to) {
	size_t start_pos = 0;
	while((start_pos = str.find(from, start_pos)) != std::string::npos) {
		str.replace(start_pos, from.length(), to);
		start_pos += to.length();
	}
	return str;
}

// def fix_windows_path(path: str, to_linux: bool):
// 	if to_linux:
// 		return path.replace("\\", "/")
// 	else:
// 		return path.replace("/", os.sep)

std::string fix_windows_path(std::string path, bool to_linux)
{
	if (to_linux)
	{
		return replace(path, "\\", "/");
	}
	else
	{
		return replace(path, "/", "\\");
	}
}

// def find_fighter_files(mod_directory):
// 	all_files = []
// 	# list through the dirs in the mod directory
// 	for folders in os.listdir(mod_directory):
// 		full_path = os.path.join(mod_directory, folders)
// 		if os.path.isdir(full_path):
// 			# if the entry in the folder is a directory, walk through its contents and append any files you find to the file list
// 			for root, dirs, files in os.walk(full_path):
// 				if len(files) != 0:
// 					# if files isnt nothing we "iterate" through it to append the file to the file list
// 					for file in files:
// 						full_file_path = os.path.join(root, file)
// 						#toAppend = fix_windows_path(full_file_path, True).lstrip(mod_directory + "/")
// 						toAppend = fix_windows_path(full_file_path, True).replace(mod_directory.replace("\\","/")+"/","")
// 						all_files.append(toAppend)
// 	return all_files

std::vector<std::string> find_fighter_files(std::string mod_directory)
{
	log("finding_fighter_files(%s)\n", mod_directory.c_str());

	std::vector<std::string> all_files;
	auto dir_iter = fs::directory_iterator(fs::path(mod_directory), ec);
	auto itEnd = end(dir_iter);
	for (auto entry = begin(dir_iter); entry != itEnd; entry.increment(ec))
	{
		if (entry->is_directory(ec))
		{
			auto dir_iter2 = fs::recursive_directory_iterator(entry->path(), ec);
			auto itEnd2 = end(dir_iter2);
			for (auto entry2 = begin(dir_iter2); entry2 != itEnd2; entry2.increment(ec))
			{
				if(entry2->path().has_extension())
				{
					std::string full_file_path = entry2->path().string();
					std::string toAppend = replace(fix_windows_path(full_file_path, true), replace(mod_directory, "\\", "/") + "/" , "" );
					all_files.push_back(toAppend);
				}
			}
		}
	}
	return all_files;
}

void add_new_slot(std::string dir_info, std::string source_slot, std::string new_slot, std::string share_slot);
void add_missing_files(std::vector<std::string> reslotted_files, std::string fighter_name, std::string target_alt, bool is_new_slot);

std::string strip(std::string str, char c)
{
	size_t start = 0;
	size_t end = str.length();
	while (start < end && str[start] == c)
	{
		start++;
	}
	while (end > start && str[end - 1] == c)
	{
		end--;
	}
	return str.substr(start, end - start);
}

inline std::string vec2str(std::vector<std::string> vec)
{
	if(vec.size() == 0)
		return "[]";
	std::string str = "[";
	for (std::string s : vec)
	{
		str += "'" + s + "', ";
	}
	return str.substr(0, str.length() - 2) + "]";
}

inline std::string vec2str(std::vector<int> vec)
{
	if(vec.size() == 0)
		return "[]";
	std::string str = "[";
	for (int s : vec)
	{
		str += std::to_string(s) + ", ";
	}
	return str.substr(0, str.length() - 2) + "]";
}

// def reslot_fighter_files(mod_directory, fighter_files, current_alt, target_alt, share_slot, out_dir, fighter_name):
// 	#TODO: If not excluding, only run through fighter_files once. Then properly generate a config
// 	#Maybe the fighter_files part should be moved to main()
// 	reslotted_files = []
// 	for file in fighter_files:
// 		#Exclude any other file outside of the current_alt
// 		if (not current_alt.strip('c') in file):
// 			continue

std::pair<std::vector<std::string>, std::vector<std::string>> reslot_fighter_files(std::string mod_directory, std::vector<std::string> fighter_files, std::string current_alt, std::string target_alt, std::string share_slot, std::string out_dir, std::string fighter_name)
{
	log("reslot_fighter_files(%s, %s, %s, %s, %s, %s, %s)\n", mod_directory.c_str(), vec2str(fighter_files).c_str(), current_alt.c_str(), target_alt.c_str(), share_slot.c_str(), out_dir.c_str(), fighter_name.c_str());
	
	std::vector<std::string> reslotted_files;
	std::vector<std::string> config_files;
	for (std::string file : fighter_files)
	{
		if (file.find(strip(current_alt, 'c')) == std::string::npos)
		{
			continue;
		}

// 		# Since each directory has a different structure, we have to go through each directory separately
// 		if file.startswith(f"fighter/{fighter_name}"):
// 			if (not "/"+current_alt+"/" in file):
// 				continue
			
// 			lookfor = f"/{current_alt}/"
// 			replace = f"/{target_alt}/"
// 			new_file = file.replace(lookfor, replace)
			
// 			#Used during "reconfig" to not copy files and simply add to the list of files for the config
// 			if out_dir != "":
// 				makeDirsFromFile(os.path.join(out_dir, new_file))
// 				shutil.copy(os.path.join(mod_directory, file), os.path.join(out_dir, new_file))

// 			reslotted_files.append(new_file)

		if (file.find("fighter/" + fighter_name) == 0)
		{
			if (file.find("/" + current_alt + "/") == std::string::npos)
			{
				continue;
			}

			std::string lookfor = "/" + current_alt + "/";
			std::string replace = "/" + target_alt + "/";
			std::string new_file = ::replace(file, lookfor, replace);

			if (out_dir != "")
			{
				makeDirsFromFile(out_dir + "/" + new_file);
				//log("Copying file: %s to %s\n", (mod_directory + "/" + file).c_str(), (out_dir + "/" + new_file).c_str());
				if(!copyFile(mod_directory + "/" + file, out_dir + "/" + new_file))
				{
					log("fail to copy file: %s to %s\n", (mod_directory + "/" + file).c_str(), (out_dir + "/" + new_file).c_str());
				}
			}

			reslotted_files.push_back(new_file);
		}

// 		#Unique to UI folders, we need to check if the filename contains 
// 		#"_fighter_name_" since all UI files are grouped together
// 		elif file.startswith("ui/replace/chara") or file.startswith("ui/replace_patch/chara"):
// 			lookfor = f"{current_alt.strip('c')}.bntx"
// 			replace = f"{target_alt.strip('c')}.bntx"
// 			new_file = file.replace(lookfor, replace)

// 			fighter_keys = [fighter_name]
// 			#Ice Climber / Aegis Stuff
// 			if (fighter_name=="popo" or fighter_name=="nana"):
// 				fighter_keys = ["ice_climber"]
// 			elif (fighter_name=="eflame"):
// 				fighter_keys = ["eflame_first","eflame_only"]
// 			elif (fighter_name=="elight"):
// 				fighter_keys = ["elight_first","elight_only"]

// 			for key in fighter_keys:
// 				if new_file.__contains__("_" + key + "_") and out_dir != "":
// 					makeDirsFromFile(os.path.join(out_dir, new_file))
// 					shutil.copy(os.path.join(mod_directory, file), os.path.join(out_dir, new_file))

		else if (file.find("ui/replace/chara") == 0 || file.find("ui/replace_patch/chara") == 0)
		{
			std::string lookfor = strip(current_alt, 'c') + ".bntx";
			std::string replace = strip(target_alt, 'c') + ".bntx";
			std::string new_file = ::replace(file, lookfor, replace);

			std::vector<std::string> fighter_keys = { fighter_name };

			if (fighter_name == "popo" || fighter_name == "nana")
			{
				fighter_keys = { "ice_climber" };
			}
			else if (fighter_name == "eflame")
			{
				fighter_keys = { "eflame_first", "eflame_only" };
			}
			else if (fighter_name == "elight")
			{
				fighter_keys = { "elight_first", "elight_only" };
			}

			for (std::string key : fighter_keys)
			{
				if (new_file.find("_" + key + "_") != std::string::npos && out_dir != "")
				{
					makeDirsFromFile(out_dir + "/" + new_file);
					//log("Copying file: %s to %s\n", (mod_directory + "/" + file).c_str(), (out_dir + "/" + new_file).c_str());
					if(!copyFile(mod_directory + "/" + file, out_dir + "/" + new_file))
					{
						log("fail to copy file: %s to %s\n", (mod_directory + "/" + file).c_str(), (out_dir + "/" + new_file).c_str());
					}
				}
			}
		}

// 		elif file.startswith(f"sound/bank/fighter/se_{fighter_name}") or file.startswith(f"sound/bank/fighter_voice/vc_{fighter_name}"):
// 			lookfor = f"_{current_alt}"
// 			replace = f"_{target_alt}"
// 			new_file = file.replace(lookfor, replace)

// 			if out_dir != "":
// 				makeDirsFromFile(os.path.join(out_dir, new_file))
// 				shutil.copy(os.path.join(mod_directory, file), os.path.join(out_dir, new_file))
			
// 			reslotted_files.append(new_file)

		else if (file.find("sound/bank/fighter/se_" + fighter_name) == 0 || file.find("sound/bank/fighter_voice/vc_" + fighter_name) == 0)
		{
			std::string lookfor = "_" + current_alt;
			std::string replace = "_" + target_alt;
			std::string new_file = ::replace(file, lookfor, replace);

			if (out_dir != "")
			{
				makeDirsFromFile(out_dir + "/" + new_file);
				//log("Copying file: %s to %s\n", (mod_directory + "/" + file).c_str(), (out_dir + "/" + new_file).c_str());
				if(!copyFile(mod_directory + "/" + file, out_dir + "/" + new_file))
				{
					log("fail to copy file: %s to %s\n", (mod_directory + "/" + file).c_str(), (out_dir + "/" + new_file).c_str());
				}
			}

			reslotted_files.push_back(new_file);
		}

// 		elif file.startswith(f"effect/fighter/{fighter_name}"):
// 			lookfor = f"{current_alt.strip('c')}"
// 			replace = f"{target_alt.strip('c')}"
// 			new_file = file.replace(lookfor, replace)
// 			if out_dir != "":
// 				makeDirsFromFile(os.path.join(out_dir, new_file))
// 				shutil.copy(os.path.join(mod_directory, file), os.path.join(out_dir, new_file))

// 			#Prevent duplicates
// 			reslotted_files.append(new_file)

		else if (file.find("effect/fighter/" + fighter_name) == 0)
		{
			std::string lookfor = strip(current_alt, 'c');
			std::string replace = strip(target_alt, 'c');
			std::string new_file = ::replace(file, lookfor, replace);
			if (out_dir != "")
			{
				makeDirsFromFile(out_dir + "/" + new_file);
				//log("Copying file: %s to %s\n", (mod_directory + "/" + file).c_str(), (out_dir + "/" + new_file).c_str());
				if(!copyFile(mod_directory + "/" + file, out_dir + "/" + new_file))
				{
					log("fail to copy file: %s to %s\n", (mod_directory + "/" + file).c_str(), (out_dir + "/" + new_file).c_str());
				}
			}

			reslotted_files.push_back(new_file);
		}
	}
// 	existing_files.extend(reslotted_files)
// 	append_to_file("log.txt", "existing_files:\n")
// 	for file in existing_files:
// 		append_to_file("log.txt", "\t" + file + "\n")

// 	if 7 < int(target_alt.strip("c")):
// 		current_alt_int = int(current_alt.strip("c"))
// 		share_alt_int = int(share_slot.strip("c")) % 8
// 		if current_alt_int <= 7:
// 			add_new_slot(f"fighter/{fighter_name}", current_alt, target_alt,"c0"+str(share_alt_int))
// 			add_missing_files(reslotted_files, fighter_name, target_alt,True)
// 		else:
// 			current_alt_int = int(target_alt.strip("c")) % 8
// 			add_new_slot(f"fighter/{fighter_name}", f"c0{current_alt_int}", target_alt,"c0"+str(share_alt_int))
// 			add_missing_files(reslotted_files, fighter_name, target_alt,True)
// 	else:
// 		add_missing_files(reslotted_files, fighter_name, target_alt)

// 	return reslotted_files, fighter_files

	existing_files.insert(existing_files.end(), reslotted_files.begin(), reslotted_files.end());

	if (7 < std::stoi(strip(target_alt, 'c')))
	{
		int current_alt_int = std::stoi(strip(current_alt, 'c'));
		int share_alt_int = std::stoi(strip(share_slot, 'c')) % 8;
		if (current_alt_int <= 7)
		{
			add_new_slot("fighter/" + fighter_name, current_alt, target_alt, "c0" + std::to_string(share_alt_int));
			add_missing_files(reslotted_files, fighter_name, target_alt, true);
		}
		else
		{
			current_alt_int = std::stoi(strip(target_alt, 'c')) % 8;
			add_new_slot("fighter/" + fighter_name, "c0" + std::to_string(current_alt_int), target_alt, "c0" + std::to_string(share_alt_int));
			add_missing_files(reslotted_files, fighter_name, target_alt, true);
		}
	}
	else
	{
		add_missing_files(reslotted_files, fighter_name, target_alt, false);
	}

	return std::make_pair(reslotted_files, fighter_files);
}

// def add_missing_files(reslotted_files, fighter_name, target_alt, is_new_slot=False):
// 	#append_to_file("log.txt", "add_missing_files([" + vec2str(reslotted_files) + "]" + fighter_name + ", " + target_alt + ")\n")

// 	# make a variable that holds the dirinfo path for the new slot
// 	new_dir_info = f"fighter/{fighter_name}/{target_alt}"
// 	# we have to do config separately if it's an added slot because those require extra config options

// 	if new_dir_info not in resulting_config["new-dir-files"]:
// 		resulting_config["new-dir-files"][new_dir_info] = []

// 	for file in reslotted_files:
// 		#Don't add oneslot effects to vanilla alts configs
// 		if (not is_new_slot and "effect" in file):
// 			continue
// 		if file not in known_files:
// 			resulting_config["new-dir-files"][new_dir_info].append(file)

void add_missing_files(std::vector<std::string> reslotted_files, std::string fighter_name, std::string target_alt, bool is_new_slot)
{
	log("add_missing_files(%s, %s, %s, %s)\n", vec2str(reslotted_files).c_str(), fighter_name.c_str(), target_alt.c_str(), is_new_slot ? "True" : "False");
	
	std::string new_dir_info = "fighter/" + fighter_name + "/" + target_alt;

	if (!find(resulting_config["new-dir-files"], new_dir_info))
	{
		resulting_config["new-dir-files"][new_dir_info] = json::array_t();
	}

	for (std::string file : reslotted_files)
	{
		if (!is_new_slot && file.find("effect") != std::string::npos)
		{
			continue;
		}
		if (std::find(known_files.begin(), known_files.end(), file) == known_files.end())
		{
			resulting_config["new-dir-files"][new_dir_info].push_back(file);
		}
	}
}

std::vector<std::string> split(std::string str, char c)
{
	std::vector<std::string> result;
	std::string temp;
	for (char i : str)
	{
		if (i == c)
		{
			result.push_back(temp);
			temp = "";
		}
		else
		{
			temp += i;
		}
	}
	result.push_back(temp);
	return result;
}

void addFilesToDirInfo(std::string dir_info, std::vector<int> files, std::string target_color);
void addSharedFiles(std::vector<int> src_files, std::string source_color, std::string target_color, std::string share_slot);

// def add_new_slot(dir_info, source_slot, new_slot, share_slot):
// 	#append_to_file("log.txt", "add_new_slot(" + dir_info + ", " + source_slot + ", " + new_slot + ", " + share_slot + ")\n")

// 	folders = dir_info.split("/")
// 	target_dir = dirs_data

// 	for folder in folders:
// 		target_dir = target_dir["directories"][folder]

// 	if source_slot in target_dir["directories"]:
// 		source_slot_dir = target_dir["directories"][source_slot]
// 		source_slot_path = "%s/%s" % ((dir_info, source_slot))
// 		new_slot_dir_path = "%s/%s" % ((dir_info, new_slot))
// 		share_slot_dir = target_dir["directories"][share_slot]
// 		share_slot_path = "%s/%s" % ((dir_info, share_slot))

// 		if (not new_slot_dir_path in resulting_config["new-dir-infos"]):
// 			resulting_config["new-dir-infos"].append(new_slot_dir_path)

// 		# Deal with files
// 		addFilesToDirInfo(new_slot_dir_path, share_slot_dir["files"], new_slot)
// 		addSharedFiles(share_slot_dir["files"], source_slot, new_slot,share_slot)

// 		for dir in source_slot_dir["directories"]:
// 			source_slot_base = f"{source_slot_path}/{dir}"
// 			new_slot_base = f"{new_slot_dir_path}/{dir}"
// 			share_slot_base = f"{share_slot_path}/{dir}"
// 			resulting_config["new-dir-infos-base"][new_slot_base] = share_slot_base

// 	for dir in target_dir["directories"]:
// 		target_obj = target_dir["directories"][dir]
// 		if source_slot in target_obj["directories"]:
// 			source_slot_dir = target_obj["directories"][source_slot]
// 			source_slot_path = f"{dir_info}/{dir}/{source_slot}"
// 			new_slot_dir_path = f"{dir_info}/{dir}/{new_slot}"
// 			share_slot_dir = target_obj["directories"][share_slot]
// 			share_slot_path = f"{dir_info}/{dir}/{share_slot}"

// 			if (not new_slot_dir_path in resulting_config["new-dir-infos"]):
// 				resulting_config["new-dir-infos"].append(new_slot_dir_path)

// 			# Deal with files
// 			addFilesToDirInfo(new_slot_dir_path, share_slot_dir["files"], new_slot)
// 			addSharedFiles(share_slot_dir["files"], source_slot, new_slot,share_slot)

// 			# Deal with directories
// 			for child_dir in source_slot_dir["directories"]:
// 				source_slot_base = f"{source_slot_path}/{child_dir}"
// 				new_slot_base = f"{new_slot_dir_path}/{child_dir}"
// 				share_slot_base = f"{share_slot_path}/{child_dir}"
// 				resulting_config["new-dir-infos-base"][new_slot_base] = share_slot_base

void add_new_slot(std::string dir_info, std::string source_slot, std::string new_slot, std::string share_slot)
{
	log("add_new_slot(%s, %s, %s, %s)\n", dir_info.c_str(), source_slot.c_str(), new_slot.c_str(), share_slot.c_str());
	
	std::vector<std::string> folders = split(dir_info, '/');
	json target_dir = dirs_data;

	for (std::string folder : folders)
	{
		target_dir = target_dir["directories"][folder];
	}

	if (target_dir["directories"].find(source_slot) != target_dir["directories"].end())
	{
		json source_slot_dir = target_dir["directories"][source_slot];
		std::string source_slot_path = dir_info + "/" + source_slot;
		std::string new_slot_dir_path = dir_info + "/" + new_slot;
		json share_slot_dir = target_dir["directories"][share_slot];
		std::string share_slot_path = dir_info + "/" + share_slot;

		if (std::find(resulting_config["new-dir-infos"].begin(), resulting_config["new-dir-infos"].end(), new_slot_dir_path) == resulting_config["new-dir-infos"].end())
		{
			resulting_config["new-dir-infos"].push_back(new_slot_dir_path);
		}

		auto share_slot_dir_files = share_slot_dir["files"];

		addFilesToDirInfo(new_slot_dir_path, share_slot_dir_files, new_slot);
		addSharedFiles(share_slot_dir_files, source_slot, new_slot, share_slot);

		for (auto dir : source_slot_dir["directories"].items())
		{
			std::string source_slot_base = source_slot_path + "/" + dir.key();
			std::string new_slot_base = new_slot_dir_path + "/" + dir.key();
			std::string share_slot_base = share_slot_path + "/" + dir.key();
			resulting_config["new-dir-infos-base"][new_slot_base] = share_slot_base;
		}
	}

	std::vector<std::string> target_dir_directories;
	for (auto index : target_dir["directories"].items())
	{
		target_dir_directories.push_back(index.key());
	}
	std::sort(target_dir_directories.begin(), target_dir_directories.end());

	for (auto index : target_dir_directories)
	{
		std::string dir = index;
		json target_obj = target_dir["directories"][dir];
		if (target_obj["directories"].find(source_slot) != target_obj["directories"].end())
		{
			json source_slot_dir = target_obj["directories"][source_slot];
			std::string source_slot_path = dir_info + "/" + dir + "/" + source_slot;
			std::string new_slot_dir_path = dir_info + "/" + dir + "/" + new_slot;
			json share_slot_dir = target_obj["directories"][share_slot];
			std::string share_slot_path = dir_info + "/" + dir + "/" + share_slot;

			if (std::find(resulting_config["new-dir-infos"].begin(), resulting_config["new-dir-infos"].end(), new_slot_dir_path) == resulting_config["new-dir-infos"].end())
			{
				resulting_config["new-dir-infos"].push_back(new_slot_dir_path);
			}

			addFilesToDirInfo(new_slot_dir_path, share_slot_dir["files"], new_slot);
			addSharedFiles(share_slot_dir["files"], source_slot, new_slot, share_slot);

			for (auto child_dir : source_slot_dir["directories"].items())
			{
				std::string source_slot_base = source_slot_path + "/" + child_dir.key();
				std::string new_slot_base = new_slot_dir_path + "/" + child_dir.key();
				std::string share_slot_base = share_slot_path + "/" + child_dir.key();
				resulting_config["new-dir-infos-base"][new_slot_base] = share_slot_base;
			}
		}
	}
}

// def addFilesToDirInfo(dir_info, files, target_color):
// 	#append_to_file("log.txt", "addFilesToDirInfo(" + dir_info + ", " + target_color + ")\n")

// 	if dir_info not in resulting_config["new-dir-files"]:
// 		resulting_config["new-dir-files"][dir_info] = []

// 	for index in files:
// 		file_path = file_array[index]
// 		if file_path.startswith("0x"):
// 			continue
// 		new_file_path = re.sub(r"c0[0-9]", target_color, file_path, 1)
// 		if new_file_path in resulting_config["new-dir-files"][dir_info]:
// 			continue
// 		resulting_config["new-dir-files"][dir_info].append(new_file_path)

void addFilesToDirInfo(std::string dir_info, std::vector<int> files, std::string target_color)
{
	log("addFilesToDirInfo(%s, %s, %s)\n", dir_info.c_str(), vec2str(files).c_str(), target_color.c_str());
	
	if (resulting_config["new-dir-files"].find(dir_info) == resulting_config["new-dir-files"].end())
	{
		resulting_config["new-dir-files"][dir_info] = json::array_t();
	}

	for (int index : files)
	{
 		std::string file_path = file_array[index];
		if (file_path.find("0x") == 0)
		{
			continue;
		}
		std::string new_file_path = std::regex_replace(file_path, std::regex("c0[0-9]"), target_color, std::regex_constants::format_first_only);

		bool found = false;
		for(auto& element : resulting_config["new-dir-files"][dir_info])
		{
			if (element == new_file_path)
			{
				found = true;
				break;
			}
		}
		if(found)
		{
			continue;
		}
		//if (resulting_config["new-dir-files"][dir_info].find(new_file_path) != resulting_config["new-dir-files"][dir_info].end())
		//{
		//	continue;
		//}
		resulting_config["new-dir-files"][dir_info].push_back(new_file_path);
	}
}

// def addSharedFiles(src_files, source_color, target_color, share_slot):
// 	append_to_file("log.txt", "addSharedFiles(" + source_color + ", " + target_color + ", " + share_slot + ")\n");

// 	used_files = []

// 	for index in src_files:
// 		file_path = file_array[index]
// 		append_to_file("log.txt", "for: " + file_path + "\t");
// 		if file_path.startswith("0x"):
// 			append_to_file("log.txt", "continue (0x)\n");
// 			continue
// 		temp_file_path = file_path.replace(r"/c0[0-9]/", source_color)
// 		append_to_file("log.txt", "\n\ttemp_file_path: " + temp_file_path + "\t")
// 		if temp_file_path in used_files:
// 			append_to_file("log.txt", "continue (used)\n");
// 			continue
// 		used_files.append(file_path)

// 		#file_path = file_path.replace(r"/c0[0-9]/", share_slot)

// 		new_file_path = re.sub(r"c0[0-9]", target_color, file_path, 1)
// 		append_to_file("log.txt", "\n\tnew_file_path: " + new_file_path + "\t")
// 		if new_file_path in existing_files:
// 			append_to_file("log.txt", "continue (existing file)\n");
// 			continue

// 		share_to = "share-to-vanilla"
// 		if "motion/" in file_path or "camera/" in file_path:
// 			append_to_file("log.txt", "added shareToAdded\n");
// 			share_to = "share-to-added"
// 		else:
// 			append_to_file("log.txt", "added shareToVanilla\n");
			

// 		if file_path not in resulting_config[share_to]:
// 			resulting_config[share_to][file_path] = []
		
// 		if new_file_path not in resulting_config[share_to][file_path]:
// 			resulting_config[share_to][file_path].append(new_file_path)

void addSharedFiles(std::vector<int> src_files, std::string source_color, std::string target_color, std::string share_slot)
{
	log("addSharedFiles(%s, %s, %s, %s)\n", vec2str(src_files).c_str(), source_color.c_str(), target_color.c_str(), share_slot.c_str());
	
	std::vector<std::string> used_files;

	for (int index : src_files)
	{
		std::string file_path = file_array[index];
		if (file_path.find("0x") == 0)
		{
			continue;
		}
		
		std::string temp_file_path = file_path;//std::regex_replace(file_path, std::regex("c0[0-9]"), source_color);
		if (std::find(used_files.begin(), used_files.end(), temp_file_path) != used_files.end())
		{
			continue;
		}
		used_files.push_back(file_path);

		std::string new_file_path = std::regex_replace(file_path, std::regex("c0[0-9]"), target_color, std::regex_constants::format_first_only);
		if (std::find(existing_files.begin(), existing_files.end(), new_file_path) != existing_files.end())
		{
			continue;
		}

		std::string share_to = "share-to-vanilla";
		if (file_path.find("motion/") != std::string::npos || file_path.find("camera/") != std::string::npos)
		{
			share_to = "share-to-added";
		}
		
		if (!find(resulting_config[share_to], file_path))
		{
			resulting_config[share_to][file_path] = json::array_t();
		}

		if (!find(resulting_config[share_to][file_path], new_file_path))
		{
			resulting_config[share_to][file_path].push_back(new_file_path);
		}
	}
}

static const std::vector<std::string> characterList = { "random", "mario", "donkey",
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

typedef std::array<uint8_t, 121> CostumeArray;
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

// def main(mod_directory, hashes_file, fighter_name, current_alt, target_alt, share_slot,out_dir):
// 	# load dir_info_with_files_trimmed.json for dir addition config gen
// 	global dirs_data
// 	global file_array
// 	global existing_files
// 	global existing_config
// 	global resulting_config
// 	global fighter_files
// 	fighter_files = find_fighter_files(mod_directory)
// 	#print(fighter_files)
// 	existing_config = {
// 		"new-dir-infos": [],
// 		"new-dir-infos-base": {},
// 		"share-to-vanilla": {},
// 		"share-to-added": {},
// 		"new-dir-files": {}
// 	}
// 	#If there's an existing config, load it into existing_config to be transferred to resulting_config
// 	resulting_config = existing_config

// 	existing_files = []
// 	# get all of the files in SSBU's Filesystem
// 	global known_files
// 	known_files = set(map(lambda x: x.strip(), open(hashes_file, 'r').readlines()))
// 	with open("dir_info_with_files_trimmed.json", "r") as f:
// 		res = json.load(f)
// 		dirs_data = res["dirs"]
// 		file_array = res["file_array"]
// 		f.close()
// 	if (not os.path.exists(out_dir)) and out_dir!="":
// 		os.mkdir(out_dir)

// 	reslotted_files, new_fighter_files = reslot_fighter_files(mod_directory, fighter_files, current_alt, target_alt, share_slot, out_dir, fighter_name)

static CostumeArray costumes = {};
int run(std::string mod_directory, std::string fighter_name, std::string current_alt, std::string target_alt, std::string share_slot, std::string out_dir)
{
	log("main(%s, %s, %s, %s, %s, %s)\n", mod_directory.c_str(), fighter_name.c_str(), current_alt.c_str(), target_alt.c_str(), share_slot.c_str(), out_dir.c_str());

	// load dir_info_with_files_trimmed.json for dir addition config gen
	//global_dirs_data;
	//global_file_array;
	//global_existing_files;
	//global_existing_config;
	//global_resulting_config;
	//global_fighter_files;
	fighter_files = find_fighter_files(mod_directory);

	//existing_config = {
	//	"new-dir-infos": std::vector<std::string>(),
	//	"new-dir-infos-base": std::map<std::string, std::string>(),
	//	"share-to-vanilla": std::map<std::string, std::vector<std::string>>(),
	//	"share-to-added": std::map<std::string, std::vector<std::string>>(),
	//	"new-dir-files": std::map<std::string, std::vector<std::string>>()
	//};
	//existing_config["new-dir-infos"] = json::array_t();
	//existing_config["new-dir-infos-base"] = json::object_t();
	//existing_config["share-to-vanilla"] = json::object_t();
	//existing_config["share-to-added"] = json::object_t();
	//existing_config["new-dir-files"] = json::object_t();

	//If there's an existing config, load it into existing_config to be transferred to resulting_config
	//resulting_config = existing_config;

	existing_files = std::vector<std::string>();
	// get all of the files in SSBU's Filesystem
	//global_known_files;
	// with open("dir_info_with_files_trimmed.json", "r") as f:
	// 	res = json.load(f)
	// 	dirs_data = res["dirs"]
	// 	file_array = res["file_array"]
	// 	f.close()
	if (!fs::exists(out_dir, ec) && out_dir != "")
	{
		fs::create_directory(out_dir, ec);
	}

	std::vector<std::string> reslotted_files;
	std::vector<std::string> new_fighter_files;
	std::tie(reslotted_files, new_fighter_files) = reslot_fighter_files(mod_directory, fighter_files, current_alt, target_alt, share_slot, out_dir, fighter_name);

	{
		int fighterId = std::find(characterList.begin(), characterList.end(), fighter_name) - characterList.begin();
		costumes[fighterId]++;
	}

	//consoleExit(NULL);
	return 0;
}
// if __name__ == "__main__":
//     try:
//         main(sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4], sys.argv[5], sys.argv[6],sys.argv[7])
//     except IndexError:
//         usage()

PadState pad;
void waitForInput()
{
    // Main loop
    while (appletMainLoop())
    {
        padUpdate(&pad);
		u64 kDown = padGetButtonsDown(&pad);
        if (kDown & HidNpadButton_Plus)
            break;
        consoleUpdate(NULL);
    }
}

int main_single(int argc, char** argv)
{
	consoleInit(NULL);

	if (argc < 7)
	{
		usage(argv[0]);
		return 1;
	}

	std::string out_dir = argv[6];

	//fs::remove_all(out_dir, ec);
	//fs::create_directory(out_dir, ec);

	{
		known_files = std::vector<std::string>();
		std::string hashes_file = "Hashes_all.txt";
		std::ifstream file(hashes_file);
		std::string str;
		while (std::getline(file, str))
		{
			known_files.push_back(str);
		}

		file.close();
		{
			std::ifstream f("dir_info_with_files_trimmed.json");
			json res = json::parse(f);
			dirs_data = res["dirs"];
			file_array = res["file_array"].get<nlohmann::json::array_t>();
			f.close();
		}
	}

	run(argv[1], argv[2], argv[3], argv[4], argv[5], out_dir);

	{
		std::string newConfigLocation = out_dir + "/config.json";
		std::ofstream f(newConfigLocation);
		f << resulting_config.dump(4);
		f.close();
	}
	{
		fs::path xmlPath = out_dir + "/ui/param/database/ui_chara_db.prcxml";
		makeDirsFromFile(xmlPath);
		std::string xml = generateUICharaXML(costumes);
		std::ofstream f(xmlPath);
		f << xml;
		f.close();
	}


	waitForInput();
	consoleExit(NULL);
	return 0;
}

int main_all(int argc, char** argv)
{
	consoleInit(NULL);

	padConfigureInput(1, HidNpadStyleSet_NpadStandard);
	padInitializeDefault(&pad);

	//fs::remove("log.txt", ec);

	{
		known_files = std::vector<std::string>();
		std::string hashes_file = "Hashes_all.txt";
		std::ifstream file(hashes_file);
		std::string str;
		while (std::getline(file, str))
		{
			known_files.push_back(str);
		}
		file.close();
		{
			std::ifstream f("dir_info_with_files_trimmed.json");
			json res = json::parse(f);
			dirs_data = res["dirs"];
			file_array = res["file_array"].get<nlohmann::json::array_t>();
			f.close();
		}
	}
	//fs::remove_all("/ultimate/mods/SSBUMM", ec);
	//if(ec)
	//{
	//	log("ERROR REMOVING OLD MOD DIR: %s\n", ec.message().c_str());
	//	consoleExit(NULL);
	//	return 1;
	//}

	std::string mods_directory = "test_mods";
	//std::string mod_directory = "/ultimate/mod_packs/DonkeyKong_FunkyKong";
	std::string out_dir = "output_mod";

	struct ModEntry {
		std::string mod_name;
		std::string character_name;
		std::string costume;
	};
	std::set<std::string> characterCostumePair;
	std::vector<ModEntry> modEntries;
	auto dirIter = fs::recursive_directory_iterator(mods_directory, ec);
	auto endIter = end(dirIter);
	for (; dirIter != endIter; dirIter.increment(ec))
	{
		fs::path p = dirIter->path();
		if (p.has_extension())
		{
			std::string name = "";
			for(auto characterName : characterList)
			{
				if (p.string().find("/" + characterName + "/") != std::string::npos)
				{
					name = characterName;
					log("Found character %s\n", name.c_str());
					break;
				}
			}

			if(name == "")	continue;

			//use regex to find the costume in full path matching /cXX/ where XX is a 2 digit number
			std::regex costumeRegex("/c[0-9][0-9]/");
			std::smatch match;
			std::string fullPath = p.string();
			std::string costume = "";
			if(std::regex_search(fullPath, match, costumeRegex))
			{
				costume = match[0].str();
				costume = costume.substr(1, 3);
				log("Found costume %s\n", costume.c_str());
			}

			if(costume == "")	continue;

			std::string modName = fs::proximate(p, mods_directory, ec).begin()->string();

			if(characterCostumePair.insert(modName + ":" + name + ":" + costume).second)
			{
				log("Mod: %s, Character: %s, Costume: %s\n", modName.c_str(), name.c_str(), costume.c_str());
				modEntries.push_back({modName, name, costume});
			}
		}
	}

	for(auto& modEntry : modEntries)
	{
	}

	for(auto& modEntry : modEntries)
	{
		int fighterId = std::find(characterList.begin(), characterList.end(), modEntry.character_name) - characterList.begin();
		std::string newCostume = std::to_string(costumes[fighterId] + 8);
		if(newCostume.size() == 1)
			newCostume = "0" + newCostume;
		newCostume = "c" + newCostume;
		std::string referenceCostume = modEntry.costume;
		//if referenceCostume >= 8, then we need to modulo 8 from it
		uint8_t referenceCostumeNum = std::stoi(referenceCostume.substr(1));
		if(referenceCostumeNum >= 8)
		{
			referenceCostumeNum %= 8;
			referenceCostume = std::to_string(referenceCostumeNum);
			if(referenceCostume.size() == 1)
				referenceCostume = "0" + referenceCostume;
			referenceCostume = "c" + referenceCostume;
		}
		run(mods_directory + "/" + modEntry.mod_name, modEntry.character_name, modEntry.costume, newCostume, referenceCostume, out_dir);
	}

	{
		std::string newConfigLocation = out_dir + "/config.json";
		std::ofstream f(newConfigLocation);
		f << resulting_config.dump(4);
		f.close();
	}
	{
		fs::path xmlPath = out_dir + "/ui/param/database/ui_chara_db.prcxml";
		makeDirsFromFile(xmlPath);
		std::string xml = generateUICharaXML(costumes);
		std::ofstream f(xmlPath);
		f << xml;
		f.close();
	}

	consoleUpdate(NULL);
	waitForInput();

	consoleExit(NULL);
	return 0;
}

int main(int argc, char** argv)
{
	consoleInit(NULL);
	log("SSBU Mod Manager v0.1\n");
	if(argc < 2)
	{
		log("Usage: %s (all)\n", argv[0]);
		consoleUpdate(NULL);
		waitForInput();
		consoleExit(NULL);
		return 1;
	}
	if(strcmp(argv[1], "all") == 0)
	{
		printf("Running all\n");
		main_all(argc, argv);
		consoleUpdate(NULL);
		waitForInput();
		consoleExit(NULL);
		return 0;
	}

	if(argc < 7)
	{
		log("Usage: %s <modpack directory> <character name> <costume name> <new costume name> <reference costume name>\n", argv[0]);
		consoleUpdate(NULL);
		waitForInput();
		consoleExit(NULL);
		return 1;
	}
	main_single(argc, argv);
	consoleUpdate(NULL);
	waitForInput();
	consoleExit(NULL);
	return 0;
}