/*
 * File: animation.c
 * Project: ui
 * File Created: Wednesday, 16th June 2021 10:23:13 pm
 * Author: Hayden Kowalchuk
 * -----
 * Copyright (c) 2021 Hayden Kowalchuk, Hayden Kowalchuk
 * License: BSD 3-clause "New" or "Revised" License, http://www.opensource.org/licenses/BSD-3-Clause
 */

#include "animation.h"

void anim_update_2d(anim2d *anim) {
  //AHEasingFunction ease = CircularEaseOut;
  AHEasingFunction ease = CubicEaseInOut;
  const float dt = (float)anim->time.frame_now / (float)anim->time.frame_len;
  const float dv = (*ease)(dt);
  anim->cur.x = anim->start.x + (anim->end.x - anim->start.x) * dv;
  anim->cur.y = anim->start.y + (anim->end.y - anim->start.y) * dv;
}