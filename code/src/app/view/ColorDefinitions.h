#ifndef REAL_TIME_FLUID_COLOR_DEFINITIONS_H
#define REAL_TIME_FLUID_COLOR_DEFINITIONS_H

#include <QtGui/QColor>

/**
 * Dark theme colors, used where the platform has no native dark mode
 */
namespace Color {

const QColor ACCENT = QColor(76, 141, 246);
const QColor DANGER = QColor(229, 83, 75);
const QColor SURFACE = QColor(30, 30, 32);      // input fields, chart
const QColor BACKGROUND = QColor(40, 40, 43);   // windows, sidebar
const QColor BUTTON = QColor(55, 55, 59);
const QColor BASE = QColor(72, 72, 76);         // borders
const QColor TEXT = QColor(230, 230, 230);
const QColor TEXT_SECONDARY = QColor(150, 150, 155);

/* Simulation display */
const QColor FIELD_NEGATIVE = QColor(76, 141, 246);  // negative pressure / divergence
const QColor FIELD_POSITIVE = QColor(242, 140, 74);  // positive pressure / divergence
const QColor INK = QColor(63, 213, 245);             // density
const QColor SPEED_LOW = QColor(70, 82, 110);        // velocity arrows, slow
const QColor SPEED_MID = QColor(76, 141, 246);
const QColor SPEED_HIGH = QColor(150, 235, 250);     // velocity arrows, fast
const QColor PARTICLE = QColor(255, 196, 87);
const QColor OBSTACLE = QColor(132, 132, 140);
const QColor WALL = QColor(58, 58, 63);

}

#endif //REAL_TIME_FLUID_COLOR_DEFINITIONS_H
