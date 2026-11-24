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
 * @brief This file contains the fabgl::Scene definition.
 */


#include "freertos/FreeRTOS.h"

#include "fabglconf.h"
#include "collisiondetector.h"



#define FABGL_DEFAULT_SCENETASK_STACKSIZE 2048


namespace fabgl {



/**
 * @brief Scene is an abstract class that encapsulates the functionality of a scene (sprites, collision detection and periodic updates).
 *
 * @see @ref guide_sprites
 *
 * @ingroup games
 */
class Scene {

public:

  /**
   * @brief The Scene constructor.
   *
   * @param maxSpritesCount Maximum number of sprites. Required to size the collision detector.
   * @param updateTimeMS Time between updates, in milliseconds. update() is called at each update.
   * @param width Scene width in pixels.
   * @param height Scene height in pixels.
   * @param stackSize Stack size of the update task.
   */
  Scene(int maxSpritesCount, int updateTimeMS, int width, int height, int stackSize = FABGL_DEFAULT_SCENETASK_STACKSIZE);

  virtual ~Scene();

  /**
   * @brief Determines the scene width.
   *
   * @return Scene width in pixels.
   */
  int getWidth()  { return m_width; }

  /**
   * @brief Determines the scene height.
   *
   * @return Scene height in pixels.
   */
  int getHeight() { return m_height; }

  /**
   * @brief Starts the scene updates and suspends the current task.
   *
   * @param suspendTask If true (default), the calling task is suspended immediately.
   *
   * Example:
   *
   *     extern "C" void app_main()
   *     {
   *       GameScene gameScene;
   *       gameScene.start();
   *     }
   */
  void start(bool suspendTask = true);

  /**
   * @brief Stops the scene updates and resumes the suspended task.
   */
  void stop();

  /**
   * @brief Abstract method called when the scene must be initialized.
   */
  virtual void init() = 0;

  /**
   * @brief Abstract method called whenever the scene must be updated.
   *
   * @param updateCount Number of times update() has been called.
   */
  virtual void update(int updateCount) = 0;

  /**
   * @brief Abstract method called whenever a collision has been detected.
   *
   * This method is called once for each collision detected by updateSpriteAndDetectCollisions().
   *
   * @param spriteA The first colliding sprite. This is the sprite passed to updateSpriteAndDetectCollisions().
   * @param spriteB The second colliding sprite.
   * @param collisionPoint Coordinates of a collision point.
   */
  virtual void collisionDetected(Sprite * spriteA, Sprite * spriteB, Point collisionPoint) = 0;

  /**
   * @brief Adds the specified sprite to the collision detector.
   *
   * The collision detector is updated by calling updateSprite() or updateSpriteAndDetectCollisions().<br>
   * The number of sprites cannot exceed the value specified in the Scene constructor.
   *
   * @param sprite The sprite to add.
   */
  void addSprite(Sprite * sprite)    { m_collisionDetector.addSprite(sprite); }

  /**
   * @brief Removes the specified sprite from the collision detector.
   *
   * @param sprite The sprite to remove.
   */
  void removeSprite(Sprite * sprite) { m_collisionDetector.removeSprite(sprite); }

  /**
   * @brief Updates the collision detector.
   *
   * The collision detector must be updated whenever a sprite changes its position or size.<br>
   * This method only updates the detector, without generating collision events.<br>
   * To generate collision events, call updateSpriteAndDetectCollisions() instead.
   *
   * @param sprite The sprite to update.
   */
  void updateSprite(Sprite * sprite) { m_collisionDetector.update(sprite); }

  /**
   * @brief Updates the collision detector and generates collision events.
   *
   * The collision detector must be updated whenever a sprite changes its position or size.<br>
   * This method updates the detector and generates the corresponding collision events.
   *
   * @param sprite The sprite to update and to check for collisions.
   */
  void updateSpriteAndDetectCollisions(Sprite * sprite);

private:

  static void updateTask(void * pvParameters);


  int               m_width;
  int               m_height;

  TaskHandle_t      m_updateTaskHandle;
  int               m_updateCount;
  int               m_updateTimeMS;
  SemaphoreHandle_t m_mutex;

  CollisionDetector m_collisionDetector;

  TaskHandle_t      m_suspendedTask;

  volatile bool     m_running;

};





} // end of namespace






