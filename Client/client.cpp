#include "client.h"
#include "ui_client.h"
#include <QHostAddress>
#include <QDateTime>

//-------------------------------------------------------------------------------------------------
Client::Client(QWidget *parent)
  : QMainWindow(parent)
  , ui(new Ui::Client)
  , _uiNextBlockSize(0)
  , _strInterlocutor("")
{
  ui->setupUi(this);
  ui->Username->setText("Username");
  ui->BtnSend->setEnabled(false);
  _Socket = new QTcpSocket(this);

  connect(_Socket, &QTcpSocket::readyRead, this, &Client::slotReadyRead);
  connect(_Socket, &QTcpSocket::disconnected, _Socket, &QTcpSocket::deleteLater);
  connect(ui->UserList, &QListWidget::itemDoubleClicked, this, &Client::slotGetHistory);
}
//-------------------------------------------------------------------------------------------------
Client::~Client()
{
  delete ui;
  delete _Socket;
}
//-------------------------------------------------------------------------------------------------
void Client::slotReadyRead()
{
  QDataStream streamIn(_Socket);
  if (streamIn.status() == QDataStream::Ok) {
    qDebug() << "Reading from server...";

    while (true) {
      // Если размер сообщения не известен
      if (_uiNextBlockSize == 0) {
        // Если 2 байта, содержащие размер последующего сообщения, ещё не пришли
        if (_Socket->bytesAvailable() < 2) {
          qDebug() << "In socket " << _strUsername << " less 2 bytes: " << _Socket->bytesAvailable();
          break;
        }
        streamIn >> _uiNextBlockSize;
        qDebug() <<_strUsername << " First block of msg: " << _uiNextBlockSize;
      }
      if (_Socket->bytesAvailable() < _uiNextBlockSize) {
        qDebug() << _strUsername << " Bytes for reading less size of whole msg: "
                 << _Socket->bytesAvailable();
        break;
      }
      QString strMsgBlock;
      unsigned short uiMsgType;
      streamIn >> uiMsgType;
      _uiNextBlockSize = 0;

      switch (static_cast<_EnMessageTypes>(uiMsgType)) {
        case _EnMessageTypes::enUpdateUserList: {
          ui->UserList->clear();
          streamIn >> strMsgBlock;
          QStringList listUsers = strMsgBlock.split(",");
          listUsers.removeOne(_strUsername);
          ui->UserList->addItems(listUsers);
          break;
        }

        case _EnMessageTypes::enChatMessage: {
          QDateTime ReceivedMsgDateTime;
          streamIn >> ReceivedMsgDateTime >> strMsgBlock;

          QString strInterlocutors = strMsgBlock;

          int iPosTo = strInterlocutors.indexOf(":To");
          strInterlocutors.truncate(iPosTo);

          int iPosFrom = strInterlocutors.indexOf(":From");
          QString strFromClient = strInterlocutors;
          strFromClient.truncate(iPosFrom);

          QString strToClient = strInterlocutors;
          strToClient.remove(0, iPosFrom + 5);

          if (strFromClient == _strInterlocutor) {
            strMsgBlock.replace( 0, iPosTo + 3, QString("<%1>: ").arg(strFromClient));
            ui->Chat->append(QString("\t%1\n%2").arg(ReceivedMsgDateTime.toString("dd.MM.yyyy hh:mm:ss"))
                             .arg(strMsgBlock));
            qDebug() << "Msg from server: " <<
                        QString("%1 %2").arg(ReceivedMsgDateTime.toString("dd.MM.yyyy hh:mm:ss"))
                        .arg(strMsgBlock);
          }
          else if (strFromClient == _strUsername && strToClient == _strInterlocutor) {
            strMsgBlock.replace( 0, iPosTo + 3, "<Вы>: ");
            ui->Chat->append(QString("\t%1\n%2").arg(ReceivedMsgDateTime.toString("dd.MM.yyyy hh:mm:ss"))
                             .arg(strMsgBlock));
            qDebug() << _strUsername << " Msg from server: " <<
                        QString("%1 %2").arg(ReceivedMsgDateTime.toString("dd.MM.yyyy hh:mm:ss"))
                        .arg(strMsgBlock);
          }
          else if (strFromClient != _strInterlocutor && strFromClient != _strUsername) {
            auto listItems = ui->UserList->findItems(strFromClient, Qt::MatchCaseSensitive);
            if (!listItems.empty()) {
              int rowItem = ui->UserList->row(listItems.at(0));
              auto Item = ui->UserList->takeItem(rowItem);
              Item->setText(QString("%1 +").arg(strFromClient));
              ui->UserList->insertItem(0, Item);
            }
            qDebug() << _strUsername << " Msg from server: " <<
                        QString("%1 %2").arg(ReceivedMsgDateTime.toString("dd.MM.yyyy hh:mm:ss"))
                        .arg(strMsgBlock);
          }
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
    qDebug() << "Something went wrong while reading from server.";
  }
}
//-------------------------------------------------------------------------------------------------
void Client::vSendServer(_EnMessageTypes enMsgType, QString strMsg)
{
  _arrData.clear();
  QDataStream streamOut(&_arrData, QIODevice::WriteOnly);

  switch (enMsgType) {
    case _EnMessageTypes::enSaveUsername: {
      streamOut << quint16(0) << static_cast<unsigned short>(enMsgType) << strMsg;
      // Ставим указатель записи в начало буфера, чтобы записать веса самого сообщения и типа сообщения в 2 байтовую переменную
      streamOut.device()->seek(0);
      streamOut << quint16(_arrData.size() - static_cast<int>(sizeof(quint16)));
      _Socket->write(_arrData);
      break;
    }

    case _EnMessageTypes::enChatMessage:
      streamOut << quint16(0) << static_cast<unsigned short>(enMsgType)
                << QString("%1:From%2:To%3").arg(_strUsername).arg(_strInterlocutor).arg(strMsg);
      // Ставим указатель записи в начало буфера, чтобы записать веса самого сообщения и типа сообщения в 2 байтовую переменную
      streamOut.device()->seek(0);
      streamOut << quint16(_arrData.size() - static_cast<int>(sizeof(quint16)));
      _Socket->write(_arrData);
      ui->TextEdit->clear();
      break;

    case _EnMessageTypes::enSelectHistoryDb:

      break;

    default:
      break;
  }
}
//-------------------------------------------------------------------------------------------------
void Client::on_BtnSend_clicked()
{
  //if (!_strInterlocutor.isEmpty()) { // в разработке
    vSendServer(_EnMessageTypes::enChatMessage, ui->TextEdit->toPlainText());
  //}
}
//-------------------------------------------------------------------------------------------------
void Client::on_BtnConnect_clicked()
{
  _Socket->connectToHost(QHostAddress::LocalHost, 1234);
  _strUsername = ui->Username->text();
  _strUsername = _strUsername.trimmed();
  ui->BtnConnect->setText("Подключены");
  ui->BtnConnect->setEnabled(false);
  ui->Username->setEnabled(false);
  vSendServer(_EnMessageTypes::enSaveUsername, _strUsername);
}
//-------------------------------------------------------------------------------------------------
void Client::slotGetHistory(QListWidgetItem* Item) // в разработке
{
  _strInterlocutor = Item->text();
  // if (_strInterlocutor.contains(" +")) {
  //   int iPosMarkerMsg = _strInterlocutor.indexOf(" +");
  //   _strInterlocutor.truncate(iPosMarkerMsg);
  //   Item->setText(_strInterlocutor);
  // }
  ui->BtnSend->setEnabled(true);
  ui->InterlocutorLbl->setText(ui->InterlocutorLbl->text() + _strInterlocutor);

  // vSendServer(_EnMessageTypes::enSelectHistoryDb, _strInterlocutor);
}
//-------------------------------------------------------------------------------------------------
void Client::on_Client_destroyed()
{
  _Socket->disconnectFromHost();
  qDebug() << "Client " << _strUsername << " Disconnect with error " << _Socket->error();
}
//-------------------------------------------------------------------------------------------------
