// src/messagebox.cc – Message box displayer for plstim

#include "messagebox.h"
using namespace plstim;


Message::Message (Type t, const QString& str)
  : type(t), msg (str)
{
}

MessageBox::MessageBox (QWidget* parent)
  : QWidget (parent)
{
}

void
MessageBox::add (Message* msg)
{
  // Store the message
  messages << msg;

  // Mark associated widgets
  for (auto w : msg->widgets) {
    auto p = w->palette ();
    if (msg->type == Message::Type::WARNING)
      p.setColor (QPalette::Base, QColor (0xfe, 0xc9, 0x7d));
    else
      p.setColor (QPalette::Base, QColor (0xfe, 0xab, 0xa0));
    w->setPalette (p);
  }

  qDebug ()
    << (msg->type == Message::Type::ERROR ? "error:" : "warning:" )
    << msg->msg;
}

void
MessageBox::remove (Message* msg)
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
