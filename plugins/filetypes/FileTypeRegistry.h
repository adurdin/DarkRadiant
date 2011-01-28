#ifndef FILETYPEREGISTRY_H_
#define FILETYPEREGISTRY_H_

#include "ifiletypes.h"
#include <map>

/**
 * Implementation of the file type registry. The registry is associating file types
 * with ModuleTypeLists, the latter of which are modules => named file patterns.
 *
 * <type> => [ <modulename> => <patternDescription>, ... ]
 *
 * "map" => [	"Doom3MapLoader" => "*.map",
 *				"Doom3MapLoader" => "*.reg",
 *				"Doom3PrefabLoader" => "*.pfb" ]
 *
 * "model" => [ "ModelLoaderASE" => "*.ase",
 *				"ModelLoaderLWO" => "*.lwo",
 *				... ]
 *
 * This mapping can be used to retrieve a list of modules capable of
 * loading a file with a given extension. Furthermore it is used by the
 * gtkutil::FileChooser to populate the file type dropdown list.
 */
class RadiantFileTypeRegistry :
	public IFileTypeRegistry
{
	// Map of named ModuleTypeListPtrs. Each ModuleTypeList is a list of structs
	// associating a module name with a filetype pattern
	typedef std::map<std::string, ModuleTypeListPtr> TypeListMap;
	TypeListMap _typeLists;

public:

	/*
	 * Constructor, adds the All Files type.
	 */
	RadiantFileTypeRegistry();

	/*
	 * Add a type.
	 */
	void addType(const std::string& moduleType,
				 const std::string& moduleName,
				 const FileTypePattern& type);

	/*
	 * Return list of types for an associated module type.
	 */
	ModuleTypeListPtr getTypesFor(const std::string& moduleType);

	// Look for a module which loads the given extension, by searching under the
	// given type category
	virtual std::string findModuleName(const std::string& moduleType, const std::string& extension);

	// RegisterableModule implementation
	virtual const std::string& getName() const;
	virtual const StringSet& getDependencies() const;
	virtual void initialiseModule(const ApplicationContext& ctx);

}; // class FileTypeRegistry

typedef boost::shared_ptr<RadiantFileTypeRegistry> RadiantFileTypeRegistryPtr;

#endif /*FILETYPEREGISTRY_H_*/
