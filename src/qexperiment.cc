// plstim/qexperiment.cc – Base class for experimental stimuli


#include <iomanip>
#include <iostream>
#include <sstream>
using namespace std;

#include "qexperiment.h"
#include "messagebox.h"
using namespace plstim;


#include <QMenuBar>
#include <QHostInfo>

#include <QtCore>
#include <QtQuick>

using namespace H5;


bool
QExperiment::exec ()
{
  win->show ();

  // Load an experiment if given as command line argument
  auto args = app.arguments ();
  if (args.size () == 2) {
    load_experiment (args.at (1));
  }

#ifdef HAVE_POWERMATE
  // Watch for PowerMate events in a background thread
  PowerMateWatcher watcher;
  QThread wthread;
  connect (&wthread, SIGNAL (started ()), &watcher, SLOT (watch ()));
  watcher.moveToThread (&wthread);
  wthread.start ();
#endif // HAVE_POWERMATE


  return app.exec () == 0;
}

void
QExperiment::paintPage (Page* page, QImage& img, QPainter& painter)
{
    QPainter::RenderHints render_hints = QPainter::Antialiasing|QPainter::SmoothPixmapTransform|QPainter::HighQualityAntialiasing;

    // Wraps the QPainter for QML
    Painter wrappedPainter (painter);

    // Single frames
    if (! page->animated ()) {
	painter.begin (&img);
	// Reset QImage/QPainter states
	img.fill (0); painter.setPen (Qt::NoPen);

	painter.setRenderHints (render_hints);

	emit page->paint (&wrappedPainter, 0);

	painter.end ();

	//img.save (QString ("page-") + page->name () + ".png");
	stim->addFixedFrame (page->name (), img);
    }

    // Multiple frames
    else {
	//timer.start ();
	stim->deleteAnimatedFrames (page->name ());
	//qDebug () << "deleting unamed took: " << timer.elapsed () << " milliseconds" << endl;
	//timer.start ();

	qDebug () << "number of frames to be painted:"
		  << page->frameCount ();
	for (int i = 0; i < page->frameCount (); i++) {

	    painter.begin (&img);
	    // Reset QImage/QPainter states
	    img.fill (0); painter.setPen (Qt::NoPen);

	    painter.setRenderHints (render_hints);

	    emit page->paint (&wrappedPainter, i);

	    painter.end ();
#if 0
	    QString filename;
	    filename.sprintf ("page-%s-%04d.png", qPrintable (page->name ()), i);
	    img.save (filename);
#endif
	    //glwidget->add_animated_frame (page->title, img);
	    stim->addAnimatedFrame (page->name (), img);
	}
	//qDebug () << "generating frames took: " << timer.elapsed () << " milliseconds" << endl;
    }
}

void
QExperiment::run_trial ()
{
  timer.start ();

  // Emit the newTrial () signal
  emit m_experiment->newTrial ();

  // Paint each per-trial frame
  int tex_size = m_experiment->textureSize ();
  QImage img (tex_size, tex_size, QImage::Format_RGB32);
  QPainter painter;
  for (int i = 0; i < m_experiment->pageCount (); i++) {
      auto page = m_experiment->page (i);
      if (page->paintTime () == Page::TRIAL)
	  paintPage (page, img, painter);
  }

  // Record trial parameters
  for (auto it : trial_offsets) {
    const QString& param_name = it.first;
    auto name = it.first.toUtf8 ();
    size_t offset = it.second;

    QVariant prop = m_experiment->property (name);
    if (prop.canConvert<float> ()) {
	float* pos = (float*) (((char*) trial_record)+offset);
	*pos = prop.toFloat ();
	qDebug () << "storing" << *pos << "for" << param_name;
    }
    else if (prop.canConvert<plstim::Vector*> ()) {
	plstim::Vector* vec = prop.value<plstim::Vector*> ();
	float* pos = (float*) (((char*) trial_record)+offset);
	int len = vec->length ();
	for (int i = 0; i <= len; i++)
	    pos[i] = vec->at (i);
	qDebug () << "storing" << "array" << "for" << param_name;
    }
  }

#ifdef HAVE_EYELINK
  // Save trial timestamp in EDF file
  if (hf != NULL)
    eyemsg_printf ("trial %d", current_trial+1);
#endif // HAVE_EYELINK

  current_page = -1;
  nextPage ();
}

void
QExperiment::show_page (int index)
{
    auto page = m_experiment->page (index);

    // Save page parameters
    auto it = record_offsets.find (page->name ());
    if (it != record_offsets.end ()) {
	const std::map<QString,size_t>& params = it->second;
	for (auto jt : params) {
	    auto param_name = jt.first;
	    size_t begin_offset = jt.second;
	    if (param_name == "begin") {
		qint64* pos = (qint64*) (((char*) trial_record)+begin_offset);
		qint64 nsecs = timer.nsecsElapsed ();
		*pos = nsecs;
	    }
	    else if (param_name == "key") {
		// Key will be known at the end of the page
	    }
#ifdef HAVE_POWERMATE
	    else if (param_name == "rotation") {
	    }
#endif
	    else {
		/*double* pos = (qint64*) (((char*) trial_record)+begin_offset);
		  double value = timer.nsecsElapsed ();
		 *pos = nsecs;*/
		qDebug () << "NYI page param";
	    }
	}
    }

    qDebug () << ">>> showing page" << page->name ();
#ifdef HAVE_EYELINK
  // Save page timestamp in EDF file
  if (hf != NULL)
    eyemsg_printf ("showing page %s", page->name ().toUtf8 ().data ());
#endif // HAVE_EYELINK

  if (page->animated ()) {
      stim->showAnimatedFrames (page->name ());
  }
  else {
      stim->showFixedFrame (page->name ());
  }

  current_page = index;

#ifdef HAVE_EYELINK
  // Check for maintained fixation
  if (false) {//page->fixation ()) {
    qDebug () << "page" << page->name () << "wants a" << page->fixation () << "ms fixation";
    QElapsedTimer timer;
    // Target
    float tx = (float) res_x_edit->value () / 2.0;
    float ty = (float) res_y_edit->value () / 2.0;
    float fix_threshold = m_experiment->degreesToPixels (1.0);
    qDebug () << "checking for fix at" << tx << ty << "threshold is" << fix_threshold << "px";

    waiting_fixation = true;
    timer.start ();

    float dst = 2*fix_threshold;
    while (waiting_fixation
	   && (dst > fix_threshold
	       || timer.elapsed () < page->fixation ())) {
      // Check for eye data
      if (eyelink_newest_float_sample (NULL) > 0) {
	// Get eye position
	ALLF_DATA evt;
	eyelink_newest_float_sample (&evt);
	// Compute distance to the target
	float gx = evt.fs.gx[RIGHT_EYE];
	float gy = evt.fs.gy[RIGHT_EYE];
	//qDebug () << "eye at" << gx << gy;
	float dx = gx - tx;
	float dy = gy - ty;
	//qDebug () << "X/Y distances" << dx << dy;
	dst = sqrt (dx*dx + dy*dy);
	//qDebug () << "distance:" << dst << "pixels";
	if (dst > fix_threshold)
	  timer.restart ();
      }
      // Allow calibration and forced skip
      else
	app.processEvents ();
    }

    waiting_fixation = false;
    nextPage ();
  }
#endif // HAVE_EYELINK

    // Fixed frame of defined duration
    if (page->duration () && ! page->animated ()) {
	qDebug () << "fixed frame for" << page->duration () << "ms";
	QTimer::singleShot (page->duration (), this, SLOT (nextPage ()));
    }

    // If no keyboard event expected, go to the next page
    else if (! page->waitKey ()
#ifdef HAVE_POWERMATE
	    && ! page->waitRotation ()
#endif // HAVE_POWERMATE
       ) {
	qDebug () << "nothing to wait, going to next page";
	nextPage ();
    }
}

void
QExperiment::nextPage ()
{
#ifdef HAVE_EYELINK
    // Exit the inner fixation loop
    if (waiting_fixation) {
	// TODO: record event
	qDebug () << "aborting fixation";
	waiting_fixation = false;
	return;	// Will jump back here eventually
    }
#endif // HAVE_EYELINK

    // Save the page record on HDF5
    if (hf != NULL) {
	hsize_t one = 1;
	DataSpace fspace = dset.getSpace ();
	hsize_t hframe = current_trial;
	fspace.selectHyperslab (H5S_SELECT_SET, &one, &hframe);
	DataSpace mspace (1, &one);
	dset.write (trial_record, *record_type, mspace, fspace);
    }
  
    // End of trial
    auto page = current_page < 0 ? NULL : m_experiment->page (current_page);
    if ((page && page->last ())
	    || current_page + 1 == m_experiment->pageCount ()) {
	qDebug () << "End of trial " << current_trial << "of" << m_experiment->trialCount ();

	// Next trial
	if (++current_trial < m_experiment->trialCount ()) {
	    run_trial ();
	}
	// End of session
	else {
	    if (hf != NULL) {
#if HAVE_EYELINK
		// Stop recording
		stop_recording ();
		// Choose a name for the local EDF
		auto subject_id = subject_cbox->currentText ();
		auto edf_name = QString ("%1-%2-%3.edf").arg (xp_name).arg (subject_id).arg (session_number);
		// Buggy Eyelink headers do not care about const
		auto res = receive_data_file ((char*) "", edf_name.toLocal8Bit ().data (), 0);
		switch (res) {
		case 0:
		    error ("EyeLink data file transfer cancelled");
		    break;
		case FILE_CANT_OPEN:
		    error ("Cannot open EyeLink data file");
		    break;
		case FILE_XFER_ABORTED:
		    error ("EyeLink data file transfer aborted");
		    break;
		}
		check_eyelink (close_data_file (), "close_data_file");
#endif // HAVE_EYELINK
		hf->flush (H5F_SCOPE_GLOBAL);	// Store everything on file
	    }
	    //glwidget->normal_screen ();
	    stim->hide ();
	    session_running = false;
	}
    }
    // There is a next page
    else {
	// Check if there is a next page user defined
	if (page) {
	    QString next = page->nextPage ();
	    if (! next.isEmpty ()) {
		for (int i = 0; i < m_experiment->pageCount (); i++) {
		    Page* p = m_experiment->page (i);
		    if (p->name () == next) {
			current_page = i - 1;
			break;
		    }
		}
	    }
	}
	
	show_page (current_page + 1);
    }
}

void
QExperiment::savePageParameter (const QString& pageTitle,
				const QString& paramName,
				int paramValue)
{
    auto it = record_offsets.find (pageTitle);
    if (it != record_offsets.end ()) {
	const std::map<QString,size_t>& params = it->second;
	auto jt = params.find (paramName);
	if (jt != params.end ()) {
	    size_t key_offset = jt->second;
	    int* pos = (int*) (((char*) trial_record)+key_offset);
	    *pos = paramValue;
	}
    }
}

#ifdef HAVE_POWERMATE
void
QExperiment::powerMateRotation (PowerMateEvent* evt)
{
    if (! session_running)
	return;

    auto page = m_experiment->page (current_page);
    if (page->waitRotation ()) {
	qDebug () << "RECORDING PowerMate event with step of" << evt->step;
	savePageParameter (page->name (), "rotation", evt->step);

	// Notify the page of a rotation
	emit page->rotation (evt->step);

	nextPage ();
    }
}

void
QExperiment::powerMateButtonPressed (PowerMateEvent* evt)
{
    Q_UNUSED (evt);
    //qDebug () << "PowerMate button pressed!";

    if (! session_running)
	return;

    auto page = m_experiment->page (current_page);
    if (page->waitKey () && page->acceptAnyKey ()) {
	qDebug () << "powermate button → next page";
	nextPage ();
    }
}
#endif // HAVE_POWERMATE

void
QExperiment::stimKeyPressed (QKeyEvent* evt)
{
  // Keyboard events are only handled in sessions
  if (! session_running)
    return;

  auto page = m_experiment->page (current_page);
  
  // Go to the next page
  if (page->waitKey ()) {
#ifdef HAVE_EYELINK
    // Allows EyeLink calibration and validation
    if (page->fixation () > 0
	&& evt->key () == Qt::Key_C) {
      calibrate_eyelink ();
      return;
    }
#endif // HAVE_EYELINK
    // Check if the key is accepted
    if ((page->acceptAnyKey () && nextPageKeys.contains (evt->key ()))
	    || page->acceptKey (evt->key ())) {
#if 0
      // Save pressed key
      if (! page->accepted_keys.empty ())
	  savePageParameter (page->title, "key", evt->key ());
#endif

      // Move to the next page
      qDebug () << "stim key → next page";
      nextPage ();
    }
  }
}


void
QExperiment::open_recent ()
{
  auto action = qobject_cast<QAction*> (sender());
  if (action)
    load_experiment (action->data ().toString ());
}

void
QExperiment::abortExperiment ()
{
  current_page = -1;
  stim->hide ();
  session_running = false;
}

void
QExperiment::unloadExperiment ()
{
  abortExperiment ();

  // Erase all components from QML engine memroy
  m_engine.clearComponentCache ();
  if (m_component) {
      delete m_component;
      m_component = NULL;
  }

#ifdef HAVE_EYELINK
  if (eyelink_connected)
    close_eyelink_connection ();
#endif

  xp_name.clear ();

  param_spins.clear ();

  if (xp_item)
    delete xp_item;
  if (subject_item)
    delete subject_item;

  if (trial_record != NULL) {
    free (trial_record);
    trial_record = NULL;
  }
  if (record_type != NULL) {
    delete record_type;
    record_type = NULL;
  }
  record_size = 0;
  trial_offsets.clear ();
  record_offsets.clear ();
  xp_keys.clear ();
  if (hf != NULL) {
    hf->close ();
    hf = NULL;
  }

  stim->clear ();

  xp_label->setText ("No experiment loaded");
}

bool
QExperiment::load_experiment (const QString& path)
{

  // Canonicalise the script path
  QFileInfo fileinfo (path);
  auto script_path = fileinfo.canonicalFilePath ();

  // Store the experiment path in the recent list
  auto rlist = settings->value ("recents").toStringList ();
  rlist.removeAll (script_path);
  rlist.prepend (script_path);
  while (rlist.size () > max_recents)
    rlist.removeLast ();
  settings->setValue ("recents", rlist);
  updateRecents ();

  // TODO: cleanup any existing experiment
  unloadExperiment ();
  
  // Add the experiment to the settings
  xp_name = fileinfo.fileName ();
  if (xp_name.endsWith (".qml"))
    xp_name = xp_name.left (xp_name.size () - 4);
  settings->beginGroup (QString ("experiments/%1").arg (xp_name));
  if (settings->contains ("path")) {
    if (settings->value ("path").toString () != script_path) {
      error ("Homonymous experiment");
      xp_name.clear ();
      return false;
    }
  }
  else {
    settings->setValue ("path", script_path);
  }
  settings->endGroup ();

  m_component = new QQmlComponent (&m_engine, script_path);
  if (m_component->isError ()) {
      qDebug () << "error: could not load the QML experiment";
      for (auto& err : m_component->errors ())
	  qDebug () << err;
      return false;
  }
  QObject* xp = m_component->create ();
  if (m_component->isError ()) {
      qDebug () << "error: could not instantiate the QML experiment";
      for (auto& err : m_component->errors ())
	  qDebug () << err;
      return false;
  }
  m_experiment = qobject_cast<Experiment*> (xp);
  qDebug () << "[QML] Number of trials:" << m_experiment->trialCount ();

  // Add trial parameters to the record
  const QVariantMap& trialParameters = m_experiment->trialParameters ();
  QMapIterator<QString,QVariant> it (trialParameters);
  while (it.hasNext ()) {
      it.next ();
      const QString& param_name = it.key ();
      qDebug () << "found trial parameter" << param_name;
      
      // Get the current value of the parameter to
      // check it’s type
      QVariant prop = m_experiment->property (param_name.toUtf8 ());
      if (prop.isValid ()) {
	  if (prop.canConvert<float> ()) {
	      qDebug () << "Adding trial parameter" << param_name << "as float";
	      trial_types[param_name] = PredType::NATIVE_FLOAT;
	      trial_offsets[param_name] = record_size;
	      record_size += sizeof (float);
	  }
	  else if (prop.canConvert<plstim::Vector*> ()) {
	      plstim::Vector* vec = prop.value<plstim::Vector*> ();
	      // Get the current size of the array
	      int len = vec->length ();
	      qDebug () << "Adding trial parameter" << param_name << "as double array of" << len << "elements";
	      hsize_t hlen = len;
	      trial_types[param_name] = ArrayType (PredType::NATIVE_FLOAT, 1, &hlen);
	      trial_offsets[param_name] = record_size;
	      record_size += len * sizeof (float);
	  }
	  else {
	      qDebug () << "Unknown type for trial parameter" << param_name;
	  }
      }
  }
  
  // Check for modules to be loaded
  const QVariantList& modules = m_experiment->modules ();
  for (int i = 0; i < modules.size (); i++) {
      QVariant var = modules.at (i);
      if (var.canConvert<QString> ()) {
	  QString name = var.toString ();
#ifdef HAVE_EYELINK
	  if (name == "eyelink") {
	      load_eyelink ();
	  }
#endif
      }
  }

  // Create the pages defined in the experiment
  for (int i = 0; i < m_experiment->pageCount (); i++) {
      Page* page = m_experiment->page (i);
      QString page_title = page->name ();
      // Compute required memory for the page
      record_offsets[page_title]["begin"] = record_size;
      record_size += sizeof (qint64);	// Start page presentation
#if 0
      if (! accepted_keys.empty ()) {
	  record_offsets[page_title]["key"] = record_size;
	  record_size += sizeof (int);	// Pressed key
      }
#endif
#ifdef HAVE_POWERMATE
      if (page->waitRotation ()) {
	  record_offsets[page_title]["rotation"] = record_size;
	  record_size += sizeof (int);	// PowerMate rotation
      }
#endif // HAVE_POWERMATE
  }

  // Define the trial record
  if (record_size) {
      qDebug () << "record size set to" << record_size;
      trial_record = malloc (record_size);
      record_type = new CompType (record_size);
      // Add trial info to the record
      for (auto it : trial_offsets) {
	  const QString & param_name = it.first;
	  qDebug () << "  Trial for" << it.first << "at" << it.second;
	  record_type->insertMember (param_name.toUtf8 ().data (), it.second, trial_types[param_name]);
      }
      // Add pages info to the record
      QString fmt ("%1_%2");
      for (int i = 0; i < m_experiment->pageCount (); i++) {
	  plstim::Page* page = m_experiment->page (i);
	  auto page_name = page->name ();
	  for (auto param : record_offsets[page_name]) {
	      auto param_name = param.first;
	      auto offset = param.second;
	      H5::DataType param_type;
	      if (param_name == "begin")
		  param_type = PredType::NATIVE_LONG;
	      else if (param_name == "key")
		  param_type = PredType::NATIVE_INT;
#ifdef HAVE_POWERMATE
	      else if (param_name == "rotation")
		  param_type = PredType::NATIVE_INT;
#endif // HAVE_POWERMATE
	      else
		  param_type = PredType::NATIVE_DOUBLE;
	      qDebug () << "  Record for page" << page_name << "param" << param_name << "at" << offset;
	      record_type->insertMember (fmt.arg (page_name).arg (param_name).toUtf8 ().data (), offset, param_type);
	  }
      }
      qDebug () << "Trial record size:" << record_size;
  }


  // Setup an experiment item in the left toolbox
  xp_item = new QWidget;
  auto flayout = new QFormLayout;
  // Experiment name flayout->addRow ("Experiment", new QLabel (xp_name)); // Number of trials
  ntrials_spin = new QSpinBox;
  ntrials_spin->setRange (0, 8000);
  connect (ntrials_spin, SIGNAL (valueChanged (int)),
	   this, SLOT (set_trial_count (int)));
  flayout->addRow ("Number of trials", ntrials_spin);
  // Add spins for experiment specific parameters
#if 0
  lua_getglobal (lstate, "parameters");
  if (lua_istable (lstate, -1)) {
    lua_pushnil (lstate);
    while (lua_next (lstate, -2) != 0) {
      if (lua_isstring (lstate, -2)
	  && lua_istable (lstate, -1)) {
	QString param_name (lua_tostring (lstate, -2));

	lua_rawgeti (lstate, -1, 1);
	QString param_desc (lua_tostring (lstate, -1));
	lua_pop (lstate, 1);

	lua_rawgeti (lstate, -1, 2);
	auto param_unit = QString (" %1").arg(lua_tostring (lstate, -1));
	lua_pop (lstate, 1);

	auto spin = new QDoubleSpinBox;
	spin->setRange (-8000, 8000);
	spin->setSuffix (param_unit);
	spin->setProperty ("plstim_param_name", param_name);
	connect (spin, SIGNAL (valueChanged (double)),
		 this, SLOT (xp_param_changed (double)));
	flayout->addRow (param_desc, spin);
	
	param_spins[param_name] = spin;
      }
      lua_pop (lstate, 1);
    }
  }
  lua_pop (lstate, 1);
#endif
  // Finish the experiment item
  xp_item->setLayout (flayout);
  tbox->addItem (xp_item, "Experiment");

  set_trial_count (m_experiment->trialCount ());
#if 0
  // Fetch experiment parameters
  for (auto it : param_spins) {
    lua_getglobal (lstate, it.first.toUtf8().data ());
    if (lua_isnumber (lstate, -1))
      it.second->setValue (lua_tonumber (lstate, -1));
    lua_pop (lstate, 1);
  }
#endif

  xp_label->setText (script_path);


  // Setup a subject item in the left toolbox
  subject_item = new QWidget;
  flayout = new QFormLayout;
  subject_cbox = new QComboBox;
  subject_cbox->addItem (tr ("None"));
  flayout->addRow ("Subject", subject_cbox);
  connect (subject_cbox, SIGNAL (currentIndexChanged (const QString&)),
	   this, SLOT (subject_changed (const QString&)));
  subject_databutton = new QPushButton ( tr( "Subject data file"));
  subject_databutton->setDisabled (true);
  connect (subject_databutton, SIGNAL (clicked ()),
	   this, SLOT (select_subject_datafile ()));
  flayout->addRow ("Data file", subject_databutton);
  subject_item->setLayout (flayout);
  tbox->addItem (subject_item, "Subject");

  // Load the available subject ids for the experiment
  QString subject_path ("experiments/%1/subjects");
  settings->beginGroup (subject_path.arg (xp_name));
  for (auto s : settings->childKeys ())
    subject_cbox->addItem (s);
  settings->endGroup ();

  // Prepare for running the experiment
  tbox->setCurrentWidget (subject_item);

  // Initialise the experiment
  setup_updated ();

  return true;
}

void
QExperiment::xp_param_changed (double value)
{
  auto spin = qobject_cast<QDoubleSpinBox*> (sender());
  if (spin) {
    auto param = spin->property ("plstim_param_name").toString ();
#if 0
    lua_pushnumber (lstate, value);
    lua_setglobal (lstate, param.toUtf8 ().data ());
#endif
  }
}

void
QExperiment::set_trial_count (int num_trials)
{
  ntrials_spin->setValue (num_trials);
}

static const QRegExp session_name_re ("session_[0-9]+");

herr_t
find_session_maxi (hid_t group_id, const char * dset_name, void* data)
{
  (void) group_id;	// Unused

  QString dset (dset_name);

  // Check if the dataset is a session
  if (session_name_re.exactMatch (dset)) {
    int* maxi = (int*) data;
    int num = dset.right (dset.size () - 8).toInt ();
    if (num > *maxi)
      *maxi = num;
  }
  return 0;
}


void
QExperiment::init_session ()
{
  current_trial = 0;

  // Check if a subject datafile is opened
  if (hf != NULL) {
    // Parse the HDF5 datasets to get a block number
    int session_maxi = 0;
    int idx = 0;
    hf->iterateElems ("/", &idx, find_session_maxi, &session_maxi);
    session_number = session_maxi + 1;

    // Give a number to the current block
    auto session_name = QString ("session_%1").arg (session_number);

    // Create a new dataset for the block/session
    hsize_t htrials = m_experiment->trialCount ();
    DataSpace dspace (1, &htrials);
    dset = hf->createDataSet (session_name.toUtf8 ().data (),
			      *record_type, dspace);
    
    // Save subject ID
    auto subject = subject_cbox->currentText ().toUtf8 ();
    StrType str_type (PredType::C_S1, subject.size ());
    DataSpace scalar_space (H5S_SCALAR);
    dset.createAttribute ("subject", str_type, scalar_space)
      .write (str_type, subject.data ());

    // Save machine information
    auto hostname = QHostInfo::localHostName ().toUtf8 ();
    StrType sysname_type (PredType::C_S1, hostname.size ());
    dset.createAttribute ("hostname", sysname_type, scalar_space)
      .write (sysname_type, hostname.data ());

    // Save current date and time
    qint64 now = QDateTime::currentMSecsSinceEpoch ();
    dset.createAttribute ("datetime", PredType::STD_U64LE, scalar_space)
      .write (PredType::NATIVE_UINT64, &now);

    // Save key mapping
    size_t rowsize = sizeof (int) + sizeof (char*);
    char* km = new char[xp_keys.size () * rowsize];
    unsigned int i = 0;
    for (auto k : xp_keys) {
      auto utf_key = k.toUtf8 ();

      int* code = (int*) (km+rowsize*i);
      char** name = (char**) (km+rowsize*i+sizeof (int));

      *code = key_mapping[k];
      *name = new char[utf_key.size ()+1];
      strcpy (*name, utf_key.data ());
      i++;
    }
    StrType keys_type (PredType::C_S1, H5T_VARIABLE);
    keys_type.setCset (H5T_CSET_UTF8);
    hsize_t nkeys = xp_keys.size ();
    DataSpace kspace (1, &nkeys);

    CompType km_type (rowsize);
    km_type.insertMember ("code", 0, PredType::NATIVE_INT);
    km_type.insertMember ("name", sizeof (int), keys_type);
    dset.createAttribute ("keys", km_type, kspace)
      .write (km_type, km);

    hf->flush (H5F_SCOPE_GLOBAL);
    for (i = 0; i < nkeys; i++) {
      char** name = (char**) (km+rowsize*i+sizeof (int));
      //free (*name);
      delete [] (*name);
    }
    delete [] km;

#ifdef HAVE_EYELINK
    // Note, we do not record EyeTracker session if we 
    // do not have an HDF5 datafile were to reference it
    auto edf_name = QString ("plst-%1.edf").arg (session_number);
    qDebug () << "storing in temporary EDF" << edf_name;
    open_data_file (edf_name.toLocal8Bit ().data ());
    // Data stored in the EDF file
    eyecmd_printf ("file_sample_data = LEFT,RIGHT,GAZE,GAZERES,AREA,STATUS");
    eyecmd_printf ("file_event_filter = LEFT,RIGHT,FIXATION,SACCADE,BLINK,MESSAGE");
#endif // HAVE_EYELINK
  }
}

void
QExperiment::run_session ()
{
  // No experiment loaded
  if (! m_experiment) return;

  init_session ();
#ifdef HAVE_EYELINK
  // Run the calibration (no need to wait OpenGL full screen)
  calibrate_eyelink ();
  // Start recording eye movements
  start_recording (1, 1, 1, 1);
#endif // HAVE_EYELINK

  // Launch a stimulus window
  stim->showFullScreen (off_x_edit->value (), off_y_edit->value ());
  session_running = true;
  run_trial ();
}

void
QExperiment::runSessionInline ()
{
  // No experiment loaded
  if (! m_experiment) return;

  init_session ();
  int tex_size = m_experiment->textureSize ();
  stim->resize (tex_size, tex_size);
  stim->show ();
  //glwidget->setFocus (Qt::OtherFocusReason);
  session_running = true;
  run_trial ();
}

bool
QExperiment::clear_screen ()
{
  return false;
}

void
QExperiment::quit ()
{
  cout << "Goodbye!" << endl;
  app.quit ();
}

void
QExperiment::about_to_quit ()
{
  // Save the GUI state
  QSize sz = win->size ();
  settings->setValue ("gui_width", sz.width ());
  settings->setValue ("gui_height", sz.height ());
}

QSpinBox*
QExperiment::make_setup_spin (int min, int max, const char* suffix)
{
  auto spin = new QSpinBox;
  spin->setRange (min, max);
  spin->setSuffix (suffix);
  connect (spin, SIGNAL (valueChanged (int)),
	   this, SLOT (setup_param_changed ()));
  return spin;
}

QExperiment::QExperiment (int & argc, char** argv)
  : app (argc, argv),
    save_setup (false),
    bin_dist (0, 1),
    real_dist (0, 1),
    trial_record (NULL),
    record_size (0),
    hf (NULL)
{
  plstim::initialise ();

  win = new QMainWindow;
  m_component = NULL;
  m_experiment = NULL;
  xp_item = NULL;
  subject_item = NULL;
  record_type = NULL;

  session_running = false;
  creating_subject = false;

#ifdef HAVE_EYELINK
  eyelink_connected = false;
  waiting_fixation = false;
#ifdef DUMMY_EYELINK
  eyelink_dummy = true;
#else
  eyelink_dummy = false;
#endif
#endif

  // Mapping of key names to keys for Lua
  key_mapping["down"] = Qt::Key_Down;
  key_mapping["left"] = Qt::Key_Left;
  key_mapping["right"] = Qt::Key_Right;
  key_mapping["up"] = Qt::Key_Up;

  // Accepted keys for next page presentation
  nextPageKeys << Qt::Key_Return
	       << Qt::Key_Enter
	       << Qt::Key_Space;

  // Get the experimental setup

  stim = NULL;

  // Create a basic menu
  xp_menu = win->menuBar ()->addMenu (tr ("&Experiment"));
  // 
  auto action = xp_menu->addAction ("&Open");
  action->setShortcut (tr ("Ctrl+O"));
  action->setStatusTip (tr ("&Open an experiment"));
  connect (action, SIGNAL (triggered ()), this, SLOT (open ()));

  // Run an experiment in full screen
  action = xp_menu->addAction ("&Run");
  action->setShortcut (tr ("Ctrl+R"));
  action->setStatusTip (tr ("&Run the experiment"));
  connect (action, SIGNAL (triggered ()), this, SLOT (run_session ()));
  
  // Run an experiment inside the main window
  action = xp_menu->addAction ("Run &inline");
  action->setShortcut (tr ("Ctrl+Shift+R"));
  action->setStatusTip (tr ("&Run the experiment inside the window"));
  connect (action, SIGNAL (triggered ()), this, SLOT (runSessionInline ()));

  // Open an existing experiment
  action = xp_menu->addAction ("&Open");
  action->setShortcut (tr ("Ctrl+O"));
  action->setStatusTip (tr ("&Open an experiment"));
  connect (action, SIGNAL (triggered ()), this, SLOT (open ()));

  // Close the current simulation
  close_action = xp_menu->addAction ("&Close");
  close_action->setShortcut (tr ("Ctrl+W"));
  close_action->setStatusTip (tr ("&Close the experiment"));
  connect (close_action, SIGNAL (triggered ()), this, SLOT (unloadExperiment ()));
  xp_menu->addSeparator ();

  // Maximum number of recent experiments displayed
  max_recents = 5;
  recent_actions = new QAction*[max_recents];
  for (int i = 0; i < max_recents; i++) {
    recent_actions[i] = new QAction (xp_menu);
    recent_actions[i]->setVisible (false);
    xp_menu->addAction (recent_actions[i]);
    connect (recent_actions[i], SIGNAL (triggered ()),
	     this, SLOT (open_recent ()));
  }
  xp_menu->addSeparator ();

  // Terminate the program
  action = xp_menu->addAction ("&Quit");
  action->setShortcut(tr("Ctrl+Q"));
  action->setStatusTip(tr("&Quit the program"));
  connect (action, SIGNAL (triggered ()), this, SLOT (quit ()));

  // Setup menu
  auto menu = win->menuBar ()->addMenu (tr ("&Setup"));
  action = menu->addAction (tr ("&New"));
  action->setStatusTip (tr ("&Create a new setup"));
  connect (action, SIGNAL (triggered ()), this, SLOT (new_setup ()));

  // Subject menu
  menu = win->menuBar ()->addMenu (tr ("&Subject"));
  action = menu->addAction (tr ("&New"));
  action->setShortcut (tr ("Ctrl+N"));
  action->setStatusTip (tr ("&Create a new subject"));
  connect (action, SIGNAL (triggered ()), this, SLOT (new_subject ()));

  // Top toolbar
  //toolbar = new QToolBar ("Simulation");
  //action = 

  // Left toolbox
  tbox = new QToolBox;

  // Horizontal splitter (top: GLwidget, bottom: message box)
  hsplitter = new QSplitter (Qt::Vertical);
  hsplitter->addWidget (tbox);
  logtab = new QTabWidget;
  msgbox = new MyMessageBox;
  logtab->addTab (msgbox, "Messages");
  hsplitter->addWidget (logtab);

  win->setCentralWidget (hsplitter);

  // Status bar with current experiment name
  xp_label = new QLabel ("No experiment loaded");
  win->statusBar ()->addWidget (xp_label);

  // Experimental setup
  setup_item = new QWidget;
  setup_cbox = new QComboBox;
  connect (setup_cbox, SIGNAL (currentIndexChanged (const QString&)),
	   this, SLOT (update_setup ()));
  screen_sbox = new QSpinBox;
  connect (screen_sbox, SIGNAL (valueChanged (int)),
	   this, SLOT (setup_param_changed ()));
  // Set minimum to mode 13h, and maximum to 4320p
  off_x_edit = make_setup_spin (0, 7680, " px");
  off_y_edit = make_setup_spin (0, 4320, " px");
  res_x_edit = make_setup_spin (320, 7680, " px");
  res_y_edit = make_setup_spin (200, 4320, " px");
  phy_width_edit = make_setup_spin (10, 8000, " mm");
  phy_height_edit = make_setup_spin (10, 8000, " mm");
  dst_edit = make_setup_spin (10, 8000, " mm");
  lum_min_edit = make_setup_spin (1, 800, " cd/m²");
  lum_max_edit = make_setup_spin (1, 800, " cd/m²");
  refresh_edit = make_setup_spin (1, 300, " Hz"); 

  auto flayout = new QFormLayout;
  flayout->addRow ("Setup name", setup_cbox);
  flayout->addRow ("Screen", screen_sbox);
  flayout->addRow ("Horizontal offset", off_x_edit);
  flayout->addRow ("Vertical offset", off_y_edit);
  flayout->addRow ("Horizontal resolution", res_x_edit);
  flayout->addRow ("Vertical resolution", res_y_edit);
  flayout->addRow ("Physical width", phy_width_edit);
  flayout->addRow ("Physical height", phy_height_edit);
  flayout->addRow ("Distance", dst_edit);
  flayout->addRow ("Minimum luminance", lum_min_edit);
  flayout->addRow ("Maximum luminance", lum_max_edit);
  flayout->addRow ("Refresh rate", refresh_edit);
  setup_item->setLayout (flayout);
  tbox->addItem (setup_item, "Setup");
  
  /*
  auto gdsk = dsk.screenGeometry ();
  QString dsk_msg ("Found %1 screens in a %2 desktop of %3×%4");
  error (dsk_msg.arg (dsk.screenCount ()).arg (dsk.isVirtualDesktop () ? "virtual" : "normal").arg (gdsk.width ()).arg (gdsk.height ()));
  */

  stim = new StimWindow;
  connect (stim, &StimWindow::keyPressed,
	  this, &QExperiment::stimKeyPressed);
#ifdef HAVE_POWERMATE
  connect (stim, &StimWindow::powerMateRotation,
	   this, &QExperiment::powerMateRotation);
  connect (stim, &StimWindow::powerMateButtonPressed,
	   this, &QExperiment::powerMateButtonPressed);
#endif // HAVE_POWERMATE

  // Show OpenGL version in GUI

  // Try to fetch back setup
  settings = new QSettings;
  settings->beginGroup ("setups");
  auto groups = settings->childGroups ();

  // No setup previously defined, infer a new one
  if (groups.empty ()) {
    auto hostname = QHostInfo::localHostName ();
    qDebug () << "creating a new setup for" << hostname;
    setup_cbox->addItem (hostname);

    // Search for a secondary screen
    int i;
    for (i = 0; i < dsk.screenCount (); i++)
      if (i != dsk.primaryScreen ())
	break;

    // Get screen geometry
    auto geom = dsk.screenGeometry (i);
    qDebug () << "screen geometry: " << geom.width () << "x"
              << geom.height () << "+" << geom.x () << "+" << geom.y ();
    screen_sbox->setValue (i);
    res_x_edit->setValue (geom.width ());
    res_y_edit->setValue (geom.height ());
    settings->endGroup ();
  }

  // Use an existing setup
  else {
    // Add all the setups to the combo box
    for (auto g : groups) {
      if (g != "General") {
	setup_cbox->addItem (g);
      }
    }
    settings->endGroup ();

    // Check if there was a ‘last’ setup
    if (settings->contains ("last_setup")) {
      auto last = settings->value ("last_setup").toString ();
      // Make sure the setup still exists
      int idx = setup_cbox->findText (last);
      if (idx >= 0)
	setup_cbox->setCurrentIndex (idx);
    }
    
    // Restore setup
    update_setup ();
  }

  // Restore GUI state
  if (settings->contains ("gui_width"))
    win->resize (settings->value ("gui_width").toInt (),
		settings->value ("gui_height").toInt ());

  // Set recent experiments
  updateRecents ();

  // Constrain the screen selector
  screen_sbox->setMinimum (0);
  screen_sbox->setMaximum (dsk.screenCount () - 1);

  save_setup = true;

  // Close event termination
  connect (&app, SIGNAL (aboutToQuit ()),
	   this, SLOT (about_to_quit ()));

  // Make sure the timer is monotonic
  if (! timer.isMonotonic ())
    error (tr ("No monotonic timer available on this platform"));

  // Register QML types for PlStim
  qmlRegisterType<plstim::Vector> ("PlStim", 1, 0, "Vector");
  qmlRegisterType<plstim::Pen> ("PlStim", 1, 0, "Pen");
  qmlRegisterType<plstim::PainterPath> ("PlStim", 1, 0, "PainterPath");
  qmlRegisterUncreatableType<plstim::Painter> ("PlStim", 1, 0, "Painter", "Painter objects cannot be created from QML");
  qmlRegisterType<plstim::Page> ("PlStim", 1, 0, "Page");
  qmlRegisterType<plstim::Experiment> ("PlStim", 1, 0, "Experiment");
}

void
QExperiment::error (const QString& msg, const QString& desc)
{
    Q_UNUSED (desc);
    msgbox->add (new Message (MESSAGE_TYPE_ERROR, msg));
}

void
QExperiment::new_subject ()
{
  creating_subject = true;

  tbox->setCurrentWidget (subject_item);
  subject_cbox->setEditable (true);
  auto le = new MyLineEdit;
  subject_cbox->setLineEdit (le);
  le->clear ();

  connect (le, SIGNAL (returnPressed ()),
	   this, SLOT (new_subject_validated ()));
  connect (le, SIGNAL (escapePressed ()),
	   this, SLOT (new_subject_cancelled ()));

  QRegExp rx ("[a-zA-Z0-9\\-_]{1,10}");
  le->setValidator (new QRegExpValidator (rx));
  subject_cbox->setFocus (Qt::OtherFocusReason);
}

void
QExperiment::new_setup ()
{
  tbox->setCurrentWidget (setup_item);
  setup_cbox->setEditable (true);
  auto le = new MyLineEdit;
  setup_cbox->setLineEdit (le);
  le->clear ();

  connect (le, SIGNAL (returnPressed ()),
	   this, SLOT (new_setup_validated ()));
  connect (le, SIGNAL (escapePressed ()),
	   this, SLOT (new_setup_cancelled ()));

  QRegExp rx ("[a-zA-Z0-9\\-_]{1,10}");
  le->setValidator (new QRegExpValidator (rx));
  setup_cbox->setFocus (Qt::OtherFocusReason);
}

void
QExperiment::new_setup_validated ()
{
  auto le = qobject_cast<MyLineEdit*> (sender());
  auto setup_name = le->text ();
  auto setup_path = QString ("setups/%1").arg (setup_name);
  
  // Make sure the setup name is not already present
  if (settings->contains (setup_path)) {
    error (tr ("Setup name already existing"));
    return;
  }

  setup_cbox->setEditable (false);
}

void
QExperiment::new_setup_cancelled ()
{
  auto le = qobject_cast<MyLineEdit*> (sender());
  if (le) {
    le->clear ();
    setup_cbox->setEditable (false);
  }
}

void
QExperiment::new_subject_validated ()
{
  creating_subject = false;

  auto le = qobject_cast<MyLineEdit*> (sender());
  auto subject_id = le->text ();
  auto subject_path = QString ("experiments/%1/subjects/%2")
    .arg (xp_name).arg (subject_id);

  // Make sure the subject ID is not already present
  // and is not the ‘no subject’ option
  if (subject_cbox->currentIndex () == 0
      || settings->contains (subject_path)) {
    error (tr ("Subject ID already existing"));
    return;
  }

  // Set a default datafile for the subject
  auto subject_datafile = QString ("%1-%2.h5").arg (xp_name).arg (subject_id);
  QFileInfo fi (subject_datafile);
  int flags = H5F_ACC_EXCL;
  if (fi.exists ())
    flags = H5F_ACC_RDWR;

  // Set the datafile button info
  subject_databutton->setText (fi.fileName ());
  subject_databutton->setToolTip (fi.absoluteFilePath ());

  // Create the HDF5 file
  // TODO: handle HDF5 exceptions here
  if (hf != NULL) {
    hf->close ();
    hf = NULL;
  }
  hf = new H5File (subject_datafile.toLocal8Bit ().data (), flags);
  hf->flush (H5F_SCOPE_GLOBAL);

  subject_cbox->setEditable (false);

  // Create a new subject
  settings->setValue (subject_path, fi.absoluteFilePath ());
}

void
QExperiment::new_subject_cancelled ()
{
  creating_subject = false;

  auto le = qobject_cast<MyLineEdit*> (sender());
  if (le) {
    le->clear ();
    subject_cbox->setEditable (false);
  }
}

void
QExperiment::select_subject_datafile ()
{
  QFileDialog dialog (win, tr ("Select datafile"), last_datafile_dir);
  dialog.setDefaultSuffix (".h5");
  dialog.setNameFilter (tr ("Subject data (*.h5 *.hdf);;All files (*)"));
  if (dialog.exec () == QDialog::Accepted) {
    last_datafile_dir = dialog.directory ().absolutePath ();
    set_subject_datafile (dialog.selectedFiles ().at (0));
  }
}

void
QExperiment::subject_changed (const QString& subject_id)
{
  if (creating_subject)
    return;

  // Selection removal
  if (subject_cbox->currentIndex () == 0) {
    subject_databutton->setText (tr ("Subject data file"));
    subject_databutton->setToolTip (QString ());
    subject_databutton->setDisabled (true);
  }
  else {
    auto subject_path = QString ("experiments/%1/subjects/%2")
      .arg (xp_name).arg (subject_id);

    auto datafile = settings->value (subject_path).toString ();
    QFileInfo fi (datafile);

    // Make sure the datafile still exists
    if (! fi.exists ())
      error (tr ("Subject data file is missing"));
    // Open the HDF5 datafile
    else {
      // TODO: handle HDF5 exceptions here
      if (hf != NULL) {
	hf->close ();
	hf = NULL;
      }
      hf = new H5File (datafile.toLocal8Bit ().data (),
		       H5F_ACC_RDWR);
    }

    subject_databutton->setText (fi.fileName ());
    subject_databutton->setToolTip (fi.absoluteFilePath ());
    subject_databutton->setDisabled (false);
  }
}

void
QExperiment::set_subject_datafile (const QString& path)
{
  QFileInfo fi (path);
  subject_databutton->setText (fi.fileName ());
  subject_databutton->setToolTip (path);
}

void
QExperiment::setup_updated ()
{
  cout << "Setup updated" << endl;
  // Make sure converters are up to date

  if (m_experiment) {
    // Compute the best swap interval
#if 0
    lua_getglobal (lstate, "refresh");
    swap_interval = 1;
    if (lua_isnumber (lstate, -1)) {
      double wanted_freq = lua_tonumber (lstate, -1);
      double mon_rate = monitor_rate ();
      double coef = round (mon_rate / wanted_freq);
      if ((mon_rate/coef - wanted_freq)/wanted_freq> 0.01)
	qDebug () << "error: cannot set monitor frequency to 1% of desired frequency";
      qDebug () << "Swap interval:" << coef;
      //set_swap_interval (coef);
      swap_interval = coef;
    }
    lua_pop (lstate, 1);
#endif
    float distance = dst_edit->value ();

    float hres = (float) res_x_edit->value ()
	/ phy_width_edit->value ();
    float vres = (float) res_y_edit->value ()
	/ phy_height_edit->value ();

    float refreshRate = (float) refresh_edit->value ();

    m_experiment->setDistance (distance);
    m_experiment->setHorizontalResolution (hres);
    m_experiment->setVerticalResolution (vres);
    m_experiment->setRefreshRate (refreshRate);

    // Compute the minimal texture size
    float size_degs = m_experiment->size ();
    double size_px = ceil (m_experiment->degreesToPixels (size_degs));
    qDebug () << "Stimulus size:" << size_px << "from" << size_degs;

    // Minimal base-2 texture size
    int tex_size = 1 << (int) floor (log2 (size_px));
    if (tex_size < size_px)
	tex_size <<= 1;

    qDebug () << "Texture size:" << tex_size << "x" << tex_size;

    // Update experiment property
    m_experiment->setTextureSize (tex_size);

    // Make sure the new size is acceptable on the target screen
    auto geom = dsk.screenGeometry (screen_sbox->value ());
    if (tex_size > geom.width () || tex_size > geom.height ()) {
      // TODO: use the message GUI logging facilities
      qDebug () << "error: texture too big for the screen";
      return;
    }

    // Notify the GLWidget of a new texture size
    stim->setTextureSize (tex_size, tex_size);

    // Notify of setup changes
    emit m_experiment->setupUpdated ();

    // QImage and associated QPainter for frames painting
    QImage img (tex_size, tex_size, QImage::Format_RGB32);
    QPainter painter;

    swap_interval = 1;
    for (int i = 0; i < m_experiment->pageCount (); i++) {
	auto page = m_experiment->page (i);
	if (page->animated ()) {
	    // Make sure animated frames have updated number of frames
	    int nframes = (int) round ((monitor_rate () / swap_interval)*page->duration ()/1000.0);
	    qDebug () << "Displaying" << nframes << "frames for" << page->name ();
	    qDebug () << "  " << monitor_rate () << "/" << swap_interval;
	    page->setFrameCount (nframes);
	}

	if (page->paintTime () == Page::EXPERIMENT)
	    paintPage (page, img, painter);
    }
  }
}

void
QExperiment::updateRecents ()
{
  auto rlist = settings->value ("recents").toStringList ();
  int i = 0;
  QString label ("&%1 %2");
  for (auto path : rlist) {
    QFileInfo fi (path);
    QString name = fi.fileName ();
    if (name.endsWith (".qml"))
      name = name.left (name.size () - 4);
    recent_actions[i]->setText (label.arg (i+1).arg (name));
    recent_actions[i]->setData (path);
    recent_actions[i]->setVisible (true);
    i++;
  }
  while (i < max_recents)
    recent_actions[i++]->setVisible (false);
}

QExperiment::~QExperiment ()
{
    delete settings;
    delete stim;
    delete win;
}

void
QExperiment::setup_param_changed ()
{
  if (! save_setup)
    return;

  qDebug () << "changing setup param value";

  settings->beginGroup ("setups");
  settings->beginGroup (setup_cbox->currentText ());
  settings->setValue ("scr", screen_sbox->value ());
  settings->setValue ("off_x", off_x_edit->value ());
  settings->setValue ("off_y", off_y_edit->value ());
  settings->setValue ("res_x", res_x_edit->value ());
  settings->setValue ("res_y", res_y_edit->value ());
  settings->setValue ("phy_w", phy_width_edit->value ());
  settings->setValue ("phy_h", phy_height_edit->value ());
  settings->setValue ("dst", dst_edit->value ());
  settings->setValue ("lmin", lum_min_edit->value ());
  settings->setValue ("lmax", lum_max_edit->value ());
  settings->setValue ("rate", refresh_edit->value ());
  settings->endGroup ();
  settings->endGroup ();

  // Emit the signal
  setup_updated ();
}

void
QExperiment::update_setup ()
{
  auto sname = setup_cbox->currentText ();
  qDebug () << "restoring setup for" << sname;

  settings->beginGroup ("setups");
  settings->beginGroup (sname);
  screen_sbox->setValue (settings->value ("scr").toInt ());
  off_x_edit->setValue (settings->value ("off_x").toInt ());
  off_y_edit->setValue (settings->value ("off_y").toInt ());
  res_x_edit->setValue (settings->value ("res_x").toInt ());
  res_y_edit->setValue (settings->value ("res_y").toInt ());
  phy_width_edit->setValue (settings->value ("phy_w").toInt ());
  phy_height_edit->setValue (settings->value ("phy_h").toInt ());
  dst_edit->setValue (settings->value ("dst").toInt ());
  lum_min_edit->setValue (settings->value ("lmin").toInt ());
  lum_max_edit->setValue (settings->value ("lmax").toInt ());
  refresh_edit->setValue (settings->value ("rate").toInt ());
  settings->endGroup ();
  settings->endGroup ();

  settings->setValue ("last_setup", sname);

  setup_updated ();
}

float
QExperiment::monitor_rate () const
{
  return (float) refresh_edit->value ();
}

void
QExperiment::open ()
{
  QFileDialog dialog (win, tr ("Open experiment"), last_dialog_dir);
  dialog.setFileMode (QFileDialog::ExistingFile);
  dialog.setNameFilter (tr ("Experiment (*.lua);;All files (*)"));

  if (dialog.exec () == QDialog::Accepted) {
    last_dialog_dir = dialog.directory ().absolutePath ();
    load_experiment (dialog.selectedFiles ().at (0));
  }
}
