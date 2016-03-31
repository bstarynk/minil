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

static const Mit_Ensemble mi_ensvide =
{
  .mi_type = MiTy_Ensemble,
  .mi_taille = 0,
  .mi_marq = true,
  .mi_hash = 11,
  .mi_elements = {NULL}
};

#define MI_ENSHASH_NMAGIQ 0x30e595d7	/*820352471 */




void
mi_calculer_hash_ensemble (Mit_Ensemble * e)
{
  unsigned c = e->mi_taille;
  assert (e->mi_type == MiTy_Ensemble && e->mi_hash == 0);
  unsigned h1 = 0, h2 = c;
  for (unsigned ix = 0; ix < c; ix++)
    {
      const Mit_Symbole *sy = e->mi_elements[ix];
      assert (sy != NULL);
      assert (sy->mi_type == MiTy_Symbole);
      if (ix % 2)
        h1 = (25031 * sy->mi_hash + ix) ^ (271 * h1);
      else
        h2 = (31033 * sy->mi_hash) ^ (151 * h2 + ix * 11);
    }
  unsigned h = (11 * h1) ^ (31 * h2);
  if (!h)
    h = (h1 & 0xffff) + (h2 & 0xfffff) + (c % 31091) + 11;
  e->mi_hash = h;
}


const Mit_Ensemble *
mi_ensemble_vide ()
{
  return &mi_ensvide;
}				/* fin mi_ensemble_vide */

const Mit_Ensemble *
mi_creer_ensemble_enshash (struct Mi_EnsHash_st *eh)
{
  if (!eh || eh->eh_magiq != MI_ENSHASH_NMAGIQ)
    return NULL;
  unsigned c = eh->eh_compte;
  unsigned t = eh->eh_taille;
  if (c == 0)
    return mi_ensemble_vide ();
  assert (c < t);
  unsigned n = 0;
  Mit_Ensemble *e = mi_allouer_valeur (MiTy_Ensemble,
                                       sizeof (Mit_Ensemble) +
                                       c * sizeof (Mit_Symbole *));
  for (unsigned ix = 0; ix < t; ix++)
    {
      const Mit_Symbole *sy = eh->eh_table[ix];
      if (!sy || sy == MI_TROU_SYMBOLE)
        continue;
      assert (sy->mi_type == MiTy_Symbole);
      assert (n < c);
      e->mi_elements[n++] = (Mit_Symbole *) sy;
    }
  assert (n == c);
  qsort (e->mi_elements, n, sizeof (Mit_Symbole *), mi_cmp_symboleptr);
  e->mi_taille = c;
  mi_calculer_hash_ensemble (e);
  return e;
}				// fin mi_creer_ensemble_enshash

const Mit_Ensemble *
mi_creer_ensemble_symboles (unsigned nb, const Mit_Symbole **tab)
{
  if (nb && !tab)
    return NULL;
  if (nb == 0)
    return mi_ensemble_vide ();
  struct Mi_EnsHash_st eh = { };
  mi_enshash_initialiser (&eh, 4 * nb / 3 + 5);
  for (unsigned ix = 0; ix < nb; ix++)
    {
      const Mit_Symbole *sy = tab[ix];
      if (!sy || sy == MI_TROU_SYMBOLE || sy->mi_type != MiTy_Symbole)
        continue;
      mi_enshash_ajouter (&eh, sy);
    }
  const Mit_Ensemble *ens = mi_creer_ensemble_enshash (&eh);
  mi_enshash_detruire (&eh);
  return ens;

}				// fin mi_creer_ensemble_symboles

const Mit_Ensemble *
mi_creer_ensemble_valeurs (unsigned nb, const Mit_Val *tabval)
{
  if (nb && !tabval)
    return NULL;
  if (nb == 0)
    return mi_ensemble_vide ();
  struct Mi_EnsHash_st eh = { };
  mi_enshash_initialiser (&eh, 4 * nb / 3 + nb / 8 + 5);
  for (unsigned ix = 0; ix < nb; ix++)
    {
      mi_enshash_ajouter_valeur (&eh, tabval[ix]);
    }
  const Mit_Ensemble *ens = mi_creer_ensemble_enshash (&eh);
  mi_enshash_detruire (&eh);
  return ens;
}				// fin mi_creer_ensemble_valeurs

const Mit_Ensemble *
mi_creer_ensemble_varsym (unsigned nb, ...)
{
  va_list args;
  struct Mi_EnsHash_st eh = { };
  if (nb == 0)
    return mi_ensemble_vide ();
  mi_enshash_initialiser (&eh, 4 * nb / 3 + 5);
  va_start (args, nb);
  for (unsigned ix = 0; ix < nb; ix++)
    {
      const Mit_Symbole *sy = va_arg (args, Mit_Symbole *);
      if (!sy || sy == MI_TROU_SYMBOLE || sy->mi_type != MiTy_Symbole)
        continue;
      mi_enshash_ajouter (&eh, sy);
    }
  va_end (args);
  const Mit_Ensemble *ens = mi_creer_ensemble_enshash (&eh);
  mi_enshash_detruire (&eh);
  return ens;

}				// fin mi_creer_ensemble_varsym

const Mit_Ensemble *
mi_creer_ensemble_varval (unsigned nb, ...)
{
  va_list args;
  if (nb == 0)
    return mi_ensemble_vide ();
  struct Mi_EnsHash_st eh = { };
  mi_enshash_initialiser (&eh, 4 * nb / 3 + 5);
  va_start (args, nb);
  for (unsigned ix = 0; ix < nb; ix++)
    {
      const Mit_Val v = va_arg (args, Mit_Val);
      mi_enshash_ajouter_valeur (&eh, v);
    }
  va_end (args);
  const Mit_Ensemble *ens = mi_creer_ensemble_enshash (&eh);
  mi_enshash_detruire (&eh);
  return ens;
}				// fin mi_creer_ensemble_varval


void mi_ensemble_iterer (const Mit_Ensemble * en, mi_itersymb_sigt * f,
                         void *client);

void
mi_enshash_initialiser (struct Mi_EnsHash_st *eh, unsigned nb)
{
  unsigned nouvtail = mi_nombre_premier_apres (4 * nb / 3 + 5);
  if (!nouvtail)
    MI_FATALPRINTF
    ("nombre trop grand %u pour initialiser un ensemble de hash", nb);
  const Mit_Symbole **tabsy = calloc (nouvtail, sizeof (Mit_Symbole *));
  if (!tabsy)
    MI_FATALPRINTF
    ("impossible d'allouer tableau de %u symboles en ensemble de hash (%s)",
     nouvtail, strerror (errno));
  assert (eh != NULL);
  memset (eh, 0, sizeof (*eh));
  eh->eh_magiq = MI_ENSHASH_NMAGIQ;
  eh->eh_compte = 0;
  eh->eh_taille = nouvtail;
  eh->eh_table = tabsy;
}

static int
mi_enshash_pos (const struct Mi_EnsHash_st *eh, const Mit_Symbole *sy)
{
  int pos = -1;
  assert (eh && eh->eh_magiq == MI_ENSHASH_NMAGIQ);
  assert (sy && sy->mi_type == MiTy_Symbole);
  unsigned h = sy->mi_hash;
  assert (h > 0);
  unsigned t = eh->eh_taille;
  assert (t > 2 && eh->eh_compte < t);
  unsigned ideb = h % t;

  for (unsigned ix = ideb; ix < t; ix++)
    {
      const Mit_Symbole *syc = eh->eh_table[ix];
      if (syc == sy)
        return (int) ix;
      else if (syc == MI_TROU_SYMBOLE)
        {
          if (pos < 0)
            pos = (int) ix;
          continue;
        }
      else if (syc == NULL)
        {
          if (pos < 0)
            pos = (int) ix;
          return pos;
        }
    }

  for (unsigned ix = 0; ix < ideb; ix++)
    {
      const Mit_Symbole *syc = eh->eh_table[ix];
      if (syc == sy)
        return (int) ix;
      else if (syc == MI_TROU_SYMBOLE)
        {
          if (pos < 0)
            pos = (int) ix;
          continue;
        }
      else if (syc == NULL)
        {
          if (pos < 0)
            pos = (int) ix;
          return pos;
        }
    }
  return pos;
}				// fin mi_enshash_pos

void
mi_enshash_reserver (struct Mi_EnsHash_st *eh, unsigned nb)
{
  if (!eh || eh->eh_magiq != MI_ENSHASH_NMAGIQ)
    return;
  unsigned t = eh->eh_taille;
  unsigned c = eh->eh_compte;
  if (5 * (c + nb) + 3 >= 4 * t || 3 * (c + nb) + 2 < t)
    {
      unsigned nouvtail =
        mi_nombre_premier_apres (4 * (c + nb) / 3 + nb / 8 + c / 32 + 3);
      if (!nouvtail || nouvtail)
        MI_FATALPRINTF ("débordement ensemble de hash (%d+%d)", c, nb);
      const Mit_Symbole **nouvtable =
        calloc (nouvtail, sizeof (Mit_Symbole *));
      if (!nouvtable)
        MI_FATALPRINTF
        ("impossible d'allouer %u symboles pour ensemble de hash (%s)",
         nouvtail, strerror (errno));
      const Mit_Symbole **anctable = eh->eh_table;
      eh->eh_taille = nouvtail;
      eh->eh_table = nouvtable;
      eh->eh_compte = 0;
      for (unsigned ix = 0; ix < t; ix++)
        {
          const Mit_Symbole *syc = anctable[ix];
          if (!syc || syc == MI_TROU_SYMBOLE)
            continue;
          int pos = mi_enshash_pos (eh, syc);
          assert (pos >= 0 && pos < (int) nouvtail);
          assert (nouvtable[pos] == NULL);
          nouvtable[pos] = syc;
          eh->eh_compte++;
        }
      assert (eh->eh_compte == c);
    }
}				// fin mi_enshash_reserver

void
mi_enshash_detruire (struct Mi_EnsHash_st *eh)
{
  assert (eh && eh->eh_magiq == MI_ENSHASH_NMAGIQ);
  free (eh->eh_table);
  memset (eh, 0, sizeof (*eh));
}				// fin mi_enshash_detruire

void
mi_enshash_ajouter (struct Mi_EnsHash_st *eh, const Mit_Symbole *sy)
{
  if (!eh || eh->eh_magiq != MI_ENSHASH_NMAGIQ)
    return;
  if (!sy || sy == MI_TROU_SYMBOLE || sy->mi_type != MiTy_Symbole)
    return;
  unsigned t = eh->eh_taille;
  unsigned c = eh->eh_compte;
  if (5 * c + 3 > 4 * t)
    {
      mi_enshash_reserver (eh, 5 + c / 64);
      t = eh->eh_taille;
      assert (eh->eh_compte == c);
    }
  int pos = mi_enshash_pos (eh, sy);
  assert (pos >= 0 && pos < (int) t);
  if (eh->eh_table[pos] == sy)
    return;
  eh->eh_table[pos] = sy;
  eh->eh_compte++;
}				// fin mi_enshash_ajouter


void
mi_enshash_ajouter_valeur (struct Mi_EnsHash_st *eh, const Mit_Val va)
{
  if (!eh || eh->eh_magiq != MI_ENSHASH_NMAGIQ)
    return;
  switch (mi_vtype (va))
    {
    case MiTy_Symbole:
      mi_enshash_ajouter (eh, va.miva_sym);
      return;
    case MiTy_Ensemble:
    {
      const Mit_Ensemble *e = va.miva_ens;
      unsigned c = e->mi_taille;
      mi_enshash_reserver (eh, 5 * c / 4 + 2);
      for (unsigned ix = 0; ix < c; ix++)
        mi_enshash_ajouter (eh, e->mi_elements[ix]);
    }
    break;
    case MiTy_Tuple:
    {
      const Mit_Tuple *tu = va.miva_tup;
      unsigned t = tu->mi_taille;
      mi_enshash_reserver (eh, 5 * t / 4 + 2);
      for (unsigned ix = 0; ix < t; ix++)
        mi_enshash_ajouter (eh, tu->mi_composants[ix]);
      default:
        return;
      }
    }
}				// fin mi_enshash_ajouter_valeur

void
mi_enshash_oter (struct Mi_EnsHash_st *eh, const Mit_Symbole *sy)
{
  if (!eh || eh->eh_magiq != MI_ENSHASH_NMAGIQ)
    return;
  if (!sy || sy == MI_TROU_SYMBOLE || sy->mi_type != MiTy_Symbole)
    return;
  int pos = mi_enshash_pos (eh, sy);
  if (pos >= 0 && pos < (int) (eh->eh_taille) && eh->eh_table[pos] == sy)
    {
      eh->eh_table[pos] = MI_TROU_SYMBOLE;
      eh->eh_compte--;
    }
  if (3 * eh->eh_compte < eh->eh_taille)
    mi_enshash_reserver (eh, 3);
}				// fin mi_enshash_oter


bool
mi_enshash_contient (const struct Mi_EnsHash_st *eh, const Mit_Symbole *sy)
{
  if (!eh || eh->eh_magiq != MI_ENSHASH_NMAGIQ)
    return false;
  if (!sy || sy == MI_TROU_SYMBOLE || sy->mi_type != MiTy_Symbole)
    return false;
  int pos = mi_enshash_pos (eh, sy);
  if (pos >= 0 && pos < (int) (eh->eh_taille) && eh->eh_table[pos] == sy)
    return true;
  return false;
}				// fin mi_enshash_contient


void
mi_enshash_iterer (struct Mi_EnsHash_st *eh, mi_itersymb_sigt * f,
                   void *client)
{
  if (!eh || eh->eh_magiq != MI_ENSHASH_NMAGIQ || !f)
    return;
  unsigned t = eh->eh_taille;
  for (unsigned ix = 0; ix < t; ix++)
    {
      const Mit_Symbole *sy = eh->eh_table[ix];
      if (!sy || sy == MI_TROU_SYMBOLE)
        continue;
      if ((*f) ((Mit_Symbole *) sy, client))
        return;
    }
}				// fin mi_enshash_iterer


void
mi_ensemble_iterer (const Mit_Ensemble * en, mi_itersymb_sigt * f,
                    void *client)
{
  if (!en || en->mi_type != MiTy_Ensemble || !f)
    return;
  unsigned t = en->mi_taille;
  for (unsigned ix = 0; ix < t; ix++)
    if ((*f) (en->mi_elements[ix], client))
      return;
}


const Mit_Ensemble *
mi_ensemble_union (const Mit_Ensemble * en1, const Mit_Ensemble * en2)
{
  unsigned ca1 = 0, ca2 = 0;
  if (!en1 || en1->mi_type != MiTy_Ensemble)
    en1 = NULL;
  if (!en2 || en2->mi_type != MiTy_Ensemble)
    en2 = NULL;
  ca1 = en1 ? en1->mi_taille : 0;
  ca2 = en2 ? en2->mi_taille : 0;
  if (ca1 == 0 && ca2 == 0)
    return mi_ensemble_vide ();
  unsigned ta = ca1 + ca2 + 1;
  const Mit_Symbole **tab = calloc (ta, sizeof (Mit_Symbole *));
  if (!tab)
    MI_FATALPRINTF ("impossible d'allouer table de %u symboles", ta);
  unsigned i1 = 0, i2 = 0, nbun = 0;
  while (i1 < ca1 && i2 < ca2)
    {
      const Mit_Symbole *sy1 = en1->mi_elements[i1];
      const Mit_Symbole *sy2 = en2->mi_elements[i2];
      assert (sy1 && sy1->mi_type == MiTy_Symbole);
      assert (sy2 && sy2->mi_type == MiTy_Symbole);
      assert (nbun < ta);
      int cmp = mi_cmp_symbole (sy1, sy2);
      if (cmp < 0)
        {
          tab[nbun++] = sy1;
          i1++;
        }
      else if (cmp > 0)
        {
          tab[nbun++] = sy2;
          i2++;
        }
      else
        {
          assert (sy1 == sy2);
          tab[nbun++] = sy1;
          i1++, i2++;
        }
    }
  if (i1 < ca1)
    for (unsigned ix = i1; ix < ca1; ix++)
      tab[nbun++] = en1->mi_elements[ix];
  else if (i2 < ca2)
    for (unsigned ix = i2; ix < ca2; ix++)
      tab[nbun++] = en2->mi_elements[ix];
  if (nbun > INT_MAX / 2)
    MI_FATALPRINTF ("trop d'elements %d dans l'union de deux ensembles",
                    nbun);
  Mit_Ensemble *enr = mi_allouer_valeur (MiTy_Ensemble,
                                         sizeof (Mit_Ensemble) +
                                         nbun * sizeof (Mit_Symbole *));
  enr->mi_taille = nbun;
  if (nbun > 0)
    memcpy (enr->mi_elements, tab, nbun * sizeof (Mit_Symbole *));
  mi_calculer_hash_ensemble (enr);
  free (tab);
  return enr;
}				/* fin mi_ensemble_union */


const Mit_Ensemble *
mi_ensemble_intersection (const Mit_Ensemble * en1, const Mit_Ensemble * en2)
{
  unsigned ca1 = 0, ca2 = 0;
  if (!en1 || en1->mi_type != MiTy_Ensemble)
    en1 = NULL;
  if (!en2 || en2->mi_type != MiTy_Ensemble)
    en2 = NULL;
  ca1 = en1 ? en1->mi_taille : 0;
  ca2 = en2 ? en2->mi_taille : 0;
  if (ca1 == 0 || ca2 == 0)
    return mi_ensemble_vide ();
  unsigned ta = ((ca1 > ca2) ? ca1 : ca2) + 1;
  const Mit_Symbole **tab = calloc (ta, sizeof (Mit_Symbole *));
  if (!tab)
    MI_FATALPRINTF ("impossible d'allouer table de %u symboles", ta);
  unsigned i1 = 0, i2 = 0, nbin = 0;
  while (i1 < ca1 && i2 < ca2)
    {
      const Mit_Symbole *sy1 = en1->mi_elements[i1];
      const Mit_Symbole *sy2 = en2->mi_elements[i2];
      assert (sy1 && sy1->mi_type == MiTy_Symbole);
      assert (sy2 && sy2->mi_type == MiTy_Symbole);
      assert (nbin < ta);
      int cmp = mi_cmp_symbole (sy1, sy2);
      if (cmp < 0)
        i1++;
      else if (cmp > 0)
        i2++;
      else
        {
          assert (sy1 == sy2);
          tab[nbin++] = sy1;
          i1++, i2++;
        }
    }
  if (nbin == 0)
    {
      free (tab);
      return mi_ensemble_vide ();
    }
  Mit_Ensemble *enr = mi_allouer_valeur (MiTy_Ensemble,
                                         sizeof (Mit_Ensemble) +
                                         nbin * sizeof (Mit_Symbole *));
  enr->mi_taille = nbin;
  if (nbin > 0)
    memcpy (enr->mi_elements, tab, nbin * sizeof (Mit_Symbole *));
  mi_calculer_hash_ensemble (enr);
  free (tab);
  return enr;
}				/* fin mi_ensemble_intersection */
