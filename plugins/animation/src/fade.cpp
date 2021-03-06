/*
 * Animation plugin for compiz/beryl
 *
 * fade.cpp
 *
 * Copyright : (C) 2006 Erkin Bahceci
 * E-mail    : erkinbah@gmail.com
 *
 * Based on Wobbly and Minimize plugins by
 *           : David Reveman
 * E-mail    : davidr@novell.com>
 *
 * Particle system added by : (C) 2006 Dennis Kasprzyk
 * E-mail                   : onestone@beryl-project.org
 *
 * Beam-Up added by : Florencio Guimaraes
 * E-mail           : florencio@nexcorp.com.br
 *
 * Hexagon tessellator added by : Mike Slegeir
 * E-mail                       : mikeslegeir@mail.utexas.edu>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "private.h"

// =====================  Effect: Fade  =========================

FadeAnim::FadeAnim (CompWindow       *w,
		    WindowEvent      curWindowEvent,
		    float            duration,
		    const AnimEffect info,
		    const CompRect   &icon) :
    Animation::Animation (w, curWindowEvent, duration, info, icon)
{
}

void
FadeAnim::updateAttrib (GLWindowPaintAttrib &attrib)
{
    attrib.opacity = (GLushort) (mStoredOpacity * (1 - getFadeProgress ()));
}

void
FadeAnim::updateBB (CompOutput &output)
{
    mAWindow->expandBBWithWindow ();
}
