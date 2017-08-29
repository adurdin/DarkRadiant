#pragma once

#include "string/string.h"
#include "os/path.h"

#include <map>
#include <iostream>

namespace archive
{

inline unsigned int path_get_depth(const char* path)
{
  unsigned int depth = 0;
  while(path != 0 && path[0] != '\0')
  {
    path = strchr(path, '/');
    if(path != 0)
    {
      ++path;
    }
    ++depth;
  }
  return depth;
}

/// \brief A generic unix-style file-system which maps paths to files and directories.
/// Provides average O(log n) find and insert methods.
/// \param file_type The data type which represents a file.
template<typename file_type>
class GenericFileSystem
{
  class Path
  {
    std::string m_path;
    unsigned int m_depth;
  public:
	Path(const std::string& path) :
		m_path(path),
		m_depth(path_get_depth(m_path.c_str()))
	{}

	Path(const char* start, std::size_t length) :
		m_path(start, length),
		m_depth(path_get_depth(m_path.c_str()))
	{}

    bool operator<(const Path& other) const
    {
      return string_less_nocase(c_str(), other.c_str());
    }
    unsigned int depth() const
    {
      return m_depth;
    }
    const char* c_str() const
    {
      return m_path.c_str();
    }

	const std::string& string() const
	{
		return m_path;
	}
  };

	class Entry
	{
		std::shared_ptr<file_type> _file;
	public:
		Entry()
		{}

		Entry(const std::shared_ptr<file_type>& file) : 
			_file(file)
		{}

		std::shared_ptr<file_type>& file()
		{
			return _file;
		}

		bool is_directory() const
		{
			return !_file;
		}
	};

  typedef std::map<Path, Entry> Entries;
  Entries m_entries;

public:
  typedef typename Entries::iterator iterator;
  typedef typename Entries::value_type value_type;
  typedef Entry entry_type;

  iterator begin()
  {
    return m_entries.begin();
  }
  iterator end()
  {
    return m_entries.end();
  }

	void clear()
	{
		m_entries.clear();
	}

	/// \brief Returns the file at \p path.
	/// Creates all directories below \p path if they do not exist.
	/// O(log n) on average.
	entry_type& operator[](const Path& path)
	{
		const char* start = path.c_str();
		const char* end = path_remove_directory(path.c_str());

		while (end[0] != '\0')
		{
			// greebo: Take the substring from start to end
			Path dir(start, end - start);

			// And insert it as directory (NULL)
			m_entries.insert(value_type(dir, Entry(NULL)));

			end = path_remove_directory(end);
		}

		return m_entries[path];
	}

  /// \brief Returns the file at \p path or end() if not found.
  iterator find(const Path& path)
  {
    return m_entries.find(path);
  }

  iterator begin(const std::string& root)
  {
    if(root[0] == '\0')
    {
      return m_entries.begin();
    }
    iterator i = m_entries.find(root);
    if(i == m_entries.end())
    {
      return i;
    }
    return ++i;
  }

  /// \brief Performs a depth-first traversal of the file-system subtree rooted at \p root.
  /// Traverses the entire tree if \p root is "".
  /// Calls \p visitor.file() with the path to each file relative to the filesystem root.
  /// Calls \p visitor.directory() with the path to each directory relative to the filesystem root.
  template<typename visitor_type>
  void traverse(visitor_type visitor, const std::string& root)
  {
    unsigned int start_depth = path_get_depth(root.c_str());
    unsigned int skip_depth = 0;
    for(iterator i = begin(root); i != end() && i->first.depth() > start_depth; ++i)
    {
      if(i->first.depth() == skip_depth)
      {
        skip_depth = 0;
      }
      if(skip_depth == 0)
      {
        if(!i->second.is_directory())
        {
          visitor.file(i->first.string());
        }
        else if(visitor.directory(i->first.string(), i->first.depth() - start_depth))
        {
          skip_depth = i->first.depth();
        }
      }
    }
  }
};

}