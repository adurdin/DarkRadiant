#pragma once

#include <map>
#include "math/Vector3.h"
#include "string/convert.h"
#include "xmlutil/Node.h"

namespace ui
{

/* The ColourItem represents a single colour. This ia a simple derivative of
 * Vector3 which provides an additional constructor to extract the colour information
 * from an XML node.
 */

class ColourItem : 
	public Vector3
{
public:

    /** Default constructor. Creates a black colour.
     */
    ColourItem() : 
		Vector3(0, 0, 0)
    {}

    /** Construct a ColourItem from an XML Node.
     */
    ColourItem(const xml::Node& colourNode) : 
		Vector3(string::convert<Vector3>(colourNode.getAttributeValue("value")))
    {}
};

typedef std::map<std::string, ColourItem> ColourItemMap;

/*  A colourscheme is basically a collection of ColourItems
 */
class ColourScheme
{
private:
    // The name of this scheme
    std::string _name;

    // The ColourItems Map
    ColourItemMap _colours;

    // True if the scheme must not be edited
    bool _readOnly;

    /* Empty Colour, this serves as return value for
        non-existing, but requested colours */
    ColourItem _emptyColour;

public:
    // Constructors
	ColourScheme();

    // Constructs a ColourScheme from a given xml::node
    ColourScheme(const xml::Node& schemeNode);

    // Returns the list of ColourItems
	ColourItemMap& getColourMap();

    // Returns the requested colour object
    ColourItem& getColour(const std::string& colourName);

    // returns the name of this colour scheme
	const std::string& getName() const;

    // returns true if the scheme is read-only
	bool isReadOnly() const;

    // set the read-only status of this scheme
	void setReadOnly(bool isReadOnly);

	// Tries to add any missing items from the given scheme into this one
	void mergeMissingItemsFromScheme(const ColourScheme& other);
};

} // namespace ui
