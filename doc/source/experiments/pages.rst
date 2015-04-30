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

.. _QPainter: http://doc.qt.io/qt-5/qpainter.html
