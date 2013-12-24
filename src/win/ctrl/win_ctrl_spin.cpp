/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010-2012, Eren Okka
** 
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "win_ctrl.h"

namespace win {

// =============================================================================

bool Spin::GetPos32(int& value) {
  BOOL result;
  value = ::SendMessage(window_, UDM_GETPOS32, 0, reinterpret_cast<LPARAM>(&result));
  return result == 0;
}

HWND Spin::SetBuddy(HWND hwnd) {
  return reinterpret_cast<HWND>(::SendMessage(window_, UDM_SETBUDDY, reinterpret_cast<WPARAM>(hwnd), 0));
}

int Spin::SetPos32(int position) {
  return ::SendMessage(window_, UDM_SETPOS32, 0, position);
}

void Spin::SetRange32(int lower_limit, int upper_limit) {
  ::SendMessage(window_, UDM_SETRANGE32, lower_limit, upper_limit);
}

}  // namespace win