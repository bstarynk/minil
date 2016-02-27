// fichier mivect.c - gestion des vecteurs
/* la notice de copyright est legalement en anglais */

// (C) 2016 Basile Starynkevitch
//   this file mivect.c is part of Minil
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

#define MI_VECTEUR_NMAGIQ 0x1eefe87d	/*519039101 */
struct Mi_Vecteur_st
{
  unsigned vec_mag;		// toujours MI_VECTEUR_NMAGIQ
  unsigned vec_taille;
  unsigned vec_compte;
  Mit_Val vec_tableau[];
};


struct Mi_Vecteur_st *
mi_vecteur_reserver (struct Mi_Vecteur_st *v, unsigned nb)
{
  if (!v || v->vec_mag != MI_VECTEUR_NMAGIQ)
    {
      unsigned nouvtail = mi_nombre_premier_apres (9 * nb / 8 + 2);
      if (!nouvtail || nouvtail > INT_MAX / 2)
        MI_FATALPRINTF ("vecteur trop grand (%d)", nb);
      v =
        calloc (1,
                sizeof (struct Mi_Vecteur_st) + nouvtail * sizeof (Mit_Val));
      if (!v)
        MI_FATALPRINTF ("memoire pleine pour vecteur de %d (%s)", nouvtail,
                        strerror (errno));
      v->vec_mag = MI_VECTEUR_NMAGIQ;
      v->vec_taille = nouvtail;
      return v;
    }
  else if (v->vec_compte + nb >= v->vec_taille)
    {
      unsigned nouvtail =
        mi_nombre_premier_apres (9 * (nb + v->vec_compte) / 8 + 2);
      if (nouvtail == 0 || nouvtail > INT_MAX / 2)
        MI_FATALPRINTF ("vecteur trop grand (%d)", nouvtail);
      struct Mi_Vecteur_st *nouvec =
      calloc (1,
                sizeof (struct Mi_Vecteur_st) + nouvtail * sizeof (Mit_Val));
      if (!nouvec)
        MI_FATALPRINTF ("vecteur trop grand de taille %d (%s)", nouvtail,
                        strerror (errno));
      nouvec->vec_mag = MI_VECTEUR_NMAGIQ;
      nouvec->vec_taille = nouvtail;
      memcpy (nouvec->vec_tableau, v->vec_tableau,
              v->vec_compte * sizeof (Mit_Val));
      nouvec->vec_compte = v->vec_compte;
      free (v);
      return nouvec;
    }
  else
    return v;
}				/* fin mi_vecteur_reserver */


struct Mi_Vecteur_st *
mi_vecteur_ajouter (struct Mi_Vecteur_st *v, const Mit_Val va)
{
  unsigned cnt = 0;
  if (v && v->vec_mag == MI_VECTEUR_NMAGIQ)
    cnt = v->vec_compte;
  if (!v || v->vec_mag != MI_VECTEUR_NMAGIQ || cnt + 2 >= v->vec_taille)
    v = mi_vecteur_reserver (v, cnt / 16 + 5);
  if (cnt >= INT_MAX / 2)
    MI_FATALPRINTF ("vecteur trop grand (%d)", cnt);
  assert (v && v->vec_mag == MI_VECTEUR_NMAGIQ && v->vec_taille > cnt + 1);
  v->vec_tableau[cnt] = va;
  v->vec_compte = cnt + 1;
  return v;
}				/* fin mi_vecteur_ajouter */


struct Mi_trouve_st
mi_vecteur_comp (struct Mi_Vecteur_st *v, int rang)
{
  struct Mi_trouve_st r = { MI_NILV, false };
  if (!v || v->vec_mag != MI_VECTEUR_NMAGIQ)
    return r;
  unsigned cnt = v->vec_compte;
  assert (cnt <= v->vec_taille);
  if (rang < 0)
    rang += (int) cnt;
  if (rang >= 0 && rang < (int) cnt)
    {
      r.t_pres = true;
      r.t_val = v->vec_tableau[rang];
    };
  return r;
}				// fin mi_vecteur_comp

void
mi_vecteur_mettre (struct Mi_Vecteur_st *v, int rang, const Mit_Val va)
{
  if (!v || v->vec_mag != MI_VECTEUR_NMAGIQ)
    return;
  unsigned cnt = v->vec_compte;
  assert (cnt <= v->vec_taille);
  if (rang < 0)
    rang += (int) cnt;
  if (rang >= 0 && rang < (int) cnt)
    v->vec_tableau[rang] = va;
}				/* fin mi_vecteur_mettre */

unsigned
mi_vecteur_taille (const struct Mi_Vecteur_st *v)
{
  if (!v || v->vec_mag != MI_VECTEUR_NMAGIQ)
    return 0;
  return v->vec_compte;
}

/// la fonction d'iteration renvoie true pour arrêter l'itération
typedef bool mi_vect_sigt (const Mit_Val va, unsigned ix, void *client);
void
mi_vecteur_iterer (const struct Mi_Vecteur_st *v, mi_vect_sigt * f,
                   void *client)
{
  if (!v || v->vec_mag != MI_VECTEUR_NMAGIQ)
    return;
  if (!f)
    return;
  unsigned cnt = v->vec_compte;
  assert (cnt <= v->vec_taille);
  for (unsigned ix = 0; ix < cnt; ix++)
    if ((*f) (v->vec_tableau[ix], ix, client))
      return;
}
