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

#define MI_LECTEUR_NMAGIQ 345961075	/*0x149ef273 */
const char mi_lecture_symbole_absent[] = "symbole absent";

struct Mi_Lecteur_st
{
  unsigned lec_nmagiq;		/* toujours MI_LECTEUR_NMAGIQ */
  bool lec_pascreer;		/* vrai si on veut lire sans allocation */
  const char *lec_errptr;	/* en erreur, le pointer courant */
  const char *lec_errfin;	/* en erreur, le pointer finissant le mot, etc.. */
  const char *lec_errmsg;	/* en erreur, un message */
  jmp_buf lec_jb;		/* pour sortir en erreur via longjmp */
  struct Mi_Assoc_st *lec_assoctrou;	/* l'association des "trous" $nom */
};
#define MI_ERREUR_LECTURE(Lec,Ptr,Fin,Msg) do {		\
    struct Mi_Lecteur_st*_lec = (Lec);			\
    assert (_lec != NULL				\
	    && _lec->lec_nmagiq == MI_LECTEUR_NMAGIQ);	\
    _lec->lec_errptr = (Ptr);				\
    _lec->lec_errfin = (Fin);				\
    _lec->lec_errmsg = (Msg);				\
    longjmp(_lec->lec_jb, __LINE__); } while(0)


static const Mit_Chaine *
mi_lire_chaine (struct Mi_Lecteur_st *lec, char *ps, const char **pfin)
{
  assert (lec && lec->lec_nmagiq == MI_LECTEUR_NMAGIQ);
  assert (ps != NULL && *ps == '"');
  const char *psinit = ps;
  size_t lginit = strlen (psinit);
  unsigned taille = mi_nombre_premier_apres (2 + (9 * lginit) / 8);
  unsigned lg = 0;
  char *tamp = calloc (taille, 1);
  if (!tamp)
    MI_FATALPRINTF
    ("impossible d'allouer tampon pour chaine de %d octets (%s)", taille,
     strerror (errno));
  ps++;
  while (*ps && *ps != '"')
    {
#define MI_UTF_PLACE 10
      // on veut au moins 10 octets, au cas où....
      if (lg + MI_UTF_PLACE + 1 >= taille)
        {
          unsigned nouvtaille =
            mi_nombre_premier_apres (4 * lg / 3 + MI_UTF_PLACE + 2);
          char *nouvtamp = (nouvtaille > 0) ? calloc (nouvtaille, 1) : NULL;
          if (!nouvtamp)
            MI_FATALPRINTF
            ("impossible d'agrandir tampon pour chaine de %d octets (%s)",
             nouvtaille ? nouvtaille : lg, strerror (errno));
          memcpy (nouvtamp, tamp, lg);
          free (tamp);
          tamp = nouvtamp;
        }
      if (ps[0] == '\\')
        {
          switch (ps[1])
            {
            case '\'':
            case '\"':
            case '\\':
              tamp[lg++] = ps[1];
              ps += 2;
              break;
            case 'a':
              tamp[lg++] = '\a';
              ps += 2;
              break;
            case 'b':
              tamp[lg++] = '\b';
              ps += 2;
              break;
            case 'f':
              tamp[lg++] = '\f';
              ps += 2;
              break;
            case 'n':
              tamp[lg++] = '\n';
              ps += 2;
              break;
            case 'r':
              tamp[lg++] = '\r';
              ps += 2;
              break;
            case 't':
              tamp[lg++] = '\t';
              ps += 2;
              break;
            case 'v':
              tamp[lg++] = '\v';
              ps += 2;
              break;
            case 'e':
              tamp[lg++] = '\033' /*ESCAPE*/;
              ps += 2;
              break;
#warning les conversions de caractères doivent calculer une longeur UTF8
            case 'x':
            {
              int p = -1;
              int c = 0;
              if (sscanf (ps + 2, "%02x%n", &c, &p) > 0 && p > 0)
                {
                  int l =
                    u8_uctomb ((uint8_t *) tamp, (ucs4_t) c, MI_UTF_PLACE);
                  if (l > 0)
                    lg += l;
                  ps += p + 2;
                }
            }
            break;
            case 'u':
            {
              int p = -1;
              int c = 0;
              if (sscanf (ps + 2, "%04x%n", &c, &p) > 0 && p > 0)
                {
                  int l =
                    u8_uctomb ((uint8_t *) tamp, (ucs4_t) c, MI_UTF_PLACE);
                  if (l > 0)
                    lg += l;
                  ps += p + 2;
                }
            }
            break;
            case 'U':
            {
              int p = -1;
              int c = 0;
              if (sscanf (ps + 2, "%08x%n", &c, &p) > 0 && p > 0)
                {
                  int l =
                    u8_uctomb ((uint8_t *) tamp, (ucs4_t) c, MI_UTF_PLACE);
                  if (l > 0)
                    lg += l;
                  ps += p + 2;
                }
            }
            break;
            default:
              tamp[lg++] = ps[1];
              ps += 2;
              break;
            }
        }
      else
        {
          ucs4_t uc = 0;
          int lc =
            u8_mbtouc (&uc, (const uint8_t *) ps, psinit + lginit - ps);
          if (lc <= 0)
            MI_ERREUR_LECTURE (lec, ps, NULL, "mauvaise chaine UTF8");
          memcpy (tamp + lg, ps, lc);
          lg += lc;
          ps += lc;
        }
    }
  if (*ps == '"')
    {
      if (pfin)
        *pfin = ps + 1;
    };
  const Mit_Chaine *ch = mi_creer_chaine (tamp);
  free (tamp), tamp = NULL;
  return ch;
}				/* fin mi_lire_chaine */


void
mi_afficher_chaine_encodee (FILE * fi, const char *ch)
{
  if (!fi || !ch)
    return;
  size_t ln = strlen (ch);
  if (u8_check ((const uint8_t *) ch, ln))
    MI_FATALPRINTF ("chaine %.50s... incorrecte", ch);
  size_t off = 0;
  while (off < ln)
    {
      ucs4_t uc = 0;
      int lc = u8_mbtouc_unsafe (&uc, (const uint8_t *) (ch + off), ln - off);
      if (lc <= 0)
        break;
      if (uc < 128)
        {
          switch ((char) uc)
            {
            case '\'':
            case '\"':
            case '\\':
              fprintf (fi, "\\%c", (char) uc);
              break;
            case '\a':
              fputs ("\\a", fi);
              break;
            case '\b':
              fputs ("\\b", fi);
              break;
            case '\f':
              fputs ("\\f", fi);
              break;
            case '\n':
              fputs ("\\n", fi);
              break;
            case '\r':
              fputs ("\\r", fi);
              break;
            case '\t':
              fputs ("\\t", fi);
              break;
            case '\v':
              fputs ("\\v", fi);
              break;
            case '\033' /* ESCAPE */ :
              fputs ("\\e", fi);
              break;
            default:
              if (isprint ((char) uc))
                fputc ((char) uc, fi);
              else
                fprintf (fi, "\\x%02x", uc);
              break;
            }
        }
      else if (uc < 0xffff)
        fprintf (fi, "\\u%04x", (unsigned) uc);
      else
        fprintf (fi, "\\U%08x", (unsigned) uc);
      off += lc;
    }
}				/* fin mi_afficher_chaine_encodee */


// la chaine lue est temporairement modifiée
static Mit_Val
mi_lire_primaire (struct Mi_Lecteur_st *lec, char *ps, const char **pfin)
{
  assert (lec && lec->lec_nmagiq == MI_LECTEUR_NMAGIQ);
  assert (ps != NULL);
  const char *psinit = ps;
  while (isspace (ps[0]))
    ps++;
  // lire un nombre
  if (isdigit (ps[0]) //
      || ((ps[0] == '-' || ps[1] == '+')
          && (isdigit (ps[1])
              // les flottants peuvent être NAN ou INF, traités par strtod
              ||  !strncasecmp (ps+ 1, "INF", 3)
              || !strncasecmp (ps  + 1, "NAN",  3))))
    {
      // un nombre
      char *finent = NULL;
      char *findbl = NULL;
      long e = strtol (ps, &finent, 0);
      double d = strtod (ps, &findbl);
      if (findbl > finent)
        {
          if (pfin)
            *pfin = findbl;
          if (lec->lec_pascreer)
            return MI_NILV;
          return MI_DOUBLEV (mi_creer_double (d));
        }
      else if (finent > ps)
        {
          if (pfin)
            *pfin = finent;
          if (lec->lec_pascreer)
            return MI_NILV;
          return MI_ENTIERV (mi_creer_entier (e));
        }
      else
        MI_ERREUR_LECTURE (lec, ps, NULL, "mauvais nombre");
    }
  // lire un symbole
  else if (isalpha (ps[0]))
    {
      // un symbole existant
      char *pdebsymb = ps;
      char *pfinrad = NULL;	/* la fin du radical */
      char *pblanc = NULL;	/* le blanc souligné avant indice */
      char *pfind = NULL;	/* la fin de l'indice */
      unsigned ind = 0;
      while (isalnum (*ps))
        ps++;
      pfinrad = ps;
      if (*ps == '_' && isdigit (ps[1]))
        {
          pblanc = ps;
          ind = (unsigned) strtol (ps + 1, &pfind, 10);
          *pblanc = (char) 0;
        }
      Mit_Symbole *sy = mi_trouver_symbole_chaine (pdebsymb, ind);
      if (pblanc)
        *pblanc = '_';
      if (sy)
        {
          if (pfin)
            *pfin = pfind ? pfind : pfinrad;
          if (lec->lec_pascreer)
            return MI_NILV;
          return MI_SYMBOLEV (sy);
        }
      else
        MI_ERREUR_LECTURE (lec, pdebsymb, pfinrad, mi_lecture_symbole_absent);
    }
  // une chaîne de caractères
  else if (ps[0] == '"')
    return MI_CHAINEV (mi_lire_chaine (lec, ps, pfin));
  // une chaîne de caractères verbatim, ... jusqu'à la fin
  else if (ps[0] == '\'')
    {
      char *debch = ps;
      unsigned l = strlen (ps);
      if (ps[l - 1] == '\n')
        ps[--l] = (char) 0;
      if (u8_check ((const uint8_t *) ps, l))
        MI_ERREUR_LECTURE (lec, debch, ps + l,
                           "mauvaise chaine verbatim UTF8");
      const Mit_Chaine *ch = mi_creer_chaine (ps + 1);
      if (pfin)
        *pfin = ps + l;
      return MI_CHAINEV (ch);
    }
  /// lire un trou
  else if (ps[0] == '$' && isalpha (ps[1]))
    {
      char *pdebtrou = ps;
      ps++;
      while (isalnum (*ps))
        ps++;
      char *pfintrou = ps;
      char capres = *pfintrou;
      *pfintrou = (char) 0;
      Mit_Symbole *sy = mi_trouver_symbole_chaine (pdebtrou + 1, 0);
      *pfintrou = capres;
      if (sy)
        {
          if (pfin)
            *pfin = pfintrou;
          if (lec->lec_pascreer)
            {
              if (!mi_assoc_chercher (lec->lec_assoctrou, sy).t_pres)
                {
                  lec->lec_assoctrou =	//
                    mi_assoc_mettre (lec->lec_assoctrou,
                                     sy, MI_SYMBOLEV (MI_PREDEFINI (trou)));
                };
              return MI_NILV;
            }
          else
            {
              struct Mi_trouve_st tr =	//
              mi_assoc_chercher (lec->lec_assoctrou, sy);
              // La première passe aurait dû trouver le trou...
              if (!tr.t_pres)
                MI_FATALPRINTF ("le trou $%s est manquant", pdebtrou + 1);
              return tr.t_val;
            }
        }
      else
        MI_ERREUR_LECTURE (lec, pdebtrou + 1, pfintrou,
                           mi_lecture_symbole_absent);
    }
  else if (ps[0] == '(') {
  }
}				/* fin mi_lire_primaire */



Mit_Val
mi_lire_valeur (struct Mi_Lecteur_st *lec, char *ps, const char **pfin)
{
  assert (lec && lec->lec_nmagiq == MI_LECTEUR_NMAGIQ);
  assert (ps != NULL);
}
