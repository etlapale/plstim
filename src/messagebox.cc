// src/messagebox.cc – Message box displayer for plstim

#include "messagebox.h"
using namespace plstim;

Message::Message (MessageType t, const QString& str)
  : type(t), title (str)
{
}

Message::Message (MessageType t, const QString& str, const QString& desc)
  : type(t), title (str), description (desc)
{
}

MessageEntry::MessageEntry (Message* msg)
{
  layout = new QGridLayout;
  layout->setColumnStretch (1, 1);
  setLayout (layout);

  // Icon
  icon = new QLabel;
  QStyle::StandardPixmap spix;
  
  switch (msg->type) {
  case MESSAGE_TYPE_WARNING:
    spix = QStyle::SP_MessageBoxWarning;
    break;
  case MESSAGE_TYPE_ERROR:
  default:
    spix = QStyle::SP_MessageBoxCritical;
    break;
  }

  icon->setPixmap (style ()->standardIcon (spix).pixmap (64, 64));
  layout->addWidget (icon, 0, 0);

  // Title
  QString fmt_title ("<b>%1</b>");
  title = new QLabel (fmt_title.arg (msg->title));
  layout->addWidget (title, 0, 1);

  // Description
  description = new QLabel (msg->description);
  layout->addWidget (description, 1, 1);
}

MyMessageBox::MyMessageBox (QWidget* parent)
  : QWidget (parent)
{
  layout = new QVBoxLayout;
  setLayout (layout);
}

void
MyMessageBox::add (Message* msg)
{
  // Store the message
  messages << msg;

  // Add a GUI entry
  auto entry = new MessageEntry (msg);
  layout->addWidget (entry);

  // Mark associated widgets
  for (auto w : msg->widgets) {
    auto p = w->palette ();
    if (msg->type == MESSAGE_TYPE_WARNING)
      p.setColor (QPalette::Base, QColor (0xfe, 0xc9, 0x7d));
    else
      p.setColor (QPalette::Base, QColor (0xfe, 0xab, 0xa0));
    w->setPalette (p);
  }
}

void
MyMessageBox::remove (Message* msg)
{
  messages.removeOne (msg);

  // TODO: restore to the previous colour
  // TODO: make sure no other message changed the colour
  for (auto w : msg->widgets) {
    auto p = w->palette ();
    p.setColor (QPalette::Base, QColor (0xff, 0xff, 0xff));
    w->setPalette (p);
  }
}
