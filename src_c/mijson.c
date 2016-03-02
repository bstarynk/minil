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
    case MiTy__Dernier: // ne devrait jamais arriver
      MI_FATALPRINTF("valeur impossible@%p", v.miva_ptr);
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
  unsigned nbat = mi_assoc_compte (sy->mi_attrs);
  struct mi_vectatt_st *va =	//
  calloc (1,
            sizeof (struct mi_vectatt_st) + (nbat +  1) * sizeof (Mit_Symbole *));
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



const Mit_Symbole *
mi_remplir_symbole_json (const json_t *j)
{
  if (!j)
    return NULL;
  if (!json_is_object (j))
    return NULL;
  const json_t *jsymb = json_object_get (j, "ContSymb");
  Mit_Symbole *sy = mi_en_symbole (mi_val_json (jsymb));
  if (!sy)
    return NULL;
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
  return sy;
}				// fin mi_remplir_symbole_json



// la longueur maximale d'un nom de fichier
// ce n'est pas une bonne idée,
// .... voir http://programmers.stackexchange.com/a/289441/40065
#define MI_NOMFICHMAX 384

void
mi_sauvegarde_init (struct Mi_Sauvegarde_st *sv, const char *rep)
{
  assert (sv != NULL);
  assert (rep != NULL);
  if (strlen (rep) > 2 * MI_NOMFICHMAX / 3)
    MI_FATALPRINTF ("nom de répertoire '%s' trop long (> %d)",
                    rep, 2 * MI_NOMFICHMAX / 3);
  if (!rep[0])
    rep = ".";
  if (access (rep, F_OK))
    {
      if (mkdir (rep, 0750))
        MI_FATALPRINTF ("impossible de créer le répertoire %s (%s)",
                        rep, strerror (errno));
    }
  memset (sv, 0, sizeof (*sv));
  sv->sv_magiq = MI_SAUVEGARDE_NMAGIQ;
  mi_enshash_initialiser (&sv->sv_syoubli, 5);
  mi_enshash_initialiser (&sv->sv_syconnu, 100);
  sv->sv_rep = strdup (rep);
  if (!sv->sv_rep)
    MI_FATALPRINTF ("mémoire pleine, impossible de dupliquer %s (%s)",
                    rep, strerror (errno));
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
    case MiTy__Dernier: // ne devrait jamais arriver
      MI_FATALPRINTF("valeur impossible@%p", v.miva_ptr);
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
mi_traitersymboleprimairesv (Mit_Symbole *sy, void *client)
{
  mi_sauvegarde_symbole ((struct Mi_Sauvegarde_st *) client, sy);
  return false;
}

static bool
mi_ecrirecontenusymbole (Mit_Symbole *sy, void *client)
{
  struct Mi_Sauvegarde_st *sv = client;
  assert (sy && sy->mi_type == MiTy_Symbole);
  assert (sv != NULL && sv->sv_magiq == MI_SAUVEGARDE_NMAGIQ);
  unsigned h = sy->mi_hash;
  char nomfich[MI_NOMFICHMAX];
  char sufind[16];
  snprintf (nomfich, sizeof (nomfich), "%s/data%02d", sv->sv_rep, h % 100);
  if (access (nomfich, F_OK))
    {
      if (mkdir (nomfich, 0750))
        MI_FATALPRINTF ("impossible de créer sous-répertoire %s (%s)",
                        nomfich, strerror (errno));
    }
  snprintf (nomfich, sizeof (nomfich), "%s/data%02d/%s%s.json",
            sv->sv_rep, h % 100, mi_symbole_chaine (sy),
            mi_symbole_indice_ch (sufind, sy));
  if (!access (nomfich, F_OK))
    {
      char nomvieux[MI_NOMFICHMAX];
      snprintf (nomvieux, sizeof (nomvieux), "%s~", nomfich);
      rename (nomfich, nomvieux);
    }
  FILE *fich = fopen (nomfich, "w");
  if (!fich)
    MI_FATALPRINTF ("impossible d'écrire le fichier %s (%s)",
                    nomfich, strerror (errno));
  json_t *j = mi_json_contenu_symbole (sv, sy);
  assert (json_is_object (j));
  if (json_dumpf
      (j, fich, JSON_INDENT (1) | JSON_ENSURE_ASCII | JSON_SORT_KEYS))
    MI_FATALPRINTF ("échec d'écriture de JSON dans %s", nomfich);
  fputc ('\n', fich);
  fclose (fich);
  return false;
}				// fin mi_ecrirecontenusymbole

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
  mi_enshash_iterer (&sv->sv_syconnu, mi_ecrirecontenusymbole, sv);
  // ecrire la liste des symboles connus
  const Mit_Ensemble *ensy = mi_creer_ensemble_enshash (&sv->sv_syconnu);
  assert (ensy && ensy->mi_type == MiTy_Ensemble);
  char nomfic[MI_NOMFICHMAX];
  FILE *fs = NULL;
  snprintf (nomfic, sizeof (nomfic), "%s/symbolist", sv->sv_rep);
  if (!access (nomfic, R_OK))
    {
      char nomanc[MI_NOMFICHMAX];
      snprintf (nomanc, sizeof (nomanc), "%s~", nomfic);
      rename (nomfic, nomanc);
    }
  fs = fopen (nomfic, "w");
  mi_emettre_notice_gplv3 (fs, "# ", "", "symbolist");
  unsigned nbsymb = ensy->mi_taille;
  for (unsigned ix = 0; ix < nbsymb; ix++)
    {
      char tampsuf[16];
      const Mit_Symbole *syel = ensy->mi_elements[ix];
      assert (syel && syel->mi_type == MiTy_Symbole);
      fprintf (fs, "%s%s\n",
               mi_symbole_chaine (syel),
               mi_symbole_indice_ch (tampsuf, syel));
    }
  fprintf (fs, "# fin %d symboles\n", ensy->mi_taille);
  fclose (fs), fs = NULL;
  // écrire le fichier d'entête pour les symboles prédéfinis
  snprintf (nomfic, sizeof (nomfic), "%s/_mi_predef.h", sv->sv_rep);
  if (!access (nomfic, R_OK))
    {
      char nomanc[MI_NOMFICHMAX];
      snprintf (nomanc, sizeof (nomanc), "%s~", nomfic);
      rename (nomfic, nomanc);
    }
  fs = fopen (nomfic, "w");
  int nbpredef = 0;
  mi_emettre_notice_gplv3 (fs, "/// ", "", "_mi_predef.h");
  fprintf (fs, "\n#ifndef MI_TRAITER_PREDEFINI\n"
           "#error " "MI_TRAITER_PREDEFINI indefini\n" "#endif\n\n");
  for (unsigned ix = 0; ix < ensy->mi_taille; ix++)
    {
      char tampsuf[16];
      const Mit_Symbole *syel = ensy->mi_elements[ix];
      assert (syel && syel->mi_type == MiTy_Symbole);
      if (!syel->mi_predef)
        continue;
      fprintf (fs, "MI_TRAITER_PREDEFINI(%s%s,%u)\n",
               mi_symbole_chaine (syel),
               mi_symbole_indice_ch (tampsuf, syel), syel->mi_hash);
      nbpredef++;
    }
  fprintf (fs, "\n#undef MI_TRAITER_PREDEFINI\n" "\n#undef MI_NB_PREDEFINIS\n"
           "#define MI_NB_PREDEFINIS %d\n", nbpredef);
  fprintf (fs, "// fin fichier _mi_predef.h\n");
  fclose (fs), fs = NULL;
  mi_enshash_detruire (&sv->sv_syoubli);
  mi_enshash_detruire (&sv->sv_syconnu);
  printf ("sauvegarde de %d symboles dont %d prédéfinis dans %s/\n",
          nbsymb, nbpredef, sv->sv_rep);
  free ((void *) sv->sv_rep), sv->sv_rep = NULL;
  assert (sv->sv_tet == NULL && sv->sv_que == NULL);
  memset (sv, 0, sizeof (*sv));
}				/* fin mi_sauvegarde_finir */


static void
mi_creer_symboles_charges (const char *rep)
{
  assert (rep != NULL);
  char nomfic[MI_NOMFICHMAX];
  char ligne[MI_NOMFICHMAX];
  FILE *fs = NULL;
  snprintf (nomfic, sizeof (nomfic), "%s/symbolist", rep);
  fs = fopen (nomfic, "r");
  if (!fs)
    MI_FATALPRINTF ("impossible d'ouvrir la liste de symboles %s (%s)",
                    nomfic, strerror (errno));
  int numlin = 0;
  int nbsymb = 0;
  do
    {
      memset (ligne, 0, sizeof (ligne));
      if (!fgets (ligne, sizeof (ligne) - 1, fs))
        break;
      numlin++;
      if (ligne[0] == '#')
        continue;
      if (!isalpha (ligne[0]))
        MI_FATALPRINTF ("ligne#%d de %s incorrecte: %s",
                        numlin, nomfic, ligne);
      if (strlen (ligne) >= sizeof (ligne) - 2)
        MI_FATALPRINTF ("ligne#%d de %s trop longue: %s",
                        numlin, nomfic, ligne);
      int finom = 0;
      unsigned ind = 0;
      for (finom = 0; finom < (int) sizeof (ligne) && isalnum (ligne[finom]);
           finom++);
      if (isspace (ligne[finom]))
        ligne[finom] = '\0';
      else if (ligne[finom] == '_')
        {
          ind = atoi (ligne + finom + 1);
          ligne[finom] = '\0';
        };
      if (!mi_nom_licite_chaine (ligne))
        MI_FATALPRINTF ("ligne#%d de %s illicite: %s", numlin, nomfic, ligne);
      Mit_Symbole *sy = mi_creer_symbole_chaine (ligne, ind);
      if (!sy)
        MI_FATALPRINTF ("ligne#%d de %s avec mauvais symbole: %s",
                        numlin, nomfic, ligne);
      nbsymb++;
    }
  while (!feof (fs));
  fclose (fs);
  printf ("%d symboles créés depuis %s\n", nbsymb, nomfic);
}				// fin mi_creer_symboles_charges


static long mi_compte_symbole_charge;

static void
mi_charger_contenu_symbole (Mit_Symbole *sy, const char *rep)
{
  assert (sy && sy->mi_type == MiTy_Symbole);
  assert (rep != NULL && rep[0] != '\0');
  char nomfich[MI_NOMFICHMAX];
  char sufind[16];
  unsigned h = sy->mi_hash;
  if (snprintf
      (nomfich, sizeof (nomfich), "%s/data%02d/%s%s.json", rep, h % 100,
       mi_symbole_chaine (sy), mi_symbole_indice_ch (sufind,
           sy)) >=
      (int) sizeof (nomfich) - 1)
    {
      fprintf (stderr, "nom de fichier trop long %s/data%02d/%s%s.json\n",
               rep, h % 100, mi_symbole_chaine (sy),
               mi_symbole_indice_ch (sufind, sy));
      return;
    }
  if (access (nomfich, R_OK))
    {
      fprintf (stderr, "impossible de lire %s: %s\n", nomfich,
               strerror (errno));
      return;
    }
  json_error_t jerr = { };
  memset (&jerr, 0, sizeof (jerr));
  json_t *j =
    json_load_file (nomfich, JSON_DISABLE_EOF_CHECK | JSON_REJECT_DUPLICATES,
                    &jerr);
  if (!j)
    {
      fprintf (stderr,
               "Décodage de JSON echoue dans %s, ligne %d, colonne %d, position %ld: %s\n",
               jerr.source, jerr.line, jerr.column, (long) jerr.position,
               jerr.text);
      return;
    }
  if (sy != mi_remplir_symbole_json (j))
    fprintf (stderr, "Remplissage du symbole %s%s échoue (fichier %s)\n",
             mi_symbole_chaine (sy), mi_symbole_indice_ch (sufind, sy),
             nomfich);
  json_decref (j);
  mi_compte_symbole_charge++;
}				/* fin mi_charger_contenu_symbole */


static bool
mi_chargersymbolesecondaire (Mit_Symbole *sy, void *rep)
{
  assert (sy && sy->mi_type == MiTy_Symbole);
  if (sy->mi_indice == 0)
    return false;
  mi_charger_contenu_symbole (sy, (const char *) rep);
  return false;
}				// fin mi_chargersymboleprimaire


static bool
mi_chargersymboleprimaire (Mit_Symbole *sy, void *rep)
{
  mi_charger_contenu_symbole (sy, (const char *) rep);
  mi_iterer_symbole_nomme (mi_symbole_chaine (sy),
                           mi_chargersymbolesecondaire, rep);
  return false;
}				// fin mi_chargersymboleprimaire


void
mi_charger_etat (const char *rep)
{
  if (!rep || !rep[0])
    rep = ".";
  if (access (rep, R_OK))
    MI_FATALPRINTF ("mauvais répertoire à charger %s (%s)",
                    rep, strerror (errno));
  mi_compte_symbole_charge = 0;
  mi_creer_symboles_charges (rep);
  mi_iterer_symbole_primaire (mi_chargersymboleprimaire, (void *) rep);
  printf ("%ld symboles ont été chargés de %s\n",
          mi_compte_symbole_charge, rep);
}				// fin mi_charger_etat
