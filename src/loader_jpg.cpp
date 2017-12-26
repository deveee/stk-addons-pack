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

#include "loader_jpg.hpp"
#include "file_manager.hpp"

#include <cstdio>
#include <turbojpeg.h>

Image* LoaderJPG::loadImage(std::string filename)
{
    tjhandle handle = tjInitDecompress();

    if (handle == NULL)
        return NULL;

    FileManager* file_manager = FileManager::getFileManager();
    File* file = file_manager->loadFile(filename);
    
    if (file == NULL)
    {
        tjDestroy(handle);
        return NULL;
    }
    
    int width = 0;
    int height = 0;
    int subsamp = -1;

    int err = tjDecompressHeader2(handle, (unsigned char*)file->data, 
                                  file->length, &width, &height, &subsamp);
    
    if (err < 0)
    {
        printf("Error: Decompress error for file: %s\n", filename.c_str());
        file_manager->closeFile(file);
        tjDestroy(handle);
        return NULL;
    }
    
    int dest_size = width * height * tjPixelSize[TJPF_RGB];
    
    Image* image = new Image();
    image->width = width;
    image->height = height;
    image->channels = 3;
    image->data_length = dest_size;
    image->data = new (std::nothrow) unsigned char[dest_size]();

    if (image->data == NULL)
    {
        printf("Error: Decompress error for file: %s\n", filename.c_str());
        delete image;
        file_manager->closeFile(file);
        tjDestroy(handle);
        return NULL;
    }
        
    err = tjDecompress2(handle, (unsigned char*)file->data, file->length, 
                        image->data, width, 0, height, TJPF_RGB, 0);

    if (err < 0)
    {
        printf("Error: Decompress error for file: %s\n", filename.c_str());
        delete image;
        file_manager->closeFile(file);
        tjDestroy(handle);
        return NULL;
    }

    tjDestroy(handle);
    
    file_manager->closeFile(file);
    
    return image;
}

void LoaderJPG::closeImage(Image* image)
{
    if (image == NULL)
        return;
    
    delete[] image->data;
    delete image;
}
