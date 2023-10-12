import QtQuick 2.10
import QtQuick.Controls 2.1
import QtQuick.Controls.Material  2.1
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.2
import QtQuick.Window 2.2

ApplicationWindow {
    title: conman.wintitle
    Material.theme: Material.Dark
    Material.accent: Material.Teal
	//Material.primary: Material.BlueGrey
    //Material.foreground: Material.Dark
    //Material.background: Material.Grey
    visible: true

	////////////////////////////////////////
	// 
	////////////////////////////////////////
	
	signal pushButton(string text)
	
	property var autorundelay : 2
	
	////////////////////////////////////////
	// funcitons 
	////////////////////////////////////////

	function updateConsoleText() {
		contitle.text = conman.contitle;
		conwin.text = conman.console;
	}		
	
	////////////////////////////////////////
	// UI definitions
	////////////////////////////////////////

    width: 720
    height: 560
    maximumHeight: height
    maximumWidth: width
    minimumHeight: height
    minimumWidth: width	


    Timer {
        property bool wasTriggered: false
        id: holdTimer
        interval: 200
        running: true
        repeat: true
		
        onTriggered: {
            wasTriggered = true
            updateConsoleText();
			
			if( conman.runclick == "Run" ){
				if( autorundelay == 0 ){
					conman.runclick = ""
					run.clicked()
					busyIndicator.running = false
				}else{
					autorundelay = autorundelay - 1
					busyIndicator.running = true
				}
			}
        }
    }
	
	Column {
		width: 700
		height: 560	
		x: 10
		y: 10

		Column { 
			spacing: 4

			Rectangle {
				width: 700
				height: 40
				radius: 4
			
				TextEdit {
					id: contitle
					
					x: 10
					y: 10						
					width: 580
					height: 20
					
					wrapMode: TextEdit.WordWrap
					
					color: "steelblue"
					text: qsTr("")

				}
			}
			
			Rectangle {
				width: 700
				height: 460
				radius: 4
					
				Flickable {
					id: flick

					flickableDirection: Flickable.VerticalFlick
					anchors.fill: parent

					x: 10
					y: 10						
					width: 680
					height: 280

					contentWidth: conwin.paintedWidth
					contentHeight: conwin.paintedHeight
					interactive: true
					clip: true

					boundsMovement: Flickable.StopAtBounds
					boundsBehavior: Flickable.DragOverBounds
					opacity: Math.max(0.5, 1.0 - Math.abs(verticalOvershoot) / height)
	
					ScrollBar.vertical: ScrollBar { 
						hoverEnabled: true
						active: hovered || pressed
						policy: ScrollBar.AlwaysOn
						orientation: Qt.Vertical
						snapMode: ScrollBar.SnapAlways
						background: transparent
					}
					
					Text {
						id: conwin

						x: 10
						y: 10						
						width: flick.width
						focus: true
						wrapMode: TextEdit.WordWrap

						font.family: "Courier"
						font.pixelSize: 8

						text: qsTr("")
					}
					
				}

				BusyIndicator {
					id: busyIndicator
					anchors.centerIn: parent
					running: false
					width: 150
					height: 150					
				}
				
				
			}
		}
	}
	
	
	Column {
		width: 400
		height: 40	
		x: 260	
		y: 520		

		Row {
			spacing: 20	
			
			Button {
				id: run
				width: 120
				height: 40
				visible: false

				text: "Re-run"

				onClicked: {
					pushButton("Run")
				}
				onPressed: {
					busyIndicator.running = true
				}
				onReleased: {
					busyIndicator.running = false
				}
			}	
			Button {
				id: step
				width: 120
				height: 40
				visible: false

				text: "Step"

				onClicked: {
					pushButton("Step")
				}
				onPressed: {
					busyIndicator.running = true
				}
				onReleased: {
					busyIndicator.running = false
				}
			}	
			Button {
				id: stop
				width: 160
				height: 40

				text: "OK"
				
				onClicked: {
					Qt.quit()
				}				
			}	
		}
	}
}