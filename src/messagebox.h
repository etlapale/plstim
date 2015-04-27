// messagebox.h – Message box displayer for plstim

#ifndef __PLSTIM_MESSAGEBOX_H
#define __PLSTIM_MESSAGEBOX_H

#include <QtGui>


namespace plstim
{
  enum MessageType {
    MESSAGE_TYPE_WARNING,
    MESSAGE_TYPE_ERROR
  };
  class Message {
  public:
    Message (MessageType t, const QString& title);
    Message (MessageType t, const QString& title, const QString& description);
  public:
    MessageType type;
    QString title;
    QString description;
    QList<QWidget*> widgets;
  };

  class MessageEntry : public QWidget {
  public:
    MessageEntry (Message* msg);
  protected:
    QGridLayout* layout;
    QLabel* icon;
    QLabel* title;
    QLabel* description;
  };

  class MessageBox : public QWidget {
  public:
    MessageBox (QWidget* parent=NULL);
    void add (Message* msg);
    void remove (Message* msg);
  protected:
    /// List of current messages
    QList<Message*> messages;
    QVBoxLayout* layout;
  };
}

#endif
