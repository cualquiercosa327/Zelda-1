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

#include "../../Common/GUI/GUIManager.hpp"
#include "WorldGUI.hpp"
#include "../../Common/GUI/GUIPullMenu.hpp"
#include "HUD.hpp"

CWorldGUI::CWorldGUI(CEntity *pParentEntity)
  : CGUIOverlay("world_gui", pParentEntity, CGUIManager::getSingleton().getRoot(),
                CGUIManager::getSingleton().getRoot()->createChild("DefaultWindow", "world_gui_root")) {

  CGUIManager::getSingleton().addGUIOverlay(this);

  m_pHUD = new CHUD(this, m_pRoot);
  new CGUIPullMenu("test", this, m_pRoot, CGUIPullMenu::PMD_RIGHT, 400);
}
CWorldGUI::~CWorldGUI() {
  CGUIManager::getSingleton().removeGUIOverlay(this);
}