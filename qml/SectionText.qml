import QtQuick 2.0

Column {
    property alias text : label.text

    Item {
	width : 1
	height : 10
    }

    Text {
	id : label
	color : theme.titleColor
	font.bold : true
	font.pointSize : theme.pointSize
    }
}
