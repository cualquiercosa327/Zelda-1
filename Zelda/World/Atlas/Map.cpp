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

#include "Map.hpp"
#include "../../Common/Physics/BtOgrePG.hpp"
#include "../../Common/Util/DeleteSceneNode.hpp"
#include <OgreSceneManager.h>
#include <OgreSceneNode.h>
#include <OgreEntity.h>
#include <OgreLogManager.h>
#include <OgreMeshManager.h>
#include "Region.hpp"
#include "../GlobalCollisionShapesTypes.hpp"
#include "../Objects/Object.hpp"
#include "BulletCollision/CollisionShapes/btSphereShape.h"

CMap::CMap(CEntity *pAtlas, CMapPackPtr mapPack, Ogre::SceneNode *pParentSceneNode, CWorldEntity *pPlayer)
  : CWorldEntity(mapPack->getName(), pAtlas, this),
    m_PhysicsManager(pParentSceneNode->getCreator()),
    m_MapPack(mapPack),
    m_pPlayer(pPlayer) {

  Ogre::LogManager::getSingleton().logMessage("Construction of map '" + m_MapPack->getName() + "'");

  // Create global collision shapes
  m_PhysicsManager.addCollisionShape(GLOBAL_COLLISION_SHAPES_TYPES_ID_MAP.toString(GCST_PICKABLE_OBJECT_SPHERE),
                                     CPhysicsCollisionObject(new btSphereShape(0.05)));

  m_pSceneNode = pParentSceneNode->createChildSceneNode(m_MapPack->getName() + "_RootNode");

  m_pStaticGeometry = pParentSceneNode->getCreator()->createStaticGeometry(m_MapPack->getName() + "_StaticGeometry");
  m_pStaticGeometry->setRegionDimensions(Ogre::Vector3(10, 10, 10));

  m_MapPack->init(this);


  m_SceneLoader.addCallback(this);
  m_SceneLoader.parseDotScene(m_MapPack->getSceneFile(),
                              m_MapPack->getResourceGroup(),
                              m_pSceneNode->getCreator(),
                              &m_PhysicsManager,
                              m_pSceneNode,
                              m_MapPack->getName());


  CreateCube(btVector3(0, 10, 0.2), 1);
  CreateCube(btVector3(0, 200, 0.3), 100);

  //new CObject(m_MapPack->getName() + "rupee", this, this, OBJECT_GREEN_BUSH);

  m_pStaticGeometry->build();
}

CMap::~CMap() {
  Ogre::LogManager::getSingleton().logMessage("Destruction of map '" + m_MapPack->getName() + "'");
}

void CMap::start() {
  sendCallToAll(&CEntity::start, false);
}

void CMap::exit() {
  m_pSceneNode->getCreator()->destroyStaticGeometry(m_pStaticGeometry);
  m_pStaticGeometry = nullptr;
}

void CMap::CreateCube(const btVector3 &Position, btScalar Mass)
{
    // empty ogre vectors for the cubes size and position
    Ogre::Vector3 size = Ogre::Vector3::ZERO;
    Ogre::Vector3 pos = Ogre::Vector3::ZERO;

    // Convert the bullet physics vector to the ogre vector
    pos.x = Position.getX();
    pos.y = Position.getY();
    pos.z = Position.getZ();


    // Create a cube by using a cube mesh (you can find one in the ogre samples)
    Ogre::Entity *entity =

  m_pSceneNode->getCreator()->createEntity(
        "Cylinder.mesh");

    // This gets us the size of the bounding box but since the bounding box in Ogre
    // is a little bigger than the object itself we will cut that down slightly to make the physics
    // more accurate.
    Ogre::AxisAlignedBox boundingB = entity->getBoundingBox();
    size = boundingB.getSize()*0.95f;

    // apply a material to the cube so it isn't gray like the ground
    entity->setMaterialName("Examples/Rockwall");
    entity->setCastShadows(true);

    // Create a sceneNode and attach the entity for rendering
    Ogre::SceneNode *node = m_pSceneNode->createChildSceneNode();
    node->attachObject(entity);
    node->setPosition(pos);  // This gives it the initial position as provided

    // Physics
    btTransform Transform;
    Transform.setIdentity();
    Transform.setOrigin(Position);  // Set the position of the rigid body to match the sceneNode

    // Give it to the motion state
    BtOgre::RigidBodyState *MotionState = new BtOgre::RigidBodyState(node, Transform);

   // Being new myself I'm not sure why this happen but we give the rigid body half the size
   // of our cube and tell it to create a BoxShape (cube)
    btVector3 HalfExtents(size.x*0.5f,size.y*0.5f,size.z*0.5f);
    btCollisionShape *Shape = new btCylinderShape(HalfExtents);

    // Add Mass to the object so it is appropriately affected by other objects and gravity
    // In this case we pass the mass into the function when we call it.
    btVector3 LocalInertia;
    Shape->calculateLocalInertia(Mass, LocalInertia);

    // Create the rigid body object
    btRigidBody *RigidBody = new btRigidBody(Mass, MotionState, Shape, LocalInertia);

    // Add it to the physics world
    m_PhysicsManager.getWorld()->addRigidBody(RigidBody, 32, 1023);
}

void CMap::moveMap(const Ogre::Vector3 &offset) {
  m_bPauseUpdate = true;
  m_pSceneNode->translate(offset);
}

void CMap::update(Ogre::Real tpf) {
  CWorldEntity::update(tpf);
}

bool CMap::frameStarted(const Ogre::FrameEvent& evt) {
  if (m_bPauseUpdate) {return true;}
  m_PhysicsManager.update(evt.timeSinceLastFrame);
  return CWorldEntity::frameStarted(evt);
}

bool CMap::frameEnded(const Ogre::FrameEvent& evt) {
  return CWorldEntity::frameEnded(evt);
}

// ############################################################################3
// MapPackParserListener

void CMap::parsePlayer(const tinyxml2::XMLElement *pElem) {
  m_pPlayer->readEventsFromXMLElement(pElem, true);
}

void CMap::parseRegion(const SRegionInfo &region) {
  new CRegion(this, region);
}

// ############################################################################3
// CDotSceneLoaderCallback

void CMap::staticObjectAdded(Ogre::Entity *pEntity, Ogre::SceneNode *pParent) {
  m_pStaticGeometry->addEntity(pEntity, pParent->getPosition(), pParent->getOrientation());
  //destroySceneNode(pParent, true);
}

CDotSceneLoaderCallback::EResults CMap::preEntityAdded(tinyxml2::XMLElement *XMLNode, Ogre::SceneNode *pParent, CUserData &userData) {
  CWorldEntity *pEntity(nullptr);
  if (strcmp(XMLNode->Attribute("meshFile"), "GreenBush.mesh") == 0) {
    pEntity = new CObject(pParent->getName(), this, this, OBJECT_GREEN_BUSH, pParent);

    pEntity->setPosition(pParent->getPosition());
    return R_CANCEL;
  }
  return R_CONTINUE;
}
