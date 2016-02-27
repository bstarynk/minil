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

#define MI_SAUVEGARDE_NMAGIQ 0x1b455e11	/*457530897 */
bool
mi_sauvegarde_symbole_oublie (struct Mi_Sauvegarde_st *sv,
                              const Mit_Symbole *sy)
{
  if (!sv)
    return false;
  assert (sv->sv_magiq == MI_SAUVEGARDE_NMAGIQ);
  if (!sy || sy->mi_type != MiTy_Symbole)
    return false;
  return mi_enshash_contient (&sv->sv_syoubli, sy);
}

bool
mi_sauvegarde_symbole_connu (struct Mi_Sauvegarde_st * sv,
                             const Mit_Symbole *sy)
{
  if (!sv)
    return false;
  assert (sv->sv_magiq == MI_SAUVEGARDE_NMAGIQ);
  if (!sy || sy->mi_type != MiTy_Symbole)
    return false;
  return mi_enshash_contient (&sv->sv_syconnu, sy);
}

//// sérialisation d'une valeur en JSON
json_t *
mi_json_val (struct Mi_Sauvegarde_st *sv, const Mit_Val v)
{
  assert (sv != NULL && sv->sv_magiq == MI_SAUVEGARDE_NMAGIQ);
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
      if (mi_sauvegarde_symbole_oublie (sv, sy))
        return json_null ();
      if (!mi_sauvegarde_symbole_connu (sv, sy))
        return json_null ();
      if (ind)
        return json_pack ("{sssi}", "symb", chn, "ind", ind);
      else
        return json_pack ("{ss}", "symb", chn);
    }
    break;
    case MiTy_Ensemble:
    {
      const Mit_Ensemble *en = mi_en_ensemble (v);
      unsigned t = en->mi_taille;
      json_t *jel = json_array ();
      for (unsigned ix = 0; ix < t; ix++)
        {
          const Mit_Symbole *sy = en->mi_elements[ix];
          if (mi_sauvegarde_symbole_connu (sv, sy))
            json_array_append_new (jel,
                                   mi_json_val (sv,
                                                MI_SYMBOLEV ((Mit_Symbole *)
                                                    sy)));
        }
      return json_pack ("{so}", "elem", jel);
    }
    break;
    case MiTy_Noeud:
    {
      const Mit_Noeud *nd = mi_en_noeud (v);
      const Mit_Symbole *sycon = mi_connective_noeud (nd);
      if (mi_sauvegarde_symbole_oublie (sv, sycon))
        return json_null ();
      if (!mi_sauvegarde_symbole_connu (sv, sycon))
        return json_null ();
      unsigned ar = mi_arite_noeud (nd);
      const json_t *jsymb =
        mi_json_val (sv, MI_SYMBOLEV ((Mit_Symbole *) sycon));
      json_t *jfils = json_array ();
      for (unsigned ix = 0; ix < ar; ix++)
        json_array_append_new (jfils,
                               (json_t *) mi_json_val (sv,
                                   nd->mi_fils[ix]));
      return json_pack ("{soso}", "conn", jsymb, "fils", jfils);
    }
    }
  return NULL;
}				// fin mi_json_val



/// déserialisation d'un JSON en valeur
Mit_Val
mi_val_json (const json_t *j)
{
  if (!j || json_is_null (j))
    return MI_NILV;
  if (json_is_integer (j))
    return MI_ENTIERV (mi_creer_entier (json_integer_value (j)));
  else if (json_is_real (j))
    return MI_DOUBLEV (mi_creer_double (json_real_value (j)));
  else if (json_is_string (j))
    return MI_CHAINEV (mi_creer_chaine (json_string_value (j)));
  else if (json_is_object (j))
    {
      json_t *js = json_object_get (j, "symb");
      if (js && json_is_string (js))
        {
          unsigned ind = json_integer_value (json_object_get (j, "ind"));
          return
            MI_SYMBOLEV (mi_creer_symbole_chaine
                         (json_string_value (js), ind));
        };
      json_t *je = json_object_get (j, "elem");
      if (je && json_is_array (je))
        {
          struct Mi_EnsHash_st eh = { };
          unsigned ln = json_array_size (je);
          mi_enshash_initialiser (&eh, 4 * ln / 3 + ln / 32 + 5);
          for (unsigned ix = 0; ix < ln; ix++)
            {
              Mit_Symbole *sy =
                mi_en_symbole (mi_val_json (json_array_get (je, ix)));
              if (sy)
                mi_enshash_ajouter (&eh, sy);
            }
          const Mit_Ensemble *e = mi_creer_ensemble_enshash (&eh);
          mi_enshash_detruire (&eh);
          return MI_ENSEMBLEV (e);
        }
      json_t *jc = json_object_get (j, "conn");
      json_t *jf = json_object_get (j, "fils");
      if (jc && json_is_array (jf))
        {
          Mit_Val petitab[8] = { };
          Mit_Symbole *sycon = mi_en_symbole (mi_val_json (jc));
          unsigned ar = json_array_size (jc);
          if (!sycon)
            return MI_NILV;
          Mit_Val *tab =
            (ar <
             sizeof (petitab) / sizeof (petitab[0])) ? petitab : calloc (ar +
                 1,
                 sizeof
                 (Mit_Val));
          if (!tab)
            MI_FATALPRINTF ("impossible d'allouer tampon pour %d fils", ar);
          for (unsigned ix = 0; ix < ar; ix++)
            tab[ix] = mi_val_json (json_array_get (jc, ix));
          Mit_Val res = MI_NOEUDV (mi_creer_noeud (sycon, ar, tab));
          if (tab != petitab)
            free (tab);
          return res;
        }
    }
  fprintf (stderr, "JSON incorrect:\n");
  json_dumpf (j, stderr, JSON_INDENT (1) | JSON_SORT_KEYS);
  fputc ('\n', stderr);
  return MI_NILV;
}				// fin mi_val_json



struct mi_vectatt_st
{
  unsigned vat_taille;
  unsigned vat_compte;
  const Mit_Symbole *vat_symb[];
};

static bool
mi_iterateur_attr (const Mit_Symbole *sy, const Mit_Val va, void *client)
{
  struct mi_vectatt_st *vat = client;
  assert (va.miva_ptr != (void *) -1);
  if (vat->vat_compte < vat->vat_taille)
    {
      vat->vat_symb[vat->vat_compte++] = sy;
      return false;
    }
  else
    return true;
}

json_t *
mi_json_contenu_symbole (struct Mi_Sauvegarde_st *sv, const Mit_Symbole *sy)
{
  if (!sv || !sy)
    return NULL;
  if (!mi_sauvegarde_symbole_connu (sv, sy))
    return json_null ();
  json_t *jsym = mi_json_val (sv, MI_SYMBOLEV ((Mit_Symbole *) sy));
  unsigned nbat = mi_assoc_taille (sy->mi_attrs);
  struct mi_vectatt_st *va =	//
  calloc (1,
            sizeof (struct mi_vectatt_st) + (nbat +
                1) * sizeof (Mit_Symbole *));
  va->vat_taille = nbat + 1;
  if (!va)
    MI_FATALPRINTF ("impossible d'allouer vecteur de %d attributs (%s)",
                    nbat, strerror (errno));
  mi_assoc_iterer (sy->mi_attrs, mi_iterateur_attr, va);
  assert (va->vat_compte == nbat);
  if (nbat > 1)
    qsort (va->vat_symb, nbat, sizeof (Mit_Symbole *), mi_cmp_symboleptr);
  json_t *jattrs = sy->mi_attrs ? json_array () : json_null ();
  for (unsigned ix = 0; ix < nbat; ix++)
    {
      const Mit_Symbole *syat = va->vat_symb[ix];
      if (!mi_sauvegarde_symbole_connu (sv, syat))
        continue;
      struct Mi_trouve_st tr = mi_assoc_chercher (sy->mi_attrs, syat);
      if (!tr.t_pres)
        continue;
      json_t *jent = json_pack ("{soso}", "at",
                                mi_json_val (sv,
                                             MI_SYMBOLEV ((Mit_Symbole *)
                                                 syat)),
                                "va", mi_json_val (sv, tr.t_val));

      json_array_append_new (jattrs, jent);
    };
  json_t *jcomps = sy->mi_comps ? json_array () : json_null ();
  int nbcomp = mi_vecteur_taille (sy->mi_comps);
  for (unsigned ix = 0; ix < (unsigned) nbcomp; ix++)
    {
      json_array_append_new (jcomps,
                             mi_json_val (sv,
                                          mi_vecteur_comp (sy->mi_comps,
                                              ix).t_val));
    }
  json_t *jcont = json_pack ("{sososo}", "ContSymb", jsym,
                             "attrs", jattrs,
                             "comps", jcomps);
  return jcont;
}				/* fin de mi_json_contenu_symbole */



void
mi_remplir_symbole_json (const json_t *j)
{
  if (!j)
    return;
  if (!json_is_object (j))
    return;
  const json_t *jsymb = json_object_get (j, "ContSymb");
  Mit_Symbole *sy = mi_en_symbole (mi_val_json (jsymb));
  if (!sy)
    return;
  const json_t *jattrs = json_object_get (j, "attrs");
  if (json_is_array (jattrs))
    {
      unsigned nbat = json_array_size (jattrs);
      sy->mi_attrs = mi_assoc_reserver (NULL, 4 * nbat / 3 + nbat / 32 + 3);
      for (unsigned ix = 0; ix < nbat; ix++)
        {
          const json_t *jent = json_array_get (jattrs, ix);
          if (!json_is_object (jent))
            continue;
          Mit_Symbole *syat =
            mi_en_symbole (mi_val_json (json_object_get (jent, "at")));
          if (!syat)
            continue;
          Mit_Val aval = mi_val_json (json_object_get (jent, "va"));
          sy->mi_attrs = mi_assoc_mettre (sy->mi_attrs, syat, aval);
        }
    }
  const json_t *jcomps = json_object_get (j, "comps");
  if (json_is_array (jcomps))
    {
      unsigned nbcomp = json_array_size (jcomps);
      sy->mi_comps =
        mi_vecteur_reserver (NULL, 6 * nbcomp / 5 + 2 + nbcomp / 32);
      for (unsigned ix = 0; ix < nbcomp; ix++)
        {
          Mit_Val cval = mi_val_json (json_array_get (jcomps, ix));
          sy->mi_comps = mi_vecteur_ajouter (sy->mi_comps, cval);
        }
    }
}				// fin mi_remplir_symbole_json

void
mi_sauvegarde_init (struct Mi_Sauvegarde_st *sv, const char *dir)
{
  assert (sv != NULL);
  assert (dir != NULL);
  if (access (dir, F_OK))
    {
      if (mkdir (dir, 0750))
        MI_FATALPRINTF ("impossible de créer le répertoire %s (%s)",
                        dir, strerror (errno));
    }
  memset (sv, 0, sizeof (*sv));
  sv->sv_magiq = MI_SAUVEGARDE_NMAGIQ;
  mi_enshash_initialiser (&sv->sv_syoubli, 5);
  mi_enshash_initialiser (&sv->sv_syconnu, 100);
  sv->sv_rep = strdup (dir);
  if (!sv->sv_rep)
    MI_FATALPRINTF ("mémoire pleine, impossible de dupliquer %s (%s)",
                    dir, strerror (errno));
}

void
mi_sauvegarde_symbole (struct Mi_Sauvegarde_st *sv, const Mit_Symbole *sy)
{
  assert (sv != NULL && sv->sv_magiq == MI_SAUVEGARDE_NMAGIQ);
  if (!sy || sy == MI_TROU_SYMBOLE || sy->mi_type != MiTy_Symbole)
    return;
  if (mi_sauvegarde_symbole_connu (sv, sy))
    return;
  if (mi_sauvegarde_symbole_oublie (sv, sy))
    return;
  mi_enshash_ajouter (&sv->sv_syconnu, sy);
  if (!sv->sv_tet)		// aucune queue de symboles
    {
      assert (!sv->sv_que);
      struct Mi_Queuesauve_st *qu =
      calloc (1, sizeof (struct Mi_Queuesauve_st));
      if (!qu)
        MI_FATALPRINTF ("manque de mémoire pour queue de symboles (%s)",
                        strerror (errno));
      sv->sv_tet = sv->sv_que = qu;
      qu->q_symb[0] = sy;
      return;
    }
  else if (sv->sv_tet == sv->sv_que)	// une queue de un seul morceau
    {
      struct Mi_Queuesauve_st *qu = sv->sv_tet;
      assert (qu->q_suiv == NULL);
      for (unsigned ix = 0; ix < MI_QUEUE_LONG_ELEM; ix++)
        if (!qu->q_symb[ix])
          {
            qu->q_symb[ix] = sy;
            return;
          };
      // on ajoute un morceau en queue...
      struct Mi_Queuesauve_st *nouvqu =
      calloc (1, sizeof (struct Mi_Queuesauve_st));
      if (!nouvqu)
        MI_FATALPRINTF ("manque de mémoire pour queue de symboles (%s)",
                        strerror (errno));
      nouvqu->q_symb[0] = sy;
      qu->q_suiv = nouvqu;
      sv->sv_que = nouvqu;
    }
  else				// une queue de plusieurs morceaux
    {
      struct Mi_Queuesauve_st *qu = sv->sv_que;
      assert (qu->q_suiv == NULL);
      for (unsigned ix = 0; ix < MI_QUEUE_LONG_ELEM; ix++)
        if (!qu->q_symb[ix])
          {
            qu->q_symb[ix] = sy;
            return;
          };
      // on ajoute un morceau en queue...
      struct Mi_Queuesauve_st *nouvqu =
      calloc (1, sizeof (struct Mi_Queuesauve_st));
      if (!nouvqu)
        MI_FATALPRINTF ("manque de mémoire pour queue de symboles (%s)",
                        strerror (errno));
      nouvqu->q_symb[0] = sy;
      qu->q_suiv = nouvqu;
      sv->sv_que = nouvqu;
    }
}				/* fin mi_sauvegarde_symbole */

void
mi_sauvegarde_oublier (struct Mi_Sauvegarde_st *sv, const Mit_Symbole *sy)
{
  assert (sv != NULL && sv->sv_magiq == MI_SAUVEGARDE_NMAGIQ);
  mi_enshash_ajouter (&sv->sv_syoubli, sy);
}				// fin mi_sauvegarde_oublier

static void
mi_sauvegarde_balayer (struct Mi_Sauvegarde_st *sv, const Mit_Val v)
{
  assert (sv != NULL && sv->sv_magiq == MI_SAUVEGARDE_NMAGIQ);
  switch (mi_vtype (v))
    {
    case MiTy_Nil:
    case MiTy_Chaine:
    case MiTy_Entier:
    case MiTy_Double:
      return;
    case MiTy_Noeud:
    {
      const Mit_Noeud *nd = v.miva_noe;
      const Mit_Symbole *sycon = nd->mi_conn;
      if (mi_sauvegarde_symbole_oublie (sv, sycon))
        return;
      mi_sauvegarde_symbole (sv, sycon);
      unsigned t = nd->mi_arite;
      for (unsigned ix = 0; ix < t; ix++)
        mi_sauvegarde_balayer (sv, nd->mi_fils[ix]);
    }
    return;
    case MiTy_Symbole:
    {
      const Mit_Symbole *sy = v.miva_sym;
      if (mi_sauvegarde_symbole_oublie (sv, sy))
        return;
      mi_sauvegarde_symbole (sv, sy);
    }
    return;
    case MiTy_Ensemble:
    {
      const Mit_Ensemble *en = v.miva_ens;
      unsigned t = en->mi_taille;
      for (unsigned ix = 0; ix < t; ix++)
        {
          const Mit_Symbole *sy = en->mi_elements[ix];
          if (mi_sauvegarde_symbole_oublie (sv, sy))
            continue;
          mi_sauvegarde_balayer (sv, MI_SYMBOLEV ((Mit_Symbole *) sy));
        }
    }
    return;
    }
}				/* fin mi_sauvegarde_balayer */

static bool
mi_sauvassocsy (const Mit_Symbole *sy, const Mit_Val va, void *client)
{
  struct Mi_Sauvegarde_st *sv = client;
  assert (sv && sv->sv_magiq == MI_SAUVEGARDE_NMAGIQ);
  if (mi_sauvegarde_symbole_oublie (sv, sy))
    return false;
  mi_sauvegarde_symbole (sv, sy);
  mi_sauvegarde_balayer (sv, va);
  return false;
}

static bool
mi_sauvcomp (const Mit_Val va, unsigned ix, void *client)
{
  struct Mi_Sauvegarde_st *sv = client;
  assert (sv && sv->sv_magiq == MI_SAUVEGARDE_NMAGIQ);
  assert (ix < INT_MAX / 2);
  mi_sauvegarde_balayer (sv, va);
  return false;

}

static void
mi_sauvegarde_balayer_contenu_symbole (struct Mi_Sauvegarde_st *sv,
                                       const Mit_Symbole *sy)
{
  assert (sv != NULL && sv->sv_magiq == MI_SAUVEGARDE_NMAGIQ);
  assert (sy != NULL && sy != MI_TROU_SYMBOLE && sy->mi_type == MiTy_Symbole);
  if (mi_sauvegarde_symbole_oublie (sv, sy))
    return;
  if (sy->mi_attrs)
    mi_assoc_iterer (sy->mi_attrs, mi_sauvassocsy, sv);
  if (sy->mi_comps)
    mi_vecteur_iterer (sy->mi_comps, mi_sauvcomp, sv);
}				/* fin mi_sauvegarde_balayer_contenu_symbole */

static bool
mi_traitersymboleprimairesv (const Mit_Symbole *sy, void *client)
{
  mi_sauvegarde_symbole ((struct Mi_Sauvegarde_st *) client, sy);
  return false;
}

void
mi_sauvegarde_finir (struct Mi_Sauvegarde_st *sv)
{
  assert (sv != NULL && sv->sv_magiq == MI_SAUVEGARDE_NMAGIQ);
  mi_iterer_symbole_primaire (mi_traitersymboleprimairesv, sv);
  while (sv->sv_tet != NULL)
    {
      struct Mi_Queuesauve_st *sq = sv->sv_tet;
      if (sv->sv_tet == sv->sv_que)
        sv->sv_tet = sv->sv_que = NULL;
      else
        sv->sv_tet = sq->q_suiv;
      sq->q_suiv = NULL;
      for (unsigned ix = 0; ix < MI_QUEUE_LONG_ELEM; ix++)
        {
          const Mit_Symbole *sy = sq->q_symb[ix];
          sq->q_symb[ix] = NULL;
          if (sy && sy != MI_TROU_SYMBOLE)
            mi_sauvegarde_balayer_contenu_symbole (sv, sy);
        }
      free (sq);
    }
  assert (sv->sv_tet == NULL && sv->sv_que == NULL);
  // faut parcourir la queue et appeler mi_sauvegarde_balayer_contenu_symbole
#warning mi_sauvegarde_finir incomplet
}
