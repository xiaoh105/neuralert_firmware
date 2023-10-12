import QtQuick 2.10
import QtQuick.Controls 2.1
import QtQuick.Controls.Material  2.1
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.2
import QtQuick.Window 2.2

ApplicationWindow {
    title: envman.wintitle
    Material.theme: Material.Dark
    Material.accent: Material.Teal
	//Material.primary: Material.BlueGrey
    //Material.foreground: Material.Dark
    //Material.background: Material.Grey	
    visible: false

	////////////////////////////////////////
	// 
	////////////////////////////////////////
	
	////////////////////////////////////////
	// funcitons 
	////////////////////////////////////////

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
		text: qsTr("Enabler Debug Config")
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

					text: envman.debugmask0

					Keys.onReleased: {
						conmask.checked = getdbgmask_con(debugmask0.text)
						swdmask.checked = getdbgmask_swd(debugmask0.text)
					}
				
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

					text: envman.debugmask1
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

					text: envman.debugmask2
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

					text: envman.debugmask3
				}	
			}
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

					text: envman.debuglock0
					
					Keys.onReleased: {
						conlock.checked = getdbgmask_con(debuglock0.text)
						swdlock.checked = getdbgmask_swd(debuglock0.text)
					}
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

					text: envman.debuglock1
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

					text: envman.debuglock2
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

					text: envman.debuglock3
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
				checked: getdbgmask_con(envman.debugmask0)

				onCheckedChanged: {
						debugmask0.text = dbgmaskupdate( debugmask0.text, conmask.checked, swdmask.checked )
				}
			}
			
			CheckBox {
				id: swdmask
				height: 40
				text: qsTr("SWD-Enable")
				checked: getdbgmask_swd(envman.debugmask0)

				onCheckedChanged: {
						debugmask0.text = dbgmaskupdate( debugmask0.text, conmask.checked, swdmask.checked )
				}
			}

			CheckBox {
				id: conlock
				height: 40
				text: qsTr("CON-Lock")
				checked: getdbgmask_con(envman.debuglock0)

				onCheckedChanged: {
						debuglock0.text = dbglockupdate( debuglock0.text, conlock.checked, swdlock.checked )
				}
			}
			
			CheckBox {
				id: swdlock
				height: 40
				text: qsTr("SWD-Lock")
				checked: getdbgmask_swd(envman.debuglock0)

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
				ToolTip.text: qsTr("Update Enabler-cfg")		
				
				onClicked: {
					envman.text = 
						   debugmask0label.text + "=" + debugmask0.text + "&" 
						 + debugmask1label.text + "=" + debugmask1.text + "&" 
						 + debugmask2label.text + "=" + debugmask2.text + "&" 
						 + debugmask3label.text + "=" + debugmask3.text + "&" 
						 + debuglock0label.text + "=" + debuglock0.text + "&" 
						 + debuglock1label.text + "=" + debuglock1.text + "&" 
						 + debuglock2label.text + "=" + debuglock2.text + "&" 
						 + debuglock3label.text + "=" + debuglock3.text 
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