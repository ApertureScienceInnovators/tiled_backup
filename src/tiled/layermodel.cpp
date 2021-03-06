/*
 * layermodel.cpp
 * Copyright 2008-2017, Thorbjørn Lindeijer <thorbjorn@lindeijer.nl>
 *
 * This file is part of Tiled.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "layermodel.h"

#include "changelayer.h"
#include "grouplayer.h"
#include "map.h"
#include "mapdocument.h"
#include "layer.h"
#include "renamelayer.h"
#include "tilelayer.h"

#include <QApplication>
#include <QStyle>

using namespace Tiled;
using namespace Tiled::Internal;

LayerModel::LayerModel(QObject *parent):
    QAbstractItemModel(parent),
    mMapDocument(nullptr),
    mMap(nullptr),
    mTileLayerIcon(QLatin1String(":/images/16x16/layer-tile.png")),
    mObjectGroupIcon(QLatin1String(":/images/16x16/layer-object.png")),
    mImageLayerIcon(QLatin1String(":/images/16x16/layer-image.png"))
{
    mTileLayerIcon.addFile(QLatin1String(":images/32x32/layer-tile.png"));
    mObjectGroupIcon.addFile(QLatin1String(":images/32x32/layer-object.png"));
}

QModelIndex LayerModel::index(int row, int column, const QModelIndex &parent) const
{
    // Top-level layer index
    if (!parent.isValid()) {
        if (row < mMap->layerCount())
            return createIndex(row, column, nullptr);
        return QModelIndex();
    }

    // Child of a group layer index
    TiledLayer *layer = toLayer(parent);
    Q_ASSERT(layer);
    if (GroupLayer *groupLayer = layer->asGroupLayer())
        if (row < groupLayer->layerCount())
            return createIndex(row, column, groupLayer);

    return QModelIndex();
}

QModelIndex LayerModel::parent(const QModelIndex &index) const
{
    if (auto groupLayer = static_cast<GroupLayer*>(index.internalPointer()))
        return LayerModel::index(groupLayer);
    return QModelIndex();
}

/**
 * Returns the number of rows.
 */
int LayerModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        TiledLayer *layer = toLayer(parent);
        Q_ASSERT(layer);
        if (GroupLayer *groupLayer = layer->asGroupLayer())
            return groupLayer->layerCount();
        return 0;
    }

    return mMap ? mMap->layerCount() : 0;
}

int LayerModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 1;
}

/**
 * Returns the data stored under the given <i>role</i> for the item
 * referred to by the <i>index</i>.
 */
QVariant LayerModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0)
        return QVariant();

    TiledLayer *layer = toLayer(index);

    switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
        return layer->name();
    case Qt::DecorationRole:
        switch (layer->layerType()) {
        case TiledLayer::TileLayerType:
            return mTileLayerIcon;
        case TiledLayer::ObjectGroupType:
            return mObjectGroupIcon;
        case TiledLayer::ImageLayerType:
            return mImageLayerIcon;
        case TiledLayer::GroupLayerType:
            return QApplication::style()->standardIcon(QStyle::SP_DirIcon);
        }
    case Qt::CheckStateRole:
        return layer->isVisible() ? Qt::Checked : Qt::Unchecked;
    case OpacityRole:
        return layer->opacity();
    default:
        return QVariant();
    }
}

/**
 * Allows for changing the name, visibility and opacity of a layer.
 */
bool LayerModel::setData(const QModelIndex &index, const QVariant &value,
                         int role)
{
    if (!index.isValid())
        return false;

    TiledLayer *layer = toLayer(index);

    if (role == Qt::CheckStateRole) {
        Qt::CheckState c = static_cast<Qt::CheckState>(value.toInt());
        const bool visible = (c == Qt::Checked);
        if (visible != layer->isVisible()) {
            QUndoCommand *command = new SetLayerVisible(mMapDocument,
                                                        layer,
                                                        visible);
            mMapDocument->undoStack()->push(command);
        }
        return true;
    } else if (role == OpacityRole) {
        bool ok;
        const qreal opacity = value.toDouble(&ok);
        if (ok) {
            if (layer->opacity() != opacity) {
                QUndoCommand *command = new SetLayerOpacity(mMapDocument,
                                                            layer,
                                                            opacity);
                mMapDocument->undoStack()->push(command);
            }
            return true;
        }
    } else if (role == Qt::EditRole) {
        const QString newName = value.toString();
        if (layer->name() != newName) {
            RenameLayer *rename = new RenameLayer(mMapDocument, layer,
                                                  newName);
            mMapDocument->undoStack()->push(rename);
        }
        return true;
    }

    return false;
}

/**
 * Makes sure the items are checkable and names editable.
 */
Qt::ItemFlags LayerModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags rc = QAbstractItemModel::flags(index);
    if (index.column() == 0)
        rc |= Qt::ItemIsUserCheckable | Qt::ItemIsEditable;
    return rc;
}

/**
 * Returns the headers for the table.
 */
QVariant LayerModel::headerData(int section, Qt::Orientation orientation,
                                int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
        case 0: return tr("Layer");
        }
    }
    return QVariant();
}

QModelIndex LayerModel::index(TiledLayer *layer) const
{
    if (!layer)
        return QModelIndex();

    Q_ASSERT(layer->map() == mMap);

    if (auto parentLayer = layer->parentLayer()) {
        int row = parentLayer->layers().indexOf(layer);
        Q_ASSERT(row != -1);
        return createIndex(row, 0, parentLayer);
    }

    int row = mMap->layers().indexOf(layer);
    Q_ASSERT(row != -1);
    return createIndex(row, 0, nullptr);
}

TiledLayer *LayerModel::toLayer(const QModelIndex &index) const
{
    if (!index.isValid())
        return nullptr;

    if (auto groupLayer = static_cast<GroupLayer*>(index.internalPointer()))
        return groupLayer->layerAt(index.row());

    return mMap->layerAt(index.row());
}

/**
 * Sets the map document associated with this model.
 */
void LayerModel::setMapDocument(MapDocument *mapDocument)
{
    if (mMapDocument == mapDocument)
        return;

    beginResetModel();
    mMapDocument = mapDocument;
    mMap = mMapDocument->map();
    endResetModel();
}

/**
 * Adds a layer to this model's map, inserting it at as a child of
 * \a parentLayer at the given \a index.
 */
void LayerModel::insertLayer(GroupLayer *parentLayer, int index, TiledLayer *layer)
{
    QModelIndex parent = LayerModel::index(parentLayer);
    beginInsertRows(parent, index, index);
    if (parentLayer)
        parentLayer->insertLayer(index, layer);
    else
        mMap->insertLayer(index, layer);
    endInsertRows();
    emit layerAdded(layer);
}

/**
 * Removes the layer at the given \a index from this model's map and
 * returns it. The caller becomes responsible for the lifetime of this
 * layer.
 */
TiledLayer *LayerModel::takeLayerAt(GroupLayer *parentLayer, int index)
{
    emit layerAboutToBeRemoved(parentLayer, index);
    QModelIndex parent = LayerModel::index(parentLayer);
    beginRemoveRows(parent, index, index);
    TiledLayer *layer;
    if (parentLayer)
        layer = parentLayer->takeLayerAt(index);
    else
        layer = mMap->takeLayerAt(index);
    endRemoveRows();
    emit layerRemoved(layer);
    return layer;
}

/**
 * Replaces the given \a layer with the \a replacement layer.
 *
 * The map will take ownership over the replacement layer, whereas the caller
 * becomes responsible for the replaced layer.
 */
void LayerModel::replaceLayer(TiledLayer *layer, TiledLayer *replacement)
{
    Q_ASSERT(layer->map() == mMapDocument->map());
    Q_ASSERT(!replacement->map());

    auto currentLayer = mMapDocument->currentLayer();

    auto parentLayer = layer->parentLayer();
    auto index = layer->siblingIndex();

    takeLayerAt(parentLayer, index);
    insertLayer(parentLayer, index, replacement);

    if (layer == currentLayer)
        mMapDocument->setCurrentLayer(replacement);
}

/**
 * Sets whether the layer at the given index is visible.
 */
void LayerModel::setLayerVisible(TiledLayer *layer, bool visible)
{
    if (layer->isVisible() == visible)
        return;

    layer->setVisible(visible);

    const QModelIndex modelIndex = index(layer);
    emit dataChanged(modelIndex, modelIndex);
    emit layerChanged(layer);
}

/**
 * Sets the opacity of the layer at the given index.
 */
void LayerModel::setLayerOpacity(TiledLayer *layer, float opacity)
{
    if (layer->opacity() == opacity)
        return;

    layer->setOpacity(opacity);
    emit layerChanged(layer);
}

/**
 * Sets the offset of the layer at the given index.
 */
void LayerModel::setLayerOffset(TiledLayer *layer, const QPointF &offset)
{
    if (layer->offset() == offset)
        return;

    layer->setOffset(offset);
    emit layerChanged(layer);
}

/**
 * Renames the layer at the given index.
 */
void LayerModel::renameLayer(TiledLayer *layer, const QString &name)
{
    if (layer->name() == name)
        return;

    layer->setName(name);

    const QModelIndex modelIndex = index(layer);
    emit dataChanged(modelIndex, modelIndex);
    emit layerChanged(layer);
}

/**
 * Collects sibling layers, including siblings of all parents.
 */
static QList<TiledLayer *> collectAllSiblings(TiledLayer *layer)
{
    QList<TiledLayer *> collected;

    while (layer) {
        const auto& siblings = layer->siblings();
        for (TiledLayer *sibling : siblings) {
            if (sibling != layer)
                collected.append(sibling);
        }
        layer = layer->parentLayer();
    }

    return collected;
}

/**
  * Show or hide all other layers except the given \a layer.
  * If any other layer is visible then all layers will be hidden, otherwise
  * the layers will be shown.
  */
void LayerModel::toggleOtherLayers(TiledLayer *layer)
{
    const auto& otherLayers = collectAllSiblings(layer);
    if (otherLayers.isEmpty())
        return;

    bool visibility = true;
    for (TiledLayer *l : otherLayers) {
        if (l->isVisible()) {
            visibility = false;
            break;
        }
    }

    QUndoStack *undoStack = mMapDocument->undoStack();
    if (visibility)
        undoStack->beginMacro(tr("Show Other Layers"));
    else
        undoStack->beginMacro(tr("Hide Other Layers"));

    for (TiledLayer *l : otherLayers) {
        if (visibility != l->isVisible())
            undoStack->push(new SetLayerVisible(mMapDocument, l, visibility));
    }

    undoStack->endMacro();
}
