import QtQuick 2.10
import QtQuick.Controls 2.1
import QtQuick.Controls.Material  2.1
import QtQuick.Dialogs 1.2
import QtQuick.Window 2.2

ApplicationWindow {
    title: rmovman.wintitle
    Material.theme: Material.Dark
    Material.accent: Material.Teal
	//Material.primary: Material.BlueGrey
    //Material.foreground: Material.Dark
    //Material.background: Material.Grey
    visible: true

	// 
	// 
	////////////////////////////////////////
	

	////////////////////////////////////////
	// funcitons 
	////////////////////////////////////////

	
	////////////////////////////////////////
	// UI definitions
	////////////////////////////////////////
	
    width: 400
    height: 320
    maximumHeight: height
    maximumWidth: width
    minimumHeight: height
    minimumWidth: width	

	Column {
		width: 520
		height: 40	
		x: 80	
		y: 60		

		spacing: 20	
		
		Button {
			id: rmvpub
			width: 240
			height: 80

			text: "SB Publisher   "
			ToolTip.visible: hovered
			ToolTip.text: qsTr("Remove Secrets to deliver this folder to SB Publisher.")	
				
			onClicked: {
					rmovman.text = "SB Publisher"
					Qt.quit()

			}			
		}	

		Button {
			id: rmvdev
			width: 240
			height: 80

			text: "SB/SD Publisher"
			ToolTip.visible: hovered
			ToolTip.text: qsTr("Remove Secrets to deliver this folder to SB/SD Publisher.")	
			
			onClicked: {
					rmovman.text = "SB/SD Publisher"
					Qt.quit()

			}				
		}	

	}
}