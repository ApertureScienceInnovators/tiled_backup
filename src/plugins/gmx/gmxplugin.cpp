/*
 * GMX Tiled Plugin
 * Copyright 2016, Jones Blunt <mrjonesblunt@gmail.com>
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

#include "gmxplugin.h"

#include "map.h"
#include "tile.h"
#include "tilelayer.h"
#include "mapobject.h"
#include "objectgroup.h"

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSaveFile>
#include <QXmlStreamWriter>

using namespace Tiled;
using namespace Gmx;

template <typename T>
static T optionalProperty(const Object *object, const QString &name, const T &def)
{
    const QVariant var = object->property(name);
    return var.isValid() ? var.value<T>() : def;
}

template <typename T>
static QString toString(T number)
{
    return QString::number(number);
}

static QString toString(bool b)
{
    return QString::number(b ? -1 : 0);
}

template <typename T>
static void writeProperty(QXmlStreamWriter &writer,
                          const Object *object,
                          const QString &name,
                          const T &def)
{
    const T value = optionalProperty(object, name, def);
    writer.writeTextElement(name, toString(value));
}

static QString sanitizeName(const QString &name)
{
    QString sanitized = name;
    sanitized.replace(QRegularExpression(QLatin1String("[^a-zA-Z0-9]")),
                      QLatin1String("_"));
    return sanitized;
}

GmxPlugin::GmxPlugin()
{
}

bool GmxPlugin::write(const Map *map, const QString &fileName)
{
    QSaveFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        mError = tr("Could not open file for writing.");
        return false;
    }

    QXmlStreamWriter stream(&file);

    stream.setAutoFormatting(true);
    stream.setAutoFormattingIndent(2);

    stream.writeComment("This Document is generated by Tiled, if you edit it by hand then you do so at your own risk!");

    stream.writeStartElement("room");

    int mapPixelWidth = map->tileWidth() * map->width();
    int mapPixelHeight = map->tileHeight() * map->height();

    stream.writeTextElement("width", QString::number(mapPixelWidth));
    stream.writeTextElement("height", QString::number(mapPixelHeight));
    stream.writeTextElement("vsnap", QString::number(map->tileHeight()));
    stream.writeTextElement("hsnap", QString::number(map->tileWidth()));
    writeProperty(stream, map, "speed", 30);
    writeProperty(stream, map, "persistent", false);
    writeProperty(stream, map, "clearDisplayBuffer", true);

    stream.writeTextElement("isometric", toString(map->orientation() == Map::Orientation::Isometric));

    stream.writeStartElement("instances");

    QSet<QString> usedNames;

    for (const Layer *layer : map->layers()) {
        if (layer->layerType() != Layer::ObjectGroupType)
            continue;

        const ObjectGroup *objectLayer = static_cast<const ObjectGroup*>(layer);

        for (const MapObject *object : objectLayer->objects()) {
            if (object->type().isEmpty())
                continue;

            stream.writeStartElement("instance");

            // The type is used to refer to the name of the object
            stream.writeAttribute("objName", sanitizeName(object->type()));

            QPointF pos = object->position();
            qreal scaleX = 1;
            qreal scaleY = 1;

            if (!object->cell().isEmpty()) {
                // Tile objects have bottom-left origin in Tiled, so the
                // position needs to be translated for top-left origin in
                // GameMaker, taking into account the rotation.
                QTransform transform;
                transform.rotate(object->rotation());

                pos += transform.map(QPointF(0, -object->height()));

                // For tile objects we can support scaling and flipping, though
                // flipping in combination with rotation doesn't work in GameMaker.
                if (auto tile = object->cell().tile) {
                    const QSize tileSize = tile->size();
                    scaleX = object->width() / tileSize.width();
                    scaleY = object->height() / tileSize.height();

                    if (object->cell().flippedHorizontally) {
                        scaleX *= -1;
                        pos += transform.map(QPointF(object->width(), 0));
                    }
                    if (object->cell().flippedVertically) {
                        scaleY *= -1;
                        pos += transform.map(QPointF(0, object->height()));
                    }
                }
            }

            stream.writeAttribute("x", QString::number(qRound(pos.x())));
            stream.writeAttribute("y", QString::number(qRound(pos.y())));

            // Include object ID in the name when necessary because duplicates are not allowed
            if (object->name().isEmpty()) {
                stream.writeAttribute("name", QString("inst_%1").arg(object->id()));
            } else {
                QString name = sanitizeName(object->name());

                while (usedNames.contains(name))
                    name += QString("_%1").arg(object->id());

                usedNames.insert(name);
                stream.writeAttribute("name", name);
            }

            stream.writeAttribute("scaleX", QString::number(scaleX));
            stream.writeAttribute("scaleY", QString::number(scaleY));
            stream.writeAttribute("rotation", QString::number(-object->rotation()));

            stream.writeEndElement();
        }
    }

    stream.writeEndElement();


    stream.writeStartElement("tiles");

    uint tileId = 0u;
    int currentLayer = map->layers().size();

    for (const Layer *layer : map->layers()) {
        QString depth = QString::number(optionalProperty(layer, QLatin1String("depth"), currentLayer));

        switch (layer->layerType()) {
        case Layer::TileLayerType: {
            auto tileLayer = static_cast<const TileLayer*>(layer);
            for (int y = 0; y < tileLayer->height(); ++y) {
                for (int x = 0; x < tileLayer->width(); ++x) {
                    const Cell &cell = tileLayer->cellAt(x, y);
                    if (const Tile *tile = cell.tile) {
                        const Tileset *tileset = tile->tileset();

                        stream.writeStartElement("tile");

                        int pixelX = x * map->tileWidth();
                        int pixelY = y * map->tileHeight();
                        qreal scaleX = 1;
                        qreal scaleY = 1;

                        if (cell.flippedHorizontally) {
                            scaleX = -1;
                            pixelX += tile->width();
                        }
                        if (cell.flippedVertically) {
                            scaleY = -1;
                            pixelY += tile->height();
                        }

                        stream.writeAttribute("bgName", tileset->name());
                        stream.writeAttribute("x", QString::number(pixelX));
                        stream.writeAttribute("y", QString::number(pixelY));
                        stream.writeAttribute("w", QString::number(tile->width()));
                        stream.writeAttribute("h", QString::number(tile->height()));

                        int xInTilesetGrid = tile->id() % tileset->columnCount();
                        int yInTilesetGrid = (int)(tile->id() / tileset->columnCount());
                        stream.writeAttribute("xo", QString::number(tileset->margin() + (tileset->tileSpacing() + tileset->tileWidth()) * xInTilesetGrid));
                        stream.writeAttribute("yo", QString::number(tileset->margin() + (tileset->tileSpacing() + tileset->tileHeight()) * yInTilesetGrid));

                        stream.writeAttribute("id", QString::number(++tileId));
                        stream.writeAttribute("depth", depth);

                        stream.writeAttribute("scaleX", QString::number(scaleX));
                        stream.writeAttribute("scaleY", QString::number(scaleY));

                        stream.writeEndElement();
                    }
                }
            }
            break;
        }

        case Layer::ObjectGroupType: {
            auto objectGroup = static_cast<const ObjectGroup*>(layer);
            auto objects = objectGroup->objects();

            // Make sure the objects export in the rendering order
            if (objectGroup->drawOrder() == ObjectGroup::TopDownOrder) {
                qStableSort(objects.begin(), objects.end(),
                            [](const MapObject *a, const MapObject *b) { return a->y() < b->y(); });
            }

            foreach (const MapObject *object, objects) {
                // Objects with types are already exported as instances
                if (!object->type().isEmpty())
                    continue;

                // Non-typed tile objects are exported as tiles. Rotation is
                // not supported here, but scaling is.
                if (const Tile *tile = object->cell().tile) {
                    const Tileset *tileset = tile->tileset();

                    const QSize tileSize = tile->size();
                    qreal scaleX = object->width() / tileSize.width();
                    qreal scaleY = object->height() / tileSize.height();
                    qreal x = object->x();
                    qreal y = object->y() - object->height();

                    if (object->cell().flippedHorizontally) {
                        scaleX *= -1;
                        x += object->width();
                    }
                    if (object->cell().flippedVertically) {
                        scaleY *= -1;
                        y += object->height();
                    }

                    stream.writeStartElement("tile");

                    stream.writeAttribute("bgName", tileset->name());
                    stream.writeAttribute("x", QString::number(qRound(x)));
                    stream.writeAttribute("y", QString::number(qRound(y)));
                    stream.writeAttribute("w", QString::number(tile->width()));
                    stream.writeAttribute("h", QString::number(tile->height()));

                    int xInTilesetGrid = tile->id() % tileset->columnCount();
                    int yInTilesetGrid = (int)(tile->id() / tileset->columnCount());
                    stream.writeAttribute("xo", QString::number(tileset->margin() + (tileset->tileSpacing() + tileset->tileWidth()) * xInTilesetGrid));
                    stream.writeAttribute("yo", QString::number(tileset->margin() + (tileset->tileSpacing() + tileset->tileHeight()) * yInTilesetGrid));

                    stream.writeAttribute("id", QString::number(++tileId));
                    stream.writeAttribute("depth", depth);

                    stream.writeAttribute("scaleX", QString::number(scaleX));
                    stream.writeAttribute("scaleY", QString::number(scaleY));

                    stream.writeEndElement();
                }
            }
            break;
        }
        }

        currentLayer--;
    }

    stream.writeEndElement();

    writeProperty(stream, map, "PhysicsWorld", false);
    writeProperty(stream, map, "PhysicsWorldTop", 0);
    writeProperty(stream, map, "PhysicsWorldLeft", 0);
    writeProperty(stream, map, "PhysicsWorldRight", mapPixelWidth);
    writeProperty(stream, map, "PhysicsWorldBottom", mapPixelHeight);
    writeProperty(stream, map, "PhysicsWorldGravityX", 0.0);
    writeProperty(stream, map, "PhysicsWorldGravityY", 10.0);
    writeProperty(stream, map, "PhysicsWorldPixToMeters", 0.1);

    stream.writeEndDocument();

    if (!file.commit()) {
        mError = file.errorString();
        return false;
    }

    return true;
}

QString GmxPlugin::errorString() const
{
    return mError;
}

QString GmxPlugin::nameFilter() const
{
    return tr("GameMaker room files (*.room.gmx)");
}
