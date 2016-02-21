// fichier mival.c - gestion des valeurs
/* la notice de copyright est legalement en anglais */

// (C) 2016 Basile Starynkevitch
//   this file minil.h is part of Minil
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
const Mit_Chaine *
mi_creer_chaine (const char *ch)
{
  if (!ch)
    return NULL;
  size_t ln = strlen (ch);
  if (ln >= MI_MAXLONGCHAINE)
    MI_FATALPRINTF ("chaine %.50s trop longue (%ld)", ch, (long) ln);
  Mit_Chaine *valch = mi_allouer_valeur (MiTy_Chaine,
					 (unsigned) (ln +
						     sizeof (Mit_Chaine) +
						     1));
  strcpy (valch->mi_car, ch);
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
