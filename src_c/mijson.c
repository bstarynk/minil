// fichier mival.c - sérialisation en JSON
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

//// sérialisation d'une valeur en JSON
json_t *
mi_json_val (const Mit_Val v)
{
  enum mi_typeval_en ty = mi_vtype (v);
  switch (ty)
    {
    case MiTy_Nil:
      return json_null ();
    case MiTy_Entier:
      return json_integer (mi_vald_entier (v, 0));
    case MiTy_Double:
      return json_real (mi_vald_double (v, 0.0));
    case MiTy_Chaine:
      return json_string (mi_vald_chaine (v, ""));
    case MiTy_Symbole:
      {
	const Mit_Symbole *sy = mi_en_symbole (v);
	const char *chn = mi_symbole_chaine (sy);
	unsigned ind = mi_symbole_indice (sy);
	if (ind)
	  return json_pack ("{sssi}", "symb", chn, "ind", ind);
	else
	  return json_pack ("{ss}", "symb", chn);
      }
      break;
    case MiTy_Noeud:
      {
	const Mit_Noeud *nd = mi_en_noeud (v);
	const Mit_Symbole *sycon = mi_connective_noeud (nd);
	unsigned ar = mi_arite_noeud (nd);
	const json_t *jsymb = mi_json_val ((Mit_Val) {.miva_sym =
					   (Mit_Symbole *) sycon
					   });
	json_t *jfils = json_array ();
	for (unsigned ix = 0; ix < ar; ix++)
	  json_array_append_new (jfils,
				 (json_t *) mi_json_val (nd->mi_fils[ix]));
	return json_pack ("{soso}", "conn", jsymb, "fils", jfils);
      }
    }
  return NULL;
}				// fin mi_json_val



/// déserialisation d'un JSON en valeur
Mit_Val
mi_val_json (const json_t * j)
{
  if (!j)
    return (Mit_Val)
    {
    .miva_ptr = NULL};
#warning mi_val_json doit être codé
  MI_FATALPRINTF ("mi_val_json à implementer");

}				// fin mi_val_json
