// fichier milire.c - lecture
/* la notice de copyright est légalement en anglais */

// (C) 2016 Basile Starynkevitch
//   this file mijson.c is part of Minil
//   Minil is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   Minil is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with Minil.  If not, see <http://www.gnu.org/licenses/>.

#include "minil.h"

#define MI_LECTEUR_NMAGIQ 345961075 /*0x149ef273*/
struct Mi_Lecteur_st
{
  unsigned lec_nmagiq;
};

static Mit_Val mi_lire_primaire(struct Mi_Lecteur_st*lec, const char*ps, int*poff)
{
  assert (lec && lec->lec_nmagiq == MI_LECTEUR_NMAGIQ);
  assert (ps != NULL);
}

Mit_Val mi_lire_valeur(struct Mi_Lecteur_st*lec, const char*ps, int*poff)
{
  assert (lec && lec->lec_nmagiq == MI_LECTEUR_NMAGIQ);
  assert (ps != NULL);
}

