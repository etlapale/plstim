import QtQuick 2.0

Rectangle {

    Style { id : theme }

    width : 400
    height : 800
    color : "#040f15"

    Column {
	width : parent.width

	Text {
	    text : "Setup"
	    color : theme.titleColor
	    font.bold : true
	    font.pointSize : theme.pointSize
	}

	IntInput {
	    label : "Horizontal offset"
	    value : "0 px"
	}

	IntInput {
	    label : "Vertical offset"
	    value : "0 px"
	}

	IntInput {
	    label : "Horizontal resolution"
	    value : "1280 px"
	}

	IntInput {
	    label : "Vertical resolution"
	    value : "1024 px"
	}

	IntInput {
	    label : "Physical width"
	    value : "394 mm"
	}

	IntInput {
	    label : "Physical height"
	    value : "292 mm"
	}

	IntInput {
	    label : "Distance"
	    value : "750 mm"
	}

	IntInput {
	    label : "Minimum luminance"
	    value : "15 cd/m²"
	}

	IntInput {
	    label : "Maximum luminance"
	    value : "150 cd/m²"
	}

	IntInput {
	    label : "Refresh rate"
	    value : "85 Hz"
	}

	Item {
	    width : 1
	    height : 10
	}

	Text {
	    text : "Experiment"
	    color : theme.titleColor
	    font.bold : true
	    font.pointSize : theme.pointSize
	}

	IntInput {
	    label : "Number of trials"
	    value : "160"
	}

	Item {
	    width : 1
	    height : 10
	}

	Text {
	    text : "Subject"
	    color : theme.titleColor
	    font.bold : true
	    font.pointSize : theme.pointSize
	}

	Text {
	    text : "No subject loaded"
	    font.bold : true
	    color : theme.warningColor
	    font.pointSize : theme.pointSize
	}

	Item {
	    width : 1
	    height : 30
	}

	Row {
	    spacing : 20
	    anchors.horizontalCenter: parent.horizontalCenter

	    Button {
		objectName : "runButton"
		text : "Run"
	    }
	    Button {
		objectName : "runInlineButton"
		text : "Run inline"
	    }
	    Button {
		objectName : "quitButton"
		text : "Quit"
	    }
	}
    }
}
