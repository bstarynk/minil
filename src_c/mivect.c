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

#define MI_VECTEUR_NMAGIQ 0x1eefe87d /*519039101*/
struct Mi_Vecteur_st
{
  unsigned vec_mag; // toujours MI_VECTEUR_NMAGIQ
  unsigned vec_taille;
  unsigned vec_compte;
  Mit_Val vec_tableau[];
};


struct Mi_Vecteur_st*mi_vecteur_reserver(struct Mi_Vecteur_st*v, unsigned nb)
{
  if (!v || v->vec_mag != MI_VECTEUR_NMAGIQ)
    {
      unsigned nouvtail = mi_nombre_premier_apres(9*nb/8+2);
      v = calloc(1, sizeof(struct Mi_Vecteur_st)+nouvtail*sizeof(Mit_Val));
      v->vec_mag = MI_VECTEUR_NMAGIQ;
      v->vec_taille = nouvtail;
      return v;
    }
  else if (v->vec_compte + nb >= v->vec_taille) {
    unsigned nouvtail = mi_nombre_premier_apres(9*(nb+v->vec_compte)/8+2);
      struct Mi_Vecteur_st*nouvec = calloc(1, sizeof(struct Mi_Vecteur_st)+nouvtail*sizeof(Mit_Val));
      nouvec->vec_mag = MI_VECTEUR_NMAGIQ;
      nouvec->vec_taille = nouvtail;
      memcpy (nouvec->vec_tableau, v->vec_tableau, v->vec_compte*sizeof(Mit_Val));
      nouvec->vec_compte = v->vec_compte;
      free (v);
      return nouvec;
  }
  else return v;
} /* fin mi_vecteur_reserver */

#warning operations de vecteurs manquantes

struct Mi_Vecteur_st*mi_vecteur_ajouter(struct Mi_Vecteur_st*v, const Mit_Val va);
struct Mi_trouve_st mi_vecteur_comp(struct Mi_Vecteur_st*v, int rang);
void mi_vecteur_mettre(struct Mi_Vecteur_st*v, int rank, const Mit_Val va);
unsigned mi_vecteur_taille (const struct Mi_Vecteur_st*v);
/// la fonction d'iteration renvoie true pour arrêter l'itération
typedef bool mi_vect_sigt(const Mit_Val va, unsigned ix, void*client);
void mi_vecteur_iterer(const struct Mi_Vecteur_st*v, mi_vect_sigt*f, void*client);
