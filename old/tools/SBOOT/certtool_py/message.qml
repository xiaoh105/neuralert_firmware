import QtQuick 2.10
import QtQuick.Controls 2.1
import QtQuick.Controls.Material  2.1
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.2
import QtQuick.Window 2.2

MessageDialog {
    title: "[Alert] " + msgman.wintitle
    icon: StandardIcon.Warning
    text: "Do you want to remove the files?"
    detailedText: "If some PEMs, keys or certificates are deleted,\n" +
				  "You can not access the devices with previous PEMs, keys or certificates applied.\n"
		 
    standardButtons: StandardButton.YesToAll | StandardButton.NoToAll | StandardButton.Abort
    
	Component.onCompleted: visible = true
    
	onYes: {
		(clickedButton == StandardButton.Yes ? msgman.text = "Yes" : msgman.text = "YesToAll" )
		Qt.quit()
	}
    onNo: {
		(clickedButton == StandardButton.No ? msgman.text = "No" : msgman.text = "NoToAll" )
		Qt.quit()
	}
    onRejected: {
		//console.log("aborted")
		Qt.quit()
	}
}