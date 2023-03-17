#define __LARGE64_FILES
#include "fs.h"
#include <dirent.h>
#include <sys/stat.h>
#include <fstream>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <stdint.h>

#include <cstring>
#include <string>
#include <cerrno>
#include <memory>
#include "str.h"

#include <switch.h>

std::string getRelativePath(std::string root, std::string path)
{
    std::string relativePath = path;
    if(relativePath.find(root) == 0)
    {
        relativePath = relativePath.substr(root.length());
    }
    return relativePath;
}

std::vector<std::string> GetAllFilesRecursively(std::string path)
{
    std::vector<std::string> files;
    DIR* dir = opendir(path.c_str());
    if(dir==NULL)
    {
        printf("Failed read files from folder \"%s\".\n", path.c_str());
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
                    std::string subpath = path + ent->d_name + "/";
                    std::vector<std::string> subfiles = GetAllFilesRecursively(subpath);
                    files.insert(files.end(), subfiles.begin(), subfiles.end());
                }
            }
            else
            {
                files.push_back(path + ent->d_name);
            }
        }
        closedir(dir);
    }
    return files;
}

//mkdir recursive
ErrorOr<void> _mkdir(std::string path, bool recursive)
{
    int ret;
    mode_t mode = 0777;
    if(!recursive)
    {
        ret = ::mkdir(path.c_str(), mode);
        if (ret != 0) {
            return ErrorOr<void>("Failed to create directory");
        }
    }
    else
    {
        ret = ::mkdir(path.c_str(), mode);
        if (ret != 0)
        {
            if (errno == ENOENT)
            {
                size_t pos = path.find_last_of('/');
                if (pos == std::string::npos)
                    return ErrorOr<void>("Failed to create directory");
                std::string parent_path = path.substr(0, pos);
                ErrorOr<void> error_msg = _mkdir(parent_path, recursive);
                if (error_msg.IsError()) {
                    return error_msg;
                }
                ret = ::mkdir(path.c_str(), mode);
                if (ret != 0) {
                    return ErrorOr<void>("Failed to create directory");
                }
            }
            else if (errno == EEXIST)
            {
                struct stat info;
                if (stat(path.c_str(), &info) != 0)
                    return ErrorOr<void>("Failed to stat directory");
                else if (info.st_mode & S_IFDIR)
                    return ErrorOr<void>();
                else
                    return ErrorOr<void>("File exists with the same name as directory");
            }
            else
            {
                return ErrorOr<void>("Failed to create directory");
            }
        }
    }
    return ErrorOr<void>();
}

ErrorOr<void> moveFile(std::string from, std::string to)
{
    auto err = _mkdir(to.substr(0, to.find_last_of('/')), true);
    if (err.IsError())
        return err;
    if (rename(from.c_str(), to.c_str()) != 0)
        return ErrorOr<void>(vformat("Failed rename \"%s\".\n", to.c_str()));
    return ErrorOr<void>();
}

ErrorOr<void> copyFile(std::string src_path, std::string dst_path) {
    const int buf_size = 4096;
    char buf[buf_size];
    int src_fd = open(src_path.c_str(), O_RDONLY);
    if (src_fd == -1) {
        return ErrorOr<void>(vformat("Failed to open source file (%s)\n", src_path.c_str()));
    }

    int dst_fd = open(dst_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dst_fd == -1) {
        close(src_fd);
        return ErrorOr<void>("Failed to open destination file\n");
    }

    ssize_t num_read;
    while ((num_read = read(src_fd, buf, buf_size)) > 0) {
        ssize_t num_written = write(dst_fd, buf, num_read);
        if (num_written == -1) {
            close(src_fd);
            close(dst_fd);
            return ErrorOr<void>("Failed to write to destination file\n");
        }
    }

    if (num_read == -1) {
        close(src_fd);
        close(dst_fd);
        return ErrorOr<void>("Failed to read from source file\n");
    }

    close(src_fd);
    close(dst_fd);
    return ErrorOr<void>();
}

bool fileExists(std::string path)
{
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

ErrorOr<void> deleteFile(std::string path)
{
    if (remove(path.c_str()) != 0)
        return ErrorOr<void>(vformat("Failed to delete file \"%s\".\n", path.c_str()));
    return ErrorOr<void>();
}

ErrorOr<void> appendToFile(std::string path, std::string text)
{
    std::ofstream file;
    file.open(path, std::ios::app);
    if (!file.is_open())
        return ErrorOr<void>(vformat("Failed to open file \"%s\".\n", path.c_str()));
    file << text;
    file.close();
    return ErrorOr<void>();
}

ErrorOr<void> deleteFolder(const std::string& path) {
    DIR *dir = nullptr;
    struct dirent *entry = nullptr;
    dir = opendir(path.c_str());

    if (dir) {
        while((entry = readdir(dir))) {
            std::string filename = entry->d_name;
            if ((filename.compare(".") == 0) || (filename.compare("..") == 0))
                continue;

            std::string file_path = path;
            file_path.append(path.compare("/") == 0? "" : "/");
            file_path.append(filename);

            if (entry->d_type & DT_DIR) {
                deleteFolder(file_path);
            }
            else {
                if (remove(file_path.c_str()) != 0) {
                    return ErrorOr<void>(vformat("Failed to delete file \"%s\".\n", file_path.c_str()));
                }
            }
        }

        closedir(dir);
    }
    else {
        return ErrorOr<void>(vformat("Failed to open path \"%s\".\n", path.c_str()));
    }

    if(rmdir(path.c_str()) != 0)
        return ErrorOr<void>(vformat("Failed to delete folder \"%s\".\n", path.c_str()));
    return ErrorOr<void>();
}

FS::Directory::Directory(std::string name, std::weak_ptr<FS::Directory> parent) : name(name), parent(parent) {}

FS::Directory::~Directory() {}

std::weak_ptr<FS::Directory> FS::Directory::GetParent()
{
    return this->parent;
}

std::string FS::Directory::GetName()
{
    return this->name;
}

std::vector<std::shared_ptr<FS::File>> FS::Directory::GetFiles()
{
    return this->files;
}

std::vector<std::shared_ptr<FS::Directory>> FS::Directory::GetDirectories()
{
    return this->directories;
}

std::vector<std::shared_ptr<FS::Directory>> FS::Directory::GetAllParentDirectories()
{
    std::vector<std::shared_ptr<FS::Directory>> parents;
    std::shared_ptr<FS::Directory> current = this->parent.lock();
    while (current)
    {
        parents.push_back(current);
        current = current->GetParent().lock();
    }
    return parents;
}

ErrorOr<std::shared_ptr<FS::Directory>> FS::Directory::GetSharedParentDir(std::shared_ptr<Directory> dir)
{
    auto parents = this->GetAllParentDirectories();
    auto dir_parents = dir->GetAllParentDirectories();
    for (auto& parent : parents)
    {
        for (auto& dir_parent : dir_parents)
        {
            if (parent == dir_parent)
                return ErrorOr<std::shared_ptr<FS::Directory>>(parent);
        }
    }
    return ErrorOr<std::shared_ptr<FS::Directory>>("No shared parent directory found");
}

ErrorOr<std::shared_ptr<FS::Directory>> FS::Directory::GetDirectory(std::string name, bool recursive)
{
    for (auto& dir : directories)
    {
        if (dir->GetName() == name)
            return ErrorOr<std::shared_ptr<FS::Directory>>(dir);
    }
    if (recursive)
    {
        for (auto& dir : directories)
        {
            auto result = dir->GetDirectory(name, true);
            if (!result.IsError())
                return result;
        }
    }
    return ErrorOr<std::shared_ptr<FS::Directory>>(vformat("Directory \"%s\" not found", name.c_str()));
}

ErrorOr<std::shared_ptr<FS::File>> FS::Directory::GetFile(std::string name, bool recursive)
{
    for (auto& file : files)
    {
        if (file->GetName() == name)
            return ErrorOr<std::shared_ptr<FS::File>>(file);
    }
    if (recursive)
    {
        for (auto& dir : directories)
        {
            auto result = dir->GetFile(name, true);
            if (!result.IsError())
                return result;
        }
    }
    return ErrorOr<std::shared_ptr<FS::File>>(vformat("File \"%s\" not found", name.c_str()));
}

ErrorOr<std::shared_ptr<FS::Directory>> FS::Directory::CreateDirectory(std::string name)
{
    if (this->GetDirectory(name, false).IsError())
    {
        auto dir = std::make_shared<FS::Directory>(name, this->parent.lock());
        this->directories.push_back(dir);
        return ErrorOr<std::shared_ptr<FS::Directory>>(dir);
    }
    return ErrorOr<std::shared_ptr<FS::Directory>>(vformat("Directory \"%s\" already exists", name.c_str()));
}

ErrorOr<std::shared_ptr<FS::File>> FS::Directory::CreateFile(std::string name)
{
    if (this->GetFile(name, false).IsError())
    {
        auto file = std::make_shared<FS::File>(name, this->parent.lock());
        this->files.push_back(file);
        return ErrorOr<std::shared_ptr<FS::File>>(file);
    }
    return ErrorOr<std::shared_ptr<FS::File>>(vformat("File \"%s\" already exists", name.c_str()));
}

ErrorOr<void> FS::Directory::Delete()
{
    if (auto parent = this->parent.lock())
    {
        bool found = false;
        for (auto it = parent->directories.begin(); it != parent->directories.end(); it++)
        {
            if (&(*it->get()) == this)
            {
                parent->directories.erase(it);
                found = true;
                break;
            }
        }
        if (!found)
            return ErrorOr<void>("Directory not found in parent directory");
    }
    return ErrorOr<void>();
}

FS::File::File(std::string name, std::weak_ptr<FS::Directory> parent) : name(name), parent(parent) {}

FS::File::~File() {}

std::weak_ptr<FS::Directory> FS::File::GetParent()
{
    return this->parent;
}

std::string FS::File::GetName()
{
    return this->name;
}

ErrorOr<void> FS::File::Delete()
{
    if (auto parent = this->parent.lock())
    {
        bool found = false;
        for (auto it = parent->files.begin(); it != parent->files.end(); it++)
        {
            if (&(*it->get()) == this)
            {
                parent->files.erase(it);
                found = true;
                break;
            }
        }
        if (!found)
            return ErrorOr<void>("File not found in parent directory");
    }
    return ErrorOr<void>();
}

ErrorOr<std::shared_ptr<FS::Directory>> LoadDirFromJSON(std::string path)
{
    nlohmann::json json;
    json.open(path);
    if (json.is_null())
        return ErrorOr<std::shared_ptr<FS::Directory>>(vformat("Failed to open JSON file \"%s\"", path.c_str()));
    
}