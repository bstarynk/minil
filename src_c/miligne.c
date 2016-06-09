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



#define MI_LECTROUS_NMAGIQ 422688701 /*0x1931b7bd*/
struct Mi_LecTrous_st
{
  unsigned mlt_nmagiq; // toujours  MI_LECTROUS_NMAGIQ
  unsigned mlt_comptrou;
  struct Mi_Lecteur_st*mlt_lect;
  const Mit_Symbole**mlt_tabtrous;
};

bool mi_ajouter_trou_lu(const Mit_Symbole*sy, const Mit_Val va __attribute__((unused)), void*client)
{
  struct Mi_LecTrous_st*ltr = client;
  assert (ltr != NULL && ltr->mlt_nmagiq == MI_LECTROUS_NMAGIQ);
  assert (ltr->mlt_lect != NULL);
  assert (ltr->mlt_comptrou < mi_assoc_compte(ltr->mlt_lect->lec_assoctrou));
  ltr->mlt_tabtrous[ltr->mlt_comptrou++] = sy;
  return false;
} /* fin mi_ajouter_trou_lu */

void mi_lire_trous(struct Mi_Lecteur_st*lec, const char*invite)
{
  assert (lec && lec->lec_nmagiq == MI_LECTEUR_NMAGIQ);
  assert (invite!=NULL);
  unsigned nbtrous = mi_assoc_compte(lec->lec_assoctrou);
  struct Mi_LecTrous_st lectrous;
  memset (&lectrous, 0, sizeof(lectrous));
  lectrous.mlt_nmagiq = MI_LECTROUS_NMAGIQ;
  lectrous.mlt_comptrou = 0;
  lectrous.mlt_lect = lec;
  lectrous.mlt_tabtrous = calloc (nbtrous+1, sizeof(Mit_Symbole*));
  if (!lectrous.mlt_tabtrous)
    MI_FATALPRINTF("impossible d'allouer table pour %d trous", nbtrous);
  mi_assoc_iterer(lec->lec_assoctrou, mi_ajouter_trou_lu, &lectrous);
  assert (lectrous.mlt_comptrou == nbtrous);
  qsort (lectrous.mlt_tabtrous, nbtrous, sizeof (Mit_Symbole *), mi_cmp_symboleptr);
  printf("remplissez les %d trous pour %s\n", nbtrous, invite);
  char invitrou[40];
  jmp_buf jborig;
  memset (invitrou, 0, sizeof(invitrou));
  memcpy (&jborig, &lec->lec_jb, sizeof(jmp_buf));
  for (unsigned ixt=0; ixt<nbtrous; ixt++)
    {
      const Mit_Symbole*sytrou = lectrous.mlt_tabtrous[ixt];
      assert (sytrou && sytrou->mi_type == MiTy_Symbole);
      char comptrou[16];
      memset (comptrou, 0, sizeof(comptrou));
      snprintf(invitrou, sizeof(invitrou), "%s %s$%s%s?%s ", invite,
               MI_TERMINAL_GRAS, mi_symbole_chaine(sytrou), mi_symbole_indice_ch(comptrou, sytrou),
               MI_TERMINAL_NORMAL);
      Mit_Val vtrou = MI_NILV;
      for (;;)
        {
          char* lintrou = readline(invitrou);
          MI_DEBOPRINTF("trou %s%s: lintrou:%s",
                        mi_symbole_chaine(sytrou), mi_symbole_indice_ch(comptrou, sytrou), lintrou);
          if (!lintrou)
            MI_FATALPRINTF ("lecture de trou %s%s échoue",
                            mi_symbole_chaine(sytrou), mi_symbole_indice_ch(comptrou, sytrou));
          int errt = setjmp(lectrous.mlt_lect->lec_jb);
          char* fintrou = NULL;
          if (!errt)
            {
              vtrou = mi_lire_expression(lectrous.mlt_lect, lintrou, &fintrou);
              if (vtrou.miva_ptr != NULL && fintrou != NULL)
                {
                  lectrous.mlt_lect->lec_assoctrou = mi_assoc_mettre(lectrous.mlt_lect->lec_assoctrou, sytrou, vtrou);
                  free (lintrou), lintrou = NULL;
                  break;
                }
            }
          else
            {
              printf("%serreur de lecture%s du trou#%d %s%s%s%s (pos#%zd): %s\n",
                     MI_TERMINAL_GRAS, MI_TERMINAL_NORMAL,
                     ixt+1, MI_TERMINAL_ITALIQUE, mi_symbole_chaine(sytrou), mi_symbole_indice_ch(comptrou, sytrou), MI_TERMINAL_NORMAL,
                     lectrous.mlt_lect->lec_errptr- lintrou, lectrous.mlt_lect->lec_errmsg);
              free (lintrou), lintrou = NULL;
            }
        }
    }
  memcpy(&lec->lec_jb , &jborig, sizeof(jmp_buf));
  printf("fin du remplissage des %d trous pour %s\n\n", nbtrous, invite);
} /* fin mi_lire_trous */


// Pour readline.  Doit renvoyer un tableau dynamiquement alloué de
// chaînes dynamiquement alloué, ou bien NULL en échec.
char** mi_tenter_completion(const char*txt, int debc, int finc)
{
  MI_DEBOPRINTF("txt='%s' debc=%d finc=%d mot='%*s' ligne=%s",
                txt, debc, finc, (finc>debc)?(finc-debc):0, txt, rl_line_buffer);
  char*mot = NULL;
  if (finc>debc)
    if (asprintf(&mot, "%*s", finc-debc, txt)<0)
      MI_FATALPRINTF("impossible de dupliquer le mot '%*s'",(finc-debc), txt);
  MI_DEBOPRINTF("mot='%s'", mot);
  unsigned lgmot = mot?strlen(mot):0;
  unsigned taille = 5;
  unsigned nbcompl = 0;
  char**vec = calloc(taille, sizeof(char*));
  if (!vec)
    MI_FATALPRINTF("impossible d'allouer la completion de taille %d",
                   taille);
  const char*nomrad = NULL;
  for (struct MiSt_Radical_st *rad = mi_trouver_radical_apres_ou_egal(mot);
       rad != NULL
       && (nomrad = mi_val_chaine(MI_CHAINEV(mi_radical_nom(rad))))
       != NULL;
       rad = mi_trouver_radical_apres(nomrad))
    {
      MI_DEBOPRINTF("nbcompl=%d taille=%d nomrad:%s",
                    nbcompl, taille, nomrad);
      if (nbcompl + 2 >= taille)
        {
          unsigned nouvtaille = mi_nombre_premier_apres(5*nbcompl/4+11);
          char**nouvec = calloc(nouvtaille, sizeof(char*));
          if (!nouvec)
            MI_FATALPRINTF("impossible d'agrandir la completion de taille %d",
                           nouvtaille);
          memcpy (nouvec, vec, nbcompl*sizeof(char*));
          free (vec);
          vec = nouvec;
          taille = nouvtaille;
        }
      if (!strncmp(nomrad, mot, lgmot))
        {
          char*dupnomrad = strdup(nomrad);
          if (!dupnomrad)
            MI_FATALPRINTF("impossible de dupliquer '%s' (%s)",
                           nomrad, strerror(errno));
          vec[nbcompl] = dupnomrad;
          MI_DEBOPRINTF("vec[%d]=%s", nbcompl, vec[nbcompl]);
          nbcompl++;
        }
      else
        break;
    }
  if (!nbcompl)
    free (vec), vec= NULL;
  MI_DEBOPRINTF("nbcompl=%d vec@%p", nbcompl, vec);
#ifndef NDEBUG
  if (mi_deboguage)
    for (unsigned i=0; i<nbcompl; i++)
      MI_DEBOPRINTF("vec[%d]='%s'", i, vec[i]);
#endif
  // indiquer à readline qu'on a fini s'il y a un seul élément
  rl_attempted_completion_over = nbcompl <= 1;
  return vec;
} /* fin  mi_tenter_completion */


char*
mi_generer_completion (const char*texte, int etat)
{
  static const char*debmot;
  static const char*finmot;
  static char**vecomp;
  MI_DEBOPRINTF("texte='%s' etat=%d ligne=%s", texte, etat, rl_line_buffer);
  if (etat == 0)
    {
      debmot = texte;
      while (isspace(*debmot))
        debmot++;
      finmot = debmot;
      while (isalpha(*finmot))
        finmot++;
      MI_DEBOPRINTF("debmot='%s' finmot-debmot=%d",
                    debmot, (int)(finmot-debmot));
      vecomp =  mi_tenter_completion(texte, debmot-texte, finmot-texte);
      MI_DEBOPRINTF("vecomp@%p", vecomp);
      return vecomp?vecomp[0]:NULL;
    }
  else
    {
      assert (vecomp != NULL);
      if (vecomp[etat] != NULL && etat>0)
        {
          MI_DEBOPRINTF("etat=%d vecomp[%d]='%s'", etat, etat, vecomp[etat]);
          return vecomp[etat];
        }
      else
        {
          MI_DEBOPRINTF("etat=%d final", etat);
          vecomp=NULL;
          debmot=finmot=NULL;
          return NULL;
        }
    }
} /* fin mi_generer_completion */

char**mi_completion (const char*texte, int deb, int fin)
{
  char**res = NULL;
  MI_DEBOPRINTF("texte='%s', deb=%d, fin=%d ligne'%s'",
                texte, deb, fin, rl_line_buffer);
  bool motvalide = true;
  for (int i=deb; i<fin && motvalide; i++)
    if (!isalpha(texte[i])) motvalide = false;
  MI_DEBOPRINTF("motvalide %s", motvalide?"vrai":"faux");
  res = rl_completion_matches(texte, mi_generer_completion);
  MI_DEBOPRINTF("res@%p", res);
  return res;
} /* fin mi_completion */

void mi_lire_expressions_en_boucle(void)
{
  using_history();
  // rl_attempted_completion_function = mi_completion;
  // rl_attempted_completion_function = mi_tenter_completion;
  rl_completion_entry_function = mi_generer_completion;
  printf("Entrez des expressions en boucle, et une ligne vide pour terminer...\n");
  int cnt=0;
  int numexpcorr=0;		/* le numéro de la dernière expression correcte */
  for (;;)
    {
      int err1 = 0;
      int err2 = 0;
      cnt++;
      char promptx[32];
      snprintf(promptx, sizeof(promptx), "%sEXP#%d:%s ",
               MI_TERMINAL_PALE, cnt, MI_TERMINAL_NORMAL);
      char*lin = readline(promptx);
      if (!lin || !lin[0]) break;
      fflush(NULL);
      MI_DEBOPRINTF("lin#%d=%s numexpcorr=%d wherehist=%d",
                    cnt, lin, numexpcorr, where_history());
      volatile bool repeterlect = false;
      struct Mi_Lecteur_st lecteur;
      memset(&lecteur, 0, sizeof(lecteur));
      do
        {
          memset(&lecteur, 0, sizeof(lecteur));
          repeterlect = false;
          lecteur.lec_nmagiq = MI_LECTEUR_NMAGIQ;
          lecteur.lec_pascreer = true;
          err1 = setjmp(lecteur.lec_jb);
          MI_DEBOPRINTF("cnt#%d err1#%d", cnt, err1);
          if (!err1)
            {
              char*finexp = NULL;
              MI_DEBOPRINTF("cnt#%d lirexp lin'%s'", cnt, lin);
              Mit_Val vexp = mi_lire_expression(&lecteur,lin,&finexp);
              char posfinx[16];
              assert (vexp.miva_ptr == NULL);// première passe
              memset (posfinx, 0, sizeof(posfinx));
              if (finexp)
                {
                  while (isspace(*finexp)) finexp++;
                  snprintf(posfinx, sizeof(posfinx), "%d", (int)(finexp-lin));
                };
              MI_DEBOPRINTF("première passe finexp=%s=%s%s", finexp, finexp?"lin+":"*rien*", posfinx);
              numexpcorr = cnt;
            }
          else   //err1>0
            {
              assert (lecteur.lec_errmsg != NULL);
              assert (lecteur.lec_errptr != NULL);
              if (lecteur.lec_errfin == NULL)
                printf("%sERREUR%s /%d (pos#%d): %s%s%s\n",
                       MI_TERMINAL_GRAS, MI_TERMINAL_NORMAL,
                       err1,
                       (int)(lecteur.lec_errptr - lin),
                       MI_TERMINAL_ITALIQUE, lecteur.lec_errmsg, MI_TERMINAL_NORMAL);
              else if (lecteur.lec_errmsg == mi_lecture_symbole_absent)
                {
                  int lgsym = (int)(lecteur.lec_errfin - lecteur.lec_errptr);
                  char* nouvnom = malloc(lgsym+1);
                  if (!nouvnom)
                    MI_FATALPRINTF("impossible d'allouer mémoire pour un nouveau nom (%d c) - %s",
                                   lgsym, strerror(errno));
                  memset(nouvnom, 0, lgsym+1);
                  strncpy(nouvnom,  lecteur.lec_errptr, lgsym);
                  printf("%sERREUR%s /%d (pos#%zd-%zd): %s%s%s '%s'\n",
                         MI_TERMINAL_GRAS, MI_TERMINAL_NORMAL,
                         err1,
                         (lecteur.lec_errptr - lin),
                         (lecteur.lec_errfin - lin),
                         MI_TERMINAL_ITALIQUE, lecteur.lec_errmsg, MI_TERMINAL_NORMAL,
                         nouvnom);
                  if (mi_nom_licite_chaine(nouvnom))
                    {
                      printf("Entrez un commentaire pour créer le symbole %s%s%s, ou rien pour recommencer...\n",
                             MI_TERMINAL_ITALIQUE, nouvnom, MI_TERMINAL_NORMAL);
                      char invitcom[24];
                      memset (invitcom, 0, sizeof(invitcom));
                      if (lgsym < (int) sizeof(invitcom)-8)
                        snprintf(invitcom, sizeof(invitcom), "%s? ", nouvnom);
                      else
                        snprintf(invitcom, sizeof(invitcom), "%*s...? ", (int)sizeof(invitcom)-8, nouvnom);
                      char*licom = readline(invitcom);
                      if (licom && licom[0])
                        {
                          Mit_Symbole*nouvsymb = mi_creer_symbole_chaine(nouvnom, 0);
                          if (nouvsymb)
                            {
                              nouvsymb->mi_attrs =
                                mi_assoc_mettre (nouvsymb->mi_attrs, MI_PREDEFINI (commentaire),
                                                 MI_CHAINEV (mi_creer_chaine (licom)));
                              repeterlect = true;
                              printf("Nouveau symbole %s%s%s créé!\n", MI_TERMINAL_GRAS, nouvnom, MI_TERMINAL_NORMAL);
                            }
                        }
                      free (nouvnom), nouvnom=NULL;
                    }
                }
              else
                printf("%sERREUR%s /%d (pos#%zd-%zd): %s%s%s\n",
                       MI_TERMINAL_GRAS, MI_TERMINAL_NORMAL,
                       err1,
                       (lecteur.lec_errptr - lin),
                       (lecteur.lec_errfin - lin),
                       MI_TERMINAL_ITALIQUE, lecteur.lec_errmsg, MI_TERMINAL_NORMAL);
              fflush(NULL);
              add_history(lin);
              MI_DEBOPRINTF("err1=%d cnt=%d wherehist=%d lin='%s'", err1, cnt, where_history(), lin);
            }
        }
      while(repeterlect);
      if (err1>0) continue;
      unsigned nbtrous = mi_assoc_compte(lecteur.lec_assoctrou);
      lecteur.lec_pascreer = false;
      if (nbtrous>0)
        {
          char promptrou[24];
          snprintf(promptrou,sizeof(promptrou), "Exp#%d", cnt);
          mi_lire_trous(&lecteur,promptrou);
        }
      lecteur.lec_pascreer = false;
      err2 = setjmp(lecteur.lec_jb);
      MI_DEBOPRINTF("cnt#%d err2#%d", cnt, err2);
      if (!err2)
        {
          char*finexp = NULL;
          Mit_Val vexp = mi_lire_expression(&lecteur,lin,&finexp);
          add_history(lin);
          MI_DEBOPRINTF("ok cnt=%d wherehist=%d lin='%s'", cnt, where_history(), lin);
          printf("expression lue #%d:\n", cnt);
          if (mi_vtype(vexp) == MiTy_Symbole)
            mi_afficher_contenu_symbole(stdout, mi_en_symbole(vexp));
          else
            mi_afficher_valeur(stdout, vexp);
          putchar('\n');
          fflush(NULL);
        }
      else
        {
          assert (lecteur.lec_errmsg != NULL);
          assert (lecteur.lec_errptr != NULL);
          if (lecteur.lec_errfin == NULL)
            printf("%sERREUR%s /%d (pos#%d): %s%s%s\n",
                   MI_TERMINAL_GRAS, MI_TERMINAL_NORMAL,
                   err1,
                   (int)(lecteur.lec_errptr - lin),
                   MI_TERMINAL_ITALIQUE, lecteur.lec_errmsg, MI_TERMINAL_NORMAL);
          else
            printf("%sERREUR%s /%d (pos#%zd-%zd): %s%s%s\n",
                   MI_TERMINAL_GRAS, MI_TERMINAL_NORMAL,
                   err1,
                   (lecteur.lec_errptr - lin),
                   (lecteur.lec_errfin - lin),
                   MI_TERMINAL_ITALIQUE, lecteur.lec_errmsg, MI_TERMINAL_NORMAL);
          fflush(NULL);
        }
      MI_DEBOPRINTF("fin lin#%d=%s", cnt, lin);
    }
  printf("\n fin de lecture de %d expressions en boucle\n", cnt);
} /* fin mi_lire_expressions_en_boucle */
