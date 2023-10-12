import QtQuick 2.10
import QtQuick.Controls 2.1
import QtQuick.Controls.Material  2.1
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.2
import QtQuick.Window 2.2
//import QtGraphicalEffects 1.0

ApplicationWindow {
    id: applicationWindow
    Material.theme: Material.Dark
    Material.accent: Material.Teal
	//Material.primary: Material.BlueGrey
    //Material.foreground: Material.Dark
    //Material.background: Material.Grey
    title: keyman.wintitle
    visible: tpmstepcheck( keyman.text ) == true ? false : true

	
	////////////////////////////////////////
	// global variables
	////////////////////////////////////////

	ListModel {
		id: sdebuglist
		ListElement { text: "2 Level SD" 	 ; color: "lightblue" ; index: 2}
		ListElement { text: "3 Level SD" 	 ; color: "lightgreen" ; index: 3}
	}
	ListModel {
		id: sbootlist
		ListElement { text: "Bypass  SB"  	 ; color: "seagreen" ; index: 0}
		ListElement { text: "2 Level SB" 	 ; color: "lightblue" ; index: 2}
		ListElement { text: "3 Level SB" 	 ; color: "lightgreen" ; index: 3}
	}

	property var flag_roletest : false
	
	////////////////////////////////////////
	// funcitons 
	////////////////////////////////////////


	function findcurrentmode( textdata, targetrole) {
		var str = textdata
		var item = str.split(":")
		var role = item[2].split("|")
		
		if( role[0] != "ALL" ){
			for ( var i = 0; i < role.length ; i++ ){
				if( role[i] == targetrole ){
					return "\u2713" // Unicode Character 'CHECK MARK'
				}
			}

			return ""
		}

		return "\u2713" // Unicode Character 'CHECK MARK'
	}
	
	function findcurrentitem( listdata, index) {
		
		for( var i = 0; i < listdata.count; ++i){
			if( listdata.get(i).index == index ){
				return listdata.get(i).text
			}
		}	

		return listdata.get(0).text
	}	

	function check_gensbkey_authority(roletext, flag) {
		if( issuer.text == "" && publisher.text == ""  && flag == 3){
			return false
		}
		if( publisher.text == ""  && flag == 2){
			return false
		}
		if( flag == 0 ){
			return false
		}
		return true
	}

	function check_gensbchain_authority(roletext, flag) {
		if( top.text == "" && issuer.text == ""  && flag == 3){
			return false
		}
		if( top.text == ""  && flag == 2){
			return false
		}
		if( flag == 0 ){
			return false
		}
		return true
	}

	function check_gensdkey_authority(roletext, flag) {
		if( enabler.text == "" && developer.text == ""  && flag == 3){
			return false
		}
		if( developer.text == ""  && flag == 2){
			return false
		}
		return true
	}

	function check_gensdchain_authority(roletext, flag) {
		if( top.text == "" && enabler.text == ""  && flag == 3){
			return false
		}
		if( top.text == ""  && flag == 2){
			return false
		}
		return true
	}

	function check_genenabler_authority(roletext, flag) {
		if( (top.text != ""   && flag == 2 ) || (enabler.text != "") ){
			return true
		}
		return false
	}
	
	function conv_authority( ) {
		var authstr = ""
		
		if( top.text != "" ){
			authstr = "TOP"
		}
		if( issuer.text != "" ){
			authstr = ((authstr == "") ?  "" : (authstr + "|" ) ) + "Issuer"
		}
		if( publisher.text != "" ){
			authstr = ((authstr == "") ? ""  : (authstr + "|" ) ) + "Publisher"
		}
		if( enabler.text != "" ){
			authstr = ((authstr == "") ? ""  : (authstr + "|" ) ) + "Enabler"
		}
		if( developer.text != "" ){
			authstr = ((authstr == "") ? ""  : (authstr + "|" ) ) + "Developer"
		}

		return authstr
	}
	
	function conv_actrole() {
		var authstr = ""
		
		if( check_gensbkey_authority(keyman.text, keyman.sboot)  == true ){
			authstr = "SB1"
		}
		if( check_gensbchain_authority(keyman.text, keyman.sboot) == true ){
			authstr = ((authstr == "") ? ""  : (authstr + "|" ) ) + "SB2"
		}
		if( check_gensdkey_authority(keyman.text, keyman.sdebug)  == true ){
			authstr = ((authstr == "") ? ""  : (authstr + "|" ) ) + "SD1"
		}
		if( check_gensdchain_authority(keyman.text, keyman.sdebug) == true ){
			authstr = ((authstr == "") ? ""  : (authstr + "|" ) ) + "SD2"
		}
		if( check_genenabler_authority(keyman.text, keyman.sdebug)  == true ){
			authstr = ((authstr == "") ? ""  : (authstr + "|" ) ) + "SD3"
		}

		return authstr
	}
	
	function tpmstepcheck( textdata) {
		var str = textdata
		var item = str.split(":")
		var role = item[2].split("|")
		
		if( role[0] != "ALL" ){
			return false
		}

		return true
	}	
	
	////////////////////////////////////////
	// UI definitions
	////////////////////////////////////////
    
	Timer {
        id: timer
        running: tpmstepcheck( keyman.text )
        repeat: false
        interval: 50
        triggeredOnStart: false
		
        onTriggered: {
            batch.clicked();
        }
    }
	
    width: 640
    height: 360
    maximumHeight: height
    maximumWidth: width
    minimumHeight: height
    minimumWidth: width	

	Column {
		x: 290; y: 20
		spacing : 5

		TextArea {
			id: sboot
			
			font.pixelSize: 16
			width: 320
			height: 40
			readOnly: true

			text:  qsTr("Secure Boot Scheme : ") + findcurrentitem( sbootlist, keyman.sboot)
		}

		Button {
			id: gensbkey
			width: 320
			height: 40

			enabled : check_gensbkey_authority(keyman.text, keyman.sboot)
			font.capitalization: Font.MixedCase
			text: "1. Generate SBoot Key and Password"
			
			contentItem: Text {
				text: gensbkey.text
				font: gensbkey.font
				horizontalAlignment: Text.AlignLeft
				verticalAlignment: Text.AlignVCenter
				color: (check_gensbkey_authority(keyman.text, keyman.sboot) == true) ?  "white" : "black"
			}
			
            onClicked: {
				keyman.runclick = "action=SB1" + "&" + "role=" + conv_authority()
				Qt.quit()
			}			
		}		
		Button {
			id: gensbchain
			width: 320
			height: 40

			enabled : check_gensbchain_authority(keyman.text, keyman.sboot)
			font.capitalization: Font.MixedCase
			text: "2. Generate SBoot Key Certificate Chain"
			
			contentItem: Text {
				text: gensbchain.text
				font: gensbchain.font
				horizontalAlignment: Text.AlignLeft
				verticalAlignment: Text.AlignVCenter
				color: (check_gensbchain_authority(keyman.text, keyman.sboot) == true) ?  "white" : "black"
			}
			
            onClicked: {
				keyman.runclick = "action=SB2" + "&" + "role=" + conv_authority()
				Qt.quit()
			}			
		}		

		TextArea {
			id: sdebug
			
			font.pixelSize: 16
			width: 320
			height: 40
			readOnly: true

			text: qsTr("Secure Debug Scheme : ") + findcurrentitem( sdebuglist, keyman.sdebug)
		}

		Button {
			id: gensdkey
			width: 320
			height: 40

			enabled : check_gensdkey_authority(keyman.text, keyman.sdebug)
			font.capitalization: Font.MixedCase
			text: "1. Generate SDebug Key and Password"

			contentItem: Text {
				text: gensdkey.text
				font: gensdkey.font
				horizontalAlignment: Text.AlignLeft
				verticalAlignment: Text.AlignVCenter
				color: (check_gensdkey_authority(keyman.text, keyman.sdebug) == true) ?  "white" : "black"
			}
			
            onClicked: {
				keyman.runclick = "action=SD1" + "&" + "role=" + conv_authority()
				Qt.quit()
			}			
		}		
		Button {
			id: gensdchain
			width: 320
			height: 40

			enabled : check_gensdchain_authority(keyman.text, keyman.sdebug)
			font.capitalization: Font.MixedCase
			text: "2. Generate SDebug Key Certificate Chain"		

			contentItem: Text {
				text: gensdchain.text
				font: gensdchain.font
				horizontalAlignment: Text.AlignLeft
				verticalAlignment: Text.AlignVCenter
				color: (check_gensdchain_authority(keyman.text, keyman.sdebug) == true) ?  "white" : "black"
			}
			
            onClicked: {
				keyman.runclick = "action=SD2" + "&" + "role=" + conv_authority()
				Qt.quit()
			}			
		}		
		Button {
			id: genenabler
			width: 320
			height: 40

			enabled : check_genenabler_authority(keyman.text, keyman.sdebug)
			font.capitalization: Font.MixedCase
			text: "3. Generate Enabler Key Certificate"
			
			contentItem: Text {
				text: genenabler.text
				font: genenabler.font
				horizontalAlignment: Text.AlignLeft
				verticalAlignment: Text.AlignVCenter
				color: (check_genenabler_authority(keyman.text, keyman.sdebug) == true) ?  "white" : "black"
			}
			
            onClicked: {
				keyman.runclick = "action=SD3" + "&" + "role=" + conv_authority()
				Qt.quit()
			}			
		}		

	}
	
	Column {
		x: 10; y: 10
		spacing : 5
		
		TextArea {
			id: role
			
			font.pixelSize: 12
			width: 260
			height: 40
			readOnly: true

			text: keyman.text
		}	

		Row {
			RoundButton {
				id: top
				text: findcurrentmode( keyman.text, "TOP")
				onClicked: {
					if( flag_roletest == true ){
						top.text = (top.text == "\u2713") ? "" : "\u2713"
					}
					if( keyman.sboot != 3 ){
						issuer.text = top.text
					}
					if( keyman.sdebug == 2 ){
						enabler.text = top.text
					}
				}
			}
			TextArea {
				text : qsTr("TOP Manager")
				width: 200
				readOnly: true
			}
		}
		Row {
			RoundButton {
				id: issuer
				text: (keyman.sboot == 2) ?  findcurrentmode( keyman.text, "TOP") : findcurrentmode( keyman.text, "Issuer")
				onClicked: {
					if( keyman.sboot == 3 ){
						if( flag_roletest == true ){
							issuer.text = (issuer.text == "\u2713") ? "" : "\u2713"
						}
					}
				}
			}
			TextArea {
				text : qsTr("SB 2nd Manager, Issuer")
				width: 200
				readOnly: true
			}
		}
		Row {
			RoundButton {
				id: enabler
				text: (keyman.sdebug == 2) ?  findcurrentmode( keyman.text, "TOP") :  findcurrentmode( keyman.text, "Enabler")
				onClicked: {
					if( keyman.sdebug == 3 ){
						if( flag_roletest == true ){
							enabler.text = (enabler.text == "\u2713") ? "" : "\u2713"
						}
					}
				}
			}
			TextArea {
				text : qsTr("SD 2nd Manager, Enabler")
				width: 200
				readOnly: true
			}
		}
		Row {
			RoundButton {
				id: publisher
				text: findcurrentmode( keyman.text, "Publisher")
				onClicked: {
					if( flag_roletest == true ){
						publisher.text = (publisher.text == "\u2713") ? "" : "\u2713"
					}
				}
			}
			TextArea {
				text : qsTr("SB Publisher")
				width: 200
				readOnly: true
			}
		}
		Row {
			RoundButton {
				id: developer
				text: findcurrentmode( keyman.text, "Developer")
				onClicked: {
					if( flag_roletest == true ){
						developer.text = (developer.text == "\u2713") ? "" : "\u2713"
					}
				}
			}
			TextArea {
				text : qsTr("SD Developer")
				width: 200
				readOnly: true
			}
		}
		
		Row {
			Button {
				id: quit
				width: 120
				height: 40

				text: "Quit"
				
				onClicked: {
					Qt.quit()
				}			
			}

			Button {
				id: batch
				width: 140
				height: 40

				text: "Batch"
				
				onClicked: {
					keyman.runclick = "action=" + conv_actrole() + "&" + "role=" + conv_authority()
					Qt.quit()
				}			
			}
		}
	}
}