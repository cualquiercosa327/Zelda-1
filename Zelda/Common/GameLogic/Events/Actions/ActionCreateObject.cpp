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

#include "ActionCreateObject.hpp"
#include <BulletDynamics/Dynamics/btRigidBody.h>
#include "../../../Util/XMLHelper.hpp"
#include "../Event.hpp"
#include "../../Entity.hpp"
#include "../../../World/AbstractWorldEntity.hpp"
#include "../../../../World/Objects/Object.hpp"
#include "../../../Log.hpp"

using XMLHelper::Attribute;

namespace events {

CActionCreateObject::CActionCreateObject(const tinyxml2::XMLElement *pElem,
                                         const CEvent &owner)
  : CAction(pElem, owner),
    m_eObjectType(CObjectTypeIdMap::getSingleton().parseString(
        Attribute(pElem, "object_type"))),
    m_sLocation(Attribute(pElem, "location", "local")) {
}
CActionCreateObject::~CActionCreateObject() {
}

void CActionCreateObject::start() {
  CAbstractWorldEntity *worldEnt = dynamic_cast<CAbstractWorldEntity*>(
      m_Owner.getOwner());
  if (m_sLocation == "inner") {
    ASSERT(dynamic_cast<CObject*>(worldEnt));
    dynamic_cast<CObject*>(worldEnt)->createInnerObject(m_eObjectType);
  } else if (m_sLocation == "local") {
    CAbstractMap *pMap = worldEnt->getMap();
    CObject *pObject = new CObject(
        worldEnt->getID() + "created",
        dynamic_cast<CAbstractWorldEntity*>(worldEnt->getParent()),
        pMap,
        m_eObjectType);

    pObject->init();
    pObject->setPosition(worldEnt->getPosition());
    pObject->start();
  } else {
    LOGW("Location '%s' is unknown.", m_sLocation.c_str());
  }
}

}  // namespace events
