import QtQuick 2.0

Row {
    property string label
    property string value

    Text {
    width : 180
	text : label
	color : theme.foreground
	font.pointSize : theme.pointSize
    }
    Text {
	width : 50
	text : value
	color : theme.foreground
	font.pointSize : theme.pointSize
    }
}

