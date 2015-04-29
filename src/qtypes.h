// src/qtypes.h – Define custom QML types
//
// Copyright © 2012–2015 University of California, Irvine
// Licensed under the Simplified BSD License.

#pragma once

#include <QtCore>

namespace plstim
{
class Error : public QObject
{
  Q_OBJECT
  Q_PROPERTY (QString title READ title NOTIFY titleChanged)
  Q_PROPERTY (QString description READ description NOTIFY descriptionChanged)

public:
  Error (const QString& title="", const QString& description="",
	 QObject* parent=nullptr)
    : QObject (parent)
    , m_title (title)
    , m_description (description)
  {}

  QString title () const
  { return m_title; }

  QString description () const
  { return m_description; }

protected:
  QString m_title;
  QString m_description;

signals:
  void titleChanged (const QString& title);
  void descriptionChanged (const QString& description);
};

} // namespace plstim

// Local Variables:
// mode: c++
// End:
