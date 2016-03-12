// fichier milire.c - lecture
/* la notice de copyright est légalement en anglais */

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

#define MI_LECTEUR_NMAGIQ 345961075 /*0x149ef273*/
const char mi_lecture_symbole_absent[] = "symbole absent";

struct Mi_Lecteur_st
{
  unsigned lec_nmagiq;		/* toujours MI_LECTEUR_NMAGIQ */
  bool lec_pascreer;		/* vrai si on veut lire sans allocation */
  const char*lec_errptr; /* en erreur, le pointer courant */
  const char*lec_errfin; /* en erreur, le pointer finissant le mot, etc.. */
  const char*lec_errmsg; /* en erreur, un message */
  jmp_buf lec_jb; /* pour sortir en erreur via longjmp */
  struct Mi_Assoc_st* lec_assoctrou; /* l'association des "trous" $nom */
};
#define MI_ERREUR_LECTURE(Lec,Ptr,Fin,Msg) do {		\
    struct Mi_Lecteur_st*_lec = (Lec);			\
    assert (_lec != NULL				\
	    && _lec->lec_nmagiq == MI_LECTEUR_NMAGIQ);	\
    _lec->lec_errptr = (Ptr);				\
    _lec->lec_errfin = (Fin);				\
    _lec->lec_errmsg = (Msg);				\
    longjmp(_lec->lec_jb, __LINE__); } while(0)

// la chaine lue est temporairement modifiée
static Mit_Val
mi_lire_primaire(struct Mi_Lecteur_st*lec, char*ps, const char**pfin)
{
  assert (lec && lec->lec_nmagiq == MI_LECTEUR_NMAGIQ);
  assert (ps != NULL);
  const char*psinit = ps;
  while (isspace(ps[0])) ps++;
  if (isdigit(ps[0])
      || ((ps[0]=='-' || ps[1]=='+')
          && (isdigit(ps[1])
              // les flottants peuvent être NAN ou INF, traités par strtod
              || !strncasecmp(ps+1,"INF",3) || !strncasecmp(ps+1,"NAN",3))))
    {
      // un nombre
      char*finent = NULL;
      char*findbl = NULL;
      long e = strtol(ps, &finent, 0);
      double d = strtod(ps,&findbl);
      if (findbl>finent)
        {
          if (pfin) *pfin = findbl;
          if (lec->lec_pascreer) return MI_NILV;
          return MI_DOUBLEV(mi_creer_double(d));
        }
      else if (finent > ps)
        {
          if (pfin) *pfin = finent;
          if (lec->lec_pascreer) return MI_NILV;
          return MI_ENTIERV(mi_creer_entier(e));
        }
      else MI_ERREUR_LECTURE(lec, ps, NULL, "mauvais nombre");
    }
  else if (isalpha(ps[0]))
    {
      // un symbole existant
      char*pdebsymb = ps;
      char*pfinrad = NULL;	/* la fin du radical */
      char*pblanc = NULL; 	/* le blanc souligné avant indice */
      char*pfind = NULL;		/* la fin de l'indice */
      unsigned ind = 0;
      while (isalnum(*ps)) ps++;
      pfinrad = ps;
      if (*ps=='_' && isdigit(ps[1]))
        {
          pblanc = ps;
          ind = (unsigned) strtol(ps+1, &pfind, 10);
          *pblanc = (char)0;
        }
      Mit_Symbole*sy = mi_trouver_symbole_chaine(pdebsymb,ind);
      if (pblanc) *pblanc = '_';
      if (sy)
        {
          if (pfin)
            *pfin = pfind?pfind:pfinrad;
          if (lec->lec_pascreer) return MI_NILV;
          return MI_SYMBOLEV(sy);
        }
      else MI_ERREUR_LECTURE(lec, pdebsymb,pfinrad,
                               mi_lecture_symbole_absent);
    }
  else if (ps[0] == '$' && isalpha(ps[1]))
    {
      // un trou
      char*pdebtrou = ps;
      ps++;
      while (isalnum(*ps)) ps++;
      char*pfintrou = ps;
      char capres = *pfintrou;
      *pfintrou = (char)0;
      Mit_Symbole*sy = mi_trouver_symbole_chaine(pdebtrou+1,0);
      *pfintrou = capres;
      if (sy)
        {
          if (pfin)
            *pfin = pfintrou;
          if (lec->lec_pascreer)
            {
              if (!mi_assoc_chercher(lec->lec_assoctrou,sy).t_pres)
                {
                };
              return MI_NILV;
            }
          return MI_SYMBOLEV(sy);
        }
      else MI_ERREUR_LECTURE(lec, pdebtrou+1,pfintrou,
                               mi_lecture_symbole_absent);

    }
} /* fin mi_lire_primaire */

Mit_Val mi_lire_valeur(struct Mi_Lecteur_st*lec, char*ps, const char**pfin)
{
  assert (lec && lec->lec_nmagiq == MI_LECTEUR_NMAGIQ);
  assert (ps != NULL);
}

