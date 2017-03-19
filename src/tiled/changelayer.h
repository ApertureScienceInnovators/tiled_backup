/*
 * changelayer.h
 * Copyright 2012-2013, Thorbjørn Lindeijer <thorbjorn@lindeijer.nl>
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

#include "undocommands.h"

#include <QPointF>
#include <QUndoCommand>

namespace Tiled {

class TiledLayer;

namespace Internal {

class MapDocument;

/**
 * Used for changing layer visibility.
 */
class SetLayerVisible : public QUndoCommand
{
public:
    SetLayerVisible(MapDocument *mapDocument,
                    TiledLayer *layer,
                    bool visible);

    void undo() override { swap(); }
    void redo() override { swap(); }

private:
    void swap();

    MapDocument *mMapDocument;
    TiledLayer *mLayer;
    bool mVisible;
};

/**
 * Used for changing layer opacity.
 */
class SetLayerOpacity : public QUndoCommand
{
public:
    SetLayerOpacity(MapDocument *mapDocument,
                    TiledLayer *layer,
                    float opacity);

    void undo() override { setOpacity(mOldOpacity); }
    void redo() override { setOpacity(mNewOpacity); }

    int id() const override { return Cmd_ChangeLayerOpacity; }

    bool mergeWith(const QUndoCommand *other) override;

private:
    void setOpacity(float opacity);

    MapDocument *mMapDocument;
    TiledLayer *mLayer;
    float mOldOpacity;
    float mNewOpacity;
};

/**
 * Used for changing the layer offset.
 */
class SetLayerOffset : public QUndoCommand
{
public:
    SetLayerOffset(MapDocument *mapDocument,
                   TiledLayer *layer,
                   const QPointF &offset,
                   QUndoCommand *parent = nullptr);

    void undo() override { setOffset(mOldOffset); }
    void redo() override { setOffset(mNewOffset); }

    int id() const override { return Cmd_ChangeLayerOffset; }

private:
    void setOffset(const QPointF &offset);

    MapDocument *mMapDocument;
    TiledLayer *mLayer;
    QPointF mOldOffset;
    QPointF mNewOffset;
};

} // namespace Internal
} // namespace Tiled
