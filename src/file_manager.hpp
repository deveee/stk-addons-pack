//    STK Add-ons pack - Simple add-ons installer for Android
//    Copyright (C) 2017 Dawid Gan <deveee@gmail.com>
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef FILE_MANAGER_HPP
#define FILE_MANAGER_HPP

#include <string>
#include <vector>

struct File
{
    int length;
    char* data;
};

class FileManager
{
private:
    static FileManager* m_file_manager;
    std::vector<std::string> m_assets_list;
    
    bool createAssetsList();
    File* loadFileFromAssets(std::string file_path);
    void getFileList(std::string dir_name, std::vector<std::string>& file_list);
    
public:
    FileManager();
    ~FileManager();

    bool init();
    File* loadFile(std::string filename);
    void closeFile(File* file);
    bool extractFromAssets(std::string filename, std::string base_dir, 
                           std::string dest_dir);
    std::vector<std::string>& getAssetsList() {return m_assets_list;}
    bool fileExists(std::string path);
    bool directoryExists(std::string path);
    std::string getExtension(std::string filename);
    std::string getDirectoryPath(std::string file_path);
    bool createDirectory(std::string path);
    bool createDirectoryRecursive(std::string path);
    bool touchFile(std::string path);
    
    std::string findExternalDataDir(std::string dir_name, 
                                    std::string alternative_dir_name,
                                    std::string project_name, 
                                    std::string environment_variable);
    
    static FileManager* getFileManager() {return m_file_manager;}
};

#endif
