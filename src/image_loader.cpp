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
#include "image_loader.hpp"
#include "image_loader_jpg.hpp"
#include "image_loader_png.hpp"

Image* ImageLoader::loadImage(std::string filename)
{
    Image* image = NULL;
    
    FileManager* file_manager = FileManager::getFileManager();
    std::string extension = file_manager->getExtension(filename);
    
    if (extension == ".png")
    {
        image = ImageLoaderPNG::loadImage(filename);
    }
    else if (extension == ".jpg")
    {
        image = ImageLoaderJPG::loadImage(filename);
    }
    
    return image;
}

void ImageLoader::closeImage(Image* image)
{
    if (image == NULL)
        return;
    
    delete[] image->data;
    delete image;
}
