import QtQuick 2.10
import QtQuick.Controls 2.1
import QtQuick.Controls.Material  2.1
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.2
import QtQuick.Window 2.2

ApplicationWindow {
    title: imgman.wintitle
    Material.theme: Material.Dark
    Material.accent: Material.Teal
	//Material.primary: Material.BlueGrey
    //Material.foreground: Material.Dark
    //Material.background: Material.Grey	
    visible: (tpmstepcheck(imgman.config) == true) ? false : true

	////////////////////////////////////////
	// 
	////////////////////////////////////////
	
	ListModel {
		id: keyloadlist
		ListElement { text: "LOAD_AND_VERIFY" 	 	 ; color: "lightblue" }
		ListElement { text: "VERIFY_ONLY_IN_FLASH" 	 ; color: "lightgreen" }
		ListElement { text: "VERIFY_ONLY_IN_MEM" 	 ; color: "goldenrod" }
		ListElement { text: "LOAD_ONLY" 	 		 ; color: "seagreen" }
	}

	ListModel {
		id: keyenclist
		ListElement { text: "NO_IMAGE_ENC" 	 ; color: "lightblue" }
		ListElement { text: "ICV_CODE_ENC" 	 ; color: "lightgreen" }
		ListElement { text: "OEM_CODE_ENC" 	 ; color: "seagreen" }
	}

	ListModel {
		id: dbgcertlist
		ListElement { text: "OFF" 	 ; color: "lightblue" }
		ListElement { text: "ON" 	 ; color: "lightgreen" }
		ListElement { text: "RMA" 	 ; color: "seagreen" }
	}
	
	////////////////////////////////////////
	// funcitons 
	////////////////////////////////////////

	function findcurrentmode(model, index, textdata) {
		var str = textdata
		var item = str.split(":")
		
		for( var i = 0; i < model.count; ++i){
			if( model.get(i).text == item[index] ){
				return i
			}
		}

		return 0
	}
	
	function findcheckvalue(textdata) {
		if( textdata == '1' ){
			return true
		}
		if( textdata == 'ON' ){
			return true
		}
		return false
	}	

	function findcurrentaddr(index, textdata) {
		var str = textdata
		var item = str.split(" ")

		if( item.length > index ){
			return item[index]
		}

		return ""
	}
	
	function convertstr(checked){
		if( checked == true ){
			return "true"
		}
		return "false"
	}
	
	function visibleaddr3(checked) {
		if( checked == true ){
			comp0addr3.visible = true
			comp1addr3.visible = true
			comp2addr3.visible = true
			comp3addr3.visible = true
		}else{
			comp0addr3.visible = false
			comp1addr3.visible = false
			comp2addr3.visible = false
			comp3addr3.visible = false
		}
	}
	
	function imgviolationcheck() {
		var err = 0
		var keyowner = 1
		var lcsstate = 0
		var item = imgman.textfield.split(":")
		var msg = ""
		
		for ( var i = 0; i < item.length ; i++ ){
			if( item[i] == "hbk-id 0, CM" ){
					keyowner = 0
			}
			if( item[i] == "lcs SECURE" ){
					lcsstate = 5
					
			}
			if( item[i] == "lcs DM" ){
					lcsstate = 1
			}
		}
		if( (keyowner == 1) && (keyenc.currentText == "ICV_CODE_ENC") ){
			err = 1
			msg += "Unable to access ICV(CM) Keys."
		}else if( (keyowner == 0) && (keyenc.currentText == "OEM_CODE_ENC") ){
			err = 1
			msg += "Unable to access OEM(DM) Keys."
		}
		
		if( (keyenc.currentText != "NO_IMAGE_ENC") && (parseInt(comp1addr0.text) != 0x00000000) && (parseInt(comp1addr1.text) == 0xffffffff) ){
			err = 1
			msg += "Unable to encrypt Cache Boot Image."
		}
		
		if( (lcsstate != 1) && (lcsstate != 5) && (keyenc.currentText == "ICV_CODE_ENC") ){
			err = 1
			msg += "Can not encrypt Image in current Lcs."
		}else if( (lcsstate != 5) && (keyenc.currentText == "OEM_CODE_ENC") ){
			err = 1
			msg += "Can not encrypt Image in current Lcs."
		}
		
		if (err == 0){
			return ""
		}
		
		return ("[WARN] " + msg)
	}
	
	function tpmstepcheck(config) {
		return true
	}		
	
	////////////////////////////////////////
	// UI definitions
	////////////////////////////////////////

    Timer {
        id: timer
        running: tpmstepcheck(imgman.config)
        repeat: false
        interval: 50
        triggeredOnStart: true
		
        onTriggered: {
            update.clicked();
        }
    }
	
    width: 720
    height: 560
    maximumHeight: height
    maximumWidth: width
    minimumHeight: height
    minimumWidth: width	

	Column {
		width: 520
		height: 70	
		x: 10; y: 10
		spacing : 5
		
		TextArea {
			id: secucfg
			
			font.pixelSize: 12
			width: 560
			height: 40
			readOnly: true

			text: imgman.config
		}	
		TextArea {
			id: tpmcfg
			
			font.pixelSize: 12
			width: 560
			height: 40
			readOnly: true

			text: imgman.textfield
		}	
	}

	Column {
		width: 620
		height: 40	
		x: 10; y: 90
		spacing : 5	
		
		Row {
			Label {
				id: titlestr
				
				width: 80
				height: 40

				text: "TITLE"
				horizontalAlignment: Text.AlignLeft
				verticalAlignment: Text.AlignVCenter
			}		
			TextField {
				id: title
				
				width: 480
				height: 40

				text: imgman.title
				
				Keys.onReleased: {
				}
			}
		}
	}
	
	Column {
		width: 620
		height: 40	
		x: 10; y: 130
		spacing : 5		
	
		Row {
			Label {
				id: sfdpstr
				
				width: 120
				height: 40

				text: "SFDP"
				horizontalAlignment: Text.AlignLeft
				verticalAlignment: Text.AlignVCenter
			}		
			ComboBox {
				id: sfdp
				
				width: 240
				height: 40
				
				model: sfdpman
				textRole: "display"
				
				currentIndex : imgman.sfdpindex

				onCurrentIndexChanged: {
				}
				
				ToolTip.visible: hovered
				ToolTip.text: qsTr("SFLASH Model Name")					
			}				
		}
	}
	
	Column {
		width: 620
		height: 40	
		x: 10; y: 170
		spacing : 5		
		
		Row {
			Label {
				id: keyloadstr
				
				width: 120
				height: 40

				text: "Load Scheme"
				horizontalAlignment: Text.AlignLeft
				verticalAlignment: Text.AlignVCenter
			}		
			ComboBox {
				id: keyload
				
				width: 240
				height: 40
				
				model: keyloadlist
				textRole: 'text'
				//background: Rectangle { 
				//	color : keyloadlist.get(keyload.currentIndex).color
				//}
				contentItem: Text {
					text:  keyloadlist.get(keyload.currentIndex).text
					horizontalAlignment: Text.AlignLeft
					verticalAlignment: Text.AlignVCenter
					anchors.left : parent.left
					anchors.leftMargin: 10				
					font.bold: true
					color : keyloadlist.get(keyload.currentIndex).color
				}
				
				currentIndex: findcurrentmode(keyloadlist, 0, imgman.keyload)

				onCurrentIndexChanged: {
				}
				
				ToolTip.visible: hovered
				ToolTip.text: qsTr("verify-scheme (LOAD_AND_VERIFY; VERIFY_ONLY_IN_FLASH; VERIFY_ONLY_IN_MEM; LOAD_ONLY)")						
			}			
		}
	}
	
	Column {
		width: 620
		height: 40	
		x: 10; y: 210
		spacing : 5		
		
		Row {
			Label {
				id: keyencstr
				
				width: 120
				height: 40

				text: "Encrypt Scheme"
				horizontalAlignment: Text.AlignLeft
				verticalAlignment: Text.AlignVCenter
			}		
			ComboBox {
				id: keyenc
				
				width: 240
				height: 40
				
				model: keyenclist
				textRole: 'text'
				//background: Rectangle { 
				//	color : keyenclist.get(keyenc.currentIndex).color
				//}
				contentItem: Text {
					text:  keyenclist.get(keyenc.currentIndex).text
					horizontalAlignment: Text.AlignLeft
					verticalAlignment: Text.AlignVCenter
					anchors.left : parent.left
					anchors.leftMargin: 10				
					font.bold: true
					color : keyenclist.get(keyenc.currentIndex).color
				}
				
				currentIndex: findcurrentmode(keyenclist, 0, imgman.keyenc)

				onCurrentIndexChanged: {
				}

				ToolTip.visible: hovered
				ToolTip.text: qsTr("AES-option (NO_IMAGE_ENC; ICV_CODE_ENC; OEM_CODE_ENC)")						
			}			
		}
	
	}
	
	Column {
		width: 620
		height: 40	
		x: 10; y: 250
		spacing : 5	
				
		Row {
			spacing : 10		
			Label {
				id: flashaddrstr
				
				width: 80
				height: 40

				text: "FLASHADDR"
				horizontalAlignment: Text.AlignLeft
				verticalAlignment: Text.AlignVCenter
			}		
			TextField {
				id: flashaddr
				
				width: 100
				height: 40

				text: imgman.flashaddr
				
				Keys.onReleased: {
				}
			}
		
			CheckBox {
				id: retmem
				height: 40
				text: qsTr("RETMEM")
				checked: findcheckvalue(imgman.retmem)

				ToolTip.visible: hovered
				ToolTip.text: qsTr("Load into RETMEM ( unchecked=unload, checked=load )")						
				
				onCheckedChanged: {
					visibleaddr3(retmem.checked)
				}
			}
			CheckBox {
				id: crcon
				height: 40
				text: qsTr("CRC")
				checked: findcheckvalue(imgman.crcon)

				ToolTip.visible: hovered
				ToolTip.text: qsTr("CRC-option ( ON/OFF )")						
			}
			Label {
				id: dbgcertstr
				
				width: 80
				height: 40

				text: "Debug Cert."
				horizontalAlignment: Text.AlignLeft
				verticalAlignment: Text.AlignVCenter
			}			
			ComboBox {
				id: dbgcert
				
				width: 80
				height: 40
				
				model: dbgcertlist
				textRole: 'text'
				//background: Rectangle { 
				//	color : dbgcertlist.get(dbgcert.currentIndex).color
				//}
				contentItem: Text {
					text:  dbgcertlist.get(dbgcert.currentIndex).text
					horizontalAlignment: Text.AlignLeft
					verticalAlignment: Text.AlignVCenter
					anchors.left : parent.left
					anchors.leftMargin: 10				
					font.bold: true
					color : dbgcertlist.get(dbgcert.currentIndex).color
				}
				
				currentIndex: findcurrentmode(dbgcertlist, 0, imgman.dbgcert)

				onCurrentIndexChanged: {
				}

				ToolTip.visible: hovered
				ToolTip.text: qsTr("DebugCert tagging ( ON;OFF;RMA )")						
			}				
		}
		
		Column {
		width: 620
		height: 30	
		spacing : 5	
					
			Row {
				spacing : 5		
				Label {
					id: comp0str
					
					width: 80
					height: 30

					text: ""
				}		
				Label {
					id: comp0
					
					width: 200
					height: 40

					text: ""
					
					Keys.onReleased: {
					}
				}

				Label {
					id: comp0addr0
					
					width: 100
					height: 30

					text: "load-addr"
					horizontalAlignment: Text.AlignLeft
					verticalAlignment: Text.AlignVCenter
					
					Keys.onReleased: {
					}
				}

				Label {
					id: comp0addr1
					
					width: 100
					height: 30

					text: "store-addr"
					horizontalAlignment: Text.AlignLeft
					verticalAlignment: Text.AlignVCenter
				
					Keys.onReleased: {
					}
				}

				Label {
					id: comp0addr2
					
					width: 100
					height: 30

					text: "max-size"
					horizontalAlignment: Text.AlignLeft
					verticalAlignment: Text.AlignVCenter
					
					Keys.onReleased: {
					}
				}

				Label {
					id: comp0addr3
					
					width: 100
					height: 30

					text: "sram-addr"
					horizontalAlignment: Text.AlignLeft
					verticalAlignment: Text.AlignVCenter
					
					visible: retmem.checked
					
					Keys.onReleased: {
					}
				}
			}
		}
		
		Column {
		width: 620
		height: 40	
		spacing : 5	
		
			Row {
				spacing : 5		
				Label {
					id: comp1str
					
					width: 80
					height: 40

					text: "COMP1"
					horizontalAlignment: Text.AlignLeft
					verticalAlignment: Text.AlignVCenter
				}		
				TextField {
					id: comp1
					
					width: 200
					height: 40

					text: imgman.comp1
					
					Keys.onReleased: {
					}

					ToolTip.visible: hovered
					ToolTip.text: qsTr("1st SW component filename (w/o extention)")						
				}

				TextField {
					id: comp1addr0
					
					width: 100
					height: 40

					text: findcurrentaddr(0, imgman.comp1addr)
					
					Keys.onReleased: {
					}
				}

				TextField {
					id: comp1addr1
					
					width: 100
					height: 40

					text: findcurrentaddr(1, imgman.comp1addr)
					
					Keys.onReleased: {
					}
				}

				TextField {
					id: comp1addr2
					
					width: 100
					height: 40

					text: findcurrentaddr(2, imgman.comp1addr)
					
					Keys.onReleased: {
					}
				}

				TextField {
					id: comp1addr3
					
					width: 100
					height: 40

					text: findcurrentaddr(3, imgman.comp1addr)
					visible: retmem.checked
					
					Keys.onReleased: {
					}
				}
			}
		}
		
		Column {
		width: 620
		height: 40	
		spacing : 5	
		
			Row {
				spacing : 5
				Label {
					id: comp2str
					
					width: 80
					height: 40

					text: "COMP2"
					horizontalAlignment: Text.AlignLeft
					verticalAlignment: Text.AlignVCenter
				}		
				TextField {
					id: comp2
					
					width: 200
					height: 40

					text: imgman.comp2
					
					Keys.onReleased: {
					}

					ToolTip.visible: hovered
					ToolTip.text: qsTr("2nd SW component filename (w/o extention)")						
				}

				TextField {
					id: comp2addr0
					
					width: 100
					height: 40

					text: findcurrentaddr(0, imgman.comp2addr)
					
					Keys.onReleased: {
					}
				}

				TextField {
					id: comp2addr1
					
					width: 100
					height: 40

					text: findcurrentaddr(1, imgman.comp2addr)
					
					Keys.onReleased: {
					}
				}

				TextField {
					id: comp2addr2
					
					width: 100
					height: 40

					text: findcurrentaddr(2, imgman.comp2addr)
					
					Keys.onReleased: {
					}
				}

				TextField {
					id: comp2addr3
					
					width: 100
					height: 40

					text: findcurrentaddr(3, imgman.comp2addr)
					visible: retmem.checked
					
					Keys.onReleased: {
					}
				}
			}
		}
		
		Column {
		width: 620
		height: 40	
		spacing : 5	
		
			Row {
				spacing : 5	
				Label {
					id: comp3str
					
					width: 80
					height: 40

					text: "COMP3"
					horizontalAlignment: Text.AlignLeft
					verticalAlignment: Text.AlignVCenter
				}		
				TextField {
					id: comp3
					
					width: 200
					height: 40

					text: imgman.comp3
					
					Keys.onReleased: {
					}

					ToolTip.visible: hovered
					ToolTip.text: qsTr("3rd SW component filename (w/o extention)")						
				}

				TextField {
					id: comp3addr0
					
					width: 100
					height: 40

					text: findcurrentaddr(0, imgman.comp3addr)
					
					Keys.onReleased: {
					}
				}

				TextField {
					id: comp3addr1
					
					width: 100
					height: 40

					text: findcurrentaddr(1, imgman.comp3addr)
					
					Keys.onReleased: {
					}
				}

				TextField {
					id: comp3addr2
					
					width: 100
					height: 40

					text: findcurrentaddr(2, imgman.comp3addr)
					
					Keys.onReleased: {
					}
				}
				TextField {
					id: comp3addr3
					
					width: 100
					height: 40

					text: findcurrentaddr(3, imgman.comp3addr)
					visible: retmem.checked
					
					Keys.onReleased: {
					}
				}
			}
		}
		
		Column {
		width: 620
		height: 40	
		spacing : 5	
		
			Row {
				spacing : 5	
				Label {
					id: imagestr
					
					width: 80
					height: 40

					text: "IMAGE"
					horizontalAlignment: Text.AlignLeft
					verticalAlignment: Text.AlignVCenter
				}		
				TextField {
					id: image
					
					width: 300
					height: 40

					text: imgman.image
					
					Keys.onReleased: {
					}

					ToolTip.visible: hovered
					ToolTip.text: qsTr("target image filename (w/o extention)")						
				}
				CheckBox {
					id: dateon
					height: 40
					text: qsTr("DateTime")
					checked: findcheckvalue(imgman.dateon)		
				}
				
				Label {
					id: sndboxstr
					
					width: 80
					height: 40

					text: "  SD-SBox"
					horizontalAlignment: Text.AlignLeft
					verticalAlignment: Text.AlignVCenter
				}					
				TextField {
					id: sndbox
					
					width: 100
					height: 40

					text: ( dbgcertlist.get(dbgcert.currentIndex).text == "OFF" ) ? "0x00000000" : imgman.sandbox
					
					Keys.onReleased: {
					}
				}				
			}	
		}
	}
	
	Button {
		id: update
		x: 320; y: 510	
		width: 120
		height: 40

		text: "Update"

		ToolTip.visible: hovered
		ToolTip.text: qsTr("Update Image Config")		
		
		onClicked: {
			var msg 
			
			msg = imgviolationcheck()
			if( msg == "" ){
				imgman.text = 
					  "TITLE=" + title.text + "&"
					+ "SFDP=" + sfdp.currentText  + "&"
					+ "KEYLOAD=" + keyload.currentText + "&"
					+ "KEYENC=" + keyenc.currentText + "&"
					+ "FLASHADDR=" + flashaddr.text + "&"
					+ "RETMEM=" + convertstr( retmem.checked ) + "&"
					+ "CRCON=" + convertstr( crcon.checked ) + "&"
					+ "DBGCERT=" + dbgcert.currentText + "&"
					+ "COMP1=" + comp1.text + "&"
					+ "COMP2=" + comp2.text + "&"
					+ "COMP3=" + comp3.text + "&"
					+ "IMAGE=" + image.text + "&"
					+ "COMP1ADDR=" + comp1addr0.text + " " + comp1addr1.text + " " + comp1addr2.text + " " + comp1addr3.text + "&"
					+ "COMP2ADDR=" + comp2addr0.text + " " + comp2addr1.text + " " + comp2addr2.text + " " + comp2addr3.text + "&"
					+ "COMP3ADDR=" + comp3addr0.text + " " + comp3addr1.text + " " + comp3addr2.text + " " + comp3addr3.text + "&"
					+ "TAGDATE=" + convertstr( dateon.checked )+ "&"
					+ "SBOX="  + sndbox.text
				;
				Qt.quit()
			}else{
				tpmcfg.text = msg
			}
		}			
	}	

	Button {
		id: stop
		x: 460; y: 510	
		width: 120
		height: 40

		text: "Pass"
		
		onClicked: {
			Qt.quit()
		}			
	}	
	
}