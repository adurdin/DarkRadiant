#if 0
#ifndef MANIPULATEOBSERVER_H_
#define MANIPULATEOBSERVER_H_

#include "windowobserver.h"
#include "Device.h"
#include "RadiantSelectionSystem.h"

/* greebo: This is the class handling the manipulate-related mouse operations, it basically just
 * passes all the mouse clicks back to the SelectionSystem. The callbacks are invoked from
 * RadiantWindowObserver, which is registered in the GlobalWindowObservers list
 */
class ManipulateObserver
{
public:
	DeviceVector _epsilon;
	const render::View* _view;

	// greebo: Handles the mouseDown event and checks whether a manipulator can be made active
	bool mouseDown(DeviceVector position);

	/* greebo: Pass the mouse movement to the current selection.
	 * This is connected to the according mouse events by the RadiantWindowObserver class
	 */
  	void mouseMoved(DeviceVector position);

  	// The mouse operation is finished, update the selection and unconnect the callbacks
  	void mouseUp(DeviceVector position);
};

#endif /*MANIPULATEOBSERVER_H_*/
#endif