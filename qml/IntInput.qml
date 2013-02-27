import QtQuick 2.0

Row {
    property string label
    property string value

    Column {
	width : 180
	Text {
	    text : label
	    color : theme.foreground
	    font.pointSize : 12
	}
    }
    Column {
	width : 50
	TextInput {
	    text : value
	    color : theme.foreground
	    font.pointSize : theme.pointSize
	}
    }
}

