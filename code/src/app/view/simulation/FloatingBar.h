#ifndef REAL_TIME_FLUID_FLOATING_BAR_H
#define REAL_TIME_FLUID_FLOATING_BAR_H

#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QWidget>

/**
 * Rounded, translucent tool bar that floats above the simulation.
 */
class FloatingBar : public QWidget {
 Q_OBJECT
 private:
  QHBoxLayout *layout_ = nullptr;

 protected:
  void paintEvent(QPaintEvent *event) override;

 public:
  explicit FloatingBar(QWidget *parent = nullptr);

  /**
   * Add a tool button to the bar.
   * @param icon button icon
   * @param text text beside the icon, empty for an icon only button
   * @param tool_tip tool tip
   * @param checkable true for a toggle button
   * @return the new button
   */
  QToolButton *addButton(const QIcon &icon, const QString &text, const QString &tool_tip, bool checkable = false);

  /**
   * Add a thin vertical line between groups of buttons.
   */
  void addSeparator();
};

#endif //REAL_TIME_FLUID_FLOATING_BAR_H
