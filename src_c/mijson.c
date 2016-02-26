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

bool
mi_sauvegarde_symbole_oublie(struct Mi_Sauvegarde_st*sv,
                             const Mit_Symbole*sy)
{
  if (!sv) return false;
  if (!sy || sy->mi_type != MiTy_Symbole) return false;
  return mi_assoc_chercher(sv->sv_assosymb,sy).t_val.miva_sym
         == (MI_PREDEFINI(oublier));
}

bool
mi_sauvegarde_symbole_connu(struct Mi_Sauvegarde_st*sv,
                            const Mit_Symbole*sy)
{
  if (!sv) return false;
  if (!sy || sy->mi_type != MiTy_Symbole) return false;
  return mi_assoc_chercher(sv->sv_assosymb,sy).t_val.miva_sym
         == (MI_PREDEFINI(sauvegarder));
}

//// sérialisation d'une valeur en JSON
json_t *
mi_json_val (struct Mi_Sauvegarde_st*sv, const Mit_Val v)
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
      if (mi_sauvegarde_symbole_oublie(sv,sy))
        return json_null();
      if (!mi_sauvegarde_symbole_connu(sv,sy))
        return json_null();
      if (ind)
        return json_pack ("{sssi}", "symb", chn, "ind", ind);
      else
        return json_pack ("{ss}", "symb", chn);
    }
    break;
    case MiTy_Ensemble:
    {
      const Mit_Ensemble*en = mi_en_ensemble(v);
      unsigned t = en->mi_taille;
      json_t* jel = json_array();
      for (unsigned ix=0; ix<t; ix++)
        {
          const Mit_Symbole* sy = en->mi_elements[ix];
          if (mi_sauvegarde_symbole_connu(sv, sy))
            json_array_append_new(jel,  mi_json_val(sv, MI_SYMBOLEV((Mit_Symbole*)sy)));
        }
      return json_pack("{so}", "elem", jel);
    }
    break;
    case MiTy_Noeud:
    {
      const Mit_Noeud *nd = mi_en_noeud (v);
      const Mit_Symbole *sycon = mi_connective_noeud (nd);
      if (mi_sauvegarde_symbole_oublie(sv, sycon))
        return json_null();
      if (!mi_sauvegarde_symbole_connu(sv, sycon))
        return json_null();
      unsigned ar = mi_arite_noeud (nd);
      const json_t *jsymb = mi_json_val (sv,MI_SYMBOLEV((Mit_Symbole*)sycon));
      json_t *jfils = json_array ();
      for (unsigned ix = 0; ix < ar; ix++)
        json_array_append_new (jfils,
                               (json_t *) mi_json_val (sv,nd->mi_fils[ix]));
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
      json_t* je = json_object_get(j, "elem");
      if (je && json_is_array(je))
        {
          struct Mi_EnsHash_st eh= {};
          unsigned ln = json_array_size(je);
          mi_enshash_initialiser (&eh, 4*ln/3+ln/32+5);
          for (unsigned ix=0; ix<ln; ix++)
            {
              Mit_Symbole*sy = mi_en_symbole(mi_val_json(json_array_get(je,ix)));
              if (sy) mi_enshash_ajouter(&eh,sy);
            }
          const Mit_Ensemble*e = mi_creer_ensemble_enshash(&eh);
          mi_enshash_detruire(&eh);
          return MI_ENSEMBLEV(e);
        }
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



struct mi_vectatt_st
{
  unsigned vat_taille;
  unsigned vat_compte;
  const Mit_Symbole*vat_symb[];
};

static bool mi_iterateur_attr(const Mit_Symbole*sy, const Mit_Val va, void*client)
{
  struct mi_vectatt_st*vat = client;
  assert (va.miva_ptr != (void*)-1);
  if (vat->vat_compte < vat->vat_taille)
    {
      vat->vat_symb[vat->vat_compte++] = sy;
      return false;
    }
  else return true;
}

json_t *mi_json_contenu_symbole (struct Mi_Sauvegarde_st*sv, const Mit_Symbole*sy)
{
  if (!sv || !sy) return NULL;
  if (!mi_sauvegarde_symbole_connu(sv,sy)) return json_null();
  json_t*jsym = mi_json_val(sv, MI_SYMBOLEV((Mit_Symbole*)sy));
  unsigned nbat = mi_assoc_taille(sy->mi_attrs);
  struct mi_vectatt_st *va = //
  calloc(1, sizeof(struct mi_vectatt_st)+(nbat+1)*sizeof(Mit_Symbole*));
  va->vat_taille = nbat+1;
  if (!va)
    MI_FATALPRINTF("impossible d'allouer vecteur de %d attributs (%s)",
                   nbat, strerror(errno));
  mi_assoc_iterer(sy->mi_attrs, mi_iterateur_attr, va);
  assert (va->vat_compte==nbat);
  if (nbat>1)
    qsort(va->vat_symb, nbat, sizeof(Mit_Symbole*), mi_cmp_symboleptr);
  json_t *jattrs = json_array ();
  for (unsigned ix=0; ix < nbat; ix++)
    {
      const Mit_Symbole*syat = va->vat_symb[ix];
      if (!mi_sauvegarde_symbole_connu(sv, syat))
        continue;
      struct Mi_trouve_st tr = mi_assoc_chercher(sy->mi_attrs, syat);
      if (!tr.t_pres) continue;
      json_t*jent = json_pack ("{soso}", "at", mi_json_val(sv, MI_SYMBOLEV((Mit_Symbole*)syat)),
                               "va", mi_json_val(sv, tr.t_val));

      json_array_append_new (jattrs, jent);
    }
  json_t*jcont = json_pack("{soso}", "ContSymb", jsym,
                           "attrs", jattrs);
  return jcont;
} /* fin de mi_json_contenu_symbole */



void mi_remplir_symbole_json(const json_t*j)
{
  if (!j) return;
  if (!json_is_object(j)) return;
} // fin mi_remplir_symbole_json
