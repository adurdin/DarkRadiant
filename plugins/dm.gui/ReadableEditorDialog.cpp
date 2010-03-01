#include "ReadableEditorDialog.h"

#include "ientity.h"
#include "iregistry.h"
#include "selectionlib.h"
#include "imainframe.h"
#include "gtkutil/MultiMonitor.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/RightAlignment.h"
#include "gtkutil/FramedWidget.h"
#include "gtkutil/dialog.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/LeftAlignment.h"

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>

#include "string/string.h"

#include "XdFileChooserDialog.h"
#include "imap.h"
#include "idialogmanager.h"

namespace ui
{

// consts:
	namespace 
	{
		const std::string WINDOW_TITLE("Readable Editor");

		const std::string RKEY_READABLE_BASECLASS("game/readables/readableBaseClass");

		const char* const NO_ENTITY_ERROR = "Cannot run Readable Editor on this selection.\n"
			"Please select a single XData entity."; 

		const std::string LABEL_PAGE_RELATED("Page related statements:");

		const std::string LABEL_PAGE_OPERATIONS("Editing operations:");

		const std::string LABEL_GENERAL_PROPERTIES("General properties:");

		const int MIN_ENTRY_WIDTH = 35;

		// Widget handles for use in the _widgets std::map
		enum
		{
			WIDGET_EDIT_PANE,
			WIDGET_READABLE_NAME,
			WIDGET_XDATA_NAME,
			WIDGET_SAVEBUTTON,
			WIDGET_NUMPAGES,
			WIDGET_RADIO_ONESIDED,
			WIDGET_RADIO_TWOSIDED,
			WIDGET_CURRENT_PAGE,
			WIDGET_PAGETURN_SND,
			WIDGET_GUI_ENTRY,
			WIDGET_PAGE_LEFT,
			WIDGET_PAGE_RIGHT,
			WIDGET_PAGE_TITLE,
			WIDGET_PAGE_RIGHT_TITLE,
			WIDGET_PAGE_RIGHT_TITLE_SCROLLED,
			WIDGET_PAGE_BODY,
			WIDGET_PAGE_RIGHT_BODY,
			WIDGET_PAGE_RIGHT_BODY_SCROLLED,
			WIDGET_SHIFT_RIGHT,
			WIDGET_SHIFT_LEFT,
		};

	}

// UI creation:
	ReadableEditorDialog::ReadableEditorDialog(Entity* entity) :
		gtkutil::BlockingTransientWindow(WINDOW_TITLE, GlobalMainFrame().getTopLevelWindow()),
		_guiView(new gui::GuiView),
		_result(RESULT_CANCEL),
		_entity(entity)
	{
		// Initialization:
		_xdLoader.reset(new XData::XDataLoader());
		_currentPageIndex = 0;

		// Set the default border width in accordance to the HIG
		gtk_container_set_border_width(GTK_CONTAINER(getWindow()), 12);
		gtk_window_set_type_hint(GTK_WINDOW(getWindow()), GDK_WINDOW_TYPE_HINT_DIALOG);

		// Add a vbox for the dialog elements
		GtkWidget* vbox = gtk_vbox_new(FALSE, 6);

		// The vbox is split horizontally, left are the controls, right is the preview
		GtkWidget* hbox = gtk_hbox_new(FALSE, 6);

		// The hbox contains the controls
		gtk_box_pack_start(GTK_BOX(hbox), createEditPane(), TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(hbox), gtkutil::FramedWidget(_guiView->getWidget()), FALSE, FALSE, 0);

		gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), createButtonPanel(), FALSE, FALSE, 0);

		gtk_container_add(GTK_CONTAINER(getWindow()), vbox);
	}

	GtkWidget* ReadableEditorDialog::createEditPane()
	{
		// To Do:
		//	1) Need to connect the radiobutton signal

		GtkWidget* vbox = gtk_vbox_new(FALSE, 6);
		_widgets[WIDGET_EDIT_PANE] = vbox;

		// Add a Headline-label
		GtkWidget* generalPropertiesLabel = gtkutil::LeftAlignedLabel(
			std::string("<span weight=\"bold\">") + LABEL_GENERAL_PROPERTIES + "</span>"
			);
		gtk_box_pack_start(GTK_BOX(vbox), generalPropertiesLabel, FALSE, FALSE, 0);

		// Create the table for the widget alignment
		GtkTable* table = GTK_TABLE(gtk_table_new(4, 2, FALSE));
		gtk_table_set_row_spacings(table, 6);
		gtk_table_set_col_spacings(table, 6);
		GtkWidget* alignmentMT = gtkutil::LeftAlignment(GTK_WIDGET(table), 18, 1.0);
		gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(alignmentMT), FALSE, FALSE, 0);

		int curRow = 0;
		
		// Readable Name
		GtkWidget* nameEntry = gtk_entry_new();
		_widgets[WIDGET_READABLE_NAME] = nameEntry;
		gtk_entry_set_width_chars(GTK_ENTRY(nameEntry), MIN_ENTRY_WIDTH);
		g_signal_connect(
			G_OBJECT(nameEntry), "key-press-event", G_CALLBACK(onKeyPress), this
			);

		GtkWidget* nameLabel = gtkutil::LeftAlignedLabel("Inventory Name:");

		gtk_table_attach(table, nameLabel, 0, 1, curRow, curRow+1, GTK_FILL, GTK_FILL, 0, 0);
		gtk_table_attach_defaults(table, nameEntry, 1, 2, curRow, curRow+1);

		curRow++;

		// XData Name
		GtkWidget* xdataNameEntry = gtk_entry_new();
		_widgets[WIDGET_XDATA_NAME] = xdataNameEntry;
		g_signal_connect(
			G_OBJECT(xdataNameEntry), "key-press-event", G_CALLBACK(onKeyPress), this
			);
		g_signal_connect(
			G_OBJECT(xdataNameEntry), "focus-out-event", G_CALLBACK(onFocusOut), this
			);

		GtkWidget* xDataNameLabel = gtkutil::LeftAlignedLabel("XData Name:");

		// Add a browse-button.
		GtkWidget* browseXdButton = gtk_button_new_with_label("");
		gtk_button_set_image(GTK_BUTTON(browseXdButton), gtk_image_new_from_stock(GTK_STOCK_OPEN, GTK_ICON_SIZE_SMALL_TOOLBAR) );
		g_signal_connect(
			G_OBJECT(browseXdButton), "clicked", G_CALLBACK(onBrowseXd), this
			);

		GtkWidget* hboxXd = gtk_hbox_new(FALSE, 6);
		gtk_box_pack_start(GTK_BOX(hboxXd), GTK_WIDGET(xdataNameEntry), TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(hboxXd), GTK_WIDGET(browseXdButton), FALSE, FALSE, 0);

		gtk_table_attach(table, xDataNameLabel, 0, 1, curRow, curRow+1, GTK_FILL, GTK_FILL, 0, 0);
		gtk_table_attach_defaults(table, hboxXd, 1, 2, curRow, curRow+1);

		curRow++;

		// Page count
		GtkWidget* numPagesSpin = gtk_spin_button_new_with_range(1, XData::MAX_PAGE_COUNT, 1);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(numPagesSpin), 0);
		gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(numPagesSpin), TRUE);
		_widgets[WIDGET_NUMPAGES] = numPagesSpin;
		g_signal_connect(
			G_OBJECT(numPagesSpin), "value-changed", G_CALLBACK(onValueChanged), this
			);
		g_signal_connect(
			G_OBJECT(numPagesSpin), "key-press-event", G_CALLBACK(onKeyPress), this
			);

		GtkWidget* numPagesLabel = gtkutil::LeftAlignedLabel("Number of Pages:");

		// Page Layout:
		GtkWidget* PageLayoutLabel = gtkutil::LeftAlignedLabel("Layout:");
		GtkWidget* OneSidedRadio = gtk_radio_button_new_with_label( NULL, "One-sided" );
		gtk_widget_add_events(OneSidedRadio, GDK_BUTTON_PRESS_MASK );
		g_signal_connect(
			G_OBJECT(OneSidedRadio), "button-press-event", G_CALLBACK(onOneSided), this
			);
		_widgets[WIDGET_RADIO_ONESIDED] = OneSidedRadio;
		GSList* PageLayoutGroup = gtk_radio_button_get_group( GTK_RADIO_BUTTON(OneSidedRadio) );
		GtkWidget* TwoSidedRadio = gtk_radio_button_new_with_label( PageLayoutGroup, "Two-sided" );
		gtk_widget_add_events(TwoSidedRadio, GDK_BUTTON_PRESS_MASK );
		g_signal_connect(
			G_OBJECT(TwoSidedRadio), "button-press-event", G_CALLBACK(onTwoSided), this
			);
		_widgets[WIDGET_RADIO_TWOSIDED] = TwoSidedRadio;

		// Add the radiobuttons to an hbox
		GtkWidget* hboxPL = gtk_hbox_new(FALSE, 6);
		gtk_box_pack_start(GTK_BOX(hboxPL), GTK_WIDGET(numPagesSpin), TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(hboxPL), GTK_WIDGET(PageLayoutLabel), FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(hboxPL), GTK_WIDGET(OneSidedRadio), FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(hboxPL), GTK_WIDGET(TwoSidedRadio), FALSE, FALSE, 0);

		// Add numPages Label and hbox to the table.
		gtk_table_attach(table, numPagesLabel, 0, 1, curRow, curRow+1, GTK_FILL, GTK_FILL, 0, 0);
		gtk_table_attach_defaults(table, hboxPL, 1, 2, curRow, curRow+1);

		curRow++;

		// Pageturn Sound
		GtkWidget* pageTurnEntry = gtk_entry_new();
		_widgets[WIDGET_PAGETURN_SND] = pageTurnEntry;
		GtkWidget* pageTurnLabel = gtkutil::LeftAlignedLabel("Pageturn Sound:");

		gtk_table_attach(table, pageTurnLabel, 0, 1, curRow, curRow+1, GTK_FILL, GTK_FILL, 0, 0);
		gtk_table_attach_defaults(table, pageTurnEntry, 1, 2, curRow, curRow+1);

		curRow++;

		// Add a label for page operations
		GtkWidget* pageOperationsLabel = gtkutil::LeftAlignedLabel(
			std::string("<span weight=\"bold\">") + LABEL_PAGE_OPERATIONS + "</span>"
			);
		gtk_box_pack_start(GTK_BOX(vbox), pageOperationsLabel, FALSE, FALSE, 0);

		// Insert Button
		GtkWidget* insertButton = gtk_button_new_with_label("Insert");
		gtk_button_set_image(GTK_BUTTON(insertButton), gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_SMALL_TOOLBAR) );
		g_signal_connect(
			G_OBJECT(insertButton), "clicked", G_CALLBACK(onInsert), this
			);

		// Delete Button
		GtkWidget* deleteButton = gtk_button_new_with_label("Delete");
		gtk_button_set_image(GTK_BUTTON(deleteButton), gtk_image_new_from_stock(GTK_STOCK_REMOVE, GTK_ICON_SIZE_SMALL_TOOLBAR) );
		g_signal_connect(
			G_OBJECT(deleteButton), "clicked", G_CALLBACK(onDelete), this
			);

		// Shift-buttons
		GtkWidget* shiftRightButton = gtk_button_new_with_label("Shift");
		gtk_button_set_image(GTK_BUTTON(shiftRightButton), gtk_image_new_from_stock(GTK_STOCK_MEDIA_FORWARD, GTK_ICON_SIZE_SMALL_TOOLBAR) );
		g_signal_connect(
			G_OBJECT(shiftRightButton), "clicked", G_CALLBACK(onShiftRight), this
			);
		_widgets[WIDGET_SHIFT_RIGHT] = shiftRightButton;
		GtkWidget* shiftLeftButton = gtk_button_new_with_label("Shift");
		gtk_button_set_image(GTK_BUTTON(shiftLeftButton), gtk_image_new_from_stock(GTK_STOCK_MEDIA_REWIND, GTK_ICON_SIZE_SMALL_TOOLBAR) );
		g_signal_connect(
			G_OBJECT(shiftLeftButton), "clicked", G_CALLBACK(onShiftLeft), this
			);
		_widgets[WIDGET_SHIFT_LEFT] = shiftLeftButton;

		// Add the buttons to an hbox, stuff it into an alignment and add it to the vbox.
		GtkWidget* hboxEC = gtk_hbox_new(FALSE, 6);
		gtk_box_pack_start(GTK_BOX(hboxEC),shiftLeftButton,TRUE,TRUE,0);
		gtk_box_pack_start(GTK_BOX(hboxEC),insertButton,TRUE,TRUE,0);
		gtk_box_pack_start(GTK_BOX(hboxEC),deleteButton,TRUE,TRUE,0);
		gtk_box_pack_start(GTK_BOX(hboxEC),shiftRightButton,TRUE,TRUE,0);

		GtkWidget* alignmentEC = gtkutil::LeftAlignment(GTK_WIDGET(hboxEC), 18, 1.0); 
		gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(alignmentEC), FALSE, FALSE, 0);

		// Page Switcher
		GtkWidget* prevPage = gtk_button_new_with_label("");
		gtk_button_set_image(GTK_BUTTON(prevPage), gtk_image_new_from_stock(GTK_STOCK_GO_BACK, GTK_ICON_SIZE_SMALL_TOOLBAR) );
		g_signal_connect(
			G_OBJECT(prevPage), "clicked", G_CALLBACK(onPrevPage), this
			);
		GtkWidget* nextPage = gtk_button_new_with_label("");
		gtk_button_set_image(GTK_BUTTON(nextPage), gtk_image_new_from_stock(GTK_STOCK_GO_FORWARD, GTK_ICON_SIZE_SMALL_TOOLBAR) );
		g_signal_connect(
			G_OBJECT(nextPage), "clicked", G_CALLBACK(onNextPage), this
			);
		GtkWidget* firstPage = gtk_button_new_with_label("");
		gtk_button_set_image(GTK_BUTTON(firstPage), gtk_image_new_from_stock(GTK_STOCK_GOTO_FIRST, GTK_ICON_SIZE_SMALL_TOOLBAR) );
		g_signal_connect(
			G_OBJECT(firstPage), "clicked", G_CALLBACK(onFirstPage), this
			);
		GtkWidget* LastPage = gtk_button_new_with_label("");
		gtk_button_set_image(GTK_BUTTON(LastPage), gtk_image_new_from_stock(GTK_STOCK_GOTO_LAST, GTK_ICON_SIZE_SMALL_TOOLBAR) );
		g_signal_connect(
			G_OBJECT(LastPage), "clicked", G_CALLBACK(onLastPage), this
			);
		GtkWidget* currPageLabel = gtkutil::LeftAlignedLabel("Current Page:");
		GtkWidget* currPageDisplay = gtkutil::LeftAlignedLabel("1");
		_widgets[WIDGET_CURRENT_PAGE] = currPageDisplay;

		// Add the elements to an hbox
		GtkWidget* hboxPS = gtk_hbox_new(FALSE, 6);
		gtk_box_pack_start(GTK_BOX(hboxPS), GTK_WIDGET(firstPage), FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(hboxPS), GTK_WIDGET(prevPage), FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(hboxPS), GTK_WIDGET(currPageLabel), FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(hboxPS), GTK_WIDGET(currPageDisplay), FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(hboxPS), GTK_WIDGET(nextPage), FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(hboxPS), GTK_WIDGET(LastPage), FALSE, FALSE, 0);

		// Align the hbox to the center
		GtkWidget* currPageContainer = gtk_alignment_new(0.5,1,0,0);
		gtk_container_add(GTK_CONTAINER(currPageContainer), hboxPS);

		gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(currPageContainer), FALSE, FALSE, 0);		

		// Add a label for page related edits and add it to the vbox
		GtkWidget* pageRelatedLabel = gtkutil::LeftAlignedLabel(
			std::string("<span weight=\"bold\">") + LABEL_PAGE_RELATED + "</span>"
			);
		gtk_box_pack_start(GTK_BOX(vbox), pageRelatedLabel, FALSE, FALSE, 0);

		// Add a gui chooser with a browse-button
		GtkWidget* guiLabel = gtkutil::LeftAlignedLabel("Gui Definition:");

		GtkWidget* guiEntry = gtk_entry_new();
		_widgets[WIDGET_GUI_ENTRY] = guiEntry;

		GtkWidget* browseGuiButton = gtk_button_new_with_label("");
		gtk_button_set_image(GTK_BUTTON(browseGuiButton), gtk_image_new_from_stock(GTK_STOCK_OPEN, GTK_ICON_SIZE_SMALL_TOOLBAR) );
		g_signal_connect(
			G_OBJECT(browseGuiButton), "clicked", G_CALLBACK(onBrowseGui), this
			);

		// Add the elements to an hbox
		GtkWidget* hboxGui = gtk_hbox_new(FALSE, 12);
		gtk_box_pack_start(GTK_BOX(hboxGui), GTK_WIDGET(guiLabel), FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(hboxGui), GTK_WIDGET(guiEntry), TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(hboxGui), GTK_WIDGET(browseGuiButton), FALSE, FALSE, 0);

		// Pack it into an alignment so that it's indented.
		GtkWidget* alignment = gtkutil::LeftAlignment(GTK_WIDGET(hboxGui), 18, 1.0); 
		gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(alignment), FALSE, FALSE, 0);

		// Page title and body edit fields: Create a 3x3 table
		GtkTable* tablePE = GTK_TABLE(gtk_table_new(4, 3, FALSE));
		gtk_table_set_row_spacings(tablePE, 6);
		gtk_table_set_col_spacings(tablePE, 12);

		// Pack it into an alignment and add it to vbox
		GtkWidget* alignmentTable = gtkutil::LeftAlignment(GTK_WIDGET(tablePE), 18, 1.0);
		gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(alignmentTable), TRUE, TRUE, 0);
		curRow = 0;

		// Create "left" and "right" labels and add them to the first row of the table
		GtkWidget* pageLeftLabel = gtk_label_new("Left");
		gtk_label_set_justify(GTK_LABEL(pageLeftLabel), GTK_JUSTIFY_CENTER);
		_widgets[WIDGET_PAGE_LEFT] = pageLeftLabel;
		
		GtkWidget* pageRightLabel = gtk_label_new("Right");
		gtk_label_set_justify(GTK_LABEL(pageRightLabel), GTK_JUSTIFY_CENTER);
		_widgets[WIDGET_PAGE_RIGHT] = pageRightLabel;

		gtk_table_attach(tablePE, pageLeftLabel, 1, 2, curRow, curRow+1, GTK_FILL, GTK_FILL, 0, 0);
		gtk_table_attach(tablePE, pageRightLabel, 2, 3, curRow, curRow+1, GTK_FILL, GTK_FILL, 0, 0);

		curRow++;

		// Create "title" label and title-edit fields and add them to the second row of the table
		GtkWidget* titleLabel = gtkutil::LeftAlignedLabel("Title:");	

		GtkWidget* textViewTitle = gtk_text_view_new();
		gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(textViewTitle), GTK_WRAP_WORD);
		_widgets[WIDGET_PAGE_TITLE] = textViewTitle;	

		GtkWidget* textViewRightTitle = gtk_text_view_new();
		gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(textViewRightTitle), GTK_WRAP_WORD);
		_bufferRightTitle = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textViewRightTitle));
		_widgets[WIDGET_PAGE_RIGHT_TITLE] = textViewRightTitle;
		_widgets[WIDGET_PAGE_RIGHT_TITLE_SCROLLED] = gtkutil::ScrolledFrame(textViewRightTitle);

		gtk_table_attach(tablePE, titleLabel, 0, 1, curRow, curRow+1, GTK_FILL, GTK_FILL, 0, 0);
		gtk_table_attach(tablePE, gtkutil::ScrolledFrame(textViewTitle), 1, 2, curRow, curRow+1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GTK_FILL, 0, 0);
		gtk_table_attach(tablePE, _widgets[WIDGET_PAGE_RIGHT_TITLE_SCROLLED], 2, 3, curRow, curRow+1, GtkAttachOptions(GTK_FILL |GTK_EXPAND), GTK_FILL, 0, 0);

		curRow++;

		// Create "body" label and body-edit fields and add them to the third row of the table
		GtkWidget* bodyLabel = gtkutil::LeftAlignedLabel("Body:");

		GtkWidget* textViewBody = gtk_text_view_new();
		gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(textViewBody), GTK_WRAP_WORD);
		_widgets[WIDGET_PAGE_BODY] = textViewBody;

		GtkWidget* textViewRightBody = gtk_text_view_new();
		gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(textViewRightBody), GTK_WRAP_WORD);
		_bufferRightBody = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textViewRightBody));
		_widgets[WIDGET_PAGE_RIGHT_BODY] = textViewRightBody;
		_widgets[WIDGET_PAGE_RIGHT_BODY_SCROLLED] = gtkutil::ScrolledFrame(textViewRightBody);

		gtk_table_attach(tablePE, bodyLabel, 0, 1, curRow, curRow+1, GTK_FILL, GTK_FILL, 0, 0);
		gtk_table_attach_defaults(tablePE, gtkutil::ScrolledFrame(textViewBody), 1, 2, curRow, curRow+1);
		gtk_table_attach_defaults(tablePE, _widgets[WIDGET_PAGE_RIGHT_BODY_SCROLLED], 2, 3, curRow, curRow+1);

		curRow++;

		return vbox;
	}

	GtkWidget* ReadableEditorDialog::createButtonPanel()
	{
		GtkWidget* hbx = gtk_hbox_new(TRUE, 6);

		GtkWidget* cancelButton = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
		_widgets[WIDGET_SAVEBUTTON] = gtk_button_new_from_stock(GTK_STOCK_SAVE);
		
		g_signal_connect(
			G_OBJECT(cancelButton), "clicked", G_CALLBACK(onCancel), this
		);
		g_signal_connect(
			G_OBJECT(_widgets[WIDGET_SAVEBUTTON]), "clicked", G_CALLBACK(onSave), this
		);

		gtk_box_pack_end(GTK_BOX(hbx), _widgets[WIDGET_SAVEBUTTON], TRUE, TRUE, 0);
		gtk_box_pack_end(GTK_BOX(hbx), cancelButton, TRUE, TRUE, 0);

		return gtkutil::RightAlignment(hbx);
	}

	void ReadableEditorDialog::_postShow()
	{
		// Load the initial values from the entity
		if (!initControlsFromEntity())
		{
			// User clicked cancel, so destroy the window.
			this->destroy();
			return;
		}

		// Initialize proper editing controls.
		toggleTwoSidedEditing( _xData->getPageLayout() == XData::TwoSided );
		populateControlsFromXData();

		// Initialise the GL widget after the widgets have been shown
		_guiView->initialiseView();

		BlockingTransientWindow::_postShow();
	}

	void ReadableEditorDialog::RunDialog(const cmd::ArgumentList& args)
	{
		// Check prerequisites
		const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();

		if (info.entityCount == 1 && info.totalCount == info.entityCount)
		{
			// Check the entity type
			Entity* entity = Node_getEntity(GlobalSelectionSystem().ultimateSelected());

			if (entity != NULL && entity->getKeyValue("editor_readable") == "1")
			{
				// Show the dialog
				ReadableEditorDialog dialog(entity);
				dialog.show();

				return;
			}
		}

		// Exactly one redable entity must be selected.
		gtkutil::errorDialog(NO_ENTITY_ERROR, GlobalMainFrame().getTopLevelWindow());
	}

// private Methods:

	bool ReadableEditorDialog::initControlsFromEntity()
	{
		// Inv_name
		gtk_entry_set_text(GTK_ENTRY(_widgets[WIDGET_READABLE_NAME]), _entity->getKeyValue("inv_name").c_str());

		// Xdata contents
		gtk_entry_set_text(GTK_ENTRY(_widgets[WIDGET_XDATA_NAME]), _entity->getKeyValue("xdata_contents").c_str());

		// Load xdata
		if (_entity->getKeyValue("xdata_contents") != "")
		{
			XData::XDataMap xdMap;
			if ( _xdLoader->importDef(_entity->getKeyValue("xdata_contents"),xdMap) )
			{
				if (xdMap.size() > 1)
				{
					// The requested definition has been defined in multiple files. Use the XdFileChooserDialog to pick a file.
					// Optimally, the preview renderer would already show the selected definition.
					XData::XDataMap::iterator ChosenIt = xdMap.end();
					XdFileChooserDialog fcDialog(&ChosenIt, &xdMap);
					fcDialog.show();
					if (ChosenIt == xdMap.end())
					{
						//User clicked cancel. The window will be destroyed in _postShow()...
						return false;
					}
					_xdFilename = ChosenIt->first;
					_xData = ChosenIt->second;
				}
				else
				{
					_xdFilename = xdMap.begin()->first;
					_xData = xdMap.begin()->second;
				}
				return true;
			}
			else
			{
				//Import failed. Display the errormessage and use the default filename.
				gtkutil::errorDialog( _xdLoader->getImportSummary()[_xdLoader->getImportSummary().size()-1] + "\nCreating a new XData definition..." , GlobalMainFrame().getTopLevelWindow());
			}
		}

		//No Xdata definition was defined or failed to import. Use mapname as a filename and create a OneSidedXData-object.
		_xdFilename = GlobalMapModule().getMapName();
		std::size_t nameStartPos = _xdFilename.rfind("/")+1;
		_xdFilename = _xdFilename.substr( nameStartPos, _xdFilename.rfind(".")-nameStartPos );	//might lead to weird behavior if mapname has not been defined yet.
		_xData.reset(new XData::OneSidedXData("readables/" + _xdFilename + "/<enter name>"));
		_xdFilename += ".xd";
		_xData->setNumPages(1);

		return true;
	}

	void ReadableEditorDialog::save()
	{
		// Name
		_entity->setKeyValue("inv_name", gtk_entry_get_text(GTK_ENTRY(_widgets[WIDGET_READABLE_NAME])));

		// Xdata contents
		_entity->setKeyValue("xdata_contents", gtk_entry_get_text(GTK_ENTRY(_widgets[WIDGET_XDATA_NAME])));

		// Save xdata
		storeXData();
		std::string path_ = GlobalRegistry().get(RKEY_ENGINE_PATH) + "darkmod/xdata/" + _xdFilename;
		XData::FileStatus fst = _xData->xport( path_, XData::Merge);
		if ( fst == XData::DefinitionExists)
		{
			switch ( _xData->xport( path_, XData::MergeOverwriteExisting) )
			{
			case XData::OpenFailed: 
				gtkutil::errorDialog( "Failed to open " + _xdFilename + " for saving." , GlobalMainFrame().getTopLevelWindow());
				break;
			case XData::MergeFailed: 
				gtkutil::errorDialog( "Merging failed, because the length of the definition to be overwritten could not be retrieved.", 
					GlobalMainFrame().getTopLevelWindow()
					);
				break;
			default: break; //success!
			}
		}
		else if (fst == XData::OpenFailed)
			gtkutil::errorDialog( "Failed to open " + _xdFilename + " for saving." , GlobalMainFrame().getTopLevelWindow());
	}

	void ReadableEditorDialog::storeXData()
	{
		//NumPages does not need storing, because it's stored directly after changing it.
		_xData->setName( gtk_entry_get_text(GTK_ENTRY(_widgets[WIDGET_XDATA_NAME])) );
		_xData->setSndPageTurn( gtk_entry_get_text(GTK_ENTRY(_widgets[WIDGET_PAGETURN_SND])) );

		storeCurrentPage();
	}

	void ReadableEditorDialog::toggleTwoSidedEditing(bool show)
	{
		gtk_widget_set_sensitive(_widgets[WIDGET_SHIFT_LEFT], show);
		gtk_widget_set_sensitive(_widgets[WIDGET_SHIFT_RIGHT], show);
		if (show)
		{
			gtk_widget_show(_widgets[WIDGET_PAGE_RIGHT_BODY_SCROLLED]);
			gtk_widget_show(_widgets[WIDGET_PAGE_RIGHT_TITLE_SCROLLED]);
			gtk_widget_show(_widgets[WIDGET_PAGE_LEFT]);
			gtk_widget_show(_widgets[WIDGET_PAGE_RIGHT]);
		}
		else
		{
			gtk_widget_hide(_widgets[WIDGET_PAGE_RIGHT_BODY_SCROLLED]);
			gtk_widget_hide(_widgets[WIDGET_PAGE_RIGHT_TITLE_SCROLLED]);
			gtk_widget_hide(_widgets[WIDGET_PAGE_LEFT]);
			gtk_widget_hide(_widgets[WIDGET_PAGE_RIGHT]);
		}
	}

	void ReadableEditorDialog::showPage(std::size_t pageIndex)
	{
		// To Do: Update the renderer.
		
		// Update CurrentPage Label
		_currentPageIndex = pageIndex;
		gtk_label_set_text(GTK_LABEL(_widgets[WIDGET_CURRENT_PAGE]), boost::lexical_cast<std::string>(pageIndex+1).c_str() );

		// Update page statements textviews from xData
		std::string getString = _xData->getPageContent(XData::Title, pageIndex, XData::Left);
		gtk_text_buffer_set_text( GTK_TEXT_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(_widgets[WIDGET_PAGE_TITLE]))), getString.c_str(), getString.size() );
		getString = _xData->getPageContent(XData::Body, pageIndex, XData::Left);
		gtk_text_buffer_set_text( GTK_TEXT_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(_widgets[WIDGET_PAGE_BODY]))), getString.c_str(), getString.size() );
		if (_xData->getPageLayout() == XData::TwoSided)
		{
			getString = _xData->getPageContent(XData::Title, pageIndex, XData::Right);
			gtk_text_buffer_set_text( GTK_TEXT_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(_widgets[WIDGET_PAGE_RIGHT_TITLE]))), getString.c_str(), getString.size() );
			getString = _xData->getPageContent(XData::Body, pageIndex, XData::Right);
			gtk_text_buffer_set_text( GTK_TEXT_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(_widgets[WIDGET_PAGE_RIGHT_BODY]))), getString.c_str(), getString.size() );
			// Update Gui statement entry from xData
			getString = _xData->getGuiPage(pageIndex);
			if (getString != "")
				gtk_entry_set_text(GTK_ENTRY(_widgets[WIDGET_GUI_ENTRY]), getString.c_str());
			else
				gtk_entry_set_text(GTK_ENTRY(_widgets[WIDGET_GUI_ENTRY]), XData::DEFAULT_TWOSIDED_GUI);
		}
		else
		{
			// Update Gui statement entry from xData
			getString = _xData->getGuiPage(pageIndex);
			if (getString != "")
				gtk_entry_set_text(GTK_ENTRY(_widgets[WIDGET_GUI_ENTRY]), getString.c_str());
			else
				gtk_entry_set_text(GTK_ENTRY(_widgets[WIDGET_GUI_ENTRY]), XData::DEFAULT_ONESIDED_GUI);
		}
	}


	void ReadableEditorDialog::storeCurrentPage()
	{
		// Store the GUI-Page
		_xData->setGuiPage( gtk_entry_get_text(GTK_ENTRY(_widgets[WIDGET_GUI_ENTRY])), _currentPageIndex);

		// On OneSidedXData the Side-enum is discarded, so it's save to call this method
		_xData->setPageContent(XData::Title, _currentPageIndex, XData::Left, readTextBuffer(GTK_TEXT_VIEW(_widgets[WIDGET_PAGE_TITLE])));
		_xData->setPageContent(XData::Body, _currentPageIndex, XData::Left, readTextBuffer(GTK_TEXT_VIEW(_widgets[WIDGET_PAGE_BODY])));
		if (_xData->getPageLayout() == XData::TwoSided)
		{
			_xData->setPageContent(XData::Title, _currentPageIndex, XData::Right, readTextBuffer(_bufferRightTitle));
			_xData->setPageContent(XData::Body, _currentPageIndex, XData::Right, readTextBuffer(_bufferRightBody));
		}
	}

	void ReadableEditorDialog::populateControlsFromXData()
	{
		showPage(0);

		gtk_entry_set_text( GTK_ENTRY(_widgets[WIDGET_XDATA_NAME]), _xData->getName().c_str() );
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(_widgets[WIDGET_NUMPAGES]), _xData->getNumPages() );
		std::string getString = _xData->getSndPageTurn();
		if (getString == "")
			gtk_entry_set_text( GTK_ENTRY(_widgets[WIDGET_PAGETURN_SND]), XData::DEFAULT_SNDPAGETURN );
		else
			gtk_entry_set_text( GTK_ENTRY(_widgets[WIDGET_PAGETURN_SND]), getString.c_str() );
		if (_xData->getPageLayout() == XData::TwoSided)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_widgets[WIDGET_RADIO_TWOSIDED]), TRUE);
		else
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_widgets[WIDGET_RADIO_ONESIDED]), TRUE);
	}

	void ReadableEditorDialog::checkXDataUniqueness()
	{
		std::string xdn = gtk_entry_get_text(GTK_ENTRY(_widgets[WIDGET_XDATA_NAME]));
		_xdLoader->retrieveXdInfo();
		XData::StringVectorMap::const_iterator it = _xdLoader->getDefinitionList().find(xdn);
		if (it != _xdLoader->getDefinitionList().end())
		{
			//The definition already exists. Ask the user whether it should be imported. If not make a different name suggestion.
			IDialogPtr popup = GlobalDialogManager().createMessageBox(
				"Import definition?", "The definition " + xdn + " already exists. Should it be imported?", ui::IDialog::MESSAGE_ASK);
			
			std::string message = "";

			if (popup->run() == ui::IDialog::RESULT_YES)
			{
				XData::XDataMap xdMap;
				if ( _xdLoader->importDef(xdn,xdMap) )
				{
					if (xdMap.size() > 1)
					{
						// The requested definition has been defined in multiple files. Use the XdFileChooserDialog to pick a file.
						// Optimally, the preview renderer would already show the selected definition.
						XData::XDataMap::iterator ChosenIt = xdMap.end();
						XdFileChooserDialog fcDialog(&ChosenIt, &xdMap);
						fcDialog.show();
						if (ChosenIt == xdMap.end())
						{
							//User clicked cancel.
							return;
						}
						_xdFilename = ChosenIt->first;
						_xData = ChosenIt->second;
					}
					else
					{
						_xdFilename = xdMap.begin()->first;
						_xData = xdMap.begin()->second;
					}
					populateControlsFromXData();
					return;
				}
				else
				{
					//Import failed. Store the error message and move on to suggesting another XData name.
					message = _xdLoader->getImportSummary()[_xdLoader->getImportSummary().size()-1];
				}				
			}
			//Dialog RESULT_NO! Make a different name suggestion!
			std::string suggestion;
			for (int n=1; n>0; n++)
			{
				suggestion = xdn + boost::lexical_cast<std::string>(n);
				if (_xdLoader->getDefinitionList().find(suggestion) == _xdLoader->getDefinitionList().end())
					//The suggested XData-name does not exist.
					break;
			}
			gtk_entry_set_text(GTK_ENTRY(_widgets[WIDGET_XDATA_NAME]), suggestion.c_str());
			popup = GlobalDialogManager().createMessageBox(
				"XData has been renamed.", 
				message + "To avoid duplicated XData definitions, the current definition has been renamed to " + suggestion + ".", 
				IDialog::MESSAGE_CONFIRM
				);
			popup->run();
		}
	}

	void ReadableEditorDialog::insertPage()
	{
		storeCurrentPage();
		_xData->setNumPages(_xData->getNumPages()+1);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(_widgets[WIDGET_NUMPAGES]), _xData->getNumPages() );
		for (std::size_t n=_xData->getNumPages()-1; n>_currentPageIndex; n--)
		{
			_xData->setGuiPage( _xData->getGuiPage(n-1), n );
			_xData->setPageContent( XData::Title, n, XData::Left, 
				_xData->getPageContent(XData::Title, n-1, XData::Left)
				);
			_xData->setPageContent( XData::Body, n, XData::Left, 
				_xData->getPageContent(XData::Body, n-1, XData::Left)
				);
		}
		// New Page:
		_xData->setPageContent(XData::Title, _currentPageIndex, XData::Left, "");
		_xData->setPageContent(XData::Body, _currentPageIndex, XData::Left, "");
		_xData->setGuiPage( _xData->getGuiPage(_currentPageIndex+1), _currentPageIndex);

		//Right side:
		if (_xData->getPageLayout() == XData::TwoSided)
		{
			for (std::size_t n=_xData->getNumPages()-1; n>_currentPageIndex; n--)
			{
				_xData->setGuiPage( _xData->getGuiPage(n-1), n );
				_xData->setPageContent( XData::Title, n, XData::Right, 
					_xData->getPageContent(XData::Title, n-1, XData::Right)
					);
				_xData->setPageContent( XData::Body, n, XData::Right, 
					_xData->getPageContent(XData::Body, n-1, XData::Right)
					);
			}
			// New Page:
			_xData->setPageContent(XData::Title, _currentPageIndex, XData::Right, "");
			_xData->setPageContent(XData::Body, _currentPageIndex, XData::Right, "");
		}
		showPage(_currentPageIndex);
	}

	void ReadableEditorDialog::deletePage()
	{
		if (_currentPageIndex == _xData->getNumPages()-1)
		{
			if (_currentPageIndex == 0)
			{
				_xData->setNumPages(0);
				_xData->setNumPages(1);
				showPage(0);
				return;
			}
			showPage(_currentPageIndex-1);
			_xData->setNumPages(_currentPageIndex);
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(_widgets[WIDGET_NUMPAGES]), _currentPageIndex );
			_currentPageIndex -= 1;
		}
		else
		{
			for (std::size_t n=_currentPageIndex; n<_xData->getNumPages()-1; n++)
			{
				_xData->setGuiPage( _xData->getGuiPage(n+1), n );
				_xData->setPageContent( XData::Title, n, XData::Left, 
					_xData->getPageContent(XData::Title, n+1, XData::Left)
					);
				_xData->setPageContent( XData::Body, n, XData::Left, 
					_xData->getPageContent(XData::Body, n+1, XData::Left)
					);
			}
			//Right Side
			if (_xData->getPageLayout() == XData::TwoSided)
			{
				for (std::size_t n=_currentPageIndex; n<_xData->getNumPages()-1; n++)
				{
					_xData->setGuiPage( _xData->getGuiPage(n+1), n );
					_xData->setPageContent( XData::Title, n, XData::Right, 
						_xData->getPageContent(XData::Title, n+1, XData::Right)
						);
					_xData->setPageContent( XData::Body, n, XData::Right, 
						_xData->getPageContent(XData::Body, n+1, XData::Right)
						);
				}
			}
			_xData->setNumPages(_xData->getNumPages()-1);
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(_widgets[WIDGET_NUMPAGES]), _xData->getNumPages() );
			showPage(_currentPageIndex);
		}
	}


	void ReadableEditorDialog::shiftLeft()
	{
		for (std::size_t n = 0; n < _xData->getNumPages()-1; n++)
		{
			_xData->setPageContent(XData::Title, n, XData::Left,
				_xData->getPageContent(XData::Title, n, XData::Right) );
			_xData->setPageContent(XData::Title, n, XData::Right,
				_xData->getPageContent(XData::Title, n+1, XData::Left) );
			_xData->setPageContent(XData::Body, n, XData::Left,
				_xData->getPageContent(XData::Body, n, XData::Right) );
			_xData->setPageContent(XData::Body, n, XData::Right,
				_xData->getPageContent(XData::Body, n+1, XData::Left) );
		}
		_xData->setPageContent(XData::Title, _xData->getNumPages()-1, XData::Left,
			_xData->getPageContent(XData::Title, _xData->getNumPages()-1, XData::Right) );
		_xData->setPageContent(XData::Body, _xData->getNumPages()-1, XData::Left,
			_xData->getPageContent(XData::Body, _xData->getNumPages()-1, XData::Right) );
		_xData->setPageContent(XData::Title, _xData->getNumPages()-1, XData::Right, "");
		_xData->setPageContent(XData::Body, _xData->getNumPages()-1, XData::Right, "");

		showPage(_currentPageIndex);
	}

	void ReadableEditorDialog::shiftRight()
	{
		for (std::size_t n = _xData->getNumPages()-1; n>0; n--)
		{
			_xData->setPageContent(XData::Title, n, XData::Right,
				_xData->getPageContent(XData::Title, n, XData::Left) );
			_xData->setPageContent(XData::Title, n, XData::Left,
				_xData->getPageContent(XData::Title, n-1, XData::Right) );
			_xData->setPageContent(XData::Body, n, XData::Right,
				_xData->getPageContent(XData::Body, n, XData::Left) );
			_xData->setPageContent(XData::Body, n, XData::Left,
				_xData->getPageContent(XData::Body, n-1, XData::Right) );
		}
		_xData->setPageContent(XData::Title, 0, XData::Right,
			_xData->getPageContent(XData::Title, 0, XData::Left) );
		_xData->setPageContent(XData::Body, 0, XData::Right,
			_xData->getPageContent(XData::Body, 0, XData::Left) );
		_xData->setPageContent(XData::Title, 0, XData::Left, "");
		_xData->setPageContent(XData::Body, 0, XData::Left, "");

		showPage(_currentPageIndex);
	}
// Callback Methods:

	void ReadableEditorDialog::onCancel(GtkWidget* widget, ReadableEditorDialog* self) 
	{
		self->_result = RESULT_CANCEL;

		self->destroy();
	}

	void ReadableEditorDialog::onSave(GtkWidget* widget, ReadableEditorDialog* self) 
	{
		self->_result = RESULT_OK;

		self->save();

		// Done, just destroy the window
		self->destroy();
	}

	void ReadableEditorDialog::onBrowseXd(GtkWidget* widget, ReadableEditorDialog* self) 
	{
		//FileChooser for xd-files
	}

	void ReadableEditorDialog::onNextPage(GtkWidget* widget, ReadableEditorDialog* self) 
	{
		if (self->_currentPageIndex+1 < self->_xData->getNumPages())
		{
			self->storeCurrentPage();
			self->showPage(self->_currentPageIndex+1);
		}
		// else insert-new-page-popup
	}

	void ReadableEditorDialog::onPrevPage(GtkWidget* widget, ReadableEditorDialog* self) 
	{
		if (self->_currentPageIndex > 0)
		{
			self->storeCurrentPage();
			self->showPage(self->_currentPageIndex-1);
		}
		// else insert-new-page-popup
	}

	void ReadableEditorDialog::onBrowseGui(GtkWidget* widget, ReadableEditorDialog* self) 
	{

	}

	gboolean ReadableEditorDialog::onOneSided(GtkWidget* widget, GdkEventKey* event, ReadableEditorDialog* self) 
	{
		if (self->_xData->getPageLayout() != XData::OneSided)
		{
			// Convert TwoSided XData to OneSided and refresh controls
			self->storeXData();
			self->toggleTwoSidedEditing(false);
			self->_xData->togglePageLayout(self->_xData);
			self->populateControlsFromXData();
		}
		return FALSE;
	}

	gboolean ReadableEditorDialog::onTwoSided(GtkWidget* widget, GdkEventKey* event, ReadableEditorDialog* self) 
	{
		if (self->_xData->getPageLayout() != XData::TwoSided)
		{
			// Convert OneSided XData to TwoSided and refresh controls
			self->storeXData();
			self->toggleTwoSidedEditing(true);
			self->_xData->togglePageLayout(self->_xData);
			self->populateControlsFromXData();			
		}
		return FALSE;
	}

	gboolean ReadableEditorDialog::onFocusOut(GtkWidget* widget, GdkEventKey* event, ReadableEditorDialog* self)
	{
		self->checkXDataUniqueness();
		return FALSE;
	}

	gboolean ReadableEditorDialog::onKeyPress(GtkWidget *widget, GdkEventKey* event, ReadableEditorDialog* self)
	{
		bool xdWidget = false;
		if (widget == self->_widgets[WIDGET_NUMPAGES])
		{
			if (event->keyval != GDK_Escape)
				return FALSE;
			// Restore the old value.
			gtk_spin_button_set_value( GTK_SPIN_BUTTON( self->_widgets[WIDGET_NUMPAGES] ), self->_xData->getNumPages() );
			return TRUE;
		}
		else if ( ( xdWidget = (widget == self->_widgets[WIDGET_XDATA_NAME]) ) || (widget == self->_widgets[WIDGET_READABLE_NAME]))
		{
			switch (event->keyval)
			{
				// Some forbidden keys
				case GDK_space:
				case GDK_Tab:
				case GDK_exclam:
				case GDK_asterisk:
				case GDK_plus:
				case GDK_KP_Multiply:
				case GDK_KP_Subtract:
				case GDK_KP_Add:
				case GDK_KP_Separator:
				case GDK_comma:
				case GDK_period:
				case GDK_colon:
				case GDK_semicolon:
				case GDK_question:
				case GDK_minus: return TRUE;
				// Check Uniqueness of the XData-Name.
				case GDK_Return:
				case GDK_KP_Enter:
					if (xdWidget)
						self->checkXDataUniqueness();
					return TRUE;
				default: return FALSE;
			}
		}
		return TRUE;
	}

	void ReadableEditorDialog::onShiftLeft(GtkWidget* widget, ReadableEditorDialog* self)
	{
		self->shiftLeft();
	}
	void ReadableEditorDialog::onShiftRight(GtkWidget* widget, ReadableEditorDialog* self)
	{
		self->shiftRight();
	}
	void ReadableEditorDialog::onInsert(GtkWidget* widget, ReadableEditorDialog* self)
	{
		self->insertPage();
	}
	void ReadableEditorDialog::onDelete(GtkWidget* widget, ReadableEditorDialog* self)
	{
		self->deletePage();
	}
	void ReadableEditorDialog::onFirstPage(GtkWidget* widget, ReadableEditorDialog* self)
	{
		self->storeCurrentPage();
		self->showPage(0);
	}
	void ReadableEditorDialog::onLastPage(GtkWidget* widget, ReadableEditorDialog* self)
	{
		self->storeCurrentPage();
		self->showPage(self->_xData->getNumPages()-1);
	}

	void ReadableEditorDialog::onValueChanged(GtkWidget* widget, ReadableEditorDialog* self)
	{
		int nNP =  gtk_spin_button_get_value_as_int( GTK_SPIN_BUTTON(self->_widgets[WIDGET_NUMPAGES]) );
		self->_xData->setNumPages( nNP );
		if (self->_currentPageIndex >= nNP )
			self->showPage(nNP-1);
	}

} // namespace ui
