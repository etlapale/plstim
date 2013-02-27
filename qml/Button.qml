import QtQuick 2.0

Rectangle {
    property alias text : label.text

    color : buttonMouseArea.pressed ? Qt.darker (theme.buttonBackground, 1.5) :  theme.buttonBackground
    border.color : theme.buttonBorderColor
    width : label.width + 10
    height : label.height + 8
    radius : 4

    signal buttonClick ()

    Text {
	id : label
	color : theme.buttonForeground
	font.pointSize : theme.pointSize
	anchors.verticalCenter : parent.verticalCenter
	anchors.horizontalCenter : parent.horizontalCenter
    }

    MouseArea {
	id : buttonMouseArea
	anchors.fill: parent
	hoverEnabled : true
	onEntered : parent.border.color = theme.buttonHoverBorderColor
	onExited : parent.border.color = theme.buttonBorderColor
	onClicked : buttonClick ()
    }
}
