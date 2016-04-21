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


// lit un symbole, ou un mot qui pourrait devenir un symbole.
// positionne pfin pour en donner la position après le dernier caractère.
// renvoie un symbole, s'il est trouvé.
static Mit_Symbole *
mi_lire_symbole (char *ps, char **pfin)
{
  assert (ps != NULL);
  assert (pfin != NULL);
  while (isspace (ps[0]))
    ps++;
  if (!isalpha (ps[0]))
    {
      *pfin = NULL;
      return NULL;
    };
  char *pdebsymb = ps;
  char *pfinrad = NULL;		/* la fin du radical */
  char *pblanc = NULL;		/* le blanc souligné avant indice */
  char *pfind = NULL;		/* la fin de l'indice */
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
    {
      *pblanc = '_';
      *pfin = pfind;
    }
  else
    *pfin = pfinrad;
  return sy;
}				/* fin mi_lire_symbole */

// Etant donné une valeur, teste si c'est une expression propre, c.a.d. un
// symbole dont le type est expressif, renvoie alors le symbole
static Mit_Symbole *
mi_symbole_expressif (const Mit_Val v)
{
  Mit_Symbole *sy = mi_en_symbole (v);
  if (!sy)
    return NULL;
  Mit_Symbole *sytyp =
    mi_en_symbole (mi_symbole_attribut (sy, MI_PREDEFINI (type)));
  if (!sytyp)
    return NULL;
  if (mi_en_symbole (mi_symbole_attribut (sytyp, MI_PREDEFINI (expressif))) ==
      MI_PREDEFINI (vrai))
    return sy;
  return NULL;
}				/* fin mi_symbole_expressif */


// lire le complement d'un primaire, c.a.d. l'application d'une
// fonction ou l'indexation
static Mit_Val
mi_lire_complement (struct Mi_Lecteur_st *lec, Mit_Val v, char *ps,
                    char **pfin)
{
  assert (lec && lec->lec_nmagiq == MI_LECTEUR_NMAGIQ);
  assert (ps != NULL);
  assert (pfin != NULL);
  while (isspace (ps[0]))
    ps++;
  if (*ps == '(')
    {
      ps++;
      Mit_Symbole *syapp =	//
        (lec->lec_pascreer) ? NULL :
        mi_cloner_symbole (MI_PREDEFINI (application));
      if (syapp)
        {
          mi_symbole_mettre_attribut
          (syapp, MI_PREDEFINI (type),
           MI_SYMBOLEV (MI_PREDEFINI (application)));
          mi_symbole_mettre_attribut (syapp, MI_PREDEFINI (application), v);
        }
      int nbarg = 0;
      int tailargs = 0;		// taille allouée de tabargs
      const Mit_Symbole **tabargs = NULL;
      if (!lec->lec_pascreer)
        {
          tailargs = 5;
          tabargs = calloc (tailargs, sizeof (Mit_Symbole *));
          if (!tabargs)
            MI_FATALPRINTF ("impossible d'allouer tableau pour %d arguments",
                            tailargs);
        }
      for (;;)
        {
          while (isspace (*ps))
            ps++;
          if (*ps == ')' || !*ps)
            break;
          char *pdebarg = ps;
          char *pfinarg = NULL;
          Mit_Val varg = mi_lire_expression (lec, pdebarg, &pfinarg);
          if (!pfinarg || pfinarg == pdebarg)
            MI_ERREUR_LECTURE (lec, pdebarg, NULL, "argument manquant");
          if (!lec->lec_pascreer && tailargs >= nbarg)
            {
              int nouvtailargs = mi_nombre_premier_apres (5 * nbarg / 4 + 3);
              const Mit_Symbole **nouvtabargs =
                calloc (nouvtailargs, sizeof (Mit_Symbole *));
              if (!nouvtabargs)
                MI_FATALPRINTF
                ("impossible d'agrandir le tableau pour %d arguments",
                 nouvtailargs);
              if (nbarg > 0)
                memcpy (nouvtabargs, tabargs, nbarg * sizeof (Mit_Symbole *));
              free (tabargs);
              tabargs = nouvtabargs;
              tailargs = nouvtailargs;
            }
          nbarg++;
          Mit_Symbole *syarg =	//
            (lec->lec_pascreer) ? NULL :
            mi_cloner_symbole (MI_PREDEFINI (arg));
          if (syarg)
            {
              mi_symbole_mettre_attribut
              (syarg, MI_PREDEFINI (type),
               MI_SYMBOLEV (MI_PREDEFINI (arg)));
              mi_symbole_mettre_attribut (syarg, MI_PREDEFINI (arg), varg);
              if (syapp)
                mi_symbole_mettre_attribut
                (syarg, MI_PREDEFINI (dans), MI_SYMBOLEV (syapp));
              mi_symbole_mettre_attribut
              (syarg, MI_PREDEFINI (indice),
               MI_ENTIERV (mi_creer_entier (nbarg)));
              assert (tabargs != NULL && tailargs > nbarg);
              tabargs[nbarg] = syarg;
            }
        }
      if (*ps == ')') ps++;
      if (tabargs)
        {
          assert (syapp != NULL);
          const Mit_Tuple *tupargs = mi_creer_tuple_symboles (nbarg, tabargs);
          mi_symbole_mettre_attribut
          (syapp, MI_PREDEFINI (arg), MI_TUPLEV (tupargs));
        }
      *pfin = ps;
      v = MI_SYMBOLEV(syapp);
      return v;
    }
  else if (*ps == '[')
    {
      ps++;
      while (isspace (*ps))
        ps++;
      Mit_Symbole *syind =	//
        (lec->lec_pascreer) ? NULL :
        mi_cloner_symbole (MI_PREDEFINI (indice));
      char *pdebarg = ps;
      char *pfinarg = NULL;
      Mit_Val varg = mi_lire_expression (lec, pdebarg, &pfinarg);
      if (!pfinarg || pfinarg == pdebarg)
        MI_ERREUR_LECTURE (lec, pdebarg, NULL, "indice manquant");
      ps = pfinarg;
      while (isspace(*ps)) ps++;
      if (*ps != ']')
        MI_ERREUR_LECTURE (lec, ps, NULL, "crochet fermant dans indice");
      ps++;
      if (syind)
        {
          mi_symbole_mettre_attribut
          (syind, MI_PREDEFINI (type),
           MI_SYMBOLEV (MI_PREDEFINI (indice)));
          Mit_Symbole*sygch = mi_symbole_expressif(v);
          Mit_Symbole*sydrt = mi_symbole_expressif(varg);
          if (sygch)
            {
              mi_symbole_mettre_attribut (sygch,
                                          MI_PREDEFINI(dans), MI_SYMBOLEV(syind));
              mi_symbole_mettre_attribut (sygch,
                                          MI_PREDEFINI(indice), MI_SYMBOLEV(MI_PREDEFINI(gauche)));
            }
          if (sydrt)
            {
              mi_symbole_mettre_attribut (sydrt,
                                          MI_PREDEFINI(dans), MI_SYMBOLEV(syind));
              mi_symbole_mettre_attribut (sydrt,
                                          MI_PREDEFINI(indice), MI_SYMBOLEV(MI_PREDEFINI(droit)));
            }
          mi_symbole_mettre_attribut (syind, MI_PREDEFINI (gauche), v);
          mi_symbole_mettre_attribut (syind, MI_PREDEFINI (droit), varg);
        }
      *pfin = ps;
      v = MI_SYMBOLEV(syind);
      return v;
    }
  else
    {
      *pfin = ps;
      return v;
    }
}				// fin mi_lire_complement


// la chaine lue est temporairement modifiée, notamment pour les blancs soulignés
static Mit_Val
mi_lire_primaire (struct Mi_Lecteur_st *lec, char *ps, const char **pfin)
{
  assert (lec && lec->lec_nmagiq == MI_LECTEUR_NMAGIQ);
  assert (ps != NULL);
  while (isspace (ps[0]))
    ps++;
  // lire un nombre
  if (isdigit (ps[0])		//
      || ((ps[0] == '-' || ps[1] == '+')	//
          && (isdigit (ps[1])
              // les flottants peuvent être NAN ou INF, traités par strtod
              || !strncasecmp (ps + 1, "INF", 3)
              || !strncasecmp (ps + 1, "NAN", 3))))
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
      char *pfinsymb = NULL;
      Mit_Symbole *sy = mi_lire_symbole (pdebsymb, &pfinsymb);
      if (sy)
        {
          assert (pfinsymb != NULL && pfinsymb > pdebsymb);
          char *pc = pfinsymb;
          Mit_Val vc = mi_lire_complement (lec, MI_SYMBOLEV (sy), pc, &pc);
          if (pfin)
            *pfin = pc;
          if (lec->lec_pascreer)
            return MI_NILV;
          return vc;
        }
      else
        MI_ERREUR_LECTURE (lec, pdebsymb, pfinsymb,
                           mi_lecture_symbole_absent);
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
      char *pfintrou = NULL;
      ps++;
      Mit_Symbole *sy = mi_lire_symbole (ps, &pfintrou);
      if (sy)
        {
          struct Mi_trouve_st tr =	//
          mi_assoc_chercher (lec->lec_assoctrou, sy);
          Mit_Val vtrou = tr.t_pres ? (tr.t_val) : MI_SYMBOLEV (sy);
          char *pfincomp = NULL;
          Mit_Val vcomp =
            mi_lire_complement (lec, vtrou, pfintrou, &pfincomp);
          if (pfin)
            *pfin = pfincomp;
          if (lec->lec_pascreer)
            {
              if (!tr.t_pres)
                {
                  lec->lec_assoctrou =	//
                    mi_assoc_mettre (lec->lec_assoctrou,
                                     sy, MI_SYMBOLEV (MI_PREDEFINI (trou)));
                };
              return MI_NILV;
            }
          else
            {
              // La première passe aurait dû trouver le trou...
              if (!tr.t_pres)
                MI_FATALPRINTF ("le trou $%s est manquant", pdebtrou + 1);
              return vcomp;
            }
        }
      else
        MI_ERREUR_LECTURE (lec, pdebtrou + 1, pfintrou,
                           mi_lecture_symbole_absent);
    }
  else if (ps[0] == '(')
    {
      /* une expression entre parenthèses */
      char *pfinpar = NULL;
      Mit_Val v = mi_lire_expression (lec, ps + 1, &pfinpar);
      if (!pfinpar)
        MI_ERREUR_LECTURE (lec, ps + 1, NULL,
                           "parenthèse non suivie de valeur");
      while (isspace (*pfinpar))
        pfinpar++;
      if (*pfinpar != ')' && *pfinpar)
        MI_ERREUR_LECTURE (lec, ps, pfinpar,
                           "parenthèse fermante manquante");
      if (*pfinpar)
        pfinpar++;
      Mit_Symbole *sypar =	//
        (lec->lec_pascreer) ? NULL :
        mi_cloner_symbole (MI_PREDEFINI (parenthesage));
      if (sypar)
        {
          mi_symbole_mettre_attribut
          (sypar, MI_PREDEFINI (type),
           MI_SYMBOLEV (MI_PREDEFINI (parenthesage)));
          mi_symbole_mettre_attribut (sypar, MI_PREDEFINI (arg), v);
        }
      Mit_Val vpar = sypar ? MI_SYMBOLEV (sypar) : MI_NILV;
      char *fincomp = NULL;
      Mit_Val vcomp = mi_lire_complement (lec, vpar, pfinpar, &fincomp);
      if (pfin)
        *pfin = fincomp;
      if (lec->lec_pascreer)
        return MI_NILV;
      return vcomp;
    }
  else if (ps[0] == '-')
    {
      /* le moins unaire */
      char *debmoins = ps;
      const char *finmoins = NULL;
      ps++;
      while (*ps && isspace (*ps))
        ps++;
      Mit_Val negv = mi_lire_primaire (lec, ps, &finmoins);
      if (!finmoins || finmoins == ps)
        MI_ERREUR_LECTURE (lec, debmoins, NULL,
                           "erreur de lecture après moins unaire");
      Mit_Symbole *syopp =	//
        (lec->lec_pascreer) ? NULL :
        mi_cloner_symbole (MI_PREDEFINI (oppose));
      if (syopp)
        {
          mi_symbole_mettre_attribut
          (syopp, MI_PREDEFINI (type), MI_SYMBOLEV (MI_PREDEFINI (oppose)));
          mi_symbole_mettre_attribut (syopp, MI_PREDEFINI (arg), negv);
          return (MI_SYMBOLEV (syopp));
        }
      else
        return MI_NILV;
    }
  else if (!strncmp (ps, "\302\254", 2) /* UTF-8 U+00AC NOT SIGN ¬ */ )
    {
      /* la negation logique */
      char *debneg = ps;
      const char *finneg = NULL;
      ps += 2;
      while (*ps && isspace (*ps))
        ps++;
      Mit_Val negv = mi_lire_primaire (lec, ps, &finneg);
      if (!finneg || finneg == ps)
        MI_ERREUR_LECTURE (lec, debneg, NULL,
                           "erreur de lecture après non ¬");
      Mit_Symbole *syneg =	//
        (lec->lec_pascreer) ? NULL :
        mi_cloner_symbole (MI_PREDEFINI (negation));
      if (syneg)
        {
          mi_symbole_mettre_attribut
          (syneg, MI_PREDEFINI (type),
           MI_SYMBOLEV (MI_PREDEFINI (negation)));
          mi_symbole_mettre_attribut (syneg, MI_PREDEFINI (arg), negv);
          Mit_Symbole *syarg = mi_symbole_expressif (negv);
          if (syarg)
            {
              mi_symbole_mettre_attribut (syarg, MI_PREDEFINI (dans),
                                          MI_SYMBOLEV (syneg));
              mi_symbole_mettre_attribut (syarg, MI_PREDEFINI (indice),
                                          MI_SYMBOLEV (MI_PREDEFINI (arg)));
            }
          return (MI_SYMBOLEV (syneg));
        }
      else
        return MI_NILV;
    }
  else
    MI_ERREUR_LECTURE (lec, ps, NULL, "erreur de lecture d'un primaire");

}				/* fin mi_lire_primaire */



Mit_Val
mi_lire_terme (struct Mi_Lecteur_st *lec, char *ps, char **pfin)
{
  Mit_Val vterme = MI_NILV;
  assert (lec && lec->lec_nmagiq == MI_LECTEUR_NMAGIQ);
  assert (ps != NULL);
  assert (pfin != NULL);
  while (*ps && isspace (*ps))
    ps++;
  char *debsom = ps;
  const char *fingch = NULL;
  Mit_Val vgch = mi_lire_primaire (lec, ps, &fingch);
  if (!fingch)
    MI_ERREUR_LECTURE (lec, debsom, NULL,
                       "erreur de lecture du membre gauche d'un terme");
  vterme = vgch;
  ps = (char *) fingch;
  for (;;)
    {
      Mit_Val vdrt = MI_NILV;
      Mit_Symbole *sypro = NULL;
      const char *findrt = NULL;
      while (*ps && isspace (*ps))
        ps++;
      if (*ps != '/' && *ps != '*')
        {
          *pfin = ps;
          return vterme;
        }
      else if (*ps == '*')
        {
          vdrt = mi_lire_primaire (lec, ps + 1, &findrt);
          if (!findrt)
            MI_ERREUR_LECTURE (lec, ps, NULL,
                               "erreur de lecture du membre droit d'un produit");
          Mit_Symbole *sypro =	//
            (lec->lec_pascreer) ? NULL :
            mi_cloner_symbole (MI_PREDEFINI (produit));
          if (sypro)
            {
              mi_symbole_mettre_attribut
              (sypro, MI_PREDEFINI (type),
               MI_SYMBOLEV (MI_PREDEFINI (produit)));
              Mit_Symbole *sygch = mi_symbole_expressif (vterme);
              if (sygch)
                {
                  mi_symbole_mettre_attribut
                  (sygch, MI_PREDEFINI (dans), MI_SYMBOLEV (sypro));
                  mi_symbole_mettre_attribut
                  (sygch, MI_PREDEFINI (indice),
                   MI_SYMBOLEV (MI_PREDEFINI (gauche)));
                }
              Mit_Symbole *sydrt = mi_symbole_expressif (vdrt);
              if (sydrt)
                {
                  mi_symbole_mettre_attribut
                  (sydrt, MI_PREDEFINI (dans), MI_SYMBOLEV (sypro));
                  mi_symbole_mettre_attribut
                  (sydrt, MI_PREDEFINI (indice),
                   MI_SYMBOLEV (MI_PREDEFINI (droit)));
                }
              mi_symbole_mettre_attribut
              (sypro, MI_PREDEFINI (gauche), vterme);
              mi_symbole_mettre_attribut (sypro, MI_PREDEFINI (droit), vdrt);
              vterme = MI_SYMBOLEV (sypro);
            }
        }
      else if (*ps == '/')
        {
          vdrt = mi_lire_primaire (lec, ps + 1, &findrt);
          if (!findrt)
            MI_ERREUR_LECTURE (lec, ps, NULL,
                               "erreur de lecture du membre droit d'un quotient");
          Mit_Symbole *syquo =	//
            (lec->lec_pascreer) ? NULL :
            mi_cloner_symbole (MI_PREDEFINI (quotient));
          if (syquo)
            {
              mi_symbole_mettre_attribut
              (syquo, MI_PREDEFINI (type),
               MI_SYMBOLEV (MI_PREDEFINI (difference)));
              mi_symbole_mettre_attribut
              (syquo, MI_PREDEFINI (gauche), vterme);
              Mit_Symbole *sygch = mi_symbole_expressif (vterme);
              if (sygch)
                {
                  mi_symbole_mettre_attribut
                  (sygch, MI_PREDEFINI (dans), MI_SYMBOLEV (syquo));
                  mi_symbole_mettre_attribut
                  (sygch, MI_PREDEFINI (indice),
                   MI_SYMBOLEV (MI_PREDEFINI (gauche)));
                }
              Mit_Symbole *sydrt = mi_symbole_expressif (vdrt);
              if (sydrt)
                {
                  mi_symbole_mettre_attribut
                  (sydrt, MI_PREDEFINI (dans), MI_SYMBOLEV (syquo));
                  mi_symbole_mettre_attribut
                  (sydrt, MI_PREDEFINI (indice),
                   MI_SYMBOLEV (MI_PREDEFINI (droit)));
                }
              mi_symbole_mettre_attribut (syquo, MI_PREDEFINI (droit), vdrt);
              vterme = MI_SYMBOLEV (syquo);
            }
        }
      else // impossible
        MI_FATALPRINTF ("caractère[s] impossible[s] %s dans terme", ps);
    };
  return vterme;
}				/* fin mi_lire_terme */



Mit_Val
mi_lire_comparande (struct Mi_Lecteur_st *lec, char *ps, char **pfin)
{
  Mit_Val vsom = MI_NILV;
  assert (lec && lec->lec_nmagiq == MI_LECTEUR_NMAGIQ);
  assert (ps != NULL);
  assert (pfin != NULL);
  while (*ps && isspace (*ps))
    ps++;
  char *debsom = ps;
  char *fingch = NULL;
  Mit_Val vgch = mi_lire_terme (lec, ps, &fingch);
  if (!fingch)
    MI_ERREUR_LECTURE (lec, debsom, NULL,
                       "erreur de lecture du membre gauche d'un comparande");
  vsom = vgch;
  ps = (char *) fingch;
  for (;;)
    {
      Mit_Val vdrt = MI_NILV;
      Mit_Symbole *sysom = NULL;
      char *findrt = NULL;
      while (*ps && isspace (*ps))
        ps++;
      if (*ps != '+' && *ps != '-')
        {
          *pfin = ps;
          return vsom;
        }
      else if (*ps == '+')
        {
          vdrt = mi_lire_terme (lec, ps + 1, &findrt);
          if (!findrt)
            MI_ERREUR_LECTURE (lec, ps, NULL,
                               "erreur de lecture du membre droit d'un comparande");
          Mit_Symbole *sysom =	//
            (lec->lec_pascreer) ? NULL :
            mi_cloner_symbole (MI_PREDEFINI (somme));
          if (sysom)
            {
              Mit_Symbole *sygch = mi_symbole_expressif (vsom);
              if (sygch)
                {
                  mi_symbole_mettre_attribut
                  (sygch, MI_PREDEFINI (dans), MI_SYMBOLEV (sysom));
                  mi_symbole_mettre_attribut
                  (sygch, MI_PREDEFINI (indice),
                   MI_SYMBOLEV (MI_PREDEFINI (gauche)));
                }
              Mit_Symbole *sydrt = mi_symbole_expressif (vdrt);
              if (sydrt)
                {
                  mi_symbole_mettre_attribut
                  (sydrt, MI_PREDEFINI (dans), MI_SYMBOLEV (sysom));
                  mi_symbole_mettre_attribut
                  (sydrt, MI_PREDEFINI (indice),
                   MI_SYMBOLEV (MI_PREDEFINI (droit)));
                }
              mi_symbole_mettre_attribut
              (sysom, MI_PREDEFINI (type),
               MI_SYMBOLEV (MI_PREDEFINI (somme)));
              mi_symbole_mettre_attribut (sysom, MI_PREDEFINI (gauche), vsom);
              mi_symbole_mettre_attribut (sysom, MI_PREDEFINI (droit), vdrt);
              vsom = MI_SYMBOLEV (sysom);
            }
        }
      else if (*ps == '-')
        {
          vdrt = mi_lire_terme (lec, ps + 1, &findrt);
          if (!findrt)
            MI_ERREUR_LECTURE (lec, ps, NULL,
                               "erreur de lecture du membre droit d'une difference");
          Mit_Symbole *sydif =	//
            (lec->lec_pascreer) ? NULL :
            mi_cloner_symbole (MI_PREDEFINI (difference));
          if (sydif)
            {
              Mit_Symbole *sygch = mi_symbole_expressif (vsom);
              if (sygch)
                {
                  mi_symbole_mettre_attribut
                  (sygch, MI_PREDEFINI (dans), MI_SYMBOLEV (sydif));
                  mi_symbole_mettre_attribut
                  (sygch, MI_PREDEFINI (indice),
                   MI_SYMBOLEV (MI_PREDEFINI (gauche)));
                }
              Mit_Symbole *sydrt = mi_symbole_expressif (vdrt);
              if (sydrt)
                {
                  mi_symbole_mettre_attribut
                  (sydrt, MI_PREDEFINI (dans), MI_SYMBOLEV (sydif));
                  mi_symbole_mettre_attribut
                  (sydrt, MI_PREDEFINI (indice),
                   MI_SYMBOLEV (MI_PREDEFINI (droit)));
                }
              mi_symbole_mettre_attribut
              (sydif, MI_PREDEFINI (type),
               MI_SYMBOLEV (MI_PREDEFINI (difference)));
              mi_symbole_mettre_attribut (sydif, MI_PREDEFINI (gauche), vsom);
              mi_symbole_mettre_attribut (sydif, MI_PREDEFINI (droit), vdrt);
              vsom = MI_SYMBOLEV (sydif);
            }
        }
      else // impossible
        MI_FATALPRINTF ("caractère[s] impossible[s] %s dans comparande", ps);
    };
  return vsom;
}				/* fin mi_lire_comparande */


Mit_Val
mi_lire_comparaison (struct Mi_Lecteur_st *lec, char *ps, char **pfin)
{
  Mit_Val vsom = MI_NILV;
  assert (lec && lec->lec_nmagiq == MI_LECTEUR_NMAGIQ);
  assert (ps != NULL);
  assert (pfin != NULL);
  while (*ps && isspace (*ps))
    ps++;
  char *debsom = ps;
  char *fingch = NULL;
  Mit_Val vgch = mi_lire_comparande (lec, ps, &fingch);
  if (!fingch)
    MI_ERREUR_LECTURE (lec, debsom, NULL,
                       "erreur de lecture du membre gauche d'une comparaison");
  ps = (char *) fingch;
  while (*ps && isspace (*ps))
    ps++;
  Mit_Symbole*sycmp = NULL;
  if (*ps == '=')
    sycmp = MI_PREDEFINI(egal), ps++;
  else if (!strncmp(ps, "<=", 2))
    sycmp = MI_PREDEFINI(inferieurEgal), ps+=2;
  else if (!strncmp(ps, "\342\211\244", 3 /* U+2264 LESS-THAN OR EQUAL TO ≤ */))
    sycmp = MI_PREDEFINI(inferieurEgal), ps+=3;
  else if (!strncmp(ps, ">=", 2))
    sycmp = MI_PREDEFINI(superieurEgal), ps+=2;
  else if (!strncmp(ps, "\342\211\245", 3 /* U+2265 GREATER-THAN OR EQUAL TO ≥ */))
    sycmp = MI_PREDEFINI(superieurEgal), ps+=3;
  else if (!strncmp(ps, "\342\211\240", 3 /* U+2260 NOT EQUAL TO ≠ */))
    sycmp = MI_PREDEFINI(inegal), ps+=3;
  else if (!strncmp(ps, "!=", 2))
    sycmp = MI_PREDEFINI(inegal), ps+=2;
  else if (*ps == '<')
    sycmp = MI_PREDEFINI(inferieur), ps++;
  else if (*ps == '>')
    sycmp = MI_PREDEFINI(superieur), ps++;
  if (!sycmp) return vgch;
  char *findrt = NULL;
  Mit_Val vdrt = mi_lire_comparande (lec, ps, &findrt);
  if (!findrt)
    MI_ERREUR_LECTURE (lec, debsom, NULL,
                       "erreur de lecture du membre droit d'une comparaison");
  ps = (char *) findrt;
  Mit_Symbole *syrescomp =	//
    (lec->lec_pascreer) ? NULL :
    mi_cloner_symbole (sycmp);
  if (syrescomp)
    {
      Mit_Symbole *sygch = mi_symbole_expressif (vgch);
      if (sygch)
        {
          mi_symbole_mettre_attribut
          (sygch, MI_PREDEFINI (dans), MI_SYMBOLEV (syrescomp));
          mi_symbole_mettre_attribut
          (sygch, MI_PREDEFINI (indice),
           MI_SYMBOLEV (MI_PREDEFINI (gauche)));
        }
      Mit_Symbole *sydrt = mi_symbole_expressif (vdrt);
      if (sydrt)
        {
          mi_symbole_mettre_attribut
          (sydrt, MI_PREDEFINI (dans), MI_SYMBOLEV (syrescomp));
          mi_symbole_mettre_attribut
          (sydrt, MI_PREDEFINI (indice),
           MI_SYMBOLEV (MI_PREDEFINI (droit)));
        }
      mi_symbole_mettre_attribut
      (syrescomp, MI_PREDEFINI (type),
       MI_SYMBOLEV (sycmp));
      mi_symbole_mettre_attribut (syrescomp, MI_PREDEFINI (gauche), vsom);
      mi_symbole_mettre_attribut (syrescomp, MI_PREDEFINI (droit), vdrt);
      return MI_SYMBOLEV(syrescomp);
    }
  else
    return MI_NILV;
}

Mit_Val
mi_lire_expression (struct Mi_Lecteur_st *lec, char *ps, char **pfin)
{
  assert (lec && lec->lec_nmagiq == MI_LECTEUR_NMAGIQ);
  assert (ps != NULL);
  assert (pfin != NULL);
}				/* fin mi_lire_expression */
