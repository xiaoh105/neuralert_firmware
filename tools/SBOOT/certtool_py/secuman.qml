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
    title: secuman.wintitle
    visible: true

	
	////////////////////////////////////////
	// global variables
	////////////////////////////////////////

	property var secustring : [ "", "", "" ]
	
	property var imglist : ["pics/YellowBootCM.png", "pics/GreenBootCM.png", "pics/RedBootCM.png", "pics/YellowBootDM.png", "pics/GreenBootDM.png", "pics/RedBootDM.png"]

	////////////////////////////////////////
	//  FSM lists for Secure Manager
	////////////////////////////////////////

	ListModel {
		id: rulelist
		ListElement { viewtext: "                "; color: "lemonchiffon" ; text: "              "}
		ListElement { viewtext: "Green  Boot"; color: "palegreen"    ; text: "Secure Boot"}
		//ListElement { viewtext: "Red    Boot"; color: "lightpink"    ; text: "Insecure Boot"}
	}

	ListModel {
		id: ownlist
		ListElement { viewtext: "CM" ; color: "lightskyblue"    ; text: "Master Key - CM"}
		ListElement { viewtext: "DM" ; color: "lawngreen"       ; text: "Master Key - DM"}
	}

	ListModel {
		id: rolelist
		ListElement { viewtext: "ALL"           ; color: "yellow"    		; text: "Single"}
		//ListElement { viewtext: "TOP"          ; color: "lawngreen"      ; text: "Top Manager"}
		//ListElement { viewtext: "Issuer"       ; color: "lightskyblue"    ; text: "SB Issuer"}
		ListElement { viewtext: "Publisher"   ; color: "orange"       	; text: "SB Publisher"}
		//ListElement { viewtext: "Enabler"     ; color: "lightskyblue"    ; text: "SD Enabler"}
		//ListElement { viewtext: "Developer" ; color: "orange"       	; text: "SD Developer"}
		//ListElement { viewtext: "TOP|Issuer|Enabler"  ; color: "lawngreen"    ; text: "Administrator (TOP, Issuer, Enabler)"}
		//ListElement { viewtext: "Issuer|Enabler"         ; color: "lightskyblue"  ; text: "SB/SD Manager (Issuer, Enabler)"}
		ListElement { viewtext: "Publisher|Developer" ; color: "orange"       	; text: "SB/SD Publisher (Publisher, Developer)"}
		//ListElement { viewtext: "Issuer|Publisher"       ; color: "lightpink"       	; text: "SB Manager (Issuer, Publisher)"}
		//ListElement { viewtext: "Enabler|Developer"   ; color: "lightpink"       	; text: "SD Manager (Enabler, Developer)"}
		
	}
	
	ListModel {
		id: steplistsc
		ListElement { viewtext: "G.1"  ; color: "palegreen" ; text: "Setup CM Scenario"										; viewx: 398 ; viewy:  55 }
		ListElement { viewtext: "G.2"  ; color: "palegreen" ; text: "Generate CMPU"											; viewx: 398 ; viewy: 125 }
		ListElement { viewtext: "G.3"  ; color: "palegreen" ; text: "Generate CM-RMA @ Lcs-CM"						; viewx: 908 ; viewy: 125 }
		ListElement { viewtext: "G.16" ; color: "palegreen" ; text: "Generate CM-RMA @ Lcs-Secure"				; viewx: 908 ; viewy: 400 }
		ListElement { viewtext: "G.17" ; color: "palegreen" ; text: "Generate CM-RMA @ Lcs-DM"						; viewx: 908 ; viewy: 250 }
	}

	ListModel {
		id: steplistsd
		ListElement { viewtext: "G.4"  ; color: "palegreen" ; text: "Generate DM-RMA @ Lcs-DM"						; viewx: 810 ; viewy: 176 }
		ListElement { viewtext: "G.5"  ; color: "palegreen" ; text: "Setup DM Scenario"										; viewx: 532 ; viewy: 196 }
		ListElement { viewtext: "G.6"  ; color: "palegreen" ; text: "Build DM-Boot @ Lcs-DM"							; viewx: 532 ; viewy: 250 }
		ListElement { viewtext: "G.7"  ; color: "palegreen" ; text: "Generate DM-RMA @ Lcs-DM"						; viewx: 810 ; viewy: 242 }
		ListElement { viewtext: "G.8"  ; color: "palegreen" ; text: "Generate DMPU"											; viewx: 532 ; viewy: 340 }
		ListElement { viewtext: "G.9"  ; color: "palegreen" ; text: "Set OTP SLock"											; viewx: 532 ; viewy: 395 }
		ListElement { viewtext: "G.10"; color: "palegreen" ; text: "Update DM Scenario"   								; viewx: 682 ; viewy: 285 }
		ListElement { viewtext: "G.11"; color: "palegreen" ; text: "Build Image @ Lcs-Secure"							; viewx: 682 ; viewy: 332 }
		ListElement { viewtext: "G.12"; color: "palegreen" ; text: "Build Image w/ Dbg-Cert"								; viewx: 682 ; viewy: 378 }
		ListElement { viewtext: "G.13"; color: "palegreen" ; text: "Update NVCount "										; viewx: 682 ; viewy: 428 }		
		ListElement { viewtext: "G.14"; color: "palegreen" ; text: "Re-build DM Key Chain"								; viewx: 682 ; viewy: 464 }	
		ListElement { viewtext: "G.15"; color: "palegreen" ; text: "Generate DM-RMA @ Lcs-Secure"					; viewx: 810 ; viewy: 402 }
	}


	
	////////////////////////////////////////
	// funcitons 
	////////////////////////////////////////


	function findcurrentmode(model, index, textdata) {
		var str = textdata
		var item = str.split(":")
		
		for( var i = 0; i < model.count; ++i){
			if( model.get(i).viewtext == item[index] ){
				return i
			}
		}

		return 0
	}

	function makerelativepath(curpath, targetpath){
		var splitcpath , splittpath, i, j;
		var target ;

		splitcpath = curpath.split('\/')
		splittpath = targetpath.split('\/')		
		
		if(  splitcpath[0] != splittpath[0] ){
			return targetpath
		}
		
		target = ""

		for(i = 0; splitcpath[i] != undefined &&  splitcpath[i] != "" && splittpath[i] != undefined && splittpath[i] != "" ;  i++ ){
			if( splitcpath[i] != splittpath[i] ){
				break
			}
		}
		
		for( j = i; splitcpath[j] != undefined &&  splitcpath[j] != "" ; j++ ){
				target = target + "..\/"
		}		

		for( j = i; splittpath[j] != undefined && splittpath[j] != "" ; j++ ){
				target = target + splittpath[j] + "\/"
		}		

		return target.slice(0, -1)
	}

	function getcurrimagefolder() {
		var urlpath, cleanpath;
		
		if( folderDialog.fileUrl == Qt.resolvedUrl("") ){
			urlpath = folderDialog.folder.toString().replace(/^(file:\/{3})|(qrc:\/{2})|(http:\/{2})/,"");
			cleanpath = decodeURIComponent(urlpath);
			
			cleanpath = makerelativepath( 
				Qt.resolvedUrl(".").toString().replace(/^(file:\/{3})|(qrc:\/{2})|(http:\/{2})/,"") 
				, cleanpath )
		}else{
			urlpath = folderDialog.fileUrl.toString().replace(/^(file:\/{3})|(qrc:\/{2})|(http:\/{2})/,"");
			cleanpath = decodeURIComponent(urlpath);		
			
			cleanpath = makerelativepath( 
				Qt.resolvedUrl(".").toString().replace(/^(file:\/{3})|(qrc:\/{2})|(http:\/{2})/,"") 
				, cleanpath )
		}
		
		return cleanpath
	}
	
	function setcurrimagefolder(strcurfolder) {
		if( strcurfolder == "" ){
			return Qt.resolvedUrl( "../image" )
		}else if( strcurfolder.slice(0,2) == ".." ){
			return Qt.resolvedUrl( secuman.imagefolder)
		}
		return Qt.resolvedUrl( "FILE:\/\/\/" + secuman.imagefolder )
	}
	
	function printnotice (nowstr, cfgtxt) {
		var cleanpath;
			
		cleanpath = getcurrimagefolder();

		return (cfgtxt + "\n\nRaw Binary folder:\n" + cleanpath + "\n\n" + nowstr)
	}
	
	////////////////////////////////////////
	// UI definitions
	////////////////////////////////////////

    width: 360
    height: 540
    maximumHeight: height
    maximumWidth: width
    minimumHeight: height
    minimumWidth: width	

    FileDialog {
        id: folderDialog
        visible: false
        modality: Qt.WindowModal

        title: {
			qsTr("Folder Dialog")
		}
		
		folder: setcurrimagefolder(secuman.imagefolder)

		selectExisting: true 
        selectMultiple: false
        selectFolder: true
        nameFilters: [ "Binary files (*.bin)", "All files (*)" ]
        selectedNameFilter: "Binary files (*.bin)"
        sidebarVisible: true

        onAccepted: {
			secumessage.text = printnotice(secuman.message, secutext.text)
        }
        onRejected: { 
		}
    }	
	
	Column {
		x: 10; y: 10
		width: 340; height: 160
		spacing: 2
	
		TextField{
			id: secutext
			
			width: 340
			height: 40
			readOnly: true
			visible: false

			text: secuman.textfield
		}

		ComboBox {
			id: rule
			width: 340
			height: 50

			model: rulelist
			textRole: 'text'

			contentItem: Text {
				text:  rulelist.get(rule.currentIndex).text
				horizontalAlignment: Text.AlignLeft
				verticalAlignment: Text.AlignVCenter
				anchors.left : parent.left
				anchors.leftMargin: 10				
				font.bold: true
				color : rulelist.get(rule.currentIndex).color
			}
			
			currentIndex: 1

			onCurrentIndexChanged:  {
				rule.currentIndex = 1
			}
			
			Component.onCompleted: {
				Text.color = rulelist.get(rule.currentIndex).color
			}
		}		
		
		ComboBox {
			id: role
			width: 340
			height: 50

			model: rolelist
			textRole: 'text'

			contentItem: Text {
				text:  rolelist.get(role.currentIndex).text
				horizontalAlignment: Text.AlignLeft
				verticalAlignment: Text.AlignVCenter
				anchors.left : parent.left
				anchors.leftMargin: 10				
				font.bold: true
				color : rolelist.get(role.currentIndex).color
			}
			
			currentIndex: findcurrentmode(rolelist , 2, secuman.textfield)

			onCurrentIndexChanged:	{
				if( rolelist.get(role.currentIndex).text == "Single"){
					secuprod.visible = true
					keyrenew.visible = true
					secuboot.visible = true
					secudbg.visible = true
					securma.visible = true
					secuclean.visible = true
				}else if(rolelist.get(role.currentIndex).text == "SB Publisher"){
					secuprod.visible = false
					keyrenew.visible = false
					secuboot.visible = true
					secudbg.visible = false
					securma.visible = false
					secuclean.visible = false
				}else {
					secuprod.visible = false
					keyrenew.visible = false
					secuboot.visible = true
					secudbg.visible = true
					securma.visible = false
					secuclean.visible = false						
				}
			}
		}		
	
	}
	
	Column {
		x: 10; y: 120
		width: 340; height: 240	

		Button {
			id: secuprod
			width: 340
			height: 60

			text: "Secure Production"

            onClicked: {
				{
					var cleanpath;
				
					cleanpath = getcurrimagefolder();		
					
					secuman.text = "rule=" + rulelist.get(rule.currentIndex).viewtext + "&"
								 + "own=" + ownlist.get(1).viewtext + "&"
								 + "role=" + rolelist.get(role.currentIndex).viewtext + "&"
								 + "step=G.B1&"
								 + "image=" + cleanpath ;
		 
					Qt.quit()
				}
			}			
		}
		
			Button {
			id: keyrenew
			width: 340
			height: 60

			text: "Key Renewal"

            onClicked: {
				{
					var cleanpath;
				
					cleanpath = getcurrimagefolder();		

					secuman.text = "rule=" + rulelist.get(rule.currentIndex).viewtext + "&"
								 + "own=" + ownlist.get(1).viewtext + "&"
								 + "role=" + rolelist.get(role.currentIndex).viewtext + "&"
								 + "step=G.B2&"
								 + "image=" + cleanpath ;

					Qt.quit()
				}
			}			
		}
		
		Button {
			id: secuboot
			width: 340
			height: 60

			text: "Secure Boot"

            onClicked: {
				{
					var cleanpath;
				
					cleanpath = getcurrimagefolder();		
					
					secuman.text = "rule=" + rulelist.get(rule.currentIndex).viewtext + "&"
								 + "own=" + ownlist.get(1).viewtext + "&"
								 + "role=" + rolelist.get(role.currentIndex).viewtext + "&"
								 + "step=G.B5&"
								 + "image=" + cleanpath ;
					
					Qt.quit()
				}
			}			
		}	
		
		Button {
			id: secudbg
			width: 340
			height: 60

			text: "Secure Debug"

            onClicked: {
				{
					var cleanpath;
				
					cleanpath = getcurrimagefolder();		
					
					secuman.text = "rule=" + rulelist.get(rule.currentIndex).viewtext + "&"
								 + "own=" + ownlist.get(1).viewtext + "&"
								 + "role=" + rolelist.get(role.currentIndex).viewtext + "&"
								 + "step=G.B3&"
								 + "image=" + cleanpath ;
					
					Qt.quit()
				}
			}			
		}

		Button {
			id: securma
			width: 340
			height: 60

			text: "Secure RMA"

            onClicked: {
				{
					var cleanpath;
				
					cleanpath = getcurrimagefolder();		
					
					secuman.text = "rule=" + rulelist.get(rule.currentIndex).viewtext + "&"
								 + "own=" + ownlist.get(1).viewtext + "&"
								 + "role=" + rolelist.get(role.currentIndex).viewtext + "&"
								 + "step=G.B4&"
								 + "image=" + cleanpath ;
					
					Qt.quit()
				}
			}			
		}

		Button {
			id: secuclean
			width: 340
			height: 60

			text: "Remove Secrets"

            onClicked: {
				{
					var cleanpath;
				
					cleanpath = getcurrimagefolder();		
					
					secuman.text = "rule=" + rulelist.get(rule.currentIndex).viewtext + "&"
								 + "own=" + ownlist.get(1).viewtext + "&"
								 + "role=" + rolelist.get(role.currentIndex).viewtext + "&"
								 + "step=G.B6&"
								 + "image=" + cleanpath ;
					
					Qt.quit()
				}
			}			
		}
	}
	
	Column {
		x: 10; y: 480
		width: 340; height: 50	
		
		Button {
			id: quit
			width: 340
			height: 50

			text: "Quit"
			
            onClicked: {
				secuman.text = "" ;
				Qt.quit()
			}			
		}
	}
}