import QtQuick 2.10
import QtQuick.Controls 2.1
import QtQuick.Controls.Material  2.1
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.2
import QtQuick.Window 2.2

ApplicationWindow {
    title: envrmaman.wintitle
    Material.theme: Material.Dark
    Material.accent: Material.Teal
	//Material.primary: Material.BlueGrey
    //Material.foreground: Material.Dark
    //Material.background: Material.Grey	
    visible: false

	////////////////////////////////////////
	// 
	////////////////////////////////////////
	
	ListModel {
		id: hbkidlist
		ListElement { text: "hbk-id 0, CM" 	 ; color: "lightblue" }
		ListElement { text: "hbk-id 1, DM" 	 ; color: "lightgreen" }
	}
	
	////////////////////////////////////////
	// funcitons 
	////////////////////////////////////////

	function tpmrulecheck(config, mode, currentIndex) {
		var str = config
		var item = str.split(":")
		var index = currentIndex

		if( hbkid == mode && item[1] == "DM") {
			for( var i = 0; i < hbkidlist.count; i++ ){
				if( hbkidlist.get(i).text == "hbk-id 1, DM" ){
					index = i
					return index
				}
			}			
		}else if( hbkid == mode && item[1] == "CM") {
			for( var i = 0; i < hbkidlist.count; i++ ){
				if( hbkidlist.get(i).text == "hbk-id 0, CM" ){
					index = i
					return index
				}
			}			
		}
		
		return index
	}
	
	function tpmowncheck(config, own) {
		var str = config
		var item = str.split(":")

		if( item[1] == own ){
			return true
		}
		
		return false
	}	
	
	function dbgmaskupdate(mask0, conmsk, swdmsk) {
		var dbgmsk0 = parseInt(mask0, 16)
		
		if( conmsk == false ){
			dbgmsk0 = dbgmsk0 & 0x3fffffff
		}else{
			dbgmsk0 = dbgmsk0 | 0xC0000000
		}
		
		if( swdmsk == false ){
			dbgmsk0 = dbgmsk0 & 0xfffff3ff
		}else{
			dbgmsk0 = dbgmsk0 | 0x00000C00
		}

		if( dbgmsk0 < 0 ){
			dbgmsk0 = 0xFFFFFFFF + dbgmsk0 + 1;
		}
		
		return "0x" + dbgmsk0.toString(16);
	}	

	function dbglockupdate(lock0, conlck, swdlck) {
		var dbglck0 = parseInt(lock0, 16)
		
		if( conlck == false ){
			dbglck0 = dbglck0 & 0x3fffffff
		}else{
			dbglck0 = dbglck0 | 0xC0000000
		}
		
		if( swdlck == false ){
			dbglck0 = dbglck0 & 0xfffff3ff
		}else{
			dbglck0 = dbglck0 | 0x00000C00
		}

		if( dbglck0 < 0 ){
			dbglck0 = 0xFFFFFFFF + dbglck0 + 1;
		}

		return "0x" + dbglck0.toString(16);
	}	
	
	function getdbgmask_con(mask0) {
		var dbgmsk0 = parseInt(mask0, 16)
		
		if( (dbgmsk0 & 0xC0000000) == 0 ){
			return false
		}
		return true
	}

	function getdbgmask_swd(mask0) {
		var dbgmsk0 = parseInt(mask0, 16)
		
		if( (dbgmsk0 & 0x00000C00) == 0 ){
			return false
		}
		return true
	}
	
	function tpmstepcheck(config) {
		var str = config
		var item = str.split(":")

		if( item[2] == "G.B1") {
			return true
		}
		return false
	}		
		
	////////////////////////////////////////
	// UI definitions
	////////////////////////////////////////

    Timer {
        id: timer
        running: true
        repeat: false
        interval: 50
        triggeredOnStart: false
		
        onTriggered: {
            update.clicked();
        }
    }
	
    width: 520
    height: 460
    maximumHeight: height
    maximumWidth: width
    minimumHeight: height
    minimumWidth: width	

	TextArea {
		id: title
		width: 440
		height: 40	
		x: 40
		y: 20
		text: qsTr("Enabler RMA Debug Config")
		readOnly: true
	}
	
	Column {
		width: 460
		height: 240	
		x: 40
		y: 60
		visible:  ( style.checked == true ) ? true : false

		Column { 
			spacing: 2

			Row {
				Label {
					id: debuglock0label
					
					width: 160
					height: 40

					text: "debug-lock[0-31]"
				}
				TextField {
					id: debuglock0
					
					width: 280
					height: 40
					//visible : false

					text: envrmaman.debuglock0

					Keys.onReleased: {
						conlock.checked = getdbgmask_con(debuglock0.text)
						swdlock.checked = getdbgmask_swd(debuglock0.text)
					}
					
					ToolTip.visible: hovered
					ToolTip.text: qsTr("CON-Disable: [0-31]=0x7fffffff; CON&SWD-Disable: [0-31]=0x7ffff7ff")					
				}	
			}
			Row {
				Label {
					id: debuglock1label
					
					width: 160
					height: 40

					text: "debug-lock[32-63]"
				}
				TextField {
					id: debuglock1
					
					width: 280
					height: 40
					//visible : false

					text: envrmaman.debuglock1
				}	
			}			
			Row {
				Label {
					id: debuglock2label
					
					width: 160
					height: 40

					text: "debug-lock[64-95]"
				}
				TextField {
					id: debuglock2
					
					width: 280
					height: 40
					//visible : false

					text: envrmaman.debuglock2
				}	
			}			
			Row {
				Label {
					id: debuglock3label
					
					width: 160
					height: 40

					text: "debug-lock[96-127]"
				}
				TextField {
					id: debuglock3
					
					width: 280
					height: 40
					//visible : false

					text: envrmaman.debuglock3
				}	
			}
			Row {
				Label {
					id: hbkidlabel
					
					width: 160
					height: 40
					horizontalAlignment: Text.AlignLeft
					verticalAlignment: Text.AlignVCenter					

					text: "hbk-id"
					visible:  false
				}			
				ComboBox {
					id: hbkid
					
					width: 150
					height: 40
					
					model: hbkidlist
					textRole: 'text'

					contentItem: Text {
						text:  hbkidlist.get(hbkid.currentIndex).text
						horizontalAlignment: Text.AlignLeft
						verticalAlignment: Text.AlignVCenter
						anchors.left : parent.left
						anchors.leftMargin: 10				
						font.bold: true
						color : hbkidlist.get(hbkid.currentIndex).color
					}
					
					visible:  false
					currentIndex: tpmrulecheck(envrmaman.config, hbkid, 1)

					onCurrentIndexChanged: {
						if( tpmowncheck(envrmaman.config, "DM") == true ){
							hbkid.currentIndex = tpmrulecheck(envrmaman.config, hbkid, 1)
						}
					}					
				}
			}			
		}
	}
	
	Column {
		width: 460
		height: 240	
		x: 40
		y: 60
		visible:  ( style.checked == true ) ? false : true

		Column { 
			spacing: 2

			CheckBox {
				id: conlock
				height: 40
				text: qsTr("CON-Lock")
				checked: getdbgmask_con(envrmaman.debuglock0)

				onCheckedChanged: {
						debuglock0.text = dbglockupdate( debuglock0.text, conlock.checked, swdlock.checked )
				}
			}
			
			CheckBox {
				id: swdlock
				height: 40
				text: qsTr("SWD-Lock")
				checked: getdbgmask_swd(envrmaman.debuglock0)

				onCheckedChanged: {
						debuglock0.text = dbglockupdate( debuglock0.text, conlock.checked, swdlock.checked )
				}
			}
			
		}
	}	
	
	Column {
		width: 480
		height: 40	
		x: 60	
		y: 400		

		Row {
			spacing: 20	
			
			CheckBox {
				id: style
				height: 40
				text: qsTr("Edit Style")
				checked: false

				onCheckedChanged: {
				}
			}

			Button {
				id: update
				width: 140
				height: 40

				text: "Update"

				ToolTip.visible: hovered
				ToolTip.text: qsTr("Update Enabler-RMA-cfg")		
				
				onClicked: {
					envrmaman.text = 
						   debuglock0label.text + "=" + debuglock0.text + "&" 
						 + debuglock1label.text + "=" + debuglock1.text + "&" 
						 + debuglock2label.text + "=" + debuglock2.text + "&" 
						 + debuglock3label.text + "=" + debuglock3.text + "&" 
						 + hbkidlabel.text  + "=" + hbkid.currentText 
						;
					Qt.quit()
				}			
			}	

			Button {
				id: stop
				width: 140
				height: 40

				text: "Pass"
				
				onClicked: {
					Qt.quit()
				}				
			}	
		}
	}
}