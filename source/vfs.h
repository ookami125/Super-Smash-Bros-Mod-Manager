#include "errorOr.h"
#include <string>
#include <vector>

namespace VFS
{
    class Directory;
    class File;

    class Directory
    {
    private:
        FS::Directory* fsDir;
        Directory* parent;
        std::string name;
        std::vector<File*> files;
        std::vector<Directory*> directories;
    public:
        Directory(std::string name);
        ~Directory();
        Directory* GetParent();
        std::string GetName();
        std::vector<File*> GetFiles();
        std::vector<Directory*> GetDirectories();
        ErrorOr<Directory*> GetDirectory(std::string name, bool recursive = false);
        ErrorOr<File*> GetFile(std::string name, bool recursive = false);
        ErrorOr<Directory*> CreateDirectory(std::string name);
        ErrorOr<File*> CreateFile(std::string name);
        ErrorOr<void> Delete();
    };
}