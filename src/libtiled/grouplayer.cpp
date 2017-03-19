/*
 * grouplayer.cpp
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

#include "grouplayer.h"

#include "map.h"

namespace Tiled {

GroupLayer::GroupLayer(const QString &name, int x, int y):
    TiledLayer(GroupLayerType, name, x, y)
{
}

GroupLayer::~GroupLayer()
{
    qDeleteAll(mLayers);
}

void GroupLayer::addLayer(TiledLayer *layer)
{
    adoptLayer(layer);
    mLayers.append(layer);
}

void GroupLayer::insertLayer(int index, TiledLayer *layer)
{
    adoptLayer(layer);
    mLayers.insert(index, layer);
}

void GroupLayer::adoptLayer(TiledLayer *layer)
{
    layer->setParentLayer(this);

    if (map())
        map()->adoptLayer(layer);
    else
        layer->setMap(nullptr);
}

TiledLayer *GroupLayer::takeLayerAt(int index)
{
    TiledLayer *layer = mLayers.takeAt(index);
    layer->setMap(nullptr);
    layer->setParentLayer(nullptr);
    return layer;
}

bool GroupLayer::isEmpty() const
{
    return mLayers.isEmpty();
}

QSet<SharedTileset> GroupLayer::usedTilesets() const
{
    QSet<SharedTileset> tilesets;

    for (const TiledLayer *layer : mLayers)
        tilesets |= layer->usedTilesets();

    return tilesets;
}

bool GroupLayer::referencesTileset(const Tileset *tileset) const
{
    for (const TiledLayer *layer : mLayers)
        if (layer->referencesTileset(tileset))
            return true;

    return false;
}

void GroupLayer::replaceReferencesToTileset(Tileset *oldTileset, Tileset *newTileset)
{
    const auto &children = mLayers;
    for (TiledLayer *layer : children)
        layer->replaceReferencesToTileset(oldTileset, newTileset);
}

bool GroupLayer::canMergeWith(TiledLayer *) const
{
    // Merging group layers would be possible, but duplicating all child layers
    // is not the right approach.
    // todo: implement special case of reparenting child layers
    return false;
}

TiledLayer *GroupLayer::mergedWith(TiledLayer *) const
{
    return nullptr;
}

TiledLayer *GroupLayer::clone() const
{
    return initializeClone(new GroupLayer(mName, mX, mY));
}

void GroupLayer::setMap(Map *map)
{
    TiledLayer::setMap(map);

    if (map) {
        for (TiledLayer *layer : mLayers)
            map->adoptLayer(layer);
    } else {
        for (TiledLayer *layer : mLayers)
            layer->setMap(nullptr);
    }
}

GroupLayer *GroupLayer::initializeClone(GroupLayer *clone) const
{
    TiledLayer::initializeClone(clone);
    for (const TiledLayer *layer : mLayers)
        clone->addLayer(layer->clone());
    return clone;
}

} // namespace Tiled
