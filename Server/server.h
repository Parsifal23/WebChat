#pragma once

#include <QTcpServer>
#include <QTcpSocket>
#include <QMap>

/** @class Класс сервера */
class Server : public QTcpServer
{
  Q_OBJECT

public:
  /** @brief Конструктор */
  Server();

  /** @brief Сокет при новом подключении */
  QTcpSocket* _Socket;

public  slots:
  /** @brief Слот для входящего подключения */
  void incomingConnection(qintptr pSockDescriptor) override;

  /** @brief Слот для анализа полученных данных */
  void slotReadyRead();

private slots:
  /** @brief Слот удаления сокета */
  void slotDeleteSocket();

private:
  /** @brief Перечисление типов сообщений в сокете */
  enum class _EnMessageTypes {
    enSaveUsername = 1,
    enUpdateUserList,
    enChatMessage,
    enSelectHistoryDb
  };

  /** @brief Отправка данных клиенту*/
  void vSendClient(_EnMessageTypes enMsgType, QString strToCLient, QString strMsg);

  /** @brief Словарь всех подключенных сокетов. Ключ - сокет-клиент, значение - Username.
    * По-умолчанию, значение - "Username"*/
  QMap<QTcpSocket*, QString> _mapSockets;

  /** @brief Тип данных для передачи в сокете */
  QByteArray _arrData;

  /** @brief Размер отправляемого сообщения */
  quint16 _uiNextBlockSize;  
};
