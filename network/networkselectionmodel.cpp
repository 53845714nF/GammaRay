#include "networkselectionmodel.h"
#include "message.h"
#include "endpoint.h"

#include <QDebug>

using namespace GammaRay;

NetworkSelectionModel::NetworkSelectionModel(const QString &objectName, QAbstractItemModel* model, QObject* parent):
  QItemSelectionModel(model, parent),
  m_objectName(objectName),
  m_myAddress(Protocol::InvalidObjectAddress)
{
  connect(this, SIGNAL(currentChanged(QModelIndex,QModelIndex)), this, SLOT(slotCurrentChanged(QModelIndex,QModelIndex)));
  connect(this, SIGNAL(currentColumnChanged(QModelIndex,QModelIndex)), this, SLOT(slotCurrentColumnChanged(QModelIndex,QModelIndex)));
  connect(this, SIGNAL(currentRowChanged(QModelIndex,QModelIndex)), this, SLOT(slotCurrentRowChanged(QModelIndex,QModelIndex)));
  connect(this, SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(slotSelectionChanged(QItemSelection,QItemSelection)));
}

NetworkSelectionModel::~NetworkSelectionModel()
{
}

void NetworkSelectionModel::newMessage(const Message& msg)
{
  Q_ASSERT(msg.address() == m_myAddress);
  switch (msg.type()) {
    case Protocol::SelectionModelSelect:
    {
      qint32 flags, size;
      msg.payload() >> flags >> size;
      QItemSelection selection;
      selection.reserve(size);
      for (int i = 0; i < size; ++i) {
        Protocol::ModelIndex topLeft, bottomRight;
        msg.payload() >> topLeft >> bottomRight;
        const QModelIndex qmiTopLeft = Protocol::toQModelIndex(model(), topLeft);
        const QModelIndex qmiBottomRight = Protocol::toQModelIndex(model(), bottomRight);
        if (!qmiTopLeft.isValid() && !qmiBottomRight.isValid())
          continue;
        selection.push_back(QItemSelectionRange(qmiTopLeft, qmiBottomRight));
      }
      select(selection, QItemSelectionModel::SelectionFlags(flags));
      break;
    }
    case Protocol::SelectionModelCurrent:
    {
      qint32 flags;
      Protocol::ModelIndex index;
      msg.payload() >> flags >> index;
      const QModelIndex qmi = Protocol::toQModelIndex(model(), index);
      if (!qmi.isValid())
        break;
      setCurrentIndex(qmi, QItemSelectionModel::SelectionFlags(flags));
      break;
    }
    default:
      Q_ASSERT(false);
  }
}

void NetworkSelectionModel::slotCurrentChanged(const QModelIndex& current, const QModelIndex& previous)
{
  Q_UNUSED(previous);
  if (!Endpoint::isConnected())
    return;
  Message msg(m_myAddress, Protocol::SelectionModelCurrent);
  msg.payload() << qint32(QItemSelectionModel::Current) << Protocol::fromQModelIndex(current);
  Endpoint::send(msg);
}

void NetworkSelectionModel::slotCurrentColumnChanged(const QModelIndex& current, const QModelIndex& previous)
{
  Q_UNUSED(previous);
  if (!Endpoint::isConnected())
    return;
  Message msg(m_myAddress, Protocol::SelectionModelCurrent);
  msg.payload() << qint32(QItemSelectionModel::Current|QItemSelectionModel::Columns) << Protocol::fromQModelIndex(current);
  Endpoint::send(msg);
}

void NetworkSelectionModel::slotCurrentRowChanged(const QModelIndex& current, const QModelIndex& previous)
{
  Q_UNUSED(previous);
  if (!Endpoint::isConnected())
    return;
  Message msg(m_myAddress, Protocol::SelectionModelCurrent);
  msg.payload() << qint32(QItemSelectionModel::Current|QItemSelectionModel::Rows) << Protocol::fromQModelIndex(current);
  Endpoint::send(msg);
}

void NetworkSelectionModel::slotSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
  Q_UNUSED(deselected);
  if (!Endpoint::isConnected())
    return;
  Message msg(m_myAddress, Protocol::SelectionModelSelect);
  msg.payload() << qint32(QItemSelectionModel::ClearAndSelect) << qint32(selected.size());
  foreach(const QItemSelectionRange& range, selected)
    msg.payload() << Protocol::fromQModelIndex(range.topLeft()) << Protocol::fromQModelIndex(range.bottomRight());
  Endpoint::send(msg);
}

#include "networkselectionmodel.moc"
