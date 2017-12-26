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

#include "file_manager.hpp"

#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <sstream>

#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>

#ifdef ANDROID
#include <android/asset_manager.h>
#include <android_native_app_glue.h>

extern struct android_app* g_android_app;
#endif

const std::string data_dir = "data/";

FileManager* FileManager::m_file_manager = NULL;

FileManager::FileManager()
{
    m_file_manager = this;
}

FileManager::~FileManager()
{
    
}

bool FileManager::init()
{
    bool success = createAssetsList();
    
    return success;
}

bool FileManager::createAssetsList()
{
#ifdef ANDROID
    File* file = loadFileFromAssets("files.txt");
    
    if (file == NULL)
        return false;
    
    std::istringstream stream(file->data);
    
    if (!stream.good())
        return false;
    
    while (!stream.eof())
    {
        std::string file_path;
        std::getline(stream, file_path);
        
        if (!file_path.empty())
        {
            std::string asset_path = file_path.substr(data_dir.size());
            m_assets_list.push_back(asset_path);
        }
    }
    
    closeFile(file);
#else

    getFileList(data_dir, m_assets_list);
#endif
    
    return true;
}

File* FileManager::loadFile(std::string filename)
{
    std::string file_path = data_dir + filename;
    
    File* file = loadFileFromAssets(file_path);
    
    if (file != NULL)
        return file;
    
    std::ifstream is;
    is.open(file_path.c_str(), std::ios::binary);
    
    if (!is.good())
    {
        printf("Error: Could not open file %s\n", file_path.c_str());
        is.close();
        return NULL;
    }
    
    is.seekg(0, std::ios::end);
    int length = is.tellg();
    is.seekg(0, std::ios::beg);
    
    char* data = new (std::nothrow) char[length];
    
    if (data == NULL)
    {
        printf("Error: Could not open file %s\n", file_path.c_str());
        is.close();
        return NULL;
    }
    
    is.read(data, length);
    is.close();
    
    file = new File();
    file->data = data;
    file->length = length;
    
    return file;
}

File* FileManager::loadFileFromAssets(std::string file_path)
{
#ifdef ANDROID
    if (g_android_app == NULL)
        return NULL;
    
    AAssetManager* asset_manager = g_android_app->activity->assetManager;
    
    if (asset_manager == NULL)
        return NULL;
    
    AAsset* asset = AAssetManager_open(asset_manager, file_path.c_str(),
                                       AASSET_MODE_BUFFER);

    if (asset == NULL)
    {
        printf("Error: Could not open asset %s", file_path.c_str());
        return NULL;
    }
    
    int length = AAsset_getLength(asset);
    const void* buffer = AAsset_getBuffer(asset);
    
    char* data = new (std::nothrow) char[length];
    
    if (data == NULL)
    {
        printf("Error: Could not open asset %s", file_path.c_str());
        AAsset_close(asset);
        return NULL;
    }

    memcpy(data, buffer, length);

    AAsset_close(asset);
    
    File* file = new File();
    file->data = data;
    file->length = length;
    
    return file;
    
#else
    return NULL;
#endif
}

void FileManager::closeFile(File* file)
{
    if (file == NULL)
        return;

    delete[] file->data;
    delete file;
}


void FileManager::getFileList(std::string dir_name, 
                              std::vector<std::string>& file_list)
{
    DIR* dir = opendir(dir_name.c_str());
    
    if (dir == NULL)
        return;
        
    struct dirent* dir_entry = NULL;
    
    do
    {
        dir_entry = readdir(dir);
        
        if (dir_entry == NULL || dir_entry->d_name == NULL)
            continue;
        
        std::string filename = dir_entry->d_name;
        std::string file_path = dir_name + filename;
        
        if (filename == "." || filename == "..")
            continue;
            
        bool is_directory = directoryExists(file_path);
        
        if (is_directory)
        {
            getFileList(file_path + "/", file_list);
        }
        else
        {
            std::string asset_path = file_path.substr(data_dir.size());
            file_list.push_back(asset_path);
        }
    } 
    while (dir_entry != NULL);
    
    closedir(dir);
}

bool FileManager::extractFromAssets(std::string filename, std::string base_dir,
                                    std::string dest_dir)
{
    std::size_t pos = filename.find(base_dir);
    std::string out_filename = (pos == 0) ? filename.substr(base_dir.length()) 
                                          : filename;
    std::string file_path = dest_dir + "/" + out_filename;
    std::string dir_path = getDirectoryPath(file_path);
    
    bool success = createDirectoryRecursive(dir_path);
    
    if (!success)
    {
        printf("Error: Couldn't create directory: %s\n", dir_path.c_str());
        return false;
    }
    
    File* file = loadFile(filename);
    
    if (file == NULL)
        return false;

    std::fstream out_file(file_path, std::ios::out | std::ios::binary);
    
    if (!out_file.good())
    {
        printf("Error: Couldn't open file: %s\n", file_path.c_str());
        closeFile(file);
        return false;
    }
    
    out_file.write(file->data, file->length);

    if (out_file.fail())
    {
        printf("Error: Couldn't write to file: %s\n", file_path.c_str());
        success = false;
    }

    out_file.close();
    closeFile(file);
    
    return success;
}

bool FileManager::fileExists(std::string path)
{
    struct stat stat_info;
    int err = stat(path.c_str(), &stat_info);
    
    if (err != 0)
        return false;
        
    bool is_file = S_ISREG(stat_info.st_mode);
    
    return is_file;
}

bool FileManager::directoryExists(std::string path)
{
    struct stat stat_info;
    int err = stat(path.c_str(), &stat_info);
    
    if (err != 0)
        return false;
        
    bool is_directory = S_ISDIR(stat_info.st_mode);
    
    return is_directory;
}

std::string FileManager::getExtension(std::string filename)
{
    std::string extension;
    
    std::size_t pos = filename.rfind(".");
    
    if (pos != std::string::npos)
    {
        extension = filename.substr(pos);
    }
    
    return extension;
}

std::string FileManager::getDirectoryPath(std::string file_path)
{
    std::string dir_path;
    
    std::size_t pos = file_path.rfind("/");
    
    if (pos != std::string::npos)
    {
        dir_path = file_path.substr(0, pos);
    }
    
    return dir_path;
}

bool FileManager::createDirectory(std::string path)
{
    int error = mkdir(path.c_str(), 0755);

    bool success = (error == 0 || errno == EEXIST);
    return success;
}

bool FileManager::createDirectoryRecursive(std::string path)
{
    bool success = true;
    std::size_t pos = 0;
    
    while (pos != std::string::npos && success == true)
    {
        pos = path.find("/", pos + 1);
        std::string dir = path.substr(0, pos);
        
        success = createDirectory(dir);
    }
        
    return success;
}

bool FileManager::touchFile(std::string path)
{
    if (fileExists(path))
        return true;

    std::fstream file(path, std::ios::out | std::ios::binary);

    bool success = file.good();

    file.close();
    
    return success;
}

std::string FileManager::findExternalDataDir(std::string dir_name, 
                                             std::string alternative_dir_name,
                                             std::string project_name, 
                                             std::string environment_variable)
{
    std::vector<std::string> paths;
    
#ifdef ANDROID
    if (getenv(environment_variable.c_str()))
        paths.push_back(getenv(environment_variable.c_str()));

    if (getenv("EXTERNAL_STORAGE"))
        paths.push_back(getenv("EXTERNAL_STORAGE"));

    if (getenv("SECONDARY_STORAGE"))
        paths.push_back(getenv("SECONDARY_STORAGE"));

    if (g_android_app->activity->externalDataPath)
        paths.push_back(g_android_app->activity->externalDataPath);

    if (g_android_app->activity->internalDataPath)
        paths.push_back(g_android_app->activity->internalDataPath);

    paths.push_back("/sdcard/");
    paths.push_back("/storage/sdcard0/");
    paths.push_back("/storage/sdcard1/");
    paths.push_back("/data/data/" + project_name + "/files/");
    
#else
    paths.push_back("./external_data");
#endif

    std::string external_data_dir;

    for (std::string path : paths)
    {
        if (fileExists(path + "/" + dir_name + "/.extracted"))
        {
            external_data_dir = path + "/" + dir_name;
            break;
        }

        if (fileExists(path + "/" + alternative_dir_name + "/.extracted"))
        {
            external_data_dir = path + "/" + alternative_dir_name;
            break;
        }
    }
    
    return external_data_dir;
}
