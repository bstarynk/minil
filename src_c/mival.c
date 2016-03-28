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
    MI_FATALPRINTF ("chaine %.50s incorrecte", ch);
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

const Mit_Tuple*mi_creer_tuple_symboles(unsigned nb,
                                        const Mit_Symbole**tabsym)
{
  unsigned cnt=0;
  if (nb==0 || !tabsym) return &mi_tupvide;
  for (unsigned ix=0; ix<nb; ix++)
    {
      const Mit_Symbole*sy = tabsym[ix];
      if (sy && sy != MI_TROU_SYMBOLE && sy->mi_type == MiTy_Symbole) cnt++;
    };
  const Mit_Symbole* petitab[16] = {};
  const Mit_Symbole**tab = (cnt<sizeof(petitab)/sizeof(petitab[0]))?petitab
                           :calloc(cnt+1,sizeof(Mit_Symbole*));
  if (!tab)
    MI_FATALPRINTF("impossible d'allouer tableau de %d symboles (%s)",
                   cnt+1, strerror(errno));
  cnt=0;
  for (unsigned ix=0; ix<nb; ix++)
    {
      const Mit_Symbole*sy = tabsym[ix];
      if (sy && sy != MI_TROU_SYMBOLE && sy->mi_type == MiTy_Symbole)
        tab[cnt++] = sy;
    };
  Mit_Tuple*tu = mi_allouer_valeur(MiTy_Tuple, sizeof(Mit_Tuple)+cnt*sizeof(Mit_Symbole*));
  for (unsigned ix=0; ix<cnt; ix++)
    tu->mi_composants[ix] = (Mit_Symbole*)tab[ix];
  if (tab != petitab) free(tab);
  mi_calculer_hash_tuple(tu);
  return tu;
} // fin mi_creer_tuple_symboles


static struct Mi_Vecteur_st*
mi_vectcomp_ajouter(struct Mi_Vecteur_st*vecarg, const Mit_Val v)
{
  struct Mi_Vecteur_st*vec = vecarg;
  switch(mi_vtype(v))
    {
    case MiTy_Symbole:
      vec = mi_vecteur_ajouter(vec,v);
      break;
    case MiTy_Tuple:
    {
      const Mit_Tuple*tu=mi_en_tuple(v);
      unsigned t = tu->mi_taille;
      vec = mi_vecteur_reserver(vec, t+1);
      for (unsigned ix=0; ix<t; ix++)
        vec = mi_vectcomp_ajouter(vec,MI_SYMBOLEV(tu->mi_composants[ix]));
    }
    break;
    case MiTy_Ensemble:
    {
      const Mit_Ensemble*en=mi_en_ensemble(v);
      unsigned t = en->mi_taille;
      vec = mi_vecteur_reserver(vec, t+1);
      for (unsigned ix=0; ix<t; ix++)
        vec = mi_vectcomp_ajouter(vec,MI_SYMBOLEV(en->mi_elements[ix]));
    }
    break;
    default:
      break;
    }
  return vec;
} /* end mi_vectcomp_ajouter */

const Mit_Tuple*mi_creer_tuple_valeurs(unsigned nb,
                                       const Mit_Val*tabval)
{
  struct Mi_Vecteur_st *vec = mi_vecteur_reserver(NULL,5*nb/4+1);
  for (unsigned ix=0; ix<nb; ix++)
    vec = mi_vectcomp_ajouter(vec,tabval[ix]);
  unsigned t = mi_vecteur_taille(vec);
  if (t > 0)
    {
      Mit_Tuple*tu = mi_allouer_valeur(MiTy_Tuple, t*sizeof(Mit_Symbole*));
      for (unsigned ix=0; ix<t; ix++)
        tu->mi_composants[ix] = mi_en_symbole(mi_vecteur_comp(vec,ix).t_val);
      mi_calculer_hash_tuple(tu);
      free(vec);
      return tu;
    }
  else return &mi_tupvide;
} // fin mi_creer_tuple_valeurs

void
mi_afficher_valeur(FILE*fi, const Mit_Val v)
{
  if (!fi) return;
  enum mi_typeval_en ty = mi_vtype(v);
  switch(ty)
    {
    case MiTy_Nil:
      fputs("~", fi);
      break;
    case MiTy_Entier:
      fprintf(fi, "%ld", mi_vald_entier(v,0));
      break;
    case MiTy_Double:
    {
      double d = mi_vald_double(v, NAN);
      if (isnan(d)) fputs("+NAN", fi);
      else if (isinf(d)) fprintf(fi, "%cINF", (d>0.0)?'+':'-');
      else
        {
          // il faut absolument le point décimal
          char tamp[64];
          snprintf(tamp, sizeof(tamp), "%g", d);
          if (strchr(tamp, '.'))
            {
              fputs(tamp, fi);
              break;
            };
          snprintf(tamp, sizeof(tamp), "%.3f", d);
          if (strchr(tamp, '.') && atof(tamp) == d && strlen(tamp)<10)
            {
              fputs(tamp, fi);
              break;
            };
          snprintf(tamp, sizeof(tamp), "%.6f", d);
          if (strchr(tamp, '.') && atof(tamp) == d && strlen(tamp)<20)
            {
              fputs(tamp, fi);
              break;
            };
          snprintf(tamp, sizeof(tamp), "%.9e", d);
          if (strchr(tamp, '.') && atof(tamp) == d)
            {
              fputs(tamp, fi);
              break;
            };
          snprintf(tamp, sizeof(tamp), "%.18e", d);
          assert (strchr(tamp, '.'));
          fputs(tamp, fi);
        }
    }
    break;
    case MiTy_Chaine:
    {
      fputc('"', fi);
      mi_afficher_chaine_encodee(fi, mi_vald_chaine(v,NULL));
      fputc('"', fi);
    }
    break;
    case MiTy_Symbole:
    {
      char tampind[16];
      const Mit_Symbole*sy = mi_en_symbole(v);
      fprintf(fi, "%s%s", mi_symbole_chaine(sy), mi_symbole_indice_ch(tampind,sy));
    }
    break;
    case MiTy_Tuple:
    {
      const Mit_Tuple*tup = mi_en_tuple(v);
      unsigned ar = mi_arite_tuple(tup);
      fputc('[', fi);
      for (unsigned ix=0; ix<ar; ix++)
        {
          if (ix>0)
            {
              if (ix % 5 == 0) fputs(", ", fi);
              else fputc(',', fi);
            }
          const Mit_Symbole*sy = mi_tuple_nieme(tup, (int)ix);
          assert (sy && sy->mi_type == MiTy_Symbole);
          char tampind[16];
          fprintf(fi, "%s%s", mi_symbole_chaine(sy), mi_symbole_indice_ch(tampind,sy));
        };
      fputc(']', fi);
    }
    break;
    case MiTy_Ensemble:
    {
      const Mit_Ensemble*ens = mi_en_ensemble(v);
      unsigned car = mi_cardinal_ensemble(ens);
      fputc('{', fi);
      for (unsigned ix=0; ix<car; ix++)
        {
          if (ix>0)
            {
              if (ix % 5 == 0) fputs(", ", fi);
              else fputc(',', fi);
            }
          const Mit_Symbole*sy = mi_ensemble_nieme(ens, (int)ix);
          assert (sy && sy->mi_type == MiTy_Symbole);
          char tampind[16];
          fprintf(fi, "%s%s", mi_symbole_chaine(sy), mi_symbole_indice_ch(tampind,sy));
        };
      fputc('}', fi);
    }
    break;
    case MiTy__Dernier:
      MI_FATALPRINTF("valeur impossible @%p", v.miva_ptr);
    }
  fputc('\n', fi);
} // fin mi_afficher_valeur
