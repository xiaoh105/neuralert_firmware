import QtQuick 2.10
import QtQuick.Controls 2.1
import QtQuick.Controls.Material  2.1
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.2
import QtQuick.Window 2.2

ApplicationWindow {
    title: devman.wintitle
    Material.theme: Material.Dark
    Material.accent: Material.Teal
	//Material.primary: Material.BlueGrey
    //Material.foreground: Material.Dark
    //Material.background: Material.Grey
    visible: visiblesocid()

	////////////////////////////////////////
	// 
	////////////////////////////////////////
	
	////////////////////////////////////////
	// funcitons 
	////////////////////////////////////////

	function visiblesocid() {
		if( devman.issocid == '1' ){
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

		if( (item[2] == "G.B1") || (item[2] == "G.B2")) {
			return true
		}
		return false
	}		
		
	////////////////////////////////////////
	// UI definitions
	////////////////////////////////////////

    Timer {
        id: timer
        running: (visiblesocid() == true) ? false : true
        repeat: false
        interval: 50
        triggeredOnStart: false
		
        onTriggered: {
            update.clicked();
        }
    }
	
    width: 560
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
		text: qsTr("Developer Debug Config")
		readOnly: true
	}
	
	Column {
		width: 540
		height: 240	
		x: 40
		y: 60
		visible:  ( style.checked == true ) ? true : false

		Column { 
			spacing: 4

			Row {
				Label {
					id: debugmask0label
					
					width: 160
					height: 40

					text: "debug-mask[0-31]"
				}
				TextField {
					id: debugmask0
					
					width: 280
					height: 40
					//visible : false

					text: devman.debugmask0

					ToolTip.visible: hovered
					ToolTip.text: qsTr("CON-Disable: [0-31]=0x7fffffff; CON&SWD-Disable: [0-31]=0x7ffff7ff")					
				}	
			}
			Row {
				Label {
					id: debugmask1label
					
					width: 160
					height: 40

					text: "debug-mask[32-63]"
				}
				TextField {
					id: debugmask1
					
					width: 280
					height: 40
					//visible : false

					text: devman.debugmask1
				}	
			}			
			Row {
				Label {
					id: debugmask2label
					
					width: 160
					height: 40

					text: "debug-mask[64-95]"
				}
				TextField {
					id: debugmask2
					
					width: 280
					height: 40
					//visible : false

					text: devman.debugmask2
				}	
			}			
			Row {
				Label {
					id: debugmask3label
					
					width: 160
					height: 40

					text: "debug-mask[96-127]"
				}
				TextField {
					id: debugmask3
					
					width: 280
					height: 40
					//visible : false

					text: devman.debugmask3
				}	
			}

			Row {
				Label {
					id: socidlabel
					
					width: 80
					visible : visiblesocid()

					text: "SoC-ID"

				}
				Rectangle {
					width: 420
					height: 160
					border.width: 1
					//border.color: "#000000"
					radius: 3
					visible : visiblesocid()					
		
					TextEdit {
						id: socid

						x: 20
						y: 20						
						width: 400
						height: 160
						visible : visiblesocid()
						wrapMode: TextEdit.WordWrap
						selectByMouse: true
						
						font.family: "Courier"
						font.pointSize: 10

						text: if( visiblesocid() == true ) { devman.socid } else { qsTr("") }

						Keys.onReleased: {
							 socid0.text = socid.text
						}
						
						//ToolTip.visible: hovered
						//ToolTip.text: qsTr("32 bytes SoC-ID of the device")					
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
				id: conmask
				height: 40
				text: qsTr("CON-Enable")
				checked: getdbgmask_con(devman.debugmask0)

				onCheckedChanged: {
						debugmask0.text = dbgmaskupdate( debugmask0.text, conmask.checked, swdmask.checked )
				}
			}
			
			CheckBox {
				id: swdmask
				height: 40
				text: qsTr("SWD-Enable")
				checked: getdbgmask_swd(devman.debugmask0)

				onCheckedChanged: {
						debugmask0.text = dbgmaskupdate( debugmask0.text, conmask.checked, swdmask.checked )
				}
			}


			Label {
				id: socid0label
				
				width: 80
				visible : visiblesocid()

				text: "SoC-ID"

			}
			Rectangle {
				width: 480
				height: 120
				border.width: 1
				//border.color: "#000000"
				radius: 3
				visible : visiblesocid()					

				TextEdit {
					id: socid0

					x: 20
					y: 20						
					width: 460
					height: 120
					visible : visiblesocid()
					wrapMode: TextEdit.WordWrap
					selectByMouse: true

					font.family: "Courier"
					font.pointSize: 10

					text: if( visiblesocid() == true ) { devman.socid } else { qsTr("") }

					Keys.onReleased: {
						 socid.text = socid0.text
					}
					
					//ToolTip.visible: hovered
					//ToolTip.text: qsTr("32 bytes SoC-ID of the device")					
				}	
			}
		
			
		}
	}
	
	
	Column {
		width: 480
		height: 40	
		x: 180
		y: 400		

		Row {
			spacing: 20	

			CheckBox {
				id: style
				height: 40
				text: qsTr("Edit Style")
				checked: false
				visible: false

				onCheckedChanged: {
				}
			}
			
			Button {
				id: update
				width: 140
				height: 40

				text: "Update"

				ToolTip.visible: hovered
				ToolTip.text: qsTr("Update Developer-cfg")		
				
				onClicked: {
					devman.text = 
						   debugmask0label.text + "=" + debugmask0.text + "&" 
						 + debugmask1label.text + "=" + debugmask1.text + "&" 
						 + debugmask2label.text + "=" + debugmask2.text + "&" 
						 + debugmask3label.text + "=" + debugmask3.text 
					;
					if( visiblesocid() == true ){
						devman.socid = socid.text
					}
					Qt.quit()
				}			
			}	

			Button {
				id: stop
				width: 140
				height: 40
				visible: false

				text: "Pass"
				
				onClicked: {
					Qt.quit()
				}				
			}	
		}
	}
}