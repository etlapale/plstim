import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.1
import QtQuick.Window 2.1


ApplicationWindow {

    width : 400
    height : 800

    property bool advanced : false
    property bool loaded : false
    property bool running : false

    // Experiment selector
    FileDialog {
        id: fileDialog
        title: "Select an experiment file"
        nameFilters: ["Experiment files (*.json)", "All files (*)"]
        selectedNameFilter: "Experiment files (*.json)"
        onAccepted : {
            gui.loadExperiment(fileUrl);
        }
    }

    toolBar : ToolBar {
        RowLayout {
            ToolButton {
                text: "Load"
                visible: !loaded
                onClicked: fileDialog.open()
            }
            ToolButton {
                objectName : "runButton"
                text : "Run"
                visible : !running && loaded
                enabled : advanced || subjectList.currentIndex != 0
            }
            ToolButton {
                objectName : "runInlineButton"
                text : "Run inline"
                visible : advanced && !running && loaded
            }
            ToolButton {
                objectName : "abortButton"
                text : "Abort"
                visible : running
            }
            ToolButton {
                objectName : "quitButton"
                text : "Quit"
            }
        }
    }

    statusBar : ToolBar {
        RowLayout {
            Layout.fillWidth : true
            ToolButton {
                text : "Advanced mode"
                tooltip : "Display advanced options"
                iconSource : "qrc:/images/emblem-system.png"
                checkable : true
                onClicked : advanced = !advanced
            }
        }
    }

    ColumnLayout {

        anchors.fill : parent
        anchors.margins : 8

        GroupBox {
            title : "Machine"
            Layout.fillWidth : true

            GridLayout {
                columns : 2

                Label { text : "Hostname" }
                Label { objectName : "hostParam" }

                Label { text : "Timer" }
                Label { objectName : "timerParam" }
            }
        }

        GroupBox {
            title : "Setup"
            Layout.fillWidth : true

            GridLayout {
                columns : 2

                Label { text : "Name" }
                Label { text : setup.name }

                Label { text : "Horizontal resolution" }
                Label { text : setup.horizontalResolution + " px"}

                Label { text : "Vertical resolution" }
                Label { text : setup.verticalResolution + " px"}

                Label { text : "Physical width" }
                Label { text : setup.physicalWidth + " mm"}

                Label { text : "Physical height" }
                Label { text : setup.physicalHeight + " mm"}

                Label { text : "Distance" }
                Label { text : setup.distance + " mm"}

                // TODO: Luminance

                Label { text : "Refresh rate" }
                Label { text : setup.refreshRate + " Hz" }

                Label { text : "Data directory" }
                Label { text : setup.dataDir }
            }
        }

        GroupBox {
            title : "Experiment"
            Layout.fillWidth : true

            GridLayout {
                columns : 2

                Label { text : "Name" }
                Label { text : xp ? xp.name : "" }

                Label { text : "Number of trials" }
                Label { text : xp ? xp.trialCount : "" }

                Label { text : "Texture size" }
                Label { text : xp ? xp.textureSize + "×" + xp.textureSize + " px" : "" }
            }
        }

        GroupBox {
            title : "Subject"
            Layout.fillWidth : true

            GridLayout {
                columns : 2
                width : parent.width

                Label { text : "ID" }
                ComboBox {
                    id : subjectList
		    objectName : "subjectList"
                }

                Label {
                    visible : subjectList.currentIndex == 0
		    text : "No subject loaded"
		    font.bold : true
                    Layout.columnSpan : 2
                }

                TableView {
                    TableViewColumn {
                        role : "name"
                        title : "Name"
                        width : 120
                    }
                    TableViewColumn {
                        role : "value"
                        title : "Value"
                        width : 60
                    }
                    TableViewColumn {
                        role : "unit"
                        title : "Unit"
                        width : 40
                    }

                    objectName : "subjectParams"
                    width : parent.width
                    Layout.columnSpan : 2
                    visible : subjectList.currentIndex != 0
                }
            }
        }

	GroupBox {
	    title : "Session"
	    visible : running
            width : parent.width

            GridLayout {
                columns : 2
                width : parent.width

                Label {
                    text : "Trial"
                }
                RowLayout {
                    ProgressBar {
                        value : engine.currentTrial
                        minimumValue : 0
                        maximumValue : xp ? xp.trialCount : 0
                    }
                    Text {
                        objectName : "trialValue"
                        text : xp ? (engine.currentTrial+1) + "/" + xp.trialCount + " (" + (Math.round(100*(engine.currentTrial+1)/xp.trialCount)) + "%)" : ""
                    }
                }
                Label { text : "Est. rem. time" }
                Label {
                    text : engine.eta == 0 ? "" : (engine.eta > 60 ? Math.floor (engine.eta / 60) + " min " + (engine.eta % 60) + " s" : engine.eta + " s")
                }
            }
        }

        GroupBox {
            title : "Messages"
            Layout.fillWidth : true

            TableView {
                TableViewColumn {
                    role : "title"
                    title : "Title"
                    width : 220
                }
                TableViewColumn {
                    role : "description"
                    title : "Description"
                    width : 800
                }

                model : errorsModel
                width : parent.width
            }
        }
    }
}
