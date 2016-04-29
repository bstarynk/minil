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


void mi_lire_expressions_en_boucle(void)
{
  using_history();
  printf("Entrez des expressions en boucle, et une ligne vide pour terminer...\n");
  int cnt=0;
  int numexpcorr=0;		/* le numéro de la dernière expression correcte */
  for (;;)
    {
      cnt++;
      char promptx[32];
      snprintf(promptx, sizeof(promptx), "%sEXP#%d:%s ",
               MI_TERMINAL_PALE, cnt, MI_TERMINAL_NORMAL);
      char*lin = readline(promptx);
      if (!lin || !lin[0]) break;
      fflush(NULL);
      MI_DEBOPRINTF("lin#%d=%s numexpcorr=%d wherehist=%d",
                    cnt, lin, numexpcorr, where_history());
      struct Mi_Lecteur_st lecteur;
      memset(&lecteur, 0, sizeof(lecteur));
      lecteur.lec_nmagiq = MI_LECTEUR_NMAGIQ;
      lecteur.lec_pascreer = true;
      int err1 = setjmp(lecteur.lec_jb);
      MI_DEBOPRINTF("cnt#%d err1#%d", cnt, err1);
      if (!err1)
        {
          char*finexp = NULL;
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
          else
            printf("%sERREUR%s /%d (pos#%zd-%zd): %s%s%s\n",
                   MI_TERMINAL_GRAS, MI_TERMINAL_NORMAL,
                   err1,
                   (lecteur.lec_errptr - lin),
                   (lecteur.lec_errfin - lin),
                   MI_TERMINAL_ITALIQUE, lecteur.lec_errmsg, MI_TERMINAL_NORMAL);
          fflush(NULL);
          add_history(lin);
          continue;
        }
      unsigned nbtrous = mi_assoc_compte(lecteur.lec_assoctrou);
      lecteur.lec_pascreer = false;
      if (nbtrous>0)
        {
          char promptrou[24];
          snprintf(promptrou,sizeof(promptrou), "Exp#%d", cnt);
          mi_lire_trous(&lecteur,promptrou);
        }
      lecteur.lec_pascreer = false;
      int err2 = setjmp(lecteur.lec_jb);
      MI_DEBOPRINTF("cnt#%d err2#%d", cnt, err2);
      if (!err2)
        {
          char*finexp = NULL;
          Mit_Val vexp = mi_lire_expression(&lecteur,lin,&finexp);
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
