#include "AIFindItemComponentEditor.h"
#include "../SpecifierType.h"
#include "../Component.h"

#include "gtkutil/LeftAlignment.h"
#include "gtkutil/LeftAlignedLabel.h"

#include <gtk/gtk.h>

namespace objectives {

namespace ce {

// Registration helper, will register this editor in the factory
AIFindItemComponentEditor::RegHelper AIFindItemComponentEditor::regHelper;

// Constructor
AIFindItemComponentEditor::AIFindItemComponentEditor(Component& component) :
	_component(&component)
{
	// Main vbox
	_widget = gtk_vbox_new(FALSE, 6);

    gtk_box_pack_start(
        GTK_BOX(_widget), 
        gtkutil::LeftAlignedLabel("<b>Item:</b>"),
        FALSE, FALSE, 0
    );
	
	// TODO
}

// Destructor
AIFindItemComponentEditor::~AIFindItemComponentEditor() {
	if (GTK_IS_WIDGET(_widget)) {
		gtk_widget_destroy(_widget);
	}
}

// Get the main widget
GtkWidget* AIFindItemComponentEditor::getWidget() const {
	return _widget;
}

// Write to component
void AIFindItemComponentEditor::writeToComponent() const {
    assert(_component);
	// TODO
}

} // namespace ce

} // namespace objectives
