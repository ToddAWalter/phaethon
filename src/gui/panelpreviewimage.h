/* Phaethon - A FLOSS resource explorer for BioWare's Aurora engine games
 *
 * Phaethon is the legal property of its developers, whose names
 * can be found in the AUTHORS file distributed with this source
 * distribution.
 *
 * Phaethon is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * Phaethon is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Phaethon. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file
 *  Preview panel for image resources.
 */

#ifndef GUI_PANELPREVIEWIMAGE_H
#define GUI_PANELPREVIEWIMAGE_H

#include "src/common/types.h"

#include "src/gui/panelbase.h"

#include "src/images/decoder.h"
#include "src/images/types.h"

class QScrollArea;

namespace GUI {

class ResourceTreeItem;

class PanelPreviewImage : public PanelBase {
	W_OBJECT(PanelPreviewImage)

public:
	PanelPreviewImage(QWidget *parent);

	virtual void show(const ResourceTreeItem *item);

	// public slots:
	void slotSliderBrightness(int value);
	void slotZoomIn();
	void slotZoomOut();
	void slotZoomOriginal();
	void slotFit();
	void slotFitWidth();
	void slotShrinkFit();
	void slotShrinkFitWidth();
	void slotNearest(bool checked);

private:
	QPushButton *_buttonZoomIn { nullptr };
	QPushButton *_buttonZoomOut { nullptr };
	QPushButton *_buttonZoomOriginal { nullptr };
	QPushButton *_buttonFit { nullptr };
	QPushButton *_buttonFitWidth { nullptr };
	QPushButton *_buttonShrinkFit { nullptr };
	QPushButton *_buttonShrinkFitWidth { nullptr };

	QLabel *_labelDimensions { nullptr };
	QLabel *_labelZoomPercent { nullptr };
	QLabel *_labelImage { nullptr }; ///< Label is used to display the image.

	QCheckBox *_checkNearest { nullptr };

	QSlider *_sliderBrightness { nullptr };

	QScrollArea *_scrollAreaImage { nullptr };

	// Necessary because the way zooming is implemented modifies the pixmap.
	QPixmap _originalPixmap; ///< To reset to default zoom level.

	int _zoomLevel;

	Qt::TransformationMode _mode { Qt::SmoothTransformation }; ///< Linear/nearest.

	QImage  loadImage(const Images::Decoder &image);
	void  convertImage(const Images::Decoder &image, byte *dataOut);
	void  writePixel(const byte *&dataIn, Images::PixelFormat format, byte *&dataOut);
	void  getImageDimensions(const Images::Decoder &image, int32_t &width, int32_t &height);
	void  redrawImage();
	void  setZoomLevel(int value);
	void  zoomTo(int zoomLevel);
	void  zoomToFit(bool grow);
	void  zoomToFitWidth(bool grow);
};

} // End of namespace GUI

#endif // GUI_PANELPREVIEWIMAGE_H
