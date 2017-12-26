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

#include "loader_png.hpp"
#include "file_manager.hpp"

#include <cstdio>
#include <cstring>
#include <vector>

int LoaderPNG::m_read_pos = 0;

void LoaderPNG::readFromMemory(png_structp png_ptr, png_bytep data, 
                               png_size_t length)
{
    char* png_data = (char*)png_get_io_ptr(png_ptr);
    memcpy(data, &png_data[m_read_pos], length);
    m_read_pos += length;
}

Image* LoaderPNG::loadImage(std::string filename)
{
    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, 
                                                 NULL, NULL);

    if (!png_ptr)
    {
        printf("Error: png_create_read_struct failed\n");
        return NULL;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    
    if (!info_ptr)
    {
        printf("Error: png_create_info_struct failed\n");
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        return NULL;
    }
    
    FileManager* file_manager = FileManager::getFileManager();
    File* file = file_manager->loadFile(filename);
    
    if (file == NULL)
    {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        return NULL;
    }
    
    m_read_pos = 0;
    
    png_set_read_fn(png_ptr, file->data, readFromMemory);
    png_read_info(png_ptr, info_ptr);

    png_byte color_type = png_get_color_type(png_ptr, info_ptr);
    png_byte bit_depth = png_get_bit_depth(png_ptr, info_ptr);
    
    if (color_type != PNG_COLOR_TYPE_RGB && color_type != PNG_COLOR_TYPE_RGBA)
    {
        printf("Error: Unsupported png format. It must be RGB or RGBA\n");
        file_manager->closeFile(file);
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        return NULL;
    }
    
    // Force 8 bit per channel
    if (bit_depth < 8)
    {
        png_set_packing(png_ptr);
    }
    else if (bit_depth == 16)
    {
        png_set_strip_16(png_ptr);
    }
    
    png_read_update_info(png_ptr, info_ptr);
    
    int width = png_get_image_width(png_ptr, info_ptr);
    int height = png_get_image_height(png_ptr, info_ptr);
    int pitch = png_get_rowbytes(png_ptr, info_ptr);
    int channels = png_get_channels(png_ptr, info_ptr);

    int dest_size = height * pitch;
    
    Image* image = new Image();
    image->width = width;
    image->height = height;
    image->channels = channels;
    image->data_length = dest_size;
    image->data = new (std::nothrow) unsigned char[dest_size]();

    if (image->data == NULL)
    {
        printf("Error: Decompress error for file: %s\n", filename.c_str());
        delete image;
        file_manager->closeFile(file);
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        return NULL;
    }

    std::vector<unsigned char*> rows;
    
    for (int i = 0; i < height; i++)
    {
        rows.push_back(new unsigned char[pitch]);
    }

    png_read_image(png_ptr, &rows[0]);
    
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

    for (unsigned int i = 0; i < rows.size(); i++)
    {
        memcpy(&image->data[i * pitch], rows[i], pitch);
        delete[] rows[i];
    }
    
    file_manager->closeFile(file);
    
    return image;
}

void LoaderPNG::closeImage(Image* image)
{
    if (image == NULL)
        return;
    
    delete[] image->data;
    delete image;
}
