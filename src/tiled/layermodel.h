/*
 * layermodel.h
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

#pragma once

#include <QAbstractListModel>
#include <QIcon>

namespace Tiled {

class GroupLayer;
class TiledLayer;
class Map;

namespace Internal {

class MapDocument;

/**
 * A model wrapping the layers of a map. Used to display the layers in a view.
 * The model also allows modification of the layer stack while keeping the
 * layer views up to date.
 */
class LayerModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    /**
     * The OpacityRole allows querying and changing the layer opacity.
     */
    enum UserRoles {
        OpacityRole = Qt::UserRole
    };

    LayerModel(QObject *parent = nullptr);

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index,
                  int role = Qt::DisplayRole) const override;

    bool setData(const QModelIndex &index, const QVariant &value, int role) override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;

    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    QModelIndex index(TiledLayer *layer) const;
    TiledLayer *toLayer(const QModelIndex &index) const;

    MapDocument *mapDocument() const;
    void setMapDocument(MapDocument *mapDocument);

    void insertLayer(GroupLayer *parentLayer, int index, TiledLayer *layer);
    TiledLayer *takeLayerAt(GroupLayer *parentLayer, int index);
    void replaceLayer(TiledLayer *layer, TiledLayer *replacement);

    void setLayerVisible(TiledLayer *layer, bool visible);
    void setLayerOpacity(TiledLayer *layer, float opacity);
    void setLayerOffset(TiledLayer *layer, const QPointF &offset);

    void renameLayer(TiledLayer *layer, const QString &name);

    void toggleOtherLayers(TiledLayer *layer);

signals:
    void layerAdded(TiledLayer *layer);
    void layerAboutToBeRemoved(GroupLayer *parentLayer, int index);
    void layerRemoved(TiledLayer *layer);
    void layerChanged(TiledLayer *layer);

private:
    MapDocument *mMapDocument;
    Map *mMap;

    QIcon mTileLayerIcon;
    QIcon mObjectGroupIcon;
    QIcon mImageLayerIcon;
};

/**
 * Returns the map document associated with this model.
 */
inline MapDocument *LayerModel::mapDocument() const
{
    return mMapDocument;
}

} // namespace Internal
} // namespace Tiled
