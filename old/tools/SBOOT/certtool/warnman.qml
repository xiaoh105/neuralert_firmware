import QtQuick 2.10
import QtQuick.Controls 2.1
import QtQuick.Controls.Material  2.1
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.2
import QtQuick.Window 2.2

MessageDialog {
    title: "[WARN] " + warnman.wintitle
    icon: StandardIcon.Warning
    text: warnman.detail
		 
    standardButtons: StandardButton.Yes
    
	Component.onCompleted: visible = true
    
	onYes: {
		(clickedButton == StandardButton.Yes ? warnman.text = "Yes" : warnman.text = "YesToAll" )
		Qt.quit()
	}

    onRejected: {
		//console.log("aborted")
		Qt.quit()
	}
}