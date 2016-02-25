// fichier mijson.c - sérialisation en JSON
/* la notice de copyright est legalement en anglais */

// (C) 2016 Basile Starynkevitch
//   this file mijson.c is part of Minil
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
      const json_t *jsymb = mi_json_val (MI_SYMBOLEV((Mit_Symbole*)sycon));
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
  if (!j || json_is_null (j))
    return MI_NILV;
  if (json_is_integer (j))
    return MI_ENTIERV(mi_creer_entier(json_integer_value(j)));
  else if (json_is_real (j))
    return MI_DOUBLEV(mi_creer_double(json_real_value(j)));
  else if (json_is_string (j))
    return MI_CHAINEV(mi_creer_chaine(json_string_value(j)));
  else if (json_is_object (j))
    {
      json_t *js = json_object_get (j, "symb");
      if (js && json_is_string (js))
        {
          unsigned ind = json_integer_value (json_object_get (j, "ind"));
          return MI_SYMBOLEV( mi_creer_symbole_chaine (json_string_value (js), ind));
        };
      json_t *jc = json_object_get (j, "conn");
      json_t *jf = json_object_get (j, "fils");
      if (jc && json_is_array (jf))
        {
          Mit_Val petitab[8] = {};
          Mit_Symbole *sycon = mi_en_symbole (mi_val_json (jc));
          unsigned ar = json_array_size (jc);
          if (!sycon) return MI_NILV;
          Mit_Val* tab = (ar<sizeof(petitab)/sizeof(petitab[0]))?petitab
                         :calloc(ar+1,sizeof(Mit_Val));
          if (!tab) MI_FATALPRINTF("impossible d'allouer tampon pour %d fils", ar);
          for (unsigned ix=0; ix<ar; ix++)
            tab[ix] = mi_val_json(json_array_get(jc,ix));
          Mit_Val res = MI_NOEUDV(mi_creer_noeud(sycon,ar,tab));
          if (tab != petitab) free (tab);
          return res;
        }
    }
  fprintf(stderr, "JSON incorrect:\n");
  json_dumpf(j, stderr, JSON_INDENT(1)|JSON_SORT_KEYS);
  fputc('\n', stderr);
  return MI_NILV;
}				// fin mi_val_json
