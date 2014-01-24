import QtQuick 2.0
import QtQuick.Controls 1.1

Row {
    property string label
    property string value

    Label {
    width : 180
	text : label
        color : theme.foreground
        font.pointSize : theme.pointSize
    }
    Label {
	width : 50
	text : value
        color : theme.foreground
        font.pointSize : theme.pointSize
    }
}
