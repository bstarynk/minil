// fichier miens.c - gestion des ensembles et ensembles hashés
/* la notice de copyright est legalement en anglais */

// (C) 2016 Basile Starynkevitch
//   this file miens.c is part of Minil
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

#define MI_ENSHASH_NMAGIQ 0x30e595d7 /*820352471*/

const Mit_Ensemble* mi_creer_ensemble_enshash(struct Mi_EnsHash_st*eh);
const Mit_Ensemble* mi_creer_ensemble_symboles(unsigned nb, const Mit_Symbole**tab);
const Mit_Ensemble* mi_creer_ensemble_valeurs(unsigned nb, const Mit_Val*tabval);
const Mit_Ensemble* mi_creer_ensemble_varsym(unsigned nb, ...);
const Mit_Ensemble* mi_creer_ensemble_varval(unsigned nb, ...);
void mi_ensemble_iterer(const Mit_Ensemble*en, mi_ens_sigt*f, void*client);

void mi_enshash_initialiser(struct Mi_EnsHash_st*eh, unsigned nb)
{
  unsigned nouvtail = mi_nombre_premier_apres(4*nb/3+5);
  if (!nouvtail) MI_FATALPRINTF("nombre trop grand %u pour initialiser un ensemble de hash", nb);
  const Mit_Symbole**tabsy = calloc(nouvtail, sizeof(Mit_Symbole*));
  if (!tabsy)
    MI_FATALPRINTF("impossible d'allouer tableau de %u symboles en ensemble de hash (%s)",
                   nouvtail, strerror(errno));
  assert (eh != NULL);
  memset (eh, 0, sizeof(*eh));
  eh->eh_magic = MI_ENSHASH_NMAGIQ;
  eh->eh_compte = 0;
  eh->eh_taille = nouvtail;
  eh->eh_table = tabsy;
}

static int mi_enshash_pos(struct Mi_EnsHash_st*eh, const Mit_Symbole*sy)
{
  int pos = -1;
  assert (eh && eh->eh_magic == MI_ENSHASH_NMAGIQ);
  assert (sy && sy->mi_type == MiTy_Symbole);
  unsigned h = sy->mi_hash;
  assert (h>0);
  unsigned t = eh->eh_taille;
  assert (t>2 && t<eh->eh_compte);
  unsigned ideb = h % t;

  for (unsigned ix=ideb; ix<t; ix++)
    {
      const Mit_Symbole*syc = eh->eh_table[ix];
      if (syc == sy) return (int)ix;
      else if (syc == MI_TROU_SYMBOLE)
        {
          if (pos<0)
            pos=(int)ix;
          continue;
        }
      else if (syc == NULL)
        {
          if (pos<0)
            pos=(int)ix;
          return pos;
        }
    }

  for (unsigned ix=0; ix<ideb; ix++)
    {
      const Mit_Symbole*syc = eh->eh_table[ix];
      if (syc == sy) return (int)ix;
      else if (syc == MI_TROU_SYMBOLE)
        {
          if (pos<0)
            pos=(int)ix;
          continue;
        }
      else if (syc == NULL)
        {
          if (pos<0)
            pos=(int)ix;
          return pos;
        }
    }
  return pos;
} // fin mi_enshash_pos

void mi_enshash_reserver(struct Mi_EnsHash_st*eh, unsigned nb)
{
  if (!eh || eh->eh_magic != MI_ENSHASH_NMAGIQ) return;
  unsigned t = eh->eh_taille;
  unsigned c = eh->eh_compte;
  if (5*(c+nb) + 3 >= 4*t)
    {
      unsigned nouvtail = mi_nombre_premier_apres (4*(c+nb)/3 + nb/8 + c/32 + 3);
      if (!nouvtail || nouvtail) MI_FATALPRINTF("débordement ensemble de hash (%d+%d)", c, nb);
      const Mit_Symbole**nouvtable = calloc(nouvtail, sizeof(Mit_Symbole*));
      if (!nouvtable) MI_FATALPRINTF("impossible d'allouer %u symboles pour ensemble de hash (%s)",
                                       nouvtail, strerror(errno));
      const Mit_Symbole**anctable = eh->eh_table;
      eh->eh_taille= nouvtail;
      eh->eh_table= nouvtable;
      eh->eh_compte=0;
      for (unsigned ix=0; ix<t; ix++)
        {
          const Mit_Symbole*syc = anctable[ix];
          if (!syc || syc==MI_TROU_SYMBOLE) continue;
          int pos = mi_enshash_pos(eh, syc);
          assert (pos>=0 && pos<(int)nouvtail);
          assert (nouvtable[pos] == NULL);
          nouvtable[pos] = syc;
          eh->eh_compte++;
        }
      assert (eh->eh_compte == c);
    }
} // fin mi_enshash_reserver

void mi_enshash_detruire(struct Mi_EnsHash_st*eh)
{
  assert (eh && eh->eh_magic == MI_ENSHASH_NMAGIQ);
  free (eh->eh_table);
  memset (eh, 0, sizeof(*eh));
}// fin mi_enshash_detruire

void mi_enshash_ajouter(struct Mi_EnsHash_st*eh, const Mit_Symbole*sy)
{
  if (!eh || eh->eh_magic != MI_ENSHASH_NMAGIQ) return;
  if (!sy || sy == MI_TROU_SYMBOLE || sy->mi_type != MiTy_Symbole) return;
  unsigned t = eh->eh_taille;
  unsigned c = eh->eh_compte;
  if (5*c+3 > 4*t)
    {
      mi_enshash_reserver (eh, 5+c/64);
      t = eh->eh_taille;
      assert (eh->eh_compte == c);
    }
  int pos = mi_enshash_pos(eh, sy);
  assert (pos>=0 && pos<(int)t);
  if (eh->eh_table [pos] == sy) return;
  eh->eh_table[pos] = sy;
  eh->eh_compte++;
} // fin mi_enshash_ajouter


void mi_enshash_ajouter_valeur(struct Mi_EnsHash_st*eh, const Mit_Val va)
{
  if (!eh || eh->eh_magic != MI_ENSHASH_NMAGIQ) return;
#warning mi_enshash_ajouter_valeur incomplet
} // fin mi_enshash_ajouter_valeur

void mi_enshash_oter(struct Mi_EnsHash_st*eh, const Mit_Symbole*sy);
bool mi_enshash_contient(struct Mi_EnsHash_st*eh, const Mit_Symbole*sy);
void mi_enshash_iterer(struct Mi_EnsHash_st*eh, mi_ens_sigt*f, void*client);
