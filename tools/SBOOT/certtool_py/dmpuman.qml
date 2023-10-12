import QtQuick 2.10
import QtQuick.Controls 2.1
import QtQuick.Controls.Material  2.1
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.2
import QtQuick.Window 2.2

ApplicationWindow {
    title: dmpuman.wintitle
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
    height: 500
    maximumHeight: height
    maximumWidth: width
    minimumHeight: height
    minimumWidth: width	

	Column {
		width: 480
		height: 240	
		x: 40
		y: 20

		Column { 
			spacing: 2

			Row {
				Label {
					id: hbkdatalabel
					
					width: 160
					height: 40

					text: "hbk-data"
				}
				TextField {
					id: hbkdata
					
					width: 280
					height: 40
					//visible : false
					text: dmpuman.hbkdata
					readOnly: true
				}	
			}
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

					text: dmpuman.debugmask0
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

					text: dmpuman.debugmask1
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

					text: dmpuman.debugmask2
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

					text: dmpuman.debugmask3
				}	
			}			
			Row {
				Label {
					id: kcppkglabel
					
					width: 160
					height: 40

					text: "kcp-pkg"
				}
				TextField {
					id: kcppkg
					
					width: 280
					height: 40
					//visible : false
					text: dmpuman.kcppkg
					readOnly : true
				}	
			}			
			Row {
				Label {
					id: kcepkglabel
					
					width: 160
					height: 40

					text: "kce-pkg"
				}
				TextField {
					id: kcepkg
					
					width: 280
					height: 40
					//visible : false
					text: dmpuman.kcepkg
					readOnly : true
				}	
			}
			Row {
				Label {
					id: minversionlabel
					
					width: 160
					height: 40

					text: "minversion"
				}
				TextField {
					id: minversion
					
					width: 280
					height: 40
					//visible : false

					text: dmpuman.minversion
				}	
			}
			Row {
				Label {
					id: dmpupkglabel
					
					width: 160
					height: 40

					text: "dmpu-pkg"
				}
				TextField {
					id: dmpupkg
					
					width: 280
					height: 40
					//visible : false
					text: dmpuman.dmpupkg
					readOnly : true
				}	
			}			
		}
	}
	
	
	Column {
		width: 480
		height: 40	
		x: 160	
		y: 440		

		Row {
			spacing: 20	
			
			Button {
				id: update
				width: 140
				height: 40

				text: "Update"

				ToolTip.visible: hovered
				ToolTip.text: qsTr("Update DMPU-cfg")		
				
				onClicked: {
					dmpuman.text = hbkdatalabel.text + "=" + hbkdata.text + "&" 
						 + debugmask0label.text + "=" + debugmask0.text + "&" 
						 + debugmask1label.text + "=" + debugmask1.text + "&" 
						 + debugmask2label.text + "=" + debugmask2.text + "&" 
						 + debugmask3label.text + "=" + debugmask3.text + "&" 
						 + kcppkglabel.text  + "=" + kcppkg.text + "&" 
						 + kcepkglabel.text  + "=" + kcepkg.text + "&" 
						 + minversionlabel.text  + "=" + minversion.text + "&" 
						 + dmpupkglabel.text  + "=" + dmpupkg.text
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