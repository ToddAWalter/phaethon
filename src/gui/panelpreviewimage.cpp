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

#include <memory>

#include <QImageReader>
#include <QCheckBox>
#include <QFrame>
#include <QGraphicsView>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollBar>
#include <QSlider>
#include <QWidget>
#include <QFileInfo>

#include "external/verdigris/wobjectimpl.h"

#include "src/gui/panelpreviewimage.h"
#include "src/gui/resourcetreeitem.h"

namespace {

constexpr int kZoomLevelMin      =   10;
constexpr int kZoomLevelOriginal =  100;
constexpr int kZoomLevelMax      = 1000;
constexpr int kZoomLevelStep     =   10;

} // End of anonymous namespace

namespace GUI {

W_OBJECT_IMPL(PanelPreviewImage)

PanelPreviewImage::PanelPreviewImage(QWidget *parent) :
	PanelBase(parent) {
	QGridLayout *layoutTop = new QGridLayout(this);
	QVBoxLayout *layoutLeft = new QVBoxLayout();

	_buttonZoomIn = new QPushButton(tr("Zoom in"), this);
	_buttonZoomOut = new QPushButton(tr("Zoom out"), this);
	_buttonZoomOriginal = new QPushButton(tr("Zoom 100%"), this);
	_buttonFit = new QPushButton(tr("Fit"), this);
	_buttonFitWidth = new QPushButton(tr("Fit width"), this);
	_buttonShrinkFit = new QPushButton(tr("Shrink fit"), this);
	_buttonShrinkFitWidth = new QPushButton(tr("Shrink fit width"), this);

	_labelDimensions = new QLabel(this);
	QLabel *labelBrightness = new QLabel(tr("Background brightness"), this);
	_labelZoomPercent = new QLabel(this);
	_labelImage = new QLabel(this);

	_scrollAreaImage = new QScrollArea(this);

	_sliderBrightness = new QSlider(this);

	_checkNearest = new QCheckBox("Nearest", this);

	layoutLeft->addWidget(_labelZoomPercent);
	layoutLeft->addWidget(_buttonZoomIn);
	layoutLeft->addWidget(_buttonZoomOut);
	layoutLeft->addWidget(_buttonZoomOriginal);
	layoutLeft->addWidget(_buttonFit);
	layoutLeft->addWidget(_buttonFitWidth);
	layoutLeft->addWidget(_buttonShrinkFit);
	layoutLeft->addWidget(_buttonShrinkFitWidth);
	layoutLeft->addWidget(_checkNearest);
	layoutLeft->insertStretch(-1, 1); // get rid of spacing between buttons
	_labelZoomPercent->setAlignment(Qt::AlignCenter);

	layoutTop->addWidget(_labelDimensions, 0, 0);
	layoutTop->addWidget(labelBrightness, 1, 0);
	layoutTop->addWidget(_sliderBrightness, 1, 1);
	layoutTop->addLayout(layoutLeft, 2, 0);
	layoutTop->addWidget(_scrollAreaImage, 2, 1);

	_labelImage->setBackgroundRole(QPalette::Base);
	_labelImage->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	_labelImage->setScaledContents(true);

	_scrollAreaImage->setWidget(_labelImage);

	_sliderBrightness->setOrientation(Qt::Horizontal);
	_sliderBrightness->setMaximum(255);

	setZoomLevel(kZoomLevelOriginal);

	slotSliderBrightness(0);

	connect(_sliderBrightness, &QSlider::valueChanged, this, &PanelPreviewImage::slotSliderBrightness);
	connect(_buttonZoomIn, &QPushButton::clicked, this, &PanelPreviewImage::slotZoomIn);
	connect(_buttonZoomOut, &QPushButton::clicked, this, &PanelPreviewImage::slotZoomOut);
	connect(_buttonZoomOriginal, &QPushButton::clicked, this, &PanelPreviewImage::slotZoomOriginal);
	connect(_buttonFit, &QPushButton::clicked, this, &PanelPreviewImage::slotFit);
	connect(_buttonFitWidth, &QPushButton::clicked, this, &PanelPreviewImage::slotFitWidth);
	connect(_buttonShrinkFit, &QPushButton::clicked, this, &PanelPreviewImage::slotShrinkFit);
	connect(_buttonShrinkFitWidth, &QPushButton::clicked, this, &PanelPreviewImage::slotShrinkFitWidth);
	connect(_checkNearest, &QCheckBox::toggled, this, &PanelPreviewImage::slotNearest);
}

void PanelPreviewImage::show(const ResourceTreeItem *item) {
	PanelBase::show(item);

	_originalPixmap = QPixmap();

	if (item->getResourceType() != Aurora::kResourceImage)
		return;

	QImage image = QImage();

	try {
		std::unique_ptr<Images::Decoder> imageDecoder(item->getImage());
		if (imageDecoder) {
			image = loadImage(*imageDecoder);
		}
	} catch (Common::Exception &e) {
		// If the image fails to load (e.g., due to an unsupported image type), maybe
		// QImageReader can decode it.
		QByteArray format = QFileInfo(item->getName()).suffix().toLower().toLatin1();
		if (QImageReader::supportedImageFormats().contains(format)) {
			std::unique_ptr<Common::SeekableReadStream> resourceData(item->getResourceData());
			std::unique_ptr<Common::MemoryReadStream> mem(resourceData->readStream(resourceData->size()));
			image = QImage::fromData(mem->getData(), mem->size(), format.constData());
		}
		else {
			Common::printException(e, "WARNING: ");
		}
	}

	_labelDimensions->setText(QString("(%1x%2)").arg(image.width()).arg(image.height()));
	_originalPixmap = QPixmap::fromImage(image);

	redrawImage();
}

static void cleanupImage(void *info) {
	byte *image = static_cast<byte *>(info);

	delete[] image;
}

QImage PanelPreviewImage::loadImage(const Images::Decoder &image) {
	if ((image.getMipMapCount() == 0) || (image.getLayerCount() == 0))
		return QImage();

	int32_t width = 0, height = 0;
	getImageDimensions(image, width, height);
	if ((width <= 0) || (height <= 0))
		throw Common::Exception("Invalid image dimensions (%d x %d)", width, height);

	std::unique_ptr<byte[]> rgbaData = std::make_unique<byte[]>(width * height * 4);
	std::memset(rgbaData.get(), 0, width * height * 4);

	convertImage(image, rgbaData.get());

	QImage qImage(rgbaData.get(), width, height, QImage::Format_RGBA8888, cleanupImage, rgbaData.get());
	rgbaData.release();

	qImage.mirror();

	return qImage;
}

void PanelPreviewImage::convertImage(const Images::Decoder &image, byte *dataOut) {
	int32_t width, height;
	getImageDimensions(image, width, height);

	for (size_t i = 0; i < image.getLayerCount(); i++) {
		const Images::Decoder::MipMap &mipMap = image.getMipMap(0, i);
		const byte *mipMapData = mipMap.data.get();

		uint32_t count = mipMap.width * mipMap.height;
		while (count-- > 0)
			writePixel(mipMapData, image.getFormat(), dataOut);
	}
}

void PanelPreviewImage::writePixel(const byte *&dataIn, Images::PixelFormat format, byte *&dataOut) {
	switch (format) {
		case Images::kPixelFormatR8G8B8:
			*dataOut++ = dataIn[0];
			*dataOut++ = dataIn[1];
			*dataOut++ = dataIn[2];
			*dataOut++ = 0xFF;
			dataIn    += 3;
			break;

		case Images::kPixelFormatB8G8R8:
			*dataOut++ = dataIn[2];
			*dataOut++ = dataIn[1];
			*dataOut++ = dataIn[0];
			*dataOut++ = 0xFF;
			dataIn    += 3;
			break;

		case Images::kPixelFormatR8G8B8A8:
			*dataOut++ = dataIn[0];
			*dataOut++ = dataIn[1];
			*dataOut++ = dataIn[2];
			*dataOut++ = dataIn[3];
			dataIn    += 4;
			break;

		case Images::kPixelFormatB8G8R8A8:
			*dataOut++ = dataIn[2];
			*dataOut++ = dataIn[1];
			*dataOut++ = dataIn[0];
			*dataOut++ = dataIn[3];
			dataIn    += 4;
			break;

		case Images::kPixelFormatR5G6B5:
			{
				const uint16_t color = READ_LE_UINT16(dataIn);

				*dataOut++ =  color & 0x001F;
				*dataOut++ = (color & 0x07E0) >>  5;
				*dataOut++ = (color & 0xF800) >> 11;
				*dataOut++ = 0xFF;
				dataIn    += 2;
			}
			break;

		case Images::kPixelFormatA1R5G5B5:
			{
				const uint16_t color = READ_LE_UINT16(dataIn);

				*dataOut++ =  color & 0x001F;
				*dataOut++ = (color & 0x03E0) >>  5;
				*dataOut++ = (color & 0x7C00) >> 10;
				*dataOut++ = (color & 0x8000) ? 0xFF : 0x00;
				dataIn    += 2;
			}
			break;

		default:
			throw Common::Exception("Unsupported pixel format: %d", (int) format);
	}
}

void PanelPreviewImage::getImageDimensions(const Images::Decoder &image, int32_t &width, int32_t &height) {
	width  = image.getMipMap(0, 0).width;
	height = 0;

	for (size_t i = 0; i < image.getLayerCount(); i++) {
		const Images::Decoder::MipMap &mipMap = image.getMipMap(0, i);

		if (mipMap.width != width)
			throw Common::Exception("Unsupported image with variable layer width");

		height += mipMap.height;
	}
}

void PanelPreviewImage::slotSliderBrightness(int value) {
	QString styleSheet = QString("background-color: rgb(%1, %2, %3)").arg(value).arg(value).arg(value);
	_scrollAreaImage->viewport()->setStyleSheet(styleSheet);
}

void PanelPreviewImage::slotZoomIn() {
	zoomTo(_zoomLevel + kZoomLevelStep);
}

void PanelPreviewImage::slotZoomOut() {
	zoomTo(_zoomLevel - kZoomLevelStep);
}

void PanelPreviewImage::slotZoomOriginal() {
	zoomTo(kZoomLevelOriginal);
}

void PanelPreviewImage::slotFit() {
	zoomToFit(true);
}

void PanelPreviewImage::slotFitWidth() {
	zoomToFitWidth(true);
}

void PanelPreviewImage::slotShrinkFit() {
	zoomToFit(false);
}

void PanelPreviewImage::slotShrinkFitWidth() {
	zoomToFitWidth(false);
}

void PanelPreviewImage::slotNearest(bool checked) {
	_mode = checked ? Qt::FastTransformation : Qt::SmoothTransformation;
	redrawImage();
}

void PanelPreviewImage::redrawImage() {

	if (_originalPixmap.isNull()) {
		return;
	}

	int width = _zoomLevel * _originalPixmap.width() / kZoomLevelOriginal;
	int height = _zoomLevel * _originalPixmap.height() / kZoomLevelOriginal;

	QPixmap scaledPixmap = _originalPixmap.scaled(width, height, Qt::IgnoreAspectRatio, _mode);

	_labelImage->setPixmap(scaledPixmap);
	_labelImage->setFixedSize(width, height);
}

void PanelPreviewImage::setZoomLevel(int value) {
	_zoomLevel = CLIP<int>(value, kZoomLevelMin, kZoomLevelMax);
	_labelZoomPercent->setText(QString("%1%").arg(_zoomLevel * 100 / kZoomLevelOriginal));
	_buttonZoomIn->setEnabled(_zoomLevel < kZoomLevelMax);
	_buttonZoomOut->setEnabled(_zoomLevel > kZoomLevelMin);
	_buttonZoomOriginal->setEnabled(_zoomLevel != kZoomLevelOriginal);
}

void PanelPreviewImage::zoomTo(int zoomLevel) {
	setZoomLevel(zoomLevel);
	redrawImage();
}

void PanelPreviewImage::zoomToFit(bool grow) {
	const QWidget *viewport = _scrollAreaImage->viewport();

	int widthZoomLevel = viewport->width() * kZoomLevelOriginal / _originalPixmap.width();
	int heightZoomLevel = viewport->height() * kZoomLevelOriginal / _originalPixmap.height();
	int newZoomLevel = MIN<int>(widthZoomLevel, heightZoomLevel);

	if (grow || newZoomLevel < _zoomLevel) {
		zoomTo(newZoomLevel);
	}
}

void PanelPreviewImage::zoomToFitWidth(bool grow) {
	// Must enable vertical scrollbar in order to calculate viewport width correctly.
	_scrollAreaImage->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

	const QWidget *viewport = _scrollAreaImage->viewport();
	int newZoomLevel = viewport->width() * kZoomLevelOriginal / _originalPixmap.width();

	_scrollAreaImage->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

	if (grow || newZoomLevel < _zoomLevel) {
		zoomTo(newZoomLevel);
	}
}

} // End of namespace GUI
