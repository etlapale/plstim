  // Stimulus configuration
#if 0
  float lw_deg = sec2deg (1.02);
  lw = setup->deg2pix (lw_deg);
  cout << "Line width is " << lw_deg << " degrees, or "
       << lw << " pixels" << endl;
  float bg_cd = 20;
  bg = setup->lum2px (bg_cd);
  cout << "Background luminance is " << bg_cd << " cd/m², or "
       << bg << " in [0,1]" << endl;
#endif

  // Composition of a trial
  /*
  que_page->accept_key (Qt::Key_Up);
  que_page->accept_key (Qt::Key_Down);
	   */

void
LorenceauExperiment::update_configuration ()
{
  // Aperture mask path
  QPainterPath ape_path;
  ape_path.addRect (0, 0, tex_width, tex_height);
  ape_path.addEllipse ((int) (tex_width/2 - ap_diam_px/2),
		 (int) (tex_height/2 - ap_diam_px/2),
		 (int) ap_diam_px, (int) ap_diam_px);

  QColor bg (10, 10, 100);

  // Fixation frame
  QPainter p (img);
  //p.fillRect (0, 0, tex_width, tex_height, QColor (10, 10, 0));
  p.fillRect (0, 0, tex_width, tex_height, bg);
  p.setBrush (Qt::red);
  int fix_radius = (int) nearbyint (deg2pix (3./60));
  qDebug () << "Fixation radius:" << fix_radius;
  p.drawEllipse (tex_width/2 - fix_radius, tex_height/2 - fix_radius,
		 2*fix_radius, 2*fix_radius);
  p.setBrush (Qt::green);
  p.drawPath (ape_path);
  //p.fillRect (0, 0, 10, 30, QColor (200, 10, 100));
  

  // Question frame
  p.setBrush (Qt::green);
  p.drawPath (ape_path);
}

#if 0
bool
LorenceauExperiment::run_trial ()
{
  cout << "RUN TRIAL (NYI)" << endl;
  // Contrast
  // C%→L: C₁₂→13.44, C₂₅→15, C₃₉→16.68, C₅₂→18.24, C₇₀→20.4
  float fg_cd = luminances [lum_dist (twister)];
  fg = setup->lum2px (fg_cd);
  cout << "Foreground luminance is " << fg_cd << " cd/m², or "
       << fg << " in [0,1], or "
       << (int) (fg*255) << " in [0,255]" << endl;
  // Line length
  float ll_deg = 2.7;
  ll = setup->deg2pix (ll_deg);
  cout << "Line length is " << ll_deg << " degrees, or "
       << ll << " pixels" << endl;
#endif

#if 0
  // Wait for a key press before running the trial
  struct timespec tp_fixation;
  clock_gettime (CLOCK, &tp_fixation);

  // Display each frame of the trial
  struct timespec tp_frames;
  clock_gettime (CLOCK, &tp_frames);

  // Wait for a key press at the end of the trial
  struct timespec tp_question;
  clock_gettime (CLOCK, &tp_question);
  printf ("start: %lds %ldns\n",
	  tp_frames.tv_sec, tp_frames.tv_nsec);
  printf ("stop:  %lds %ldns\n",
	  tp_question.tv_sec, tp_question.tv_nsec);

  struct timespec tp_answer;
  clock_gettime (CLOCK, &tp_answer);

  // Prepare results
  uint8_t config = 0;
  if (cw) config |= LORENCEAU_CONFIG_CW;
  if (control) config |= LORENCEAU_CONFIG_CONTROL;
  if (up) config |= LORENCEAU_CONFIG_UP;
  SessionResult result = {
    tp_fixation.tv_sec, tp_fixation.tv_nsec,
    tp_frames.tv_sec, tp_frames.tv_nsec,
    tp_question.tv_sec, tp_question.tv_nsec,
    tp_answer.tv_sec, tp_answer.tv_nsec,
    config,
    fg_cd,
    ll_deg,
    (int) dur_ms ,
    pressed_key == XK_Up,
  };

  // Write result slab
  hsize_t one = 1;
  DataSpace fspace = dset.getSpace();
  hsize_t hframe = current_trial;
  fspace.selectHyperslab (H5S_SELECT_SET, &one, &hframe);
  DataSpace mspace (1, &one);
  dset.write (&result, session_type, mspace, fspace);

  // Flush to commit trial
  hf->flush (H5F_SCOPE_GLOBAL);
}
#endif

void
LorenceauExperiment::make_frames ()
{
  // Remove the existing frames
  glwidget->delete_unamed_frames ();

  QImage img (tex_width, tex_height, QImage::Format_RGB32);
  cout << "tex dims: " << tex_width << "×" << tex_height << endl;

  QColor bg (10, 10, 100);
  
  // Aperture mask path
  QPainterPath ape_path;
  ape_path.addRect (0, 0, tex_width, tex_height);
  ape_path.addEllipse ((int) (tex_width/2 - ap_diam_px/2),
		 (int) (tex_height/2 - ap_diam_px/2),
		 (int) ap_diam_px, (int) ap_diam_px);

  // Fixation frame
  QPainter p;

  // Set a random trial condition
  up = bin_dist (twister);
  cw = bin_dist (twister);
  control = bin_dist (twister);

  int offx = (tex_width-ap_diam_px)/2;
  int offy = (tex_height-ap_diam_px)/2;
  // Line speed and direction
  cout << "Clockwise: " << cw << endl
       << "Control: " << control << endl
       << "Up: " << up << endl;
  float orient = radians (cw ? 110 : 70);
  int bx = (int) (ll_px*cos(orient));
  int by = (int) (ll_px*sin(orient));
  int sx = abs (bx);
  int sy = abs (by);
  float direction = fmod (orient
    + (orient < M_PI/2 ? -1 : +1) * (M_PI/2)	// Orientation
    + (up ? 0 : 1) * M_PI			// Up/down
    + (orient < M_PI/2 ? -1 : 1) * (control ? 0 : 1) * radians (140)
    , 2*M_PI);
  cout << "Line direction: " << degrees (direction) << "°" << endl;
  float speed = ds2pf (6.5);
  int dx = (int) (speed*cos (direction));
  int dy = (int) (speed*sin (direction));
  cout << "Line speed is 6.5 deg/s, or "
    << speed << " px/frame ("
    << (dx >= 0 ? "+" : "") << dx
    << (dy >= 0 ? "+" : "") << dy
    << ")" << endl;
  float spacing = deg2pix (1);
  cout << "sx+spacing: " << sx << "+" << spacing << endl;

  for (int i = 0; i < nframes; i++) {
    cout << "beginning frame " << i << endl;
    p.begin (&img);
    cout << "  img began" << endl;

    // Background
    p.fillRect (0, 0, tex_width, tex_height, bg);
    cout << "  bg filled" << endl;

    // Lines
#if 0
    cr->set_line_width (lw);
    cr->set_source_rgb (fg, fg, fg);
#endif
    QPen no_pen;
    QPen lines_pen (Qt::white);
    p.setPen (lines_pen);
    for (int x = offx+i*dx - (sx+spacing); x < tex_width+sx+spacing; x+=sx+spacing) {
      for (int y = offy+i*dy; y < tex_height+sy+spacing; y+=sy+spacing) {
	p.drawLine (x, y, x+bx, y+by);
      }
    }
    
    // Fixation point
    p.setBrush (Qt::red);
    p.setPen (no_pen);
    cout << "  brush set" << endl;
    int fix_radius = (int) nearbyint (deg2pix (3./60));
    qDebug () << "Fixation radius:" << fix_radius;
    p.drawEllipse (tex_width/2 - fix_radius,
		   tex_height/2 - fix_radius,
		   2*fix_radius, 2*fix_radius);

    // Aperture
    p.setBrush (Qt::red);
    p.drawPath (ape_path);

    // Save as OpenGL texture
    p.end ();
    glwidget->add_frame (img);
    img.save ("frame.png");
  }
}

// vim:sw=2:
