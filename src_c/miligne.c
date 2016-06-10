// fichier miligne.c - ligne de commande
/* la notice de copyright est légalement en anglais */

// (C) 2016 Basile Starynkevitch
//   this file miligne.c is part of Minil
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



#define MI_LECTROUS_NMAGIQ 422688701	/*0x1931b7bd */
struct Mi_LecTrous_st
{
  unsigned mlt_nmagiq;		// toujours  MI_LECTROUS_NMAGIQ
  unsigned mlt_comptrou;
  struct Mi_Lecteur_st *mlt_lect;
  const Mit_Symbole **mlt_tabtrous;
};

bool
mi_ajouter_trou_lu (const Mit_Symbole *sy, const Mit_Val va
                    __attribute__ ((unused)), void *client)
{
  struct Mi_LecTrous_st *ltr = client;
  assert (ltr != NULL && ltr->mlt_nmagiq == MI_LECTROUS_NMAGIQ);
  assert (ltr->mlt_lect != NULL);
  assert (ltr->mlt_comptrou < mi_assoc_compte (ltr->mlt_lect->lec_assoctrou));
  ltr->mlt_tabtrous[ltr->mlt_comptrou++] = sy;
  return false;
}				/* fin mi_ajouter_trou_lu */

void
mi_lire_trous (struct Mi_Lecteur_st *lec, const char *invite)
{
  assert (lec && lec->lec_nmagiq == MI_LECTEUR_NMAGIQ);
  assert (invite != NULL);
  unsigned nbtrous = mi_assoc_compte (lec->lec_assoctrou);
  struct Mi_LecTrous_st lectrous;
  memset (&lectrous, 0, sizeof (lectrous));
  lectrous.mlt_nmagiq = MI_LECTROUS_NMAGIQ;
  lectrous.mlt_comptrou = 0;
  lectrous.mlt_lect = lec;
  lectrous.mlt_tabtrous = calloc (nbtrous + 1, sizeof (Mit_Symbole *));
  if (!lectrous.mlt_tabtrous)
    MI_FATALPRINTF ("impossible d'allouer table pour %d trous", nbtrous);
  mi_assoc_iterer (lec->lec_assoctrou, mi_ajouter_trou_lu, &lectrous);
  assert (lectrous.mlt_comptrou == nbtrous);
  qsort (lectrous.mlt_tabtrous, nbtrous, sizeof (Mit_Symbole *),
         mi_cmp_symboleptr);
  printf ("remplissez les %d trous pour %s\n", nbtrous, invite);
  char invitrou[40];
  jmp_buf jborig;
  memset (invitrou, 0, sizeof (invitrou));
  memcpy (&jborig, &lec->lec_jb, sizeof (jmp_buf));
  for (unsigned ixt = 0; ixt < nbtrous; ixt++)
    {
      const Mit_Symbole *sytrou = lectrous.mlt_tabtrous[ixt];
      assert (sytrou && sytrou->mi_type == MiTy_Symbole);
      char comptrou[16];
      memset (comptrou, 0, sizeof (comptrou));
      snprintf (invitrou, sizeof (invitrou), "%s %s$%s%s?%s ", invite,
                MI_TERMINAL_GRAS, mi_symbole_chaine (sytrou),
                mi_symbole_indice_ch (comptrou, sytrou), MI_TERMINAL_NORMAL);
      Mit_Val vtrou = MI_NILV;
      for (;;)
        {
          char *lintrou = readline (invitrou);
          MI_DEBOPRINTF ("trou %s%s: lintrou:%s",
                         mi_symbole_chaine (sytrou),
                         mi_symbole_indice_ch (comptrou, sytrou), lintrou);
          if (!lintrou)
            MI_FATALPRINTF ("lecture de trou %s%s échoue",
                            mi_symbole_chaine (sytrou),
                            mi_symbole_indice_ch (comptrou, sytrou));
          int errt = setjmp (lectrous.mlt_lect->lec_jb);
          char *fintrou = NULL;
          if (!errt)
            {
              vtrou =
                mi_lire_expression (lectrous.mlt_lect, lintrou, &fintrou);
              if (vtrou.miva_ptr != NULL && fintrou != NULL)
                {
                  lectrous.mlt_lect->lec_assoctrou =
                    mi_assoc_mettre (lectrous.mlt_lect->lec_assoctrou, sytrou,
                                     vtrou);
                  free (lintrou), lintrou = NULL;
                  break;
                }
            }
          else
            {
              printf
              ("%serreur de lecture%s du trou#%d %s%s%s%s (pos#%zd): %s\n",
               MI_TERMINAL_GRAS, MI_TERMINAL_NORMAL, ixt + 1,
               MI_TERMINAL_ITALIQUE, mi_symbole_chaine (sytrou),
               mi_symbole_indice_ch (comptrou, sytrou), MI_TERMINAL_NORMAL,
               lectrous.mlt_lect->lec_errptr - lintrou,
               lectrous.mlt_lect->lec_errmsg);
              free (lintrou), lintrou = NULL;
            }
        }
    }
  memcpy (&lec->lec_jb, &jborig, sizeof (jmp_buf));
  printf ("fin du remplissage des %d trous pour %s\n\n", nbtrous, invite);
}				/* fin mi_lire_trous */





// générateur de complétion pour une tige alphabétique; la chaîne
// renvoyée doit être dynamiquement allouée et sera libérée par
// readline.
char *
mi_generer_completion_alpha (const char *tige, int etat)
{
  static struct MiSt_Radical_st **vecrad;
  static int taillevec;
  static int nbcompl;
  MI_DEBOPRINTF ("tige='%s'@%p etat=%d vecrad@%p nbcompl=%d taillevec=%d",
                 tige, tige, etat, vecrad, nbcompl, taillevec);
  if (etat == 0)
    {
      int lgtige = strlen (tige);
      nbcompl = 0;
      // etat initial, il faut allouer et remplir vecrad
      // & nbcompl & taillevec;
      taillevec = 5;
      vecrad = calloc (taillevec, sizeof (struct MiSt_Radical_st *));
      if (!vecrad)
        MI_FATALPRINTF ("impossible d'allouer %d completions (%s)",
                        taillevec, strerror (errno));
      const char *nomrad = NULL;
      for (struct MiSt_Radical_st * rad =
             mi_trouver_radical_apres_ou_egal (tige);
           rad != NULL
           && (nomrad =
                 mi_val_chaine (MI_CHAINEV (mi_radical_nom (rad)))) != NULL;
           rad = mi_trouver_radical_apres (nomrad))
        {
          MI_DEBOPRINTF ("nbcompl=%d taillevec=%d nomrad:%s",
                         nbcompl, taillevec, nomrad);
          if (nbcompl + 2 >= taillevec)
            {
              unsigned nouvtaille =
                mi_nombre_premier_apres (5 * nbcompl / 4 + 11);
              struct MiSt_Radical_st **nouvecrad =
              calloc (nouvtaille, sizeof (char *));
              if (!nouvecrad)
                MI_FATALPRINTF
                ("impossible d'agrandir la completion de taille %d",
                 nouvtaille);
              memcpy (nouvecrad, vecrad,
                      nbcompl * sizeof (struct MiSt_Radical_st *));
              free (vecrad);
              vecrad = nouvecrad;
              taillevec = nouvtaille;
            }
          if (!strncmp (nomrad, tige, lgtige))
            {
              assert (nbcompl >= 0 && nbcompl < taillevec);
              vecrad[nbcompl] = rad;
              MI_DEBOPRINTF ("vecrad[%d]@%p nomrad='%s'", nbcompl,
                             vecrad[nbcompl], nomrad);
              nbcompl++;
            }
          else
            break;
        };
    }				// fin etat == 0
  if (etat < nbcompl)
    {
      // etat non final, il faut renvoyer une chaîne allouée dans le tas
      char *res =
        strdup (mi_val_chaine (MI_CHAINEV (mi_radical_nom (vecrad[etat]))));
      MI_DEBOPRINTF ("res='%s'@%p etat=%d nbcompl=%d", res, res, etat,
                     nbcompl);
      return res;
    }
  else
    {
      // etat final, il faut libérer
      free (vecrad), vecrad = NULL;
      taillevec = 0;
      MI_DEBOPRINTF ("final nbcompl=%d etat=%d", nbcompl, etat);
      nbcompl = -1;
      return NULL;
    }
}				/* fin mi_generer_completion_alpha */



struct mi_raccourci_delim_st
{
  const char *ra_delim;
  const char *ra_equiv;
};
const struct mi_raccourci_delim_st mi_tabracdelim[] =
{
  {"<=", "\342\211\244" /* U+2264 LESS-THAN OR EQUAL TO ≤ */ },
  {">=", "\342\211\245" /* U+2265 GREATER-THAN OR EQUAL TO ≥ */ },
  {"!=", "\342\211\245" /* U+2265 GREATER-THAN OR EQUAL TO ≥ */ },
  {"!-", "\302\254" /* UTF-8 U+00AC NOT SIGN ¬ */ },
  {"||", "\342\210\250" /*U+2228 LOGICAL OR ∨ */ },
  {"&&", "\342\210\247" /*U+2227 LOGICAL AND ∧ */ },
  {NULL, NULL}
};

/// fonction appellée par readline via rl_attempted_completion_function
static char **
mi_completion_attendue (const char *tige, int deb, int fin)
{
  // La tige pourrait être le mot à compléter, mais il vaut mieux
  // travailler sur la ligne courante avec le point
  // courant... notamment pour pouvoir compléter des opérateurs comme
  // <= ou un nom précédé d'un opérateur...
  //
  // deb et fin et rl_point sont des indices d'octets (pas des
  // positions de glyphes UTF8)
  if (mi_deboguage)		// faut forcer le saut à la ligne
    {
      fputc ('\n', stderr);
      MI_DEBOPRINTF
      ("tige='%s'@%p, deb=%d fin=%d rlbuf='%s'@%p(l%d, ul%d) rlpoint#%d rlend#%d",
       tige, tige, deb, fin, rl_line_buffer, rl_line_buffer,
       (int) strlen (rl_line_buffer),
       (int) u8_strlen ((const uint8_t *) rl_line_buffer), rl_point,
       rl_end);
    }
  // Le cas simple est quand la tige contient un mot (lexicalement
  // parlant) mais il faudrait aussi traiter les cas plus compliqués en
  // analysant la ligne rl_line_buffer et y reculant depuis rl_point en UTF8
  bool tigealpha = deb <= rl_point && fin > deb && fin <= rl_end
                   && fin <= rl_point;
  MI_DEBOPRINTF ("tigealpha initial %s", tigealpha ? "vrai" : "faux");
  for (int i = deb; i < fin && tigealpha; i++)
    {
      tigealpha = rl_line_buffer[i] == tige[i - deb]
                  && isalpha (rl_line_buffer[i]);
      // MI_DEBOPRINTF("tigealpha en i#%d %s", i, tigealpha?"vrai":"faux");
    }
  MI_DEBOPRINTF ("tigealpha final %s", tigealpha ? "vrai" : "faux");
  if (tigealpha)
    {
      MI_DEBOPRINTF ("avant rl_completion_matches completion_alpha tige='%s'",
                     tige);
      char **res = rl_completion_matches (tige, mi_generer_completion_alpha);
      MI_DEBOPRINTF ("res@%p", res);
      return res;
    }
  if (rl_point >= 2 && rl_point <= rl_end
      && ispunct (rl_line_buffer[rl_point - 1])
      && ispunct (rl_line_buffer[rl_point - 2])
      && (rl_point == 2 || !ispunct (rl_line_buffer[rl_point - 3])))
    {
      const char *d = rl_line_buffer + rl_point - 2;
      const char *rempl = NULL;
      /// il faut modifier rl_line_buffer
      for (const struct mi_raccourci_delim_st * rd = mi_tabracdelim;
           rempl == NULL && rd->ra_delim != NULL; rd++)
        {
          if (strlen (rd->ra_delim) == 2 && !strncmp (rd->ra_delim, d, 2))
            rempl = rd->ra_equiv;
        }
      if (rempl)
        {
          int vieuxpoint = rl_point;
          MI_DEBOPRINTF ("d='%c%c' rempl='%s'", d[0], d[1], rempl);
          int nbc = rl_delete_text(rl_point-2,rl_point);
          if (nbc != 2)
            MI_FATALPRINTF("echec rl_delete_text vieuxpoint=%d nbc=%d point=%d", vieuxpoint, nbc, rl_point);
          rl_point -= 2;
          rl_insert_text(rempl);
          // peut-être appeler rl_redisplay ici?
        }
      else
        {
          MI_DEBOPRINTF ("d='%c%c' sans rempl", d[0], d[1]);
        }
      MI_DEBOPRINTF ("fin compl.delim. point=%d ligne='%s'", rl_point, rl_line_buffer);
      return NULL;
    }
  MI_DEBOPRINTF ("échec tigealpha %s", tigealpha ? "vrai" : "faux");
  return NULL;
}				// fin mi_completion_attendue



////////////////
void
mi_lire_expressions_en_boucle (void)
{
  using_history ();
  rl_attempted_completion_function = mi_completion_attendue;
  // une liste de caractères qui pourrait précéder un mot à compléter
  rl_basic_word_break_characters = " \t\n\"\\'`@$><=;|&{(+-*/~#[^@?.!";
  // une liste de caractères à prefixer un mo"
  rl_special_prefixes = "$";
  rl_set_paren_blink_timeout(250000); // 250 millisecondes
  rl_variable_bind ("blink-matching-paren", "on");
  printf ("Entrez des expressions en boucle,"
          " et une ligne vide pour terminer.\n"
          "\t (utilise libreadline %s)\n\n", rl_library_version);
  int cnt = 0;
  int numexpcorr = 0;		/* le numéro de la dernière expression correcte */
  for (;;)
    {
      int err1 = 0;
      int err2 = 0;
      cnt++;
      char promptx[32];
      snprintf (promptx, sizeof (promptx), "%sEXP#%d:%s ",
                MI_TERMINAL_PALE, cnt, MI_TERMINAL_NORMAL);
      char *lin = readline (promptx);
      if (!lin || !lin[0])
        break;
      fflush (NULL);
      MI_DEBOPRINTF ("lin#%d=%s numexpcorr=%d wherehist=%d",
                     cnt, lin, numexpcorr, where_history ());
      volatile bool repeterlect = false;
      struct Mi_Lecteur_st lecteur;
      do
        {
          memset (&lecteur, 0, sizeof (lecteur));
          repeterlect = false;
          lecteur.lec_nmagiq = MI_LECTEUR_NMAGIQ;
          lecteur.lec_pascreer = true;
          err1 = setjmp (lecteur.lec_jb);
          MI_DEBOPRINTF ("cnt#%d err1#%d", cnt, err1);
          if (!err1)
            {
              char *finexp = NULL;
              MI_DEBOPRINTF ("cnt#%d lirexp lin'%s'", cnt, lin);
              Mit_Val vexp = mi_lire_expression (&lecteur, lin, &finexp);
              char posfinx[16];
              assert (vexp.miva_ptr == NULL);	// première passe
              memset (posfinx, 0, sizeof (posfinx));
              if (finexp)
                {
                  while (isspace (*finexp))
                    finexp++;
                  snprintf (posfinx, sizeof (posfinx), "%d",
                            (int) (finexp - lin));
                };
              MI_DEBOPRINTF ("première passe finexp=%s=%s%s", finexp,
                             finexp ? "lin+" : "*rien*", posfinx);
              numexpcorr = cnt;
            }
          else			//err1>0
            {
              assert (lecteur.lec_errmsg != NULL);
              assert (lecteur.lec_errptr != NULL);
              if (lecteur.lec_errfin == NULL)
                printf ("%sERREUR%s /%d (pos#%d): %s%s%s\n",
                        MI_TERMINAL_GRAS, MI_TERMINAL_NORMAL,
                        err1,
                        (int) (lecteur.lec_errptr - lin),
                        MI_TERMINAL_ITALIQUE, lecteur.lec_errmsg,
                        MI_TERMINAL_NORMAL);
              else if (lecteur.lec_errmsg == mi_lecture_symbole_absent)
                {
                  int lgsym = (int) (lecteur.lec_errfin - lecteur.lec_errptr);
                  char *nouvnom = malloc (lgsym + 1);
                  if (!nouvnom)
                    MI_FATALPRINTF
                    ("impossible d'allouer mémoire pour un nouveau nom (%d c) - %s",
                     lgsym, strerror (errno));
                  memset (nouvnom, 0, lgsym + 1);
                  strncpy (nouvnom, lecteur.lec_errptr, lgsym);
                  printf ("%sERREUR%s /%d (pos#%zd-%zd): %s%s%s '%s'\n",
                          MI_TERMINAL_GRAS, MI_TERMINAL_NORMAL,
                          err1,
                          (lecteur.lec_errptr - lin),
                          (lecteur.lec_errfin - lin),
                          MI_TERMINAL_ITALIQUE, lecteur.lec_errmsg,
                          MI_TERMINAL_NORMAL, nouvnom);
                  if (mi_nom_licite_chaine (nouvnom))
                    {
                      printf
                      ("Entrez un commentaire pour créer le symbole %s%s%s, ou rien pour recommencer...\n",
                       MI_TERMINAL_ITALIQUE, nouvnom, MI_TERMINAL_NORMAL);
                      char invitcom[24];
                      memset (invitcom, 0, sizeof (invitcom));
                      if (lgsym < (int) sizeof (invitcom) - 8)
                        snprintf (invitcom, sizeof (invitcom), "%s? ",
                                  nouvnom);
                      else
                        snprintf (invitcom, sizeof (invitcom), "%*s...? ",
                                  (int) sizeof (invitcom) - 8, nouvnom);
                      char *licom = readline (invitcom);
                      if (licom && licom[0])
                        {
                          Mit_Symbole *nouvsymb =
                            mi_creer_symbole_chaine (nouvnom, 0);
                          if (nouvsymb)
                            {
                              nouvsymb->mi_attrs =
                                mi_assoc_mettre (nouvsymb->mi_attrs,
                                                 MI_PREDEFINI (commentaire),
                                                 MI_CHAINEV (mi_creer_chaine
                                                             (licom)));
                              repeterlect = true;
                              printf ("Nouveau symbole %s%s%s créé!\n",
                                      MI_TERMINAL_GRAS, nouvnom,
                                      MI_TERMINAL_NORMAL);
                            }
                        }
                      free (nouvnom), nouvnom = NULL;
                    }
                }
              else
                printf ("%sERREUR%s /%d (pos#%zd-%zd): %s%s%s\n",
                        MI_TERMINAL_GRAS, MI_TERMINAL_NORMAL,
                        err1,
                        (lecteur.lec_errptr - lin),
                        (lecteur.lec_errfin - lin),
                        MI_TERMINAL_ITALIQUE, lecteur.lec_errmsg,
                        MI_TERMINAL_NORMAL);
              fflush (NULL);
              add_history (lin);
              MI_DEBOPRINTF ("err1=%d cnt=%d wherehist=%d lin='%s'", err1,
                             cnt, where_history (), lin);
            }
        }
      while (repeterlect);
      if (err1 > 0)
        continue;
      unsigned nbtrous = mi_assoc_compte (lecteur.lec_assoctrou);
      lecteur.lec_pascreer = false;
      if (nbtrous > 0)
        {
          char promptrou[24];
          snprintf (promptrou, sizeof (promptrou), "Exp#%d", cnt);
          mi_lire_trous (&lecteur, promptrou);
        }
      lecteur.lec_pascreer = false;
      err2 = setjmp (lecteur.lec_jb);
      MI_DEBOPRINTF ("cnt#%d err2#%d", cnt, err2);
      if (!err2)
        {
          char *finexp = NULL;
          Mit_Val vexp = mi_lire_expression (&lecteur, lin, &finexp);
          add_history (lin);
          MI_DEBOPRINTF ("ok cnt=%d wherehist=%d lin='%s'", cnt,
                         where_history (), lin);
          printf ("expression lue #%d:\n", cnt);
          if (mi_vtype (vexp) == MiTy_Symbole)
            mi_afficher_contenu_symbole (stdout, mi_en_symbole (vexp));
          else
            mi_afficher_valeur (stdout, vexp);
          putchar ('\n');
          fflush (NULL);
        }
      else
        {
          assert (lecteur.lec_errmsg != NULL);
          assert (lecteur.lec_errptr != NULL);
          if (lecteur.lec_errfin == NULL)
            printf ("%sERREUR%s /%d (pos#%d): %s%s%s\n",
                    MI_TERMINAL_GRAS, MI_TERMINAL_NORMAL,
                    err1,
                    (int) (lecteur.lec_errptr - lin),
                    MI_TERMINAL_ITALIQUE, lecteur.lec_errmsg,
                    MI_TERMINAL_NORMAL);
          else
            printf ("%sERREUR%s /%d (pos#%zd-%zd): %s%s%s\n",
                    MI_TERMINAL_GRAS, MI_TERMINAL_NORMAL,
                    err1,
                    (lecteur.lec_errptr - lin),
                    (lecteur.lec_errfin - lin),
                    MI_TERMINAL_ITALIQUE, lecteur.lec_errmsg,
                    MI_TERMINAL_NORMAL);
          fflush (NULL);
        }
      MI_DEBOPRINTF ("fin lin#%d=%s", cnt, lin);
    }
  printf ("\n fin de lecture de %d expressions en boucle\n", cnt);
}				/* fin mi_lire_expressions_en_boucle */
