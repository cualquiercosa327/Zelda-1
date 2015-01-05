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

#include "Object.hpp"
#include <BulletCollision/CollisionShapes/btCollisionShape.h>
#include <BulletDynamics/Dynamics/btRigidBody.h>
#include <string>
#include <cstdint>
#include <OgreSceneNode.h>
#include <OgreSceneManager.h>
#include <OgreEntity.h>
#include "../../Common/Physics/BtOgrePG.hpp"
#include "../../Common/Physics/PhysicsManager.hpp"
#include "../../Common/Physics/PhysicsMasks.hpp"
#include "../GlobalCollisionShapesTypes.hpp"
#include "../Atlas/Map.hpp"
#include "../../Common/Util/Assert.hpp"
#include "../../Common/GameLogic/Events/Emitter/EmitOnCollision.hpp"
#include "../../Common/GameLogic/Events/Emitter/EmitOnStatusChange.hpp"
#include "../../Common/GameLogic/Events/Actions/ActionDeleteObject.hpp"
#include "../../Common/GameLogic/Events/Actions/ActionMessage.hpp"
#include "../../Common/GameLogic/Events/Event.hpp"
#include "../../Common/Message/MessagePlayerPickupItem.hpp"
#include "../../Common/Message/MessageHandler.hpp"
#include "../Damage.hpp"


unsigned int OBJECT_INNER_OBJECT_ID_NUMBER_COUNTER = 0;

CObject::CObject(const std::string &id,
                 CWorldEntity *pParent,
                 CMap *pMap,
                 EObjectTypes eObjectType,
                 Ogre::SceneNode *pSceneNode)
  : CWorldEntity(id, pParent, pMap),
    m_ObjectTypeData(CObjectDataMap::getSingleton().toData(eObjectType)),
    mInnerObjectGenerator(InnerObject::DEFAULT_GENERATOR_DATA_MAP
                          .toData(eObjectType)) {
  setType(eObjectType);

  if (pSceneNode) {
    m_pSceneNode = pSceneNode;
  } else {
    m_pSceneNode = pParent->getSceneNode()->createChildSceneNode(id);
  }

  Ogre::SceneManager *pSceneManager = m_pSceneNode->getCreator();

  Ogre::Entity *pEntity(nullptr);

  // create entity
  if (m_ObjectTypeData.bPermanentStatic) {
    m_pMap->addStaticEntity(m_ObjectTypeData.sMeshName + ".mesh",
                            m_pSceneNode->getPosition(),
                            m_pSceneNode->getOrientation());
    setCurAndMaxHP(HP_INFINITY);
  } else {
    pEntity = pSceneManager->createEntity(pMap->getPrependNodeName() + id + "ent",
                                          m_ObjectTypeData.sMeshName + ".mesh",
                                          "World");
    if (m_ObjectTypeData.sMaterialName.size() > 0) {
      pEntity->setMaterialName(m_ObjectTypeData.sMaterialName);
    }

    m_pSceneNode->attachObject(pEntity);
    pEntity->setCastShadows(false);
  }
}

CObject::~CObject() {
}

void CObject::init() {
  createPhysics();

  switch (m_uiType) {
    case OBJECT_GREEN_RUPEE:
    case OBJECT_BLUE_RUPEE:
    case OBJECT_RED_RUPEE:
    case OBJECT_HEART:
    case OBJECT_TOOL_LAMP:
      makePickable();
      break;
    default:
      break;
  }
}

void CObject::exit() {
  destroyPhysics();
  CWorldEntity::exit();
}

void CObject::destroyPhysics() {
  if (m_pCollisionObject) {
    btRigidBody *pRigidBody(btRigidBody::upcast(m_pCollisionObject));
    m_pMap->getPhysicsManager()->getWorld()->removeRigidBody(pRigidBody);
    if (m_ObjectTypeData.eCollisionShape == GCST_COUNT) {
      delete m_pCollisionObject->getCollisionShape();
    }
    delete pRigidBody->getMotionState();
    delete pRigidBody;
    m_pCollisionObject = nullptr;
  }
}

void CObject::createPhysics() {
  ASSERT(!m_pCollisionObject);

  btCollisionShape *pCollisionShape(nullptr);
  btVector3 vCollisionShapeOffset;
  btVector3 vInertia;
  float fMass = m_ObjectTypeData.bPermanentStatic ? 0 : 0.1;

  if (m_ObjectTypeData.eCollisionShape != GCST_COUNT) {
    const CPhysicsCollisionObject &pco
        = m_pMap->getPhysicsManager()->getCollisionShape(
            CGlobalCollisionShapesTypesIdMap::getSingleton().toString(
                m_ObjectTypeData.eCollisionShape));
    pCollisionShape = pco.getShape();
    vCollisionShapeOffset = BtOgre::Convert::toBullet(pco.getOffset());
  } else {
    // Handle shape by user
  }
  //m_pSceneNode->translate(BtOgre::Convert::toOgre(vCollisionShapeOffset));

  btVector3 initPosition = BtOgre::Convert::toBullet(
      m_pSceneNode->getPosition());
  btQuaternion initRotation = BtOgre::Convert::toBullet(
      m_pSceneNode->getOrientation());

  // initPosition += vCollisionShapeOffset;


  ASSERT(pCollisionShape);

  pCollisionShape->calculateLocalInertia(fMass, vInertia);
  
  btMotionState *pMotionState(nullptr);
  if (m_ObjectTypeData.bPermanentStatic) {
    pMotionState = new btDefaultMotionState(
        btTransform(initRotation, initPosition),
        btTransform(btQuaternion::getIdentity(), vCollisionShapeOffset));
  } else {
    btQuaternion rotationOffset(btQuaternion::getIdentity());
    pMotionState
        = new BtOgre::RigidBodyState(m_pSceneNode,
                                     btTransform(initRotation,
                                                 initPosition),
                                     btTransform(btQuaternion::getIdentity(),
                                                 vCollisionShapeOffset));
  }
  btRigidBody *pRigidBody = new btRigidBody(fMass,
                                            pMotionState,
                                            pCollisionShape,
                                            vInertia);
  m_pCollisionObject = pRigidBody;

  setThisAsCollisionObjectsUserPointer();
  // m_pSceneNode->setScale(Ogre::Vector3::UNIT_SCALE * 10);


  uint16_t group = COL_INTERACTIVE;
  uint16_t mask = MASK_INTERACIVE_OBJECT_COLLIDES_WITH;

  // post creation
  switch (m_uiType) {
    case OBJECT_GREEN_RUPEE:
    case OBJECT_BLUE_RUPEE:
    case OBJECT_RED_RUPEE:
    case OBJECT_HEART:
      pRigidBody->setAngularVelocity(btVector3(0, 2, 0));
      pRigidBody->setAngularFactor(btVector3(0, 1, 0));
      pRigidBody->setLinearFactor(btVector3(1.0f, 1.0f, 1.0f) * 1);
      pRigidBody->setLinearVelocity(btVector3(0, 1.0f, 0));
      pRigidBody->setDamping(0.9, 0);
      pRigidBody->setRestitution(0.6);
      pRigidBody->setFriction(0.1);
      group = COL_DAMAGE_N;
      mask |= MASK_DAMAGE_N_COLLIDES_WITH;
      pRigidBody->setCcdMotionThreshold(0.1);
      pRigidBody->setCcdSweptSphereRadius(0.02f);
      break;
    case OBJECT_GREEN_BUSH:
    case OBJECT_LIGHT_STONE:
    case OBJECT_LIGHT_STONE_PILE:
    case OBJECT_VASE:
      if (m_eState == EST_LIFTED) {
        group = COL_DAMAGE_P;
        mask = MASK_DAMAGE_P_COLLIDES_WITH;
      }
      pRigidBody->setLinearFactor(btVector3(0, 0, 0));
      pRigidBody->setAngularFactor(0);
      break;
    case OBJECT_TOOL_LAMP:
      group = COL_DAMAGE_N;
      mask |= MASK_DAMAGE_N_COLLIDES_WITH;
      pRigidBody->setLinearFactor(btVector3(0, 0, 0));
      pRigidBody->setAngularVelocity(btVector3(0, 2, 0));
      pRigidBody->setAngularFactor(btVector3(0, 1, 0));
      break;
    default:
      break;
  }

  // m_pCollisionObject->getWorldTransform().getOrigin() -= vCollisionShapeOffset;


  m_pMap->getPhysicsManager()->getWorld()->addRigidBody(pRigidBody,
                                                        group,
                                                        mask);
}

void CObject::makePickable() {
  using events::CEvent;
  using events::CEmitter;
  using events::CAction;
  using events::CEmitOnCollision;
  using events::CActionDeleteObject;
  using events::CActionMessage;

  CEvent *pEvent = new CEvent(*this);

  CEmitter *pEmitOnCollision = new CEmitOnCollision("player", *pEvent);
  pEvent->addEmitter(pEmitOnCollision);

  CAction *pDeleteThisAction = new CActionDeleteObject(this, *pEvent);
  pEvent->addAction(pDeleteThisAction);

  CAction *pMessageAction
      = new CActionMessage(std::make_shared<CMessagePlayerPickupItem>(__MSG_LOCATION__, m_uiType), *pEvent);
  pEvent->addAction(pMessageAction);

  addEvent(pEvent);
}

void CObject::enterMap(CMap *pMap, const Ogre::Vector3 &vPosition) {
  // attach to new CEntity
  attachTo(pMap);

  // change scene node owner
  m_pSceneNode->getParent()->removeChild(m_pSceneNode);
  pMap->getSceneNode()->addChild(m_pSceneNode);

  // recreate physics
  destroyPhysics();
  m_pMap = pMap;
  createPhysics();

  setPosition(vPosition);
}

void CObject::changeState(EEntityStateTypes eState) {
  switch (m_uiType) {
  case OBJECT_GREEN_BUSH:
  case OBJECT_LIGHT_STONE:
  case OBJECT_VASE:
    if (eState == EST_LIFTED) {
      btRigidBody *pRB = btRigidBody::upcast(this->getCollisionObject());
      ASSERT(pRB);

      m_pMap->getPhysicsManager()->getWorld()->removeCollisionObject(pRB);
      m_pMap->getPhysicsManager()->getWorld()
          ->addRigidBody(pRB,
                         COL_DAMAGE_P,
                         MASK_DAMAGE_P_COLLIDES_WITH);

      // Entities now cast shadows
      Ogre::SceneNode::ObjectIterator itObject
          = this->getSceneNode()->getAttachedObjectIterator();
      while (itObject.hasMoreElements()) {
        Ogre::MovableObject* pObjectToLift
            = static_cast<Ogre::MovableObject*>(itObject.getNext());
        pObjectToLift->setCastShadows(true);
      }
    } else if (eState == EST_THROWN) {
      btRigidBody *pRB = btRigidBody::upcast(this->getCollisionObject());
      ASSERT(pRB);

      pRB->setLinearFactor(btVector3(1, 1, 1));
      pRB->setAngularFactor(0);
      pRB->activate();
    }
  break;
  }

  // create inner object
  switch (m_uiType) {
    case OBJECT_VASE:
      if (eState == EST_DELETE) {
        createInnerObject(mInnerObjectGenerator.drawRandomObject());
      }
      break;
    case OBJECT_GREEN_BUSH:
    case OBJECT_LIGHT_STONE:
      if (m_eState == EST_NORMAL) {
        createInnerObject(mInnerObjectGenerator.drawRandomObject());
      }
      break;
    default:
      break;
  }

  CWorldEntity::changeState(eState);
}

CObject::SInteractionResult CObject::interactOnCollision(
    const Ogre::Vector3 &vInteractDir,
    CWorldEntity *pSender) {
  switch (m_uiType) {
  case OBJECT_GREEN_RUPEE:
  case OBJECT_BLUE_RUPEE:
  case OBJECT_RED_RUPEE:
    // deleteLater();
    break;
  case OBJECT_GREEN_BUSH:
  case OBJECT_LIGHT_STONE:
  case OBJECT_VASE:
    if (m_eState == EST_THROWN) {
      pSender->hit(CDamage(this,
                           DMG_WORLD,
                           BtOgre::Convert::toOgre(
                               btRigidBody::upcast(this->getCollisionObject())
                               ->getLinearVelocity()).normalisedCopy(),
                           HP_ONE_HEART));
      deleteLater();
    }
    break;
  default:
    break;
  }

  return CWorldEntity::interactOnCollision(vInteractDir, pSender);
}

CObject::SInteractionResult CObject::interactOnActivate(
    const Ogre::Vector3 &vInteractDir,
    CWorldEntity *pSender) {
  switch (m_uiType) {
  case OBJECT_GREEN_BUSH:
  case OBJECT_LIGHT_STONE:
  case OBJECT_VASE:
    return SInteractionResult(IR_LIFT);
  default:
    break;
  }

  return CWorldEntity::interactOnActivate(vInteractDir, pSender);
}

CObject::EReceiveDamageResult CObject::receiveDamage(const CDamage &dmg) {
  if (m_ObjectTypeData.eDamageSourceMask & dmg.getDamageType()) {
    CWorldEntity::receiveDamage(dmg);
    return RDR_ACCEPTED;
  }
  return RDR_IGNORED;
}

CObject::EReceiveDamageResult CObject::hit(const CDamage &dmg) {
  EReceiveDamageResult r = CWorldEntity::hit(dmg);

  if (r != RDR_ACCEPTED) {
    return r;
  }


  switch (m_uiType) {
  case OBJECT_LIGHT_STONE_PILE:
  case OBJECT_GREEN_TREE:
    // Tree and piles accept damage, but always blocks it
    return RDR_BLOCKED;
  case OBJECT_HEART:
    // pickup item
    deleteLater();
    CMessageHandler::getSingleton().addMessage(
        std::make_shared<CMessagePlayerPickupItem>(__MSG_LOCATION__, m_uiType));
    break;
  default:
    break;
  }

  return r;
}

void CObject::setInnerObject(EObjectTypes eType) {
  mInnerObjectGenerator.setObject(eType);
}

void CObject::createInnerObject(EObjectTypes eType) {
  if (eType == OBJECT_COUNT) {return;}   // no inner object specified

  Ogre::Vector3 initPosition = getPosition();
  btVector3 initLinearVelocity = btVector3(0, 0.5, 0);
  switch (m_uiType) {
    case OBJECT_GREEN_TREE:
      initPosition = getPosition() + Ogre::Vector3(0.1, 0.35, 0);
      initLinearVelocity = btVector3(0.5, 0.5, 0);
      break;
    case OBJECT_LIGHT_STONE:
      initPosition = getPosition() + Ogre::Vector3(0, 0.05, 0);
      initLinearVelocity = btVector3(0, 0, 0);
  default:
    break;
  }

  CObject *pObject
      = new CObject(m_sID + "_inner"
                    + Ogre::StringConverter::toString(
                        OBJECT_INNER_OBJECT_ID_NUMBER_COUNTER++),
                    m_pMap,
                    m_pMap,
                    eType);
  // set initial position
  pObject->setPosition(initPosition);
  pObject->init();
  pObject->start();
  btRigidBody *pRB = btRigidBody::upcast(pObject->getCollisionObject());
  pRB->setLinearVelocity(initLinearVelocity);
}

void CObject::killedCallback() {
  deleteLater();
}
