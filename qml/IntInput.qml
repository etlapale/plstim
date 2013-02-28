import QtQuick 2.0

Row {
    property string label
    property int value
    property string unit

    Column {
	width : 180
	Text {
	    text : label
	    color : theme.foreground
	    font.pointSize : theme.pointSize
	}
    }
    Column {
	width : 50
	TextInput {
	    text : "" + value + (unit.length ? " " + unit : "")
	    color : theme.foreground
	    font.pointSize : theme.pointSize
	}
    }
}

