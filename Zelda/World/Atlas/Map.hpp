/*****************************************************************************
 * Copyright 2014 Christoph Wick
 *
 * This file is part of Zelda.
 *
 * Zelda is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * Zelda is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * Zelda. If not, see http://www.gnu.org/licenses/.
 *****************************************************************************/

#ifndef _MAP_HPP_
#define _MAP_HPP_

#include "../WorldEntity.hpp"
#include "../../Common/Physics/PhysicsManager.hpp"
#include "MapPack.hpp"
#include "../../Common/DotSceneLoader/DotSceneLoader.hpp"
#include "../../Common/DotSceneLoader/DotSceneLoaderCallback.hpp"
#include <btBulletDynamicsCommon.h>
#include "MapPackParserListener.hpp"
#include <OgreStaticGeometry.h>

class CMap : public CWorldEntity,
             private CMapPackParserListener,
             private CDotSceneLoaderCallback {
private:
  CPhysicsManager m_PhysicsManager;
  CMapPackPtr m_MapPack;
  Ogre::DotSceneLoader m_SceneLoader;
  CWorldEntity *m_pPlayer;
  Ogre::StaticGeometry *m_pStaticGeometry;
public:
  CMap(CEntity *pAtlas, CMapPackPtr mapPack, Ogre::SceneNode *pParentSceneNode, CWorldEntity *pPlayer);
  virtual ~CMap();

  void start();
  void exit();

  void CreateCube(const btVector3 &Position, btScalar Mass);

  void moveMap(const Ogre::Vector3 &offset);

  const CPhysicsManager *getPhysicsManager() const {return &m_PhysicsManager;}
  CPhysicsManager *getPhysicsManager() {return &m_PhysicsManager;}
  const CMapPackPtr getMapPack() const {return m_MapPack;}

  void update(Ogre::Real tpf);
  bool frameStarted(const Ogre::FrameEvent& evt);
  bool frameEnded(const Ogre::FrameEvent& evt);

private:
  void parsePlayer(const tinyxml2::XMLElement *);
  void parseRegion(const SRegionInfo &);

  // CDotSceneLoaderCallback
	void staticObjectAdded(Ogre::Entity *pEntity, Ogre::SceneNode *pParent);
  EResults preEntityAdded(tinyxml2::XMLElement *XMLNode, Ogre::SceneNode *pParent, CUserData &userData);

};
#endif // _MAP_HPP_
