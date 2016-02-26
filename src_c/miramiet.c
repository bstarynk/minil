// fichier miramiet.c - gestion de la mémoire et ramasse-miettes
/* la notice de copyright est legalement en anglais */

// (C) 2016 Basile Starynkevitch
//   this file miramiet.c is part of Minil
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

/// mis à vrai quand faut lancer un ramasse miettes
bool mi_faut_ramiet;

#define MI_MAXNBVAL (UINT_MAX/3)
static struct
{
  unsigned mm_taille;
  unsigned mm_nbval;
  Mit_Val *mm_vtab;
} mi_mem;

void *
mi_allouer_valeur (enum mi_typeval_en typv, size_t tail)
{
  assert (tail >= sizeof (typv));

  if (mi_mem.mm_nbval >= MI_MAXNBVAL)
    MI_FATALPRINTF ("trop (%d) de valeurs", mi_mem.mm_nbval);
  if (mi_mem.mm_nbval + 1 >= mi_mem.mm_taille)
    {
      mi_faut_ramiet = true;
      unsigned nouvtail =
        mi_nombre_premier_apres (5 * mi_mem.mm_nbval / 4 + 50);
      if (nouvtail >= MI_MAXNBVAL || nouvtail == 0)
        MI_FATALPRINTF ("trop (%d) de valeurs, mémoire pleine",
                        mi_mem.mm_nbval);
      Mit_Val *nouvtab = calloc (nouvtail, sizeof (Mit_Val));
      if (!nouvtab)
        MI_FATALPRINTF ("trop (%d) de valeurs, mémoire pleine (%s)",
                        nouvtail, strerror (errno));
      memcpy (nouvtab, mi_mem.mm_vtab, mi_mem.mm_nbval * sizeof (Mit_Val));
      free (mi_mem.mm_vtab);
      mi_mem.mm_vtab = nouvtab;
      mi_mem.mm_taille = nouvtail;
    }
  void *ptr = calloc (1, tail);
  if (!ptr)
    MI_FATALPRINTF
    ("manque de mémoire pour allouer une valeur de %ld octets (%s)",
     (long) tail, strerror (errno));
  *(enum mi_typeval_en *) ptr = typv;
  mi_mem.mm_vtab[mi_mem.mm_nbval++].miva_ptr = ptr;
  return ptr;
}

#warning le ramasse miettes ne doit pas libérer l'ensemble vide
