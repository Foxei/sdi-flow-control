#ifndef REAL_TIME_FLUID_STATUS_BAR_H
#define REAL_TIME_FLUID_STATUS_BAR_H

#include <QtCore/QTimer>
#include <QtGui/QIcon>
#include <QtWidgets/QLabel>
#include <QtWidgets/QStatusBar>

/**
 * Different typ of messages just effect the icon on status bar
 */
enum class MessageTyp {
  Error,
  OK,
  NONE
};

/**
 * Status bar with a message on the left and the values of the cell under
 * the mouse and the step rate on the right.
 */
class StatusBar : public QStatusBar {
 Q_OBJECT
 private:
  QIcon icons_[2];                  //< Icons which can be displayed on status bar
  QLabel *label_img = nullptr;      //< Label to display icons
  QLabel *label_message = nullptr;  //< Label to display the message
  QLabel *label_cell = nullptr;     //< Cell under the mouse
  QLabel *label_values = nullptr;   //< Speed and pressure of that cell
  QLabel *label_rate = nullptr;     //< Solver steps per second
  QTimer *timer_reset_ = nullptr;   //< Restores the ready message

  /**
   * Setting up layout for statusbar
   */
  void setUpGui();

  /**
   * Loading images for statusbar
   */
  void loadImages();

 public:
  /**
   * Will setup gui, load images and use super constructor
   * @param parent Parent QWidget
   */
  explicit StatusBar(QWidget *parent = nullptr);

 public slots:
  /**
   * Show given message and icon.
   * @param timeout milliseconds until the ready message returns, 0 to keep it
   */
  void displayMessage(const QString &message, MessageTyp typ, int timeout = 0);

  /**
   * Show the ready message.
   */
  void showReady();

  /**
   * Show the cell under the mouse and its values, a null cell hides them.
   */
  void displayCell(QPoint cell, double speed, double pressure);

  /**
   * Show the solver steps per second.
   */
  void displayRate(double rate);
};

#endif //REAL_TIME_FLUID_STATUS_BAR_H
