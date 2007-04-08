#ifndef SOUNDCHOOSER_H_
#define SOUNDCHOOSER_H_

#include <gtk/gtkwidget.h>
#include <gtk/gtktreestore.h>
#include <gtk/gtktreeselection.h>

#include <string>

namespace ui
{

/**
 * Dialog for listing and selection of sound shaders.
 */
class SoundChooser
{
	// Main dialog widget
	GtkWidget* _widget;

	// Tree store for shaders, and the tree selection
	GtkTreeStore* _treeStore;
	GtkTreeSelection* _treeSelection;
	
	// Last selected shader
	std::string _selectedShader;
	
private:

	// Widget construction
	GtkWidget* createTreeView();
	GtkWidget* createButtons();

	/* GTK CALLBACKS */
	static void _onDelete(GtkWidget*, SoundChooser*);
	static void _onOK(GtkWidget*, SoundChooser*);
	static void _onCancel(GtkWidget*, SoundChooser*);
	
public:
	
	/**
	 * Constructor creates widgets.
	 * 
	 * @parent
	 * The parent window.
	 */
	SoundChooser();

	/**
	 * Display the dialog and return the selection.
	 */
	std::string chooseSound();	
	
};

}

#endif /*SOUNDCHOOSER_H_*/
