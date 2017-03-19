/*
 * grouplayer.h
 * Copyright 2017, Thorbjørn Lindeijer <bjorn@lindeijer.nl>
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

#include "tiled_layer.h"

#include <QList>

namespace Tiled {

class TILEDSHARED_EXPORT GroupLayer : public TiledLayer
{
public:
    GroupLayer(const QString &name, int x, int y);
    ~GroupLayer();

    int layerCount() const;
    TiledLayer *layerAt(int index) const;
    const QList<TiledLayer*> &layers() const { return mLayers; }

    void addLayer(TiledLayer *layer);
    void insertLayer(int index, TiledLayer *layer);
    TiledLayer *takeLayerAt(int index);

    bool isEmpty() const override;
    QSet<SharedTileset> usedTilesets() const override;
    bool referencesTileset(const Tileset *tileset) const override;
    void replaceReferencesToTileset(Tileset *oldTileset, Tileset *newTileset) override;
    bool canMergeWith(TiledLayer *other) const override;
    TiledLayer *mergedWith(TiledLayer *other) const override;
    TiledLayer *clone() const override;

    // Enable easy iteration over children with range-based for
    QList<TiledLayer*>::iterator begin() { return mLayers.begin(); }
    QList<TiledLayer*>::iterator end() { return mLayers.end(); }
    QList<TiledLayer*>::const_iterator begin() const { return mLayers.begin(); }
    QList<TiledLayer*>::const_iterator end() const { return mLayers.end(); }

protected:
    void setMap(Map *map) override;
    GroupLayer *initializeClone(GroupLayer *clone) const;

private:
    void adoptLayer(TiledLayer *layer);

    QList<TiledLayer*> mLayers;
};


inline int GroupLayer::layerCount() const
{
    return mLayers.size();
}

inline TiledLayer *GroupLayer::layerAt(int index) const
{
    return mLayers.at(index);
}

} // namespace Tiled
