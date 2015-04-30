Experiment definition
=====================

.. highlight:: qml

.. index:: Experiment
.. index:: QML

QML_, a JavaScript dialect used by Qt5_, is the native format to
describe plstim experiments. An experiment must first import the
adequate package, and then define a single root ``Experiment`` element::

   import PlStim 1.0

   Experiment {
   }

Each experiment can define an arbitrary number of properties, in
addition to the standard ones. A minimal experiement must define the
number of trials it contains, the size of the visual display, and the
name of the experiment::

   Experiment {
       name:       "First experiment"
       trialCount: 128	// 128 trials
       size:       9	// 9×9 degrees
   }

.. index:: Page
   
Finally, the states an experiment can be in are described by *pages*,
which must be added as child to the experiment::

  Experiment {
      // […]
      Page {
          name: "fixation"
      }
      Page {
          name: "animation"
	  duration: 400     // ms
      }
      // […]
  }

.. _Qt5: http://qt.io
.. _QML: http://doc.qt.io/qt-5/qmlapplications.html
