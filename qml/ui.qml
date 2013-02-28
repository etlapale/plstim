import QtQuick 2.0


Rectangle {

    Style { id : theme }

    width : 400
    height : 800
    color : theme.background

    property bool running : false

    Column {
	width : parent.width

	SectionText {
	    text : "Machine"
	}

	TextParam {
	    objectName : "hostParam"
	    label : "Host name"
	}

	TextParam {
	    objectName : "timerParam"
	    label : "Timer"
	}

	SectionText {
	    text : "Setup"
	}

	TextParam {
	    label : "Name"
	    value : setup.name
	}

	IntInput {
	    label : "Horizontal offset"
	    value : setup.horizontalOffset
	    unit : "px"

	}

	IntInput {
	    label : "Vertical offset"
	    value : setup.verticalOffset
	    unit : "px"
	}

	IntInput {
	    label : "Horizontal resolution"
	    value : setup.horizontalResolution
	    unit : "px"
	}

	IntInput {
	    label : "Vertical resolution"
	    value : setup.verticalResolution
	    unit : "px"
	}

	IntInput {
	    label : "Physical width"
	    value : setup.physicalWidth
	    unit : "mm"
	}

	IntInput {
	    label : "Physical height"
	    value : setup.physicalHeight
	    unit : "mm"
	}

	IntInput {
	    label : "Distance"
	    value : setup.distance
	    unit : "mm"
	}

	/*IntInput {
	    label : "Minimum luminance"
	    value : "15 cd/m²"
	}

	IntInput {
	    label : "Maximum luminance"
	    value : "150 cd/m²"
	}*/

	IntInput {
	    label : "Refresh rate"
	    value : setup.refreshRate
	    unit : "Hz"
	}

	SectionText {
	    text : "Experiment"
	}

	IntInput {
	    label : "Number of trials"
	    value : 160
	}

	SectionText {
	    text : "Subject"
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
		visible : ! running
	    }
	    Button {
		objectName : "runInlineButton"
		text : "Run inline"
		visible : ! running
	    }

	    Button {
		objectName : "abortButton"
		text : "Abort"
		visible : running
	    }

	    Button {
		objectName : "quitButton"
		text : "Quit"
	    }
	}

	Item {
	    width : 1
	    height : 50
	}

	SectionText {
	    text : "Session"
	    visible : running
	}

	Row {
	    width : parent.width
	    visible : running
	    Text {
		text : "Trial"
		width : 180
		color : theme.foreground
		font.pointSize : theme.pointSize
	    }
	    Text {
		objectName : "trialText"
		width : 50
		color : theme.foreground
		font.pointSize : theme.pointSize
	    }
	}
    }
}
