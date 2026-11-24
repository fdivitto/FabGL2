/*
  Created by Fabrizio Di Vittorio (fdivitto2013@gmail.com) - <http://www.fabgl.com>
  Copyright (c) 2019-2026 Fabrizio Di Vittorio.
  All rights reserved.


* Please contact fdivitto2013@gmail.com if you need a commercial license.


* This library and related software is available under GPL v3.

  FabGL is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  FabGL is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with FabGL.  If not, see <http://www.gnu.org/licenses/>.
 */


#pragma once



/**
 * @file
 *
 * @brief This file contains the fabgl::CollisionDetector class definition
 */


#include <stdint.h>
#include <stddef.h>

#include "fabglconf.h"
#include "displaycontroller.h"



namespace fabgl {



class CollisionDetector;

class QuadTree;


struct QuadTreeObject {
  QuadTree *       owner;
  QuadTreeObject * next;
  Sprite *         sprite;

  QuadTreeObject(QuadTreeObject * next_, Sprite * sprite_)
    : owner(nullptr), next(next_), sprite(sprite_)
  {
  }
};


#define QUADTREE_LEVEL_SPLIT_THRESHOLD 3


enum QuadTreeQuadrant {
  TopLeft,
  TopRight,
  BottomLeft,
  BottomRight,
  None,
};


typedef void (*CollisionDetectionCallback)(void * callbackObj, Sprite * spriteA, Sprite * spriteB, Point collisionPoint);


class QuadTree {

public:

  QuadTree(CollisionDetector * collisionDetector, QuadTree * parent, QuadTreeQuadrant quadrant, int x, int y, int width, int height);

  void insert(QuadTreeObject * object);

  static void remove(QuadTreeObject * object);

  //void dump(int level = 0);

  QuadTreeObject * detectCollision(QuadTreeObject * object, CollisionDetectionCallback callbackFunc = nullptr, void * callbackObj = nullptr);

  bool isEmpty();

  void detachFromParent();

  static void update(QuadTreeObject * object);

private:

  QuadTreeQuadrant getQuadrant(QuadTreeObject * object);
  void createQuadrant(QuadTreeQuadrant quadrant);
  bool objectsIntersect(QuadTreeObject * objectA, QuadTreeObject * objectB);
  bool objectIntersectsQuadTree(QuadTreeObject * object, QuadTree * quadTree);
  bool checkMaskCollision(QuadTreeObject * objectA, QuadTreeObject * objectB, Point * collisionPoint);

  static bool objectInRect(QuadTreeObject * object, int x, int y, int width, int height);

  CollisionDetector * m_collisionDetector;
  QuadTree *          m_parent;
  QuadTreeQuadrant    m_quadrant;
  int                 m_x;
  int                 m_y;
  int                 m_width;
  int                 m_height;
  QuadTreeObject *    m_objects;
  int                 m_objectsCount;
  QuadTree *          m_children[4]; // index is Quadrant

};



/**
 * @brief Detects collisions between sprites.
 *
 * CollisionDetector uses a quadtree to efficiently store sprite sizes and positions and to quickly detect collisions.<br>
 * CollisionDetector is embedded in the Scene class, so applications usually do not need to instantiate it.
 *
 * @ingroup games
 */
class CollisionDetector {

friend class QuadTree;

public:

  /**
   * @brief Creates an instance of CollisionDetector.
   *
   * CollisionDetector is embedded in the Scene class, so applications usually do not need to instantiate it.
   *
   * @param maxObjectsCount Maximum number of sprites. Required to size the underlying quadtree.
   * @param width Scene width in pixels.
   * @param height Scene height in pixels.
   */
  CollisionDetector(int maxObjectsCount, int width, int height);

  ~CollisionDetector();

  /**
   * @brief Adds the specified sprite to the collision detector.
   *
   * The collision detector is updated by calling update() or updateAndDetectCollision().<br>
   * The number of sprites cannot exceed the value specified in the CollisionDetector constructor.
   *
   * @param sprite The sprite to add.
   */
  void addSprite(Sprite * sprite);

  /**
   * @brief Removes the specified sprite from the collision detector.
   *
   * @param sprite The sprite to remove.
   */
  void removeSprite(Sprite * sprite);

  /**
   * @brief Detects the first collision with the specified sprite.
   *
   * Call update() before detectCollision(), so that sprite positions and sizes are up to date.
   *
   * @param sprite The sprite to check for collisions.
   * @param removeCollidingSprites If true, the colliding sprites are automatically removed from the collision detector.
   *
   * @return The other sprite that collided with the specified sprite.
   */
  Sprite * detectCollision(Sprite * sprite, bool removeCollidingSprites = true);

  /**
   * @brief Detects multiple collisions with the specified sprite.
   *
   * Call update() before detectCollisions(), so that sprite positions and sizes are up to date.
   *
   * @param sprite The sprite to check for collisions.
   * @param callbackFunc The callback function called whenever a collision is detected.
   * @param callbackObj Pointer passed as parameter to the callback function.
   */
  void detectCollision(Sprite * sprite, CollisionDetectionCallback callbackFunc, void * callbackObj);

  /**
   * @brief Updates the collision detector.
   *
   * The collision detector must be updated whenever a sprite changes its position or size.<br>
   * This method only updates the detector, without generating collision events.
   *
   * @param sprite The sprite to update.
   */
  void update(Sprite * sprite);

  /**
   * @brief Updates the collision detector and detects a collision with the specified sprite.
   *
   * The collision detector must be updated whenever a sprite changes its position or size.<br>
   * This method updates the detector and detects collisions.
   *
   * @param sprite The sprite to update and to check for collisions.
   * @param removeCollidingSprites If true, the colliding sprites are automatically removed from the collision detector.
   *
   * @return The other sprite that collided with the specified sprite.
   */
  Sprite * updateAndDetectCollision(Sprite * sprite, bool removeCollidingSprites = true);

  /**
   * @brief Updates the collision detector and detects multiple collisions with the specified sprite.
   *
   * The collision detector must be updated whenever a sprite changes its position or size.<br>
   * This method updates the detector and detects multiple collisions.
   *
   * @param sprite The sprite to update and to check for collisions.
   * @param callbackFunc The callback function called whenever a collision is detected.
   * @param callbackObj Pointer passed as parameter to the callback function.
   */
  void updateAndDetectCollision(Sprite * sprite, CollisionDetectionCallback callbackFunc, void * callbackObj);

  //void dump();


private:

  QuadTree * initEmptyQuadTree(QuadTree * parent, QuadTreeQuadrant quadrant, int x, int y, int width, int height);


  QuadTree *       m_rootQuadTree;
  QuadTree *       m_quadTreePool;
  int              m_quadTreePoolSize;
  QuadTreeObject * m_objectPool;
  int              m_objectPoolSize;
};





} // end of namespace




