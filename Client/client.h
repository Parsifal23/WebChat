#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QListWidgetItem>

QT_BEGIN_NAMESPACE
namespace Ui { class Client; }
QT_END_NAMESPACE

class Client : public QMainWindow
{
  Q_OBJECT

public:
  /** @brief Конструктор */
  Client(QWidget *parent = nullptr);

  /** @brief Деструктор */
  ~Client();

public slots:
  /** @brief Слот для анализа полученных данных */
  void slotReadyRead();

private slots:
  /** @brief Слот кнопки Отправить */
  void on_BtnSend_clicked();

  /** @brief Слот по нажатию кнопки Подключиться */
  void on_BtnConnect_clicked();

  /** @brief Слот запроса истории по выбранному пользователю */
  void slotGetHistory(QListWidgetItem* Item);

  /** @brief Слот закрытия окна */
  void on_Client_destroyed();

private:
  /** @brief Перечисление типов сообщений в сокете */
  enum class _EnMessageTypes {
    enSaveUsername = 1,
    enUpdateUserList,
    enChatMessage,
    enSelectHistoryDb
  };

  /** @brief Отправка данных серверу */
  void vSendServer(_EnMessageTypes enMsgType, QString strMsg);

  Ui::Client *ui;

  /** @brief Сокет */
  QTcpSocket* _Socket;

  /** @brief Тип данных для передачи в сокете */
  QByteArray _arrData;

  /** @brief Размер принимаемого сообщения */
  quint16 _uiNextBlockSize;

  /** @brief Имя собеседника в чате */
  QString _strInterlocutor;

  /** @brief Имя пользователя в чате */
  QString _strUsername;
};
#endif // MAINWINDOW_H
