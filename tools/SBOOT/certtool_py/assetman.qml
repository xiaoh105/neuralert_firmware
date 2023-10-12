import QtQuick 2.10
import QtQuick.Controls 2.1
import QtQuick.Controls.Material  2.1
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.2
import QtQuick.Window 2.2


ApplicationWindow {
    title: assetman.wintitle
    Material.theme: Material.Dark
    Material.accent: Material.Teal
	//Material.primary: Material.BlueGrey
    //Material.foreground: Material.Dark
    //Material.background: Material.Grey
    visible: true

	////////////////////////////////////////
	// 
	////////////////////////////////////////
	
	ListModel {
		id: keytypelist
		ListElement { text: "CM Provisioning Key (Kpicv)" }
		ListElement { text: "DM Provisioning Key (Kcp)" }
	}
	
	////////////////////////////////////////
	// funcitons 
	////////////////////////////////////////

	function findcurrentmode(model, index, textdata) {
		
		for( var i = 0; i < model.count; ++i){
			if( model.get(i).text == textdata ){
				return i
			}
		}

		return 0
	}	
	
	function pathmanager(fileurl) {
		var urlpath, cleanpath, currpath;
		
		urlpath = fileurl.replace(/^(file:\/{3})|(qrc:\/{2})|(http:\/{2})/,"");
        cleanpath = decodeURIComponent(urlpath);

		if( absolutepath.checkState == Qt.Unchecked )
		{
			urlpath = Qt.resolvedUrl("..").replace(/^(file:\/{3})|(qrc:\/{2})|(http:\/{2})/,"")
			currpath = decodeURIComponent(urlpath);
			
			if( cleanpath.indexOf(currpath) >= 0 ){
				return ("../" + cleanpath.slice(currpath.length))
			}
		}
		return cleanpath
	}
	
	////////////////////////////////////////
	// UI definitions
	////////////////////////////////////////

    width: 520
    height: 420
    maximumHeight: height
    maximumWidth: width
    minimumHeight: height
    minimumWidth: width	

    FileDialog {
        id: fileDialog
        visible: false
        modality: Qt.WindowModal

        title: {
			qsTr("File Dialog")
		}
		
		folder: {
			Qt.resolvedUrl("..")
		}

		selectExisting: true 
        selectMultiple: false
        selectFolder: false
        nameFilters: [ "Binary files (*.bin)", "All files (*)" ]
        selectedNameFilter: "Binary files (*.bin)"
        sidebarVisible: true

        onAccepted: {
			if( fileDialog.title == qsTr("assetfile") ){
				assetfile.text = pathmanager(fileDialog.fileUrl.toString())
			}else{
				pkgfile.text = pathmanager(fileDialog.fileUrl.toString())
			}
        }
        onRejected: {
		}
    }	
	
	TextArea {
		id: title
		width: 440
		height: 40	
		x: 40
		y: 20
		text: qsTr("Secure Asset Config")
		readOnly: true
	}
	
	Column {
		width: 460
		height: 240	
		x: 40
		y: 60

		Column { 
			spacing: 2

			Row {
				Label {
					id: keyltypeabel
					
					width: 140
					height: 40

					text: "Key Type"
				}
				ComboBox {
					id: keytype
					width: 280
					height: 40

					model: keytypelist
					currentIndex: findcurrentmode(keytypelist , 1, assetman.keytype)

					onCurrentIndexChanged:	{
					}
				}				
			}
			Row {
				Label {
					id: assetidlabel
					
					width: 140
					height: 40

					text: "Asset ID"
				}
				TextField {
					id: assetid
					
					width: 280
					height: 40
					//visible : false

					text: assetman.assetid
					
					ToolTip.visible: hovered
					ToolTip.text: qsTr("32 bit Unique Asset ID (HEXA)")
				}	
			}			
			Row {
				Label {
					id: assetlabel
					
					width: 140
					height: 40

					text: "Asset File"
				}
				TextField {
					id: assetfile
					
					width: 280
					height: 40
					//visible : false

					text: assetman.assetfile
					
					onPressAndHold: {
						fileDialog.title = qsTr("assetfile")
						fileDialog.visible = true
						fileDialog.open()
					}
					
					ToolTip.visible: hovered
					ToolTip.text: qsTr("Press and hold left mouse button to open the file dialog.\nFile size must be up to 512 bytes.")						
				}	
			}			
			Row {
				Label {
					id: pkglabel
					
					width: 140
					height: 40

					text: "Secure Asset Pkg"
				}
				TextField {
					id: pkgfile
					
					width: 280
					height: 40
					//visible : false

					text: assetman.pkgfile

					onPressAndHold: {
						fileDialog.title = qsTr("pkgfile")
						fileDialog.visible = true
						fileDialog.open()
					}
					
					ToolTip.visible: hovered
					ToolTip.text: qsTr("Press and hold left mouse button to open the file dialog.")											
				}	
			}
			Row {
				CheckBox {
					id: absolutepath
					height: 40
					text: qsTr("Absolute Path")
					checked: false
					visible : false

					ToolTip.visible: hovered
					ToolTip.text: qsTr("Path is a complete list of directories")						
				}				
			}
		}
	}
	
	
	Column {
		width: 480
		height: 40	
		x: 160	
		y: 340		

		Row {
			spacing: 20	
			
			Button {
				id: generate
				width: 140
				height: 40

				text: "Generate"

				ToolTip.visible: hovered
				ToolTip.text: qsTr("Generate Secure-Asset")		
				
				onClicked: {
					assetman.text = "keytype=" + keytype.currentText + "&"
								  + "assetid=" + assetid.text + "&"
								  + "assetfile=" + assetfile.text + "&"
								  + "pkgfile=" + pkgfile.text
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