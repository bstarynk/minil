// fichier mival.c - gestion des valeurs
/* la notice de copyright est legalement en anglais */

// (C) 2016 Basile Starynkevitch
//   this file mival.c is part of Minil
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

#define MI_MAXLONGCHAINE (INT_MAX/4)
#define MI_MAXARITE (INT_MAX/4)

const Mit_Chaine *
mi_creer_chaine (const char *ch)
{
  if (!ch)
    return NULL;
  size_t ln = strlen (ch);
  if (ln >= MI_MAXLONGCHAINE)
    MI_FATALPRINTF ("chaine %.50s trop longue (%ld)", ch, (long) ln);
  if (u8_check ((const uint8_t *) ch, ln))
    MI_FATALPRINTF ("chaine %.50s invcorrecte", ch);
  Mit_Chaine *valch = mi_allouer_valeur (MiTy_Chaine,
                                         (unsigned) (ln +
                                             sizeof (Mit_Chaine) +
                                             1));
  strcpy (valch->mi_car, ch);
  valch->mi_taille = ln;
  valch->mi_long = u8_mblen ((const uint8_t *) ch, ln);
  valch->mi_hash = mi_hashage_chaine (ch);
  return valch;
}

const Mit_Chaine *
mi_creer_chaine_printf (const char *fmt, ...)
{
  char tampon[80];
  memset (tampon, 0, sizeof (tampon));
  int ln = 0;
  va_list args;
  va_start (args, fmt);
  ln = vsnprintf (tampon, sizeof (tampon), fmt, args);
  va_end (args);
  if (ln < (int) sizeof (tampon))
    return mi_creer_chaine (tampon);
  else if (ln >= MI_MAXLONGCHAINE)
    MI_FATALPRINTF ("chaine %.50s trop longue pour format %s (%d)",
                    tampon, fmt, ln);
  else
    {
      Mit_Chaine *valch =
        mi_allouer_valeur (MiTy_Chaine, ln + sizeof (Mit_Chaine) + 1);
      va_start (args, fmt);
      vsnprintf (valch->mi_car, ln, fmt, args);
      va_end (args);
      return valch;
    }
}				// fin mi_creer_chaine_printf

const Mit_Entier *
mi_creer_entier (long l)
{
  Mit_Entier *valen = mi_allouer_valeur (MiTy_Entier, sizeof (Mit_Entier));
  valen->mi_ent = l;
  return valen;
}				// fin mi_creer_entier

const Mit_Double *
mi_creer_double (double d)
{
  Mit_Double *valdb = mi_allouer_valeur (MiTy_Double, sizeof (Mit_Double));
  valdb->mi_dbl = d;
  return valdb;
}				// fin mi_creer_double

static const Mit_Tuple mi_tupvide =
{
  .mi_type= MiTy_Tuple,
  .mi_taille= 0,
  .mi_marq= true,
  .mi_hash= 89,
  .mi_composants={NULL}
};

const Mit_Tuple*mi_tuple_vide(void)
{
  return &mi_tupvide;
} // fin mi_tuple_vide


void
mi_calculer_hash_tuple (Mit_Tuple*tu)
{
  assert (tu != NULL && tu->mi_type == MiTy_Tuple);
  unsigned t = tu->mi_taille;
  assert (tu->mi_hash == 0);
  unsigned h1 = 0, h2 = t+1;
  for (unsigned ix = 0; ix < t; ix++)
    {
      const Mit_Symbole *sy = tu->mi_composants[ix];
      assert (sy != NULL && sy != MI_TROU_SYMBOLE);
      assert (sy->mi_type == MiTy_Symbole);
      if (ix % 2)
        h1 = (22367 * sy->mi_hash + ix) ^ (293 * h1);
      else
        h2 = (42391 * sy->mi_hash) ^ (191 * h2 + ix * 17);
    }
  unsigned h = (17 * h1) ^ (89 * h2);
  if (!h)
    h = (h1 & 0xffff) + (h2 & 0xfffff) + (t % 35677) + 19;
  tu->mi_hash = h;
} // fin mi_calculer_hash_tuple

#warning il manque des fonctions de tuple
const Mit_Tuple*mi_creer_tuple_symboles(unsigned nb,
                                        const Mit_Symbole**tabsym)
{

} // fin mi_creer_tuple_symboles


const Mit_Tuple*mi_creer_tuple_valeurs(unsigned nb,
                                       const Mit_Val*tabval)
{

} // fin mi_creer_tuple_valeurs
