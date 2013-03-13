// plstim/engine.cc – Base class for experimental stimuli


#include <iomanip>
#include <iostream>
#include <sstream>
using namespace std;

#include "engine.h"
using namespace plstim;



#include <QtCore>
#include <QHostInfo>
#include <QtQuick>

using namespace H5;


Error*
Engine::error (const QString& title, const QString& description)
{
    auto err = new Error (title, description, this);
    m_errors << err;
    emit errorsChanged ();
    return err;
}

void
Engine::paintPage (Page* page, QImage& img, QPainter& painter)
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
Engine::run_trial ()
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
  memset (trial_record, 0, record_size);
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
    eyemsg_printf ("trial %d", m_currentTrial+1);
#endif // HAVE_EYELINK

  current_page = -1;
  nextPage ();
}

void
Engine::show_page (int index)
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
                qDebug () << "Elapsed for" << page->name () << ":" << *pos;
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
  if (page->fixation ()) {
    qDebug () << "page" << page->name () << "wants a" << page->fixation () << "ms fixation";
    QElapsedTimer timer;
    // Target
    float tx = m_setup.horizontalResolution () / 2.0;
    float ty = m_setup.verticalResolution () / 2.0;
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
      else {
	  QCoreApplication::instance ()->processEvents ();
      }
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
Engine::nextPage ()
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

    // End of trial
    auto page = current_page < 0 ? NULL : m_experiment->page (current_page);
    if ((page && page->last ())
            || current_page + 1 == m_experiment->pageCount ()) {
        qDebug () << "End of trial " << m_currentTrial << "of" << m_experiment->trialCount ();

        // Save the page record on HDF5
        if (hf != NULL && current_page >= 0) {
            hsize_t one = 1;
            DataSpace fspace = dset.getSpace ();
            hsize_t hframe = m_currentTrial;
            fspace.selectHyperslab (H5S_SELECT_SET, &one, &hframe);
            DataSpace mspace (1, &one);
            dset.write (trial_record, *record_type, mspace, fspace);

            hf->flush (H5F_SCOPE_GLOBAL);	// Store everything on file
        }

        // Next trial
        if (m_currentTrial + 1 < m_experiment->trialCount ()) {
            setCurrentTrial (m_currentTrial + 1);
            run_trial ();
        }
        // End of session
        else
            endSession ();
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
Engine::savePageParameter (const QString& pageTitle,
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
Engine::powerMateRotation (PowerMateEvent* evt)
{
    if (! m_running)
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
Engine::powerMateButtonPressed (PowerMateEvent* evt)
{
    Q_UNUSED (evt);
    //qDebug () << "PowerMate button pressed!";

    if (! m_running)
	return;

    auto page = m_experiment->page (current_page);
    if (page->waitKey () && page->acceptAnyKey ()) {
	qDebug () << "powermate button → next page";
	nextPage ();
    }
}
#endif // HAVE_POWERMATE

void
Engine::stimKeyPressed (QKeyEvent* evt)
{
  // Keyboard events are only handled in sessions
  if (! m_running)
    return;

  auto page = m_experiment->page (current_page);
  
  // Go to the next page
  if (page->waitKey ()) {
#ifdef HAVE_EYELINK
    // Allows EyeLink calibration and validation
    if (page->fixation ()
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
Engine::endSession ()
{
    if (hf != NULL) {
#if HAVE_EYELINK
	if (m_eyelinkRecording) {
	    // Stop recording
	    stop_recording ();
	    // Choose a name for the local EDF
	    auto subject_id = m_subjectName;
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
	    m_eyelinkRecording = false;
	}
#endif // HAVE_EYELINK
        hf->flush (H5F_SCOPE_GLOBAL);	// Store everything on file
    }
    current_page = -1;
    stim->hide ();
    setRunning (false);
}

void
Engine::unloadExperiment ()
{
  endSession ();

  m_experiment = NULL;
  experimentChanged (NULL);

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
}

bool
Engine::loadExperiment (const QString& path)
{
  // Cleanup any existing experiment
  unloadExperiment ();


  // Canonicalise the experiment description path
  QFileInfo fileinfo (path);
  
  // Get the experiment short name
  xp_name = fileinfo.fileName ();
  if (xp_name.endsWith (".json"))
    xp_name = xp_name.left (xp_name.size () - 5);

  // Load the JSON description of the experiment
  QFile f (path);
  f.open (QIODevice::ReadOnly);
  QByteArray ba = f.readAll ();
  f.close ();
  QJsonParseError jerr;
  m_json = QJsonDocument::fromJson (ba, &jerr);
  if (jerr.error != QJsonParseError::NoError) {
      error ("Could not parse the JSON description",
	      jerr.errorString ());
      unloadExperiment ();
      return false;
  }
  if (! m_json.isObject ()) {
      error ("Invalid JSON description",
	      "JSON root is not an object");
      unloadExperiment ();
      return false;
  }
  QJsonObject jroot = m_json.object ();
  QString qmlPath = jroot["Source"].toString ();

  m_component = new QQmlComponent (&m_engine, qmlPath);
  if (m_component->isError ()) {
      QString errDesc;
      for (auto& err : m_component->errors ())
	  errDesc.append (err.toString ()).append ("\n");
      error ("Could not load the QML experiment", errDesc);
      unloadExperiment ();
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
  m_experiment->setSetup (&m_setup);

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
              param_type = PredType::NATIVE_INT64;
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


  set_trial_count (m_experiment->trialCount ());
  experimentChanged (m_experiment);

  // Initialise the experiment
  setup_updated ();

  return true;
}

void
Engine::set_trial_count (int num_trials)
{
  //ntrials_spin->setValue (num_trials);
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
Engine::init_session ()
{
    setCurrentTrial (0);


    // Disable screensaver
#ifdef HAVE_WIN32
    SystemParametersInfo (SPI_SETSCREENSAVEACTIVE, FALSE, NULL, 0);
#else // HAVE_WIN32
#endif // HAVE_WIN32

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
    
    // Save subject ID TODO: restore subject name
    auto subject = QString (m_subjectName).toUtf8 ();
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
    if (! xp_keys.empty ()) {
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
    }

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
Engine::runSession ()
{
  // No experiment loaded
  if (! m_experiment) return;

  init_session ();
#ifdef HAVE_EYELINK
  // Run the calibration (no need to wait OpenGL full screen)
  calibrate_eyelink ();
  // Start recording eye movements
  m_eyelinkRecording = true;
  start_recording (1, 1, 1, 1);
#endif // HAVE_EYELINK

  // Launch a stimulus window
  stim->setScreen (displayScreen ());
  stim->showFullScreen ();
  setRunning (true);
  run_trial ();
}

void
Engine::runSessionInline ()
{
  // No experiment loaded
  if (! m_experiment) return;

  init_session ();
  int tex_size = m_experiment->textureSize ();
  stim->resize (tex_size, tex_size);
  stim->show ();
  //glwidget->setFocus (Qt::OtherFocusReason);
  setRunning (true);
  run_trial ();
}

void
Engine::quit ()
{
    QCoreApplication::instance ()->quit ();
}

void
Engine::about_to_quit ()
{
    qDebug () << "About to quit";
    endSession ();
    if (hf != NULL) {
	hf->close ();
	hf = NULL;
    }
}

Engine::Engine ()
  : save_setup (false),
    bin_dist (0, 1),
    real_dist (0, 1),
    trial_record (NULL),
    record_size (0),
    hf (NULL)
{
    plstim::initialise ();

    m_component = NULL;
    m_experiment = NULL;
    record_type = NULL;

    m_running = false;

#ifdef HAVE_EYELINK
    m_eyelinkRecording = false;
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

    stim = new StimWindow;
    connect (stim, &StimWindow::keyPressed,
	    this, &Engine::stimKeyPressed);
#ifdef HAVE_POWERMATE
    connect (stim, &StimWindow::powerMateRotation,
	    this, &Engine::powerMateRotation);
    connect (stim, &StimWindow::powerMateButtonPressed,
	    this, &Engine::powerMateButtonPressed);
#endif // HAVE_POWERMATE

    // Try to fetch back setup
    m_settings = new QSettings;
    m_settings->beginGroup ("setups");
    auto groups = m_settings->childGroups ();
    m_settings->endGroup ();

    // No setup previously defined, infer a new one
    if (groups.empty ()) {
	auto hostname = QHostInfo::localHostName ();
	qDebug () << "creating a new setup for" << hostname;
#if 0
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
#endif
    }

    // Use an existing setup
    else {
	QString setupName = m_settings->contains ("lastSetup") ?
	    m_settings->value ("lastSetup").toString () :
	    groups.at (0);
	loadSetup (setupName);
    }

    save_setup = true;

    // Close event termination
    connect (QCoreApplication::instance (), SIGNAL (aboutToQuit ()),
	    this, SLOT (about_to_quit ()));
}

QVariant
Engine::evaluate (const QString& expression)
{
    /*QString expression ("2*3");
    // No experiment loaded
    if (m_component == NULL)
	return QVariant ();

    QQmlExpression expr (m_engine.rootContext (),
	    m_experiment, expression);
    qDebug () << "Evaluating" << expr.expression ()
	      << "error:" << expr.hasError ();
    return expr.evaluate ();*/

    auto expr = expression.trimmed ();

    if (expr == "currentTrial") {
	qDebug () << "Returning current trial";
	return QVariant::fromValue (currentTrial ());
    }
    return QVariant ();
}

#if 0
void
Engine::new_subject_validated ()
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
#endif

void
Engine::selectSubject (const QString& subjectName)
{
    qDebug () << "Loading subject" << subjectName;


    // No subject list found
    auto subjects = m_json.object ()["Subjects"];
    if (! subjects.isObject ()) {
        return;
    }

    // Subject not found
    auto subject = subjects.toObject ()[subjectName].toObject ();
    if (! subject.contains ("Data")) {
        return;
    }

    auto datafile = subject["Data"].toString ();
    QFile fi (datafile);

    // Make sure the datafile still exists
    if (! fi.exists ()) {
        // Try to create a data file
        bool ok = fi.open (QIODevice::WriteOnly);
        if (ok) {
            qDebug () << "Subject data file created";
            fi.close ();
        }
        else {
            error (tr ("Could not create subject data file"));
            return;
        }
    }
    // Open the HDF5 datafile
    // TODO: handle HDF5 exceptions here
    if (hf != NULL) {
        hf->close ();
        hf = NULL;
    }
    hf = new H5File (datafile.toLocal8Bit ().data (), H5F_ACC_RDWR);
    m_subjectName = subjectName;
    qDebug () << "Subject data file loaded";

    // Load subject parameters
    if (subject.contains ("Parameters")) {
	auto subjectParams = subject["Parameters"].toObject ();

	auto& params = m_experiment->subjectParameters ();
	QMapIterator<QString,QVariant> it (params);
	while (it.hasNext ()) {
	    it.next ();
	    auto& paramName = it.key ();
	    qDebug () << "Trying to load subject parameter" << paramName;
	    if (! subjectParams.contains (paramName)) {
		qDebug () << "WARNING: Missing subject parameter" << paramName;
	    }
	    else {
		QVariant currentValue = m_experiment->property (paramName.toUtf8 ().data ());
		if (! currentValue.isValid ()) {
		    qDebug () << "WARNING: Trying to set subject property" << paramName << "which is not found in the experiment";
		}
		else {
		    if (! subjectParams[paramName].isDouble ()) {
			qDebug () << "WARNING: Subject parameter" << paramName << "is not a floating number";
		    }
		    else {
			m_experiment->setProperty (paramName.toUtf8 ().data (), subjectParams[paramName].toDouble ());
		    }
		}
	    }
	}
    }
    //emit subjectLoaded (subjectName);
}

void
Engine::setup_updated ()
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

    // Compute the minimal texture size
    float size_degs = m_experiment->size ();
    double size_px = ceil (m_experiment->degreesToPixels (size_degs));

    // Minimal base-2 texture size
    int tex_size = 1 << (int) floor (log2 (size_px));
    if (tex_size < size_px)
	tex_size <<= 1;

    qDebug () << "Texture size:" << tex_size << "x" << tex_size;

    // Update experiment property
    m_experiment->setTextureSize (tex_size);

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
	    int nframes = (int) round ((m_setup.refreshRate () / swap_interval)*page->duration ()/1000.0);
	    qDebug () << "Displaying" << nframes << "frames for" << page->name ();
	    qDebug () << "  " << m_setup.refreshRate () << "/" << swap_interval;
	    page->setFrameCount (nframes);
	}

	if (page->paintTime () == Page::EXPERIMENT)
	    paintPage (page, img, painter);
    }
  }

  //emit setupUpdated (&setup);
}

Engine::~Engine ()
{
    delete m_settings;
    delete stim;
}

void
Engine::loadSetup (const QString& setupName)
{
  // Load the setup from the settings
  m_settings->beginGroup (QString ("setups/%1").arg (setupName));
  m_setup.setName (setupName);
  m_setup.setPhysicalWidth (m_settings->value ("phy_w").toInt ());
  m_setup.setPhysicalHeight (m_settings->value ("phy_h").toInt ());
  m_setup.setHorizontalResolution (m_settings->value ("res_x").toInt ());
  m_setup.setVerticalResolution (m_settings->value ("res_y").toInt ());

  m_setup.setDistance (m_settings->value ("dst").toInt ());
  m_setup.setRefreshRate (m_settings->value ("rate").toFloat ());

  m_settings->endGroup ();

  // Save the setup as the last one used
  m_settings->setValue ("lastSetup", setupName);

  setup_updated ();
}

QScreen*
Engine::displayScreen ()
{
  QScreen* scr = stim->screen ();
  for (auto s : scr->virtualSiblings ())
      if (s->availableGeometry ().x () != 0
              || s->availableGeometry ().y () != 0)
          return s;
  return scr;
}
