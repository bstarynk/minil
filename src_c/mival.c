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

const Mit_Noeud *
mi_creer_noeud (const Mit_Symbole *consymb, unsigned arite,
                const Mit_Val fils[])
{
  if (!consymb || consymb->mi_type != MiTy_Symbole)
    return NULL;
  if (arite >= MI_MAXARITE)
    MI_FATALPRINTF ("arite trop grande %u pour noeud %s",
                    arite, mi_val_chaine (MI_CHAINEV(consymb->mi_nom)));
  Mit_Noeud *valnd =		//
    mi_allouer_valeur (MiTy_Noeud,
                       sizeof (Mit_Noeud) + arite * sizeof (Mit_Val));
  valnd->mi_conn = (Mit_Symbole *) consymb;
  valnd->mi_arite = arite;
  for (unsigned ix = 0; ix < arite; ix++)
    valnd->mi_fils[ix] = fils[ix];
  return valnd;
}				// fin mi_creer_noeud

const Mit_Noeud *
mi_creer_noeud_va (const Mit_Symbole *consymb, unsigned arite, ...)
{
  if (!consymb || consymb->mi_type != MiTy_Symbole)
    return NULL;
  if (arite >= MI_MAXARITE)
    MI_FATALPRINTF ("arite trop grande %u pour noeud %s",
                    arite, mi_val_chaine ((Mit_Val) consymb->mi_nom));
  Mit_Noeud *valnd =		//
    mi_allouer_valeur (MiTy_Noeud,
                       sizeof (Mit_Noeud) + arite * sizeof (Mit_Val));
  va_list args;
  valnd->mi_conn = (Mit_Symbole *) consymb;
  va_start (args, arite);
  for (unsigned ix = 0; ix < arite; ix++)
    valnd->mi_fils[ix] = va_arg (args, Mit_Val);
  va_end (args);
  return valnd;
}				// fin mi_creer_noeud_va
