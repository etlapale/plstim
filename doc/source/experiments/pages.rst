Pages
=====

.. highlight:: qml

Pages represent states with usually different visual displays. For
instance, one page could be a fixation stimulus, presenting a single
centered dot. The experiment could then be carried on to an animated
display, for instance a moving grating. Then a question can be asked
to the subject, before coming back to the first state.

Painting a page
---------------

.. index:: onPaint

Defining a visual display for a page can be done by setting its
``onPaint`` property::

  Page {
      name: "fixation"
      onPaint: {
	  painter.setBrush(color);
	  painter.drawEllipse(/* […] */);
      }
  }

The ``onPaint`` function is called with a ``painter`` argument which
wraps a QPainter_ object from Qt.

Animated pages
--------------

.. index:: animation

By setting the ``animated`` property of a page, one can define video
to be displayed. Animated pages use the same function, ``onPaint`` to
define the content of the frames, but with an additional argument,
``frameNumber``. You must also define the ``duration`` property to
specify for how long the page should be animated::

    Page {
	name : "frames"
	animated : true
	duration : 400
	onPaint : {
	    paintDots (painter, frameNumber);
	}
    }

Here ``paintDots`` is a user-defined function of the experiment.
    
Key events
----------

.. index:: transition

By default a fixed page will be presented until the user press Enter
or the Space bar. You can configure the set of keys a fixed frame will
wait for, using the ``acceptedKey`` property, which takes an array of
key names, or specify a duration for the presentation, with
``duration``.

Keyboard events can also be listened to by defining a ``onKeyPress``
handler in the page properties::

    Page {
        acceptedKeys : ["Left", "Right"]
	onKeyPress : {
	    correct = clockwise ? key == "Left" : key == "Right";
	}
    }
    
Transitions
-----------

By default the next page to be shown in an experiment is simply the
page following the current one in the order of declaration, and the
trial will be ended when arriving at the last page. The page order can
be modified by calling the ``showPage`` function, for instance inside
a signal handler::

    onKeyPress : {
        showPage(correct ? correctPage : incorrectPage)
    }

You can also define a page to end the current trial by setting its
``last`` property.

.. _QPainter: http://doc.qt.io/qt-5/qpainter.html
