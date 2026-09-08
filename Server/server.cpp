#include "server.h"
#include <QDataStream>
#include <QDateTime>

//-------------------------------------------------------------------------------------------------
Server::Server()
{
  _uiNextBlockSize = 0;
  if (listen(QHostAddress::LocalHost, 1234)) {
    qDebug() << "Server started.";
  }
  else {
    qDebug() << "Something went wrong when server started.";
  }
}
//-------------------------------------------------------------------------------------------------
void Server::incomingConnection(qintptr iSockDescriptor)
{
  _Socket = new QTcpSocket;
  _Socket->setSocketDescriptor(iSockDescriptor);
  qDebug() << "New socket in port " << _Socket->peerPort();
  connect(_Socket, &QTcpSocket::readyRead, this, &Server::slotReadyRead);
  connect(_Socket, &QTcpSocket::disconnected, this, &Server::slotDeleteSocket);
  _mapSockets[_Socket] = "Username";
  qDebug() << "Client connected " << iSockDescriptor;
  qDebug() << "Count of sockets " << _mapSockets.size();
}
//-------------------------------------------------------------------------------------------------
void Server::slotReadyRead()
{
  _Socket = qobject_cast<QTcpSocket*>(sender());
  QDataStream streamIn(_Socket);
  if (streamIn.status() == QDataStream::Ok) {
    qDebug() << "Reading from client " << _mapSockets[_Socket];

    while (true) {
      if (_uiNextBlockSize == 0) {
        if (_Socket->bytesAvailable() < 2) {
          qDebug() << "In socket less 2 bytes: " << _Socket->bytesAvailable();
          break;
        }
        streamIn >> _uiNextBlockSize;
        qDebug() << "First block of msg: " << _uiNextBlockSize;
      }
      if (_Socket->bytesAvailable() < _uiNextBlockSize) {
        qDebug() << "Bytes for reading less size of whole msg: " << _Socket->bytesAvailable();
        break;
      }
      QString strMsgBlock;
      unsigned short uiMsgType;
      streamIn >> uiMsgType;
      _uiNextBlockSize = 0;

      switch (static_cast<_EnMessageTypes>(uiMsgType)) {
        case _EnMessageTypes::enSaveUsername: {
          streamIn >> strMsgBlock;
          _mapSockets[_Socket] = strMsgBlock;
          QString strUsers;
          if (_mapSockets.size() > 1) {
            QStringList listUsers;
            for (auto Username : _mapSockets) {
              listUsers << Username;
            }
            strUsers = listUsers.join(",");
          }
          else {
            break;
          }
          vSendClient(_EnMessageTypes::enUpdateUserList, {""}, strUsers);
          break;
        }

        case _EnMessageTypes::enChatMessage: {
          streamIn >> strMsgBlock;
          QString strInterlocutors = strMsgBlock;

          int iPosTo = strInterlocutors.indexOf(":To");
          strInterlocutors.truncate(iPosTo);

          int iPosFrom = strInterlocutors.indexOf(":From");
          QString strToCLient = strInterlocutors;
          strToCLient.remove(0, iPosFrom + 5);

          vSendClient(_EnMessageTypes::enChatMessage, strToCLient, strMsgBlock);
          qDebug() << "Msg from client: "
                   << QString("%1 %2")
                      .arg(QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss"))
                      .arg(strMsgBlock);
          break;
        }

        case _EnMessageTypes::enSelectHistoryDb:

          break;

        default:
          break;
      }
    }
  }
  else {
    qDebug() << "Something went wrong while reading from client.";
  }
}
//-------------------------------------------------------------------------------------------------
void Server::slotDeleteSocket()
{
  _Socket = qobject_cast<QTcpSocket*>(sender());
  qDebug() << "Client " << _mapSockets[_Socket] << " disconnected in port " << _Socket->peerPort();
  _mapSockets.remove(_Socket);
  qDebug() << "Count of sockets after deleting " << _mapSockets.size();
  qDebug() << "Server.Disconnect socket with error " << _Socket->error();
  _Socket->deleteLater();
}
//-------------------------------------------------------------------------------------------------
void Server::vSendClient(_EnMessageTypes enMsgType, QString strToCLient, QString strMsg)
{
  _arrData.clear();
  QDataStream streamOut(&_arrData, QIODevice::WriteOnly);

  switch (enMsgType) {
    case _EnMessageTypes::enUpdateUserList: {
      streamOut << quint16(0) << static_cast<unsigned short>(enMsgType) << strMsg;
      // Ставим указатель записи в начало буфера, чтобы записать вес самого сообщения и типа сообщения в 2 байтовую переменную
      streamOut.device()->seek(0);
      streamOut << quint16(_arrData.size() - static_cast<int>(sizeof(quint16)));
      auto listSockets = _mapSockets.keys();
      for (auto Socket : listSockets) {
        Socket->write(_arrData);
      }
      break;
    }

    case _EnMessageTypes::enChatMessage: {
      streamOut << quint16(0) << static_cast<unsigned short>(enMsgType)
                << QDateTime::currentDateTime() << strMsg;
      streamOut.device()->seek(0);
      // Ставим указатель записи в начало буфера, чтобы записать вес самого сообщения, типа сообщения, даты в 2 байтовую переменную
      streamOut << quint16(_arrData.size() - static_cast<int>(sizeof (quint16)));
      _Socket->write(_arrData);
      auto Socket = _mapSockets.key(strToCLient, nullptr);
      if (Socket != nullptr) {
        Socket->write(_arrData);
      }
      break;
    }

    case _EnMessageTypes::enSelectHistoryDb:

      break;

    default:
      break;
  }
}
//-------------------------------------------------------------------------------------------------

