import QtQuick 2.10
import QtQuick.Controls 2.1
import QtQuick.Controls.Material  2.1
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.2
import QtQuick.Window 2.2

ApplicationWindow {
    title: cfgman.wintitle
    Material.theme: Material.Dark
    Material.accent: Material.Teal
	//Material.primary: Material.BlueGrey
    //Material.foreground: Material.Dark
    //Material.background: Material.Grey
    visible: (tpmstepcheck(cfgman.config) == true) ? false : true

	// 
	// 
	////////////////////////////////////////
	
	ListModel {
		id: lcslist
		ListElement { text: "lcs CM" 	 ; color: "lightblue" }
		ListElement { text: "lcs DM" 	 ; color: "lightgreen" }
		ListElement { text: "lcs SECURE" ; color: "goldenrod" }
		ListElement { text: "lcs RMA" 	 ; color: "seagreen" }
	}
	ListModel {
		id: hbkidlist
		ListElement { text: "hbk-id 0, CM" 	 ; color: "lightblue" }
		ListElement { text: "hbk-id 1, DM" 	 ; color: "lightgreen" }
	}
	ListModel {
		id: sdebuglist
		ListElement { text: "2 Level SD" 	 ; color: "lightblue" }
		ListElement { text: "3 Level SD" 	 ; color: "lightgreen" }
	}
	ListModel {
		id: sbootlist
		ListElement { text: "Bypass  SB"  	 ; color: "seagreen" }
		ListElement { text: "2 Level SB" 	 ; color: "lightblue" }
		ListElement { text: "3 Level SB" 	 ; color: "lightgreen" }
	}

	ListModel {
		id: imgmodelist
		ListElement { text: "Split Image" 	 ; color: "lightblue" }
		ListElement { text: "Combined Image" 	 ; color: "lightgreen" }
	}
	ListModel {
		id: bootmodelist
		ListElement { text: "Cache Boot" 	 ; color: "lightblue" }
		ListElement { text: "SRAM Boot" 	 ; color: "lightgreen" }
	}

	////////////////////////////////////////
	// funcitons 
	////////////////////////////////////////

	function updatetpmcfg () {
		tpmcfg.text = lcslist.get(lcs.currentIndex).text
				+ ":" + hbkidlist.get(hbkid.currentIndex).text
				+ ":" + String(nvcount.value)
				+ ":" + sdebuglist.get(sdebug.currentIndex).text
				+ ":" + sbootlist.get(sboot.currentIndex).text
				+ ":" + imgmodelist.get(imgmode.currentIndex).text
				+ ":" + bootmodelist.get(bootmode.currentIndex).text	
		;
	}
	
	function findcurrentlcs(model, index, textdata, newlcs) {
		var str = textdata
		var item = str.split(":")

		if( newlcs != "" ){
			for( var i = 0; i < model.count; ++i){
				if( model.get(i).text == newlcs ){
					return i
				}
			}		
		}
		
		for( var i = 0; i < model.count; ++i){
			if( model.get(i).text == item[index] ){
				return i
			}
		}

		return 0
	}
	
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
	
	function findcurrentvalue(index, textdata) {
		var str = textdata
		var item = str.split(":")
		
		if( item.length > index ){
			return Number(item[index])
		}
		return 0
	}	
	
	function tpmviolationcheck() {
		var nvc 
		nvc = nvcount.value
		
		if( hbkid.currentText == "hbk-id 1, DM" ){
			if( nvc > 96 ){
				return "[WARN] DM NVCounter must be a value between 0 and 96."
			}
		}else{
			if( nvc > 64 ){
				return "[WARN] CM NVCounter must be a value between 0 and 64."
			}
		}
		
		if( sboot.currentText == "Bypass  SB" ){
			if( imgmode.currentText == "Combined Image" ){
				return "[WARN] Bypass SD can not support a RaLIB & PTIM combined Image."
			}
		}
		
		return ""
	}
	
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
		
		if( item[0] == "Red    Boot" && item[1] == "DM") {
			if( imgmode == mode  && hbkid.currentText == "hbk-id 1, DM" ){
				for( var i = 0; i < imgmodelist.count; i++ ){
					if( imgmodelist.get(i).text == "Split Image" ){
						index = i
						break
					}
				}
			}
			if( sboot == mode  && hbkid.currentText == "hbk-id 1, DM" ){
				for( var i = 0; i < sbootlist.count; i++ ){
					if( sbootlist.get(i).text == "Bypass  SB" ){
						index = i
						break
					}
				}
			}
		}
	
		return index
	}
	
	function tpmrulenvcount(config) {
		var str = config
		var item = str.split(":")

		if( item[0] == "Red    Boot" && item[1] == "DM" && hbkid.currentText == "hbk-id 1, DM") {
			return 0
		}
	
		if( hbkid.currentText == "hbk-id 0, CM" ){
			return 64
		}
		return 96
	}	
	
	function tpmstepcheck(config) {
		var str = config
		var item = str.split(":")

		//if( (item[2] == "G.B1") || (item[2] == "G.B5") || (item[1] == "CM") ) {
			return true
		//}
		//return false
	}		
	
	////////////////////////////////////////
	// UI definitions
	////////////////////////////////////////

    Timer {
        id: timer
        running: tpmstepcheck(cfgman.config)
        repeat: false
        interval: 50
        triggeredOnStart: false
		
        onTriggered: {
            update.clicked();
        }
    }
	
    width: 480
    height: 240
    maximumHeight: height
    maximumWidth: width
    minimumHeight: height
    minimumWidth: width	

	Column { 
		width: 520
		height: 70	
		x: 40
		y: 20
		spacing: 2

		TextArea {
			id: secucfg
			font.pixelSize: 12
			
			width: 520
			height: 40
			visible : false
			readOnly: true

			text: cfgman.config
		}	
		TextArea {
			id: tpmcfg
			font.pixelSize: 12
			
			width: 520
			height: 40
			visible : false
			readOnly: true

			text: cfgman.textfield
			
		}	
	}
	
	Column {
		width: 520
		height: 40	
		x: 150	
		y: 80
		
		Row {
			spacing: 10
			
			ComboBox {
				id: lcs
				
				width: 150 
				height: 40
				visible: false

				model: lcslist
				textRole: 'text'
				//background: Rectangle { 
				//	color : lcslist.get(lcs.currentIndex).color
				//}
				contentItem: Text {
					text:  lcslist.get(lcs.currentIndex).text
					horizontalAlignment: Text.AlignLeft
					verticalAlignment: Text.AlignVCenter
					anchors.left : parent.left
					anchors.leftMargin: 10				
					font.bold: true
					color : lcslist.get(lcs.currentIndex).color
				}
				
				currentIndex: findcurrentlcs(lcslist, 0, cfgman.textfield, cfgman.newlcs)
				
				onCurrentIndexChanged: {
					updatetpmcfg()
				}
			}			

			ComboBox {
				id: hbkid
				
				width: 150
				height: 40
				visible: false
				
				model: hbkidlist
				textRole: 'text'
				//background: Rectangle { 
				//	color : hbkidlist.get(hbkid.currentIndex).color
				//}
				contentItem: Text {
					text:  hbkidlist.get(hbkid.currentIndex).text
					horizontalAlignment: Text.AlignLeft
					verticalAlignment: Text.AlignVCenter
					anchors.left : parent.left
					anchors.leftMargin: 10				
					font.bold: true
					color : hbkidlist.get(hbkid.currentIndex).color
				}
				
				currentIndex: tpmrulecheck(cfgman.config, hbkid, findcurrentmode(hbkidlist, 1, cfgman.textfield))

				onCurrentIndexChanged: {
					hbkid.currentIndex = tpmrulecheck(cfgman.config, hbkid, hbkid.currentIndex)
					updatetpmcfg()
				}
			}	

			TextArea {
				id: nvtext
				
				width: 70
				height: 40

				text: "NVCount"
			}
			
			SpinBox  {
				id: nvcount
				
				width: 110
				height: 40
				stepSize: 1
				
				from: 0
				to: tpmrulenvcount(cfgman.config)

				value: findcurrentvalue(2, cfgman.textfield)
				
				Keys.onReleased: {
					updatetpmcfg()
				}
			}
			
		}
	}
	
	Column {
		width: 520
		height: 40	
		x: 40	
		y: 140
		
		Row {
			spacing: 10
		
			ComboBox {
				id: sdebug
				
				width: 150
				height: 40
				visible: false

				model: sdebuglist
				textRole: 'text'
				//background: Rectangle { 
				//	color : sdebuglist.get(sdebug.currentIndex).color
				//}
				contentItem: Text {
					text:  sdebuglist.get(sdebug.currentIndex).text
					horizontalAlignment: Text.AlignLeft
					verticalAlignment: Text.AlignVCenter
					anchors.left : parent.left
					anchors.leftMargin: 10				
					font.bold: true
					color : sdebuglist.get(sdebug.currentIndex).color
				}
				
				currentIndex: findcurrentmode(sdebuglist, 3, cfgman.textfield)

				onCurrentIndexChanged: {
					updatetpmcfg()
				}
			}	

			ComboBox {
				id: sboot
				
				width: 150
				height: 40
				visible: false
				
				model: sbootlist
				textRole: 'text'
				//background: Rectangle { 
				//	color : sbootlist.get(sboot.currentIndex).color
				//}
				contentItem: Text {
					text:  sbootlist.get(sboot.currentIndex).text
					horizontalAlignment: Text.AlignLeft
					verticalAlignment: Text.AlignVCenter
					anchors.left : parent.left
					anchors.leftMargin: 10				
					font.bold: true
					color : sbootlist.get(sboot.currentIndex).color
				}
				
				currentIndex: tpmrulecheck(cfgman.config, sboot, findcurrentmode(sbootlist, 4, cfgman.textfield))

				onCurrentIndexChanged: {
					sboot.currentIndex = tpmrulecheck(cfgman.config, sboot, sboot.currentIndex)
					updatetpmcfg()
				}
			}			
		}
	}

	Column {
		width: 520
		height: 40	
		x: 40	
		y: 180
		
		Row {
			spacing: 10

			TextArea {
				id: imgtext
				
				width: 120
				height: 40
				visible: false

				text: "RamLIB & PTIM"
			}
			
			ComboBox {
				id: imgmode
				
				width: 200
				height: 40
				visible: false
				
				model: imgmodelist
				textRole: 'text'
				//background: Rectangle { 
				//	color : imgmodelist.get(imgmode.currentIndex).color
				//}
				contentItem: Text {
					text:  imgmodelist.get(imgmode.currentIndex).text
					horizontalAlignment: Text.AlignLeft
					verticalAlignment: Text.AlignVCenter
					anchors.left : parent.left
					anchors.leftMargin: 10				
					font.bold: true
					color : imgmodelist.get(imgmode.currentIndex).color
				}
				
				currentIndex: tpmrulecheck(cfgman.config, imgmode, findcurrentmode(imgmodelist, 5, cfgman.textfield))

				onCurrentIndexChanged: {
					imgmode.currentIndex = tpmrulecheck(cfgman.config, imgmode, imgmode.currentIndex)
					updatetpmcfg()
				}
			}
		}
	}
		
	Column {
		width: 520
		height: 40	
		x: 40	
		y: 220		

		Row {
			spacing: 10
		
			TextArea {
				id: boottext
				
				width: 120
				height: 40
				visible: false

				text: "Boot Model"
			}
			
			ComboBox {
				id: bootmode
				
				width: 200
				height: 40
				visible: false
				
				model: bootmodelist
				textRole: 'text'
				//background: Rectangle { 
				//	color : bootmodelist.get(bootmode.currentIndex).color
				//}
				contentItem: Text {
					text:  bootmodelist.get(bootmode.currentIndex).text
					horizontalAlignment: Text.AlignLeft
					verticalAlignment: Text.AlignVCenter
					anchors.left : parent.left
					anchors.leftMargin: 10				
					font.bold: true
					color : bootmodelist.get(bootmode.currentIndex).color
				}
				
				currentIndex: findcurrentmode(bootmodelist, 6, cfgman.textfield)

				onCurrentIndexChanged: {
					updatetpmcfg()
				}
			}
		}
	}
	
	Column {
		width: 520
		height: 40	
		x: 180	
		y: 170		

		Row {
			spacing: 20	
			
			Button {
				id: update
				width: 120
				height: 40

				text: "Update"

				ToolTip.visible: hovered
				ToolTip.text: qsTr("Update Config")		
				
				onClicked: {
					var msg
					
					msg = tpmviolationcheck()
					if( msg == ""  ){
						updatetpmcfg();
						cfgman.text = 
							  "lcs=" + lcs.currentText + "&"
							+ "hbk-id=" + hbkid.currentText + "&"
							+ "nvcount=" + String(nvcount.value) + "&"
							+ "sdebug=" + sdebug.currentText + "&"
							+ "sboot=" + sboot.currentText + "&"
							+ "combined=" + imgmode.currentText + "&"
							+ "bootmodel=" + bootmode.currentText
						;
						Qt.quit()
					}else{
						tpmcfg.text = msg
					}
				}			
			}	

			Button {
				id: stop
				width: 120
				height: 40
				visible: false

				text: "Pass"
				
				onClicked: {
					var msg
					
					msg = tpmviolationcheck()
					if( msg == ""  ){
						cfgman.text = 
							  "lcs=" + lcs.currentText + "&"
							+ "hbk-id=" + hbkid.currentText + "&"
							+ "nvcount=" + String(nvcount.value) + "&"
							+ "sdebug=" + sdebug.currentText + "&"
							+ "sboot=" + sboot.currentText + "&"
							+ "combined=" + imgmode.currentText + "&"
							+ "bootmodel=" + bootmode.currentText
						;
						Qt.quit()
					}else{
						tpmcfg.text = msg
					}
				}				
			}	
		}
	}
}