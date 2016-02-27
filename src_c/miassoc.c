// fichier miassoc.c - gestion des associations en table de hash
/* la notice de copyright est legalement en anglais */

// (C) 2016 Basile Starynkevitch
//   this file miassoc.c is part of Minil
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

// Une association par table de hashage entre symboles et valeurs.
// les entrées sont
struct Mi_EntAss_st
{
  const Mit_Symbole *e_symb;
  Mit_Val e_val;
};

#define MI_ASSOC_NMAGIQ 0x19577fcb	/*425164747 */
// une association pour les attributs
struct Mi_Assoc_st
{
  unsigned a_mag;		// doit toujours être MI_ASSOC_NMAGIQ
  unsigned a_tai;		// la taille du tableau a_ent
  unsigned a_nbe;		// le nombre d'entrées occupées, toujours plus petit que a_tai
  struct Mi_EntAss_st a_ent[];	// les entrées elle-mêmes, peuvent avoir un MI_TROU_SYMBOLE
};

// fonction interne donnant l'indice où trouver ou bien mettre un symbole donné, ou bien -1 si plein
static int
mi_assoc_indice (const struct Mi_Assoc_st *a, const Mit_Symbole *sy)
{
  if (!a || !sy)
    return -1;
  if (a->a_mag != MI_ASSOC_NMAGIQ)
    MI_FATALPRINTF ("association@%p corrompue", a);
  if (sy->mi_type != MiTy_Symbole)
    MI_FATALPRINTF ("symbole@%p corrompu", sy);
  unsigned h = sy->mi_hash;
  unsigned t = a->a_tai;
  assert (t >= 2 && a->a_nbe < t);
  unsigned ideb = h % t;
  int pos = -1;
  for (unsigned ix = ideb; ix < t; ix++)
    {
      const Mit_Symbole *cursy = a->a_ent[ix].e_symb;
      if (cursy == sy)
        return ix;
      if (!cursy)
        {
          if (pos < 0)
            pos = ix;
          return pos;
        }
      else if (cursy == MI_TROU_SYMBOLE)
        {
          if (pos < 0)
            pos = ix;
          continue;
        }
    }
  for (unsigned ix = 0; ix < ideb; ix++)
    {
      const Mit_Symbole *cursy = a->a_ent[ix].e_symb;
      if (cursy == sy)
        return ix;
      if (!cursy)
        {
          if (pos < 0)
            pos = ix;
          return pos;
        }
      else if (cursy == MI_TROU_SYMBOLE)
        {
          if (pos < 0)
            pos = ix;
          continue;
        }
    }
  return pos;
}				// fin mi_assoc_indice

//// le type abstrait des associations entre symbole et valeur -quelconque-
struct Mi_Assoc_st *
mi_assoc_reserver (struct Mi_Assoc_st *a, unsigned nb)
{
  if (!a)
    {
      unsigned nouvtail = mi_nombre_premier_apres (6 * nb / 5 + 4);
      a =
        calloc (1,
                sizeof (struct Mi_Assoc_st) +
                nouvtail * sizeof (struct Mi_EntAss_st));
      if (!a)
        MI_FATALPRINTF
        ("mémoire pleine pour association de %d entrées (%s)", nouvtail,
         strerror (errno));
      a->a_mag = MI_ASSOC_NMAGIQ;
      a->a_tai = nouvtail;
      return a;
    }
  assert (a->a_mag == MI_ASSOC_NMAGIQ);
  unsigned t = a->a_tai;
  unsigned n = a->a_nbe;
  assert (n < t);
  if (6 * (n + nb) + 5 >= 5 * t)
    {
      unsigned nouvtail =
        mi_nombre_premier_apres (4 * (n + nb) / 3 + nb / 16 + 4);
      if (nouvtail > t)
        {
          struct Mi_Assoc_st *nouva =
          calloc (1,
                    sizeof (struct Mi_Assoc_st) +
                    nouvtail * sizeof (struct Mi_EntAss_st));
          if (!nouva)
            MI_FATALPRINTF
            ("mémoire pleine pour association de %d entrées (%s)",
             nouvtail, strerror (errno));
          nouva->a_tai = nouvtail;
          nouva->a_mag = MI_ASSOC_NMAGIQ;
          for (int ix = 0; ix < (int) t; ix++)
            {
              struct Mi_EntAss_st *ent = a->a_ent + ix;
              const Mit_Symbole *esy = ent->e_symb;
              if (esy == MI_TROU_SYMBOLE || !esy)
                continue;
              int pos = mi_assoc_indice (nouva, esy);
              assert (pos >= 0 && pos < (int) nouvtail);
              nouva->a_ent[pos] = *ent;
              nouva->a_nbe++;
            }
          free (a);
          return nouva;
        }
    }
  else if (t > 10 && 3 * (n + nb) < t)
    {
      unsigned nouvtail =
        mi_nombre_premier_apres (4 * (n + nb) / 3 + nb / 16 + 4);
      if (nouvtail < t)
        {
          struct Mi_Assoc_st *nouva =
          calloc (1,
                    sizeof (struct Mi_Assoc_st) +
                    nouvtail * sizeof (struct Mi_EntAss_st));
          if (!nouva)
            MI_FATALPRINTF
            ("mémoire pleine pour association de %d entrées (%s)",
             nouvtail, strerror (errno));
          nouva->a_tai = nouvtail;
          nouva->a_mag = MI_ASSOC_NMAGIQ;
          for (int ix = 0; ix < (int) t; ix++)
            {
              struct Mi_EntAss_st *ent = a->a_ent + ix;
              const Mit_Symbole *esy = ent->e_symb;
              if (esy == MI_TROU_SYMBOLE || !esy)
                continue;
              int pos = mi_assoc_indice (nouva, esy);
              assert (pos >= 0 && pos < (int) nouvtail);
              nouva->a_ent[pos] = *ent;
              nouva->a_nbe++;
            }
          free (a);
          return nouva;
        }
    }
  return a;
}				/* fin mi_assoc_reserver */

struct Mi_Assoc_st *
mi_assoc_mettre (struct Mi_Assoc_st *a, const Mit_Symbole *sy,
                 const Mit_Val va)
{
  if (!sy)
    return a;
  assert (sy != MI_TROU_SYMBOLE);
  assert (sy->mi_type == MiTy_Symbole);
  if (!a || 6 * a->a_tai + 2 < 5 * a->a_nbe)
    a = mi_assoc_reserver (a, 3 + a->a_nbe / 4);
  assert (a->a_mag == MI_ASSOC_NMAGIQ);
  int pos = mi_assoc_indice (a, sy);
  assert (pos >= 0 && pos < (int) a->a_tai);
  if (!a->a_ent[pos].e_symb || a->a_ent[pos].e_symb == MI_TROU_SYMBOLE)
    {
      a->a_ent[pos].e_symb = sy;
      a->a_ent[pos].e_val = va;
      a->a_nbe++;
      return a;
    }
  else
    {
      assert (a->a_ent[pos].e_symb == sy);
      a->a_ent[pos].e_val = va;
      return a;
    }
}				/* fin mi_assoc_mettre */

struct Mi_Assoc_st *
mi_assoc_enlever (struct Mi_Assoc_st *a, const Mit_Symbole *sy)
{
  if (!a || !sy || sy->mi_type != MiTy_Symbole)
    return a;
  assert (sy != MI_TROU_SYMBOLE);
  assert (a->a_mag == MI_ASSOC_NMAGIQ);
  int pos = mi_assoc_indice (a, sy);
  if (pos < 0)
    return a;
  assert (pos >= 0 && pos < (int) a->a_tai);
  if (a->a_ent[pos].e_symb == sy)
    {
      a->a_ent[pos].e_symb = MI_TROU_SYMBOLE;
      a->a_ent[pos].e_val = MI_NILV;
      a->a_nbe--;
      if (a->a_tai > 11 && 3 * a->a_nbe < a->a_tai)
        a = mi_assoc_reserver (a, 1);
    }
  return a;
}				/* fin mi_assoc_enlever */

struct Mi_trouve_st
mi_assoc_chercher (const struct Mi_Assoc_st *a, const Mit_Symbole *sy)
{
  struct Mi_trouve_st r = { MI_NILV, false };
  if (!a || !sy || sy->mi_type != MiTy_Symbole)
    return r;
  assert (a->a_mag == MI_ASSOC_NMAGIQ);
  int pos = mi_assoc_indice (a, sy);
  if (pos < 0)
    return r;
  assert (pos >= 0 && pos < (int) a->a_tai);
  if (a->a_ent[pos].e_symb == sy)
    {
      r.t_val = a->a_ent[pos].e_val;
      r.t_pres = true;
    }
  return r;
}				// fin mi_assoc_chercher

unsigned
mi_assoc_taille (const struct Mi_Assoc_st *a)
{
  if (!a)
    return 0;
  assert (a->a_mag == MI_ASSOC_NMAGIQ);
  return a->a_tai;
}				/* fin mi_assoc_taille */

void
mi_assoc_iterer (const struct Mi_Assoc_st *a, mi_assoc_sigt * f, void *client)
{
  if (!a || !f)
    return;
  assert (a->a_mag == MI_ASSOC_NMAGIQ);
  unsigned t = a->a_tai;
  for (unsigned ix = 0; ix < t; ix++)
    {
      const Mit_Symbole *sy = a->a_ent[ix].e_symb;
      if (!sy || sy == MI_TROU_SYMBOLE)
        continue;
      const Mit_Val val = a->a_ent[ix].e_val;
      if (f (sy, val, client))
        return;
    }
}				/* fin mi_assoc_iterer */
