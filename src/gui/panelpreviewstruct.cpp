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
 *  Preview panel for resources consisting of structured data, i.e.,
 *  GFF resources such as ARE and IFO.
 */

#include <memory>

#include <QFrame>
#include <QWidget>
#include <QFormLayout>
#include <QLabel>
#include <QTreeView>

#include "external/verdigris/wobjectimpl.h"

#include "src/aurora/gff3file.h"

#include "src/gui/panelpreviewstruct.h"
#include "src/gui/resourcetreeitem.h"

namespace {

const char *getGFF3FieldTypeName(const Aurora::GFF3Struct::FieldType type) {
	switch (type) {
		case Aurora::GFF3Struct::FieldType::kFieldTypeByte:
			return "Byte";
		case Aurora::GFF3Struct::FieldType::kFieldTypeChar:
			return "Char";
		case Aurora::GFF3Struct::FieldType::kFieldTypeUint16:
			return "Uint16";
		case Aurora::GFF3Struct::FieldType::kFieldTypeSint16:
			return "Sint16";
		case Aurora::GFF3Struct::FieldType::kFieldTypeUint32:
			return "Uint32";
		case Aurora::GFF3Struct::FieldType::kFieldTypeSint32:
			return "Sint32";
		case Aurora::GFF3Struct::FieldType::kFieldTypeUint64:
			return "Uint64";
		case Aurora::GFF3Struct::FieldType::kFieldTypeSint64:
			return "Sint64";
		case Aurora::GFF3Struct::FieldType::kFieldTypeFloat:
			return "Float";
		case Aurora::GFF3Struct::FieldType::kFieldTypeDouble:
			return "Double";
		case Aurora::GFF3Struct::FieldType::kFieldTypeExoString:
			return "ExoString";
		case Aurora::GFF3Struct::FieldType::kFieldTypeResRef:
			return "ResRef";
		case Aurora::GFF3Struct::FieldType::kFieldTypeLocString:
			return "LocString";
		case Aurora::GFF3Struct::FieldType::kFieldTypeVoid:
			return "Void";
		case Aurora::GFF3Struct::FieldType::kFieldTypeStruct:
			return "Struct";
		case Aurora::GFF3Struct::FieldType::kFieldTypeList:
			return "List";
		case Aurora::GFF3Struct::FieldType::kFieldTypeOrientation:
			return "Orientation";
		case Aurora::GFF3Struct::FieldType::kFieldTypeVector:
			return "Vector";
		case Aurora::GFF3Struct::FieldType::kFieldTypeStrRef:
			return "StrRef";
		default:
			return "";
	}
}

template <typename Parent>
void appendList(Parent &parent, const Aurora::GFF3List &list);

template <typename Parent>
void appendStruct(Parent &parent, const Aurora::GFF3Struct &struc) {
	const std::vector<Common::UString> &fieldNames = struc.getFieldNames();

	for (const auto &fieldName : fieldNames) {

		const Aurora::GFF3Struct::FieldType fieldType = struc.getFieldType(fieldName);

		QStandardItem *labelItem = new QStandardItem(QString::fromStdString(fieldName.c_str()));
		QStandardItem *typeItem = new QStandardItem(QString::fromStdString(getGFF3FieldTypeName(fieldType)));
		QStandardItem *valueItem = nullptr;

		switch (fieldType) {
			case Aurora::GFF3Struct::FieldType::kFieldTypeNone:
			case Aurora::GFF3Struct::FieldType::kFieldTypeVoid:
				break;
			case Aurora::GFF3Struct::FieldType::kFieldTypeStruct:
				{
					const Aurora::GFF3Struct &childStruct = struc.getStruct(fieldName);
					valueItem = new QStandardItem(QString("ID: %1").arg(childStruct.getID()));
				}
				break;
			case Aurora::GFF3Struct::FieldType::kFieldTypeList:
				{
					const Aurora::GFF3List &list = struc.getList(fieldName);
					valueItem = new QStandardItem(QString("(%1 items)").arg(list.size()));
				}
				break;
			default:
				{
					const Common::UString fieldValue = struc.getString(fieldName);
					valueItem = new QStandardItem(QString::fromStdString(fieldValue.c_str()));
				}
		}

		parent->appendRow({ labelItem, typeItem, valueItem });

		switch (fieldType) {
			case Aurora::GFF3Struct::kFieldTypeStruct:
				appendStruct(labelItem, struc.getStruct(fieldName));
				break;
			case Aurora::GFF3Struct::kFieldTypeList:
				appendList(labelItem, struc.getList(fieldName));
				break;
			default:
				break;
		}
	}
}

template <typename Parent>
void appendList(Parent &parent, const Aurora::GFF3List &list) {
	for (std::size_t i = 0; i < list.size(); ++i) {
		QStandardItem *labelItem = new QStandardItem(QString::number(i));
		QStandardItem *typeItem = new QStandardItem(QString::fromStdString("Struct"));
		QStandardItem *valueItem = new QStandardItem(QString("ID: %1").arg(list[i]->getID()));
		parent->appendRow({ labelItem, typeItem, valueItem });
		appendStruct(labelItem, *list[i]);
	}
}

} // End of anonymous namespace

namespace GUI {

W_OBJECT_IMPL(PanelPreviewStruct)

PanelPreviewStruct::PanelPreviewStruct(QWidget *parent) :
	PanelBase(parent), _model(new QStandardItemModel(nullptr)),
	_treeView(new QTreeView(nullptr)) {
	QVBoxLayout *layoutTop = new QVBoxLayout(this);

	layoutTop->addWidget(_treeView);
	_treeView->setModel(_model.get());

	_treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);

	layoutTop->setContentsMargins(0, 0, 0, 0);
}

void PanelPreviewStruct::show(const ResourceTreeItem *item) {
	PanelBase::show(item);

	if (item->getResourceType() != Aurora::kResourceStruct)
		return;

	_currentItem = item;

	setTreeData();
}

void PanelPreviewStruct::setTreeData() {
	_model->clear();

	Common::SeekableReadStream *stream = _currentItem->getResourceData();

	if (!stream)
		return;

	std::unique_ptr<Aurora::GFF3File> gff3 = nullptr;

	try {
		gff3 = std::make_unique<Aurora::GFF3File>(stream);
	} catch (Common::Exception &e) {
		Common::printException(e, "WARNING: ");
	}

	if (gff3) {
		_model->setHorizontalHeaderLabels({ "Label", "Type", "Value" });
		appendStruct(_model, gff3->getTopLevel());
	}
}

} // End of namespace GUI
