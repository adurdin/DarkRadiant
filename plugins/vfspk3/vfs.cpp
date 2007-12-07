/*
Copyright (c) 2001, Loki software, inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list 
of conditions and the following disclaimer.

Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

Neither the name of Loki software nor the names of its contributors may be used 
to endorse or promote products derived from this software without specific prior 
written permission. 

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY 
DIRECT,INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON 
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
*/

//
// Rules:
//
// - Directories should be searched in the following order: ~/.q3a/baseq3,
//   install dir (/usr/local/games/quake3/baseq3) and cd_path (/mnt/cdrom/baseq3).
//
// - Pak files are searched first inside the directories.
// - Case insensitive.
// - Unix-style slashes (/) (windows is backwards .. everyone knows that)
//
// Leonardo Zide (leo@lokigames.com)
//

#include "vfs.h"
#include "FileVisitor.h"

#include <stdio.h>
#include <stdlib.h>
#include <glib/gdir.h>
#include <glib/gstrfuncs.h>

#include "iradiant.h"
#include "idatastream.h"
#include "iarchive.h"
#include "ifilesystem.h"

#include "generic/callback.h"
#include "string/string.h"
#include "stream/stringstream.h"
#include "os/path.h"
#include "moduleobservers.h"

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/predicate.hpp>

#if defined(WIN32) && !defined(PATH_MAX)
#define PATH_MAX 260
#endif

// =============================================================================
// Global variables

Archive* OpenArchive(const char* name);

struct archive_entry_t
{
  std::string name;
  Archive* archive;
  bool is_pakfile;
};

#include <list>

typedef std::list<archive_entry_t> archives_t;

static archives_t g_archives;

// =============================================================================
// Static functions

static void FixDOSName (char *src)
{
  if (src == 0 || strchr(src, '\\') == 0)
    return;

  globalErrorStream() << "WARNING: invalid path separator '\\': " << src << "\n";

  while (*src)
  {
    if (*src == '\\')
      *src = '/';
    src++;
  }
}

static void InitPakFile (_QERArchiveTable& archiveModule, const char *filename)
{
	std::string fileExt(path_get_extension(filename));
	boost::to_upper(fileExt);
	
	// matching extension?
	if (fileExt == archiveModule.getExtension()) {
		archive_entry_t entry;
		entry.name = filename;
		entry.archive = archiveModule.m_pfnOpenArchive(filename);
		entry.is_pakfile = true;
		g_archives.push_back(entry);
		globalOutputStream() << "  pak file: " << filename << "\n";
	}
}

inline int ascii_to_upper(int c)
{
  if (c >= 'a' && c <= 'z')
	{
		return c - ('a' - 'A');
	}
  return c;
}

/*!
This behaves identically to stricmp(a,b), except that ASCII chars
[\]^`_ come AFTER alphabet chars instead of before. This is because
it converts all alphabet chars to uppercase before comparison,
while stricmp converts them to lowercase.
*/
static int string_compare_nocase_upper(const char* a, const char* b)
{
	for(;;)
  {
		int c1 = ascii_to_upper(*a++);
		int c2 = ascii_to_upper(*b++);

		if (c1 < c2)
		{
			return -1; // a < b
		}
		if (c1 > c2)
		{
			return 1; // a > b
		}
    if(c1 == 0)
    {
      return 0; // a == b
    }
	}	
}

// Arnout: note - sort pakfiles in reverse order. This ensures that
// later pakfiles override earlier ones. This because the vfs module
// returns a filehandle to the first file it can find (while it should
// return the filehandle to the file in the most overriding pakfile, the
// last one in the list that is).

//!\todo Analyse the code in rtcw/q3 to see which order it sorts pak files.
class PakLess
{
public:
  bool operator()(const std::string& self, const std::string& other) const
  {
    return string_compare_nocase_upper(self.c_str(), other.c_str()) > 0;
  }
};

typedef std::set<std::string, PakLess> Archives;

// =============================================================================
// Global functions

// reads all pak files from a dir
void InitDirectory(const char* directory, _QERArchiveTable& archiveModule)
{
  
}

// frees all memory that we allocated
// FIXME TTimo this should be improved so that we can shutdown and restart the VFS without exiting Radiant?
//   (for instance when modifying the project settings)
void Shutdown()
{
  for(archives_t::iterator i = g_archives.begin(); i != g_archives.end(); ++i)
  {
    (*i).archive->release();
  }
  g_archives.clear();
}

#define VFS_SEARCH_PAK 0x1
#define VFS_SEARCH_DIR 0x2

ArchiveTextFile* OpenTextFile(const std::string& filename)
{
  for(archives_t::iterator i = g_archives.begin(); i != g_archives.end(); ++i)
  {
    ArchiveTextFile* file = (*i).archive->openTextFile(filename.c_str());
    if(file != 0)
    {
      return file;
    }
  }

  return 0;
}

void FreeFile (void *p)
{
  free(p);
}

const char* FindFile(const char* relative)
{
  for(archives_t::iterator i = g_archives.begin(); i != g_archives.end(); ++i)
  {
    if(!(*i).is_pakfile && (*i).archive->containsFile(relative))
    {
      return (*i).name.c_str();
    }
  }

  return "";
}

const char* FindPath(const char* absolute)
{
  for(archives_t::iterator i = g_archives.begin(); i != g_archives.end(); ++i)
  {
    if(!(*i).is_pakfile && path_equal_n(absolute, (*i).name.c_str(), string_length((*i).name.c_str())))
    {
      return (*i).name.c_str();
    }
  }

  return "";
}

Quake3FileSystem::Quake3FileSystem() :
	_moduleObservers(getName()),
	_numDirectories(0)
{}

void Quake3FileSystem::initDirectory(const std::string& inputPath) {
    if (_numDirectories == (VFS_MAXDIRS-1)) {
		return;
    }

    // greebo: Normalise path: Replace backslashes and ensure trailing slash
    _directories[_numDirectories] = os::standardPathWithSlash(inputPath);

	const char* path = _directories[_numDirectories].c_str();

	_numDirectories++;
	
	// Shortcut reference to the ArchiveModule
	_QERArchiveTable& archiveModule = GlobalArchive("PK4");  

	{
		archive_entry_t entry;
		entry.name = path;
		entry.archive = OpenArchive(path);
		entry.is_pakfile = false;
		g_archives.push_back(entry);
	}

	GDir* dir= g_dir_open (path, 0, 0);

	if (dir != 0) {
		globalOutputStream() << "vfs directory: " << path << "\n";

		const char* ignore_prefix = "";
		const char* override_prefix = "";

		// greebo: hardcoded these after removing of gamemode_get stuff
		// can probably be removed but I haven't checked if that is safe
		ignore_prefix = "mp_";
		override_prefix = "sp_";

		Archives archives;
		Archives archivesOverride;
		for (;;) {
			const char* name= g_dir_read_name(dir);
			if (name == 0)
				break;

			const char *ext = strrchr(name, '.');
			if ((ext == 0) || *(++ext) == '\0' /*|| GetArchiveTable(archiveModule, ext) == 0*/)
				continue;

			// using the same kludge as in engine to ensure consistency
			if (!string_empty(ignore_prefix) && strncmp(name,
					ignore_prefix, strlen(ignore_prefix)) == 0) {
				continue;
			}
			if (!string_empty(override_prefix) && strncmp(name,
					override_prefix, strlen(override_prefix)) == 0) {
				archivesOverride.insert(name);
				continue;
			}

			archives.insert(name);
		}

		g_dir_close(dir);

		// add the entries to the vfs
		for (Archives::iterator i = archivesOverride.begin(); i
				!= archivesOverride.end(); ++i) {
			char filename[PATH_MAX];
			strcpy(filename, path);
			strcat(filename, (*i).c_str());
			InitPakFile(archiveModule, filename);
		}
		for (Archives::iterator i = archives.begin(); i != archives.end(); ++i) {
			char filename[PATH_MAX];
			strcpy(filename, path);
			strcat(filename, (*i).c_str());
			InitPakFile(archiveModule, filename);
		}
	} 
	else {
		globalErrorStream() << "vfs directory not found: " << path << "\n";
	}
}

void Quake3FileSystem::initialise() {
    globalOutputStream() << "filesystem initialised\n";
    _moduleObservers.realise();
}

void Quake3FileSystem::shutdown() {
	_moduleObservers.unrealise();
	globalOutputStream() << "filesystem shutdown\n";
	Shutdown();
	_numDirectories = 0;
}


int Quake3FileSystem::getFileCount(const std::string& filename) {
	int count = 0;
	std::string fixedFilename(os::standardPathWithSlash(filename));

	for (archives_t::iterator i = g_archives.begin(); i != g_archives.end(); ++i) {
		if (i->archive->containsFile(fixedFilename.c_str())) {
			++count;
		}
	}

	return count;
}

ArchiveFile* Quake3FileSystem::openFile(const std::string& filename) {
	if (filename.find("\\") != std::string::npos) {
		globalErrorStream() << "Filename contains backslash: " << filename.c_str() << "\n";
		return NULL;
	}
	
	for (archives_t::iterator i = g_archives.begin(); i != g_archives.end(); ++i) {
		ArchiveFile* file = i->archive->openFile(filename.c_str());
		if (file != NULL) {
			return file;
		}
	}

	return 0;
}

ArchiveTextFile* Quake3FileSystem::openTextFile(const std::string& filename) {
	return OpenTextFile(filename);
}

std::size_t Quake3FileSystem::loadFile(const std::string& filename, void **buffer) {
	std::string fixedFilename(os::standardPathWithSlash(filename));

	ArchiveFile* file = openFile(fixedFilename);

	if (file != NULL) {
		// Allocate one byte more for the trailing zero
		*buffer = malloc(file->size()+1);
		
		// we need to end the buffer with a 0
		((char*) (*buffer))[file->size()] = 0;

		std::size_t length = file->getInputStream().read(
			reinterpret_cast<InputStream::byte_type*>(*buffer), 
			file->size()
		);
		
		file->release();
		return length;
	}

	*buffer = NULL;
	return 0;
}

  void Quake3FileSystem::freeFile(void *p)
  {
    FreeFile(p);
  }

    // Call the specified callback function for each file matching extension
    // inside basedir.
    void Quake3FileSystem::forEachFile(const char* basedir, 
    				 const char* extension, 
    				 const FileNameCallback& callback, 
    				 std::size_t depth)
    {
    	// Set of visited files, to avoid name conflicts
    	std::set<std::string> visitedFiles;
    	
    	// Visit each Archive, applying the FileVisitor to each one (which in
    	// turn calls the callback for each matching file.
    	for (archives_t::iterator i = g_archives.begin(); 
    		 i != g_archives.end(); 
    		 ++i)
	    {
    		FileVisitor visitor(callback, basedir, extension, visitedFiles);
    		i->archive->forEachFile(
    						Archive::VisitorFunc(
    								visitor, Archive::eFiles, depth), basedir);
	    }
    }

const char* Quake3FileSystem::findFile(const char *name) {
	return FindFile(name);
}

const char* Quake3FileSystem::findRoot(const char *name) {
	return FindPath(name);
}

void Quake3FileSystem::attach(ModuleObserver& observer) {
	_moduleObservers.attach(observer);
}

void Quake3FileSystem::detach(ModuleObserver& observer) {
	_moduleObservers.detach(observer);
}
  
// RegisterableModule implementation
const std::string& Quake3FileSystem::getName() const {
	static std::string _name(MODULE_VIRTUALFILESYSTEM);
	return _name;
}

const StringSet& Quake3FileSystem::getDependencies() const {
	static StringSet _dependencies;

	if (_dependencies.empty()) {
		_dependencies.insert("ArchivePK4");
	}

	return _dependencies;
}

void Quake3FileSystem::initialiseModule(const ApplicationContext& ctx) {
	globalOutputStream() << "VFS::initialiseModule called\n";
}
