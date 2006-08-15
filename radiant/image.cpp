/*
Copyright (C) 2001-2006, William Joseph.
All Rights Reserved.

This file is part of GtkRadiant.

GtkRadiant is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

GtkRadiant is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GtkRadiant; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "image.h"

#include "modulesystem.h"
#include "iimage.h"
#include "ifilesystem.h"
#include "iarchive.h"

#include "generic/reference.h"
#include "os/path.h"
#include "stream/stringstream.h"

#include <iostream>

typedef Modules<_QERPlugImageTable> ImageModules;
ImageModules& Textures_getImageModules();

/// \brief Returns a new image for the first file matching \p name in one of the available texture formats, or 0 if no file is found.
Image* QERApp_LoadImage(void* environment, const char* name)
{
  Image* image = 0;
  class LoadImageVisitor : public ImageModules::Visitor
  {
    const char* m_name;
    Image*& m_image;
  public:
    LoadImageVisitor(const char* name, Image*& image)
      : m_name(name), m_image(image)
    {
    }
    
    // Visit function called for each image module. Provides the file extension and
    // a table with the loadImage function.
    
    void visit(const char* extension, const _QERPlugImageTable& table)
    {
      if(m_image == 0)
      {
		// Construct the full name of the image to load, including the prefix (e.g. "dds/")
		// and the file extension.
		std::string fullName = table.prefix + std::string(m_name) + "." + extension;
        ArchiveFile* file = GlobalFileSystem().openFile(fullName.c_str());
        if(file != 0)
        {
          m_image = table.loadImage(*file);
          file->release();
        }
      }
    }
  } visitor(name, image);

  Textures_getImageModules().foreachModule(visitor);

  return image;
}

