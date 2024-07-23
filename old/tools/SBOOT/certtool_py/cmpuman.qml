import QtQuick 2.10
import QtQuick.Controls 2.1
import QtQuick.Controls.Material  2.1
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.2
import QtQuick.Window 2.2

ApplicationWindow {
    title: cmpuman.wintitle
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
					text: cmpuman.hbkdata
					readOnly: true
				}	
			}
			Row {
				Label {
					id: debugmask0label
					
					width: 160
					height: 40

					text: "debug-lock[0-31]"
				}
				TextField {
					id: debugmask0
					
					width: 280
					height: 40
					//visible : false

					text: cmpuman.debugmask0
				}	
			}
			Row {
				Label {
					id: debugmask1label
					
					width: 160
					height: 40

					text: "debug-lock[32-63]"
				}
				TextField {
					id: debugmask1
					
					width: 280
					height: 40
					//visible : false

					text: cmpuman.debugmask1
				}	
			}			
			Row {
				Label {
					id: debugmask2label
					
					width: 160
					height: 40

					text: "debug-lock[64-95]"
				}
				TextField {
					id: debugmask2
					
					width: 280
					height: 40
					//visible : false

					text: cmpuman.debugmask2
				}	
			}			
			Row {
				Label {
					id: debugmask3label
					
					width: 160
					height: 40

					text: "debug-lock[96-127]"
				}
				TextField {
					id: debugmask3
					
					width: 280
					height: 40
					//visible : false

					text: cmpuman.debugmask3
				}	
			}			
			Row {
				Label {
					id: kpicvpkglabel
					
					width: 160
					height: 40

					text: "kpicv-pkg"
				}
				TextField {
					id: kpicvpkg
					
					width: 280
					height: 40
					//visible : false
					text: cmpuman.kpicvpkg
					readOnly : true
				}	
			}			
			Row {
				Label {
					id: kceicvpkglabel
					
					width: 160
					height: 40

					text: "kceicv-pkg"
				}
				TextField {
					id: kceicvpkg
					
					width: 280
					height: 40
					//visible : false
					text: cmpuman.kceicvpkg
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

					text: cmpuman.minversion
				}	
			}
			Row {
				Label {
					id: configwordlabel
					
					width: 160
					height: 40

					text: "configword"
				}
				TextField {
					id: configword
					
					width: 280
					height: 40
					//visible : false

					text: cmpuman.configword
				}	
			}			
			Row {
				Label {
					id: cmpupkglabel
					
					width: 160
					height: 40

					text: "cmpu-pkg"
				}
				TextField {
					id: cmpupkg
					
					width: 280
					height: 40
					//visible : false
					text: cmpuman.cmpupkg
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
				ToolTip.text: qsTr("Update CMPU-cfg")		
				
				onClicked: {
					cmpuman.text = hbkdatalabel.text + "=" + hbkdata.text + "&" 
						 + "debug-mask[0-31]" + "=" + debugmask0.text + "&" 
						 + "debug-mask[32-63]" + "=" + debugmask1.text + "&" 
						 + "debug-mask[64-95]" + "=" + debugmask2.text + "&" 
						 + "debug-mask[96-127]" + "=" + debugmask3.text + "&" 
						 + kpicvpkglabel.text  + "=" + kpicvpkg.text + "&" 
						 + kceicvpkglabel.text  + "=" + kceicvpkg.text + "&" 
						 + minversionlabel.text  + "=" + minversion.text + "&" 
						 + configwordlabel.text  + "=" + configword.text + "&" 
						 + cmpupkglabel.text  + "=" + cmpupkg.text
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