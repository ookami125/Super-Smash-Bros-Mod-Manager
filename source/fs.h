#include <string>
#include <vector>
#include <memory>
#include "errorOr.h"

std::string getRelativePath(std::string root, std::string path);
std::vector<std::string> GetAllFilesRecursively(std::string path);
ErrorOr<void> _mkdir(std::string path, bool recursive);
ErrorOr<void> moveFile(std::string from, std::string to);
ErrorOr<void> copyFile(std::string src_path, std::string dst_path);
bool fileExists(std::string path);
ErrorOr<void> deleteFile(std::string path);
ErrorOr<void> appendToFile(std::string path, std::string content);
ErrorOr<void> deleteFolder(const std::string& path);

namespace FS
{
    class File;

    class Directory
    {
    private:
        std::string name;
        std::weak_ptr<Directory> parent;
        std::vector<std::shared_ptr<File>> files;
        std::vector<std::shared_ptr<Directory>> directories;
    public:
        Directory(std::string name, std::weak_ptr<Directory> parent = {});
        ~Directory();
        std::weak_ptr<Directory> GetParent();
        std::string GetName();
        std::vector<std::shared_ptr<File>> GetFiles();
        std::vector<std::shared_ptr<Directory>> GetDirectories();
        std::vector<std::shared_ptr<Directory>> GetAllParentDirectories();
        ErrorOr<std::shared_ptr<Directory>> GetSharedParentDir(std::shared_ptr<Directory> dir);
        ErrorOr<std::shared_ptr<Directory>> GetDirectory(std::string name, bool recursive = false);
        ErrorOr<std::shared_ptr<File>> GetFile(std::string name, bool recursive = false);
        ErrorOr<std::shared_ptr<Directory>> CreateDirectory(std::string name);
        ErrorOr<std::shared_ptr<File>> CreateFile(std::string name);
        ErrorOr<void> Delete();
    };

    class File
    {
    private:
        std::string name;
        std::weak_ptr<Directory> parent;
    public:
        File(std::string name, std::weak_ptr<Directory> parent = {});
        ~File();
        std::weak_ptr<Directory> GetParent();
        std::string GetName();
        ErrorOr<void> Delete();
    };
}

ErrorOr<std::shared_ptr<FS::Directory>> LoadDirFromJSON(std::string path);
