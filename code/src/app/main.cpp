#include "controller/MainController.h"
#include "view/AppStyle.h"
#include "view/ColorDefinitions.h"
#include "view/MainWindow.h"

#include <QtGui/QIcon>
#include <QtGui/QPalette>
#include <QtGui/QStyleHints>
#include <QtWidgets/QApplication>

/*! \mainpage Real-Time Fluid
 *
 * \section intro_sec Introduction
 *
 * Interactive 2D flow simulation of a channel. The incompressible
 * Navier-Stokes equations are solved in real time ("Stable Fluids" by
 * Jos Stam). Obstacles, ink, particles and forces are added with the mouse;
 * velocity, pressure, divergence and ink density can be displayed.
 *
 * The solver (src/solver) is a plain C++ library, the GUI (src/app) runs it
 * in a worker thread.
 *
 * \author Soeren Mehnert
 * \author Markus Klaus Mizael Moser
 * \author Sharon Michael Rouf-Chowdhury
 * \author Simon Schaefer
 * \date RWTH Aachen - Summer semester 2017
 */

/**
 * Dark application palette
 */
QPalette darkPalette() {
  QPalette palette;
  palette.setColor(QPalette::Window, Color::BACKGROUND);
  palette.setColor(QPalette::WindowText, Color::TEXT);
  palette.setColor(QPalette::Base, Color::SURFACE);
  palette.setColor(QPalette::AlternateBase, Color::BACKGROUND);
  palette.setColor(QPalette::ToolTipBase, Color::BASE);
  palette.setColor(QPalette::ToolTipText, Color::TEXT);
  palette.setColor(QPalette::Text, Color::TEXT);
  palette.setColor(QPalette::PlaceholderText, Color::TEXT_SECONDARY);
  palette.setColor(QPalette::Button, Color::BUTTON);
  palette.setColor(QPalette::ButtonText, Color::TEXT);
  palette.setColor(QPalette::BrightText, Color::TEXT);
  palette.setColor(QPalette::Light, Color::BASE.lighter(130));
  palette.setColor(QPalette::Mid, Color::BASE);
  palette.setColor(QPalette::Dark, Color::SURFACE.darker(130));
  palette.setColor(QPalette::Link, Color::ACCENT);
  palette.setColor(QPalette::Highlight, Color::ACCENT);
  palette.setColor(QPalette::HighlightedText, Qt::white);
  palette.setColor(QPalette::Accent, Color::ACCENT);
  palette.setColor(QPalette::Disabled, QPalette::Text, Color::TEXT_SECONDARY);
  palette.setColor(QPalette::Disabled, QPalette::WindowText, Color::TEXT_SECONDARY);
  palette.setColor(QPalette::Disabled, QPalette::ButtonText, Color::TEXT_SECONDARY);
  return palette;
}

/**
 * Fusion style with a dark palette and the system font on all platforms.
 * Unlike the native styles, Fusion draws all inputs at the same, freely
 * chosen size. The dark color scheme keeps native title bars dark.
 */
void setStyle() {
  QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme::Dark);
  QApplication::setStyle(new AppStyle());
  QApplication::setPalette(darkPalette());
  QApplication::setWindowIcon(QIcon(":icons/app.svg"));
}

int main(int argc, char *argv[]) {
  qRegisterMetaType<FramePtr>();
  qRegisterMetaType<SolverParameters>();

  // Create application and set style
  QApplication a(argc, argv);
  QApplication::setApplicationName("RealTimeFluid");
  QApplication::setApplicationDisplayName(QObject::tr("Real-Time Fluid"));
  QApplication::setOrganizationName("RWTH SDI");
  QApplication::setApplicationVersion(PROJECT_VERSION);
  setStyle();

  // Create windows content
  MainWindow window;

  // Start the solver and connect the interface
  MainController controller(window);

  window.show();
  return QApplication::exec();
}
