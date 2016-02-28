// fichier miprog.c - le programme principal
/* la notice de copyright est legalement en anglais */

// (C) 2016 Basile Starynkevitch
//   this file miprog.c is part of Minil
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

enum
{
  xtraopt__debut=1000,
  xtraopt_predefini,
  xtraopt__fin
};

/// pour getopt_long, décrit les arguments de programme
const struct option minil_options[] =
{
  {"--help", no_argument, NULL, 'h'},
  {"--version", no_argument, NULL, 'V'},
  {"--sauvegarde", required_argument, NULL, 'S'},
  {"--charge", required_argument, NULL, 'c'},
  {"--oublier", required_argument, NULL, 'O'},
  {"--predefini", required_argument, NULL, xtraopt_predefini},
  {NULL, 0, NULL, 0}
};

// les répertoires de chargement et de sauvegarde
static const char*mi_repcharge;
static const char*mi_repsauve;
struct Mi_Sauvegarde_st *mi_sauv;

/// affiche l'usage, doit correspondre à minil_options
static void
mi_usage (const char *nomprog)
{
  printf ("usage de %s, un mini interprète\n", nomprog);
  printf (" --help | -h #message d'aide\n");
  printf (" --version | -V #donne la version\n");
  printf (" --charge | -c <repertoire> #charger l'état\n");
  printf (" --sauvegarde | -S <repertoire> #sauvegarder l'état\n");
  printf (" --oublier | -O <symbole> #oublier un symbole\n");
  printf (" --predefini <symbole> #créer un symbole predefini\n");
}

static void
mi_arguments_programme (int argc, char **argv)
{
  int ch = 0;
  while ((ch = getopt_long (argc, argv, "hVc:S:O:",
                            minil_options, NULL)) > 0)
    {
      switch (ch)
        {
        case 'h':		// --help
          mi_usage (argv[0]);
          break;
        case 'V':		// --version
          printf ("%s daté du %s, modif. %s, somme de contrôle %s\n",
                  argv[0], minil_timestamp, minil_lastgitcommit,
                  minil_checksum);
          exit (EXIT_SUCCESS);
          break;
        case 'c': // --charge <rep>
          mi_repcharge = optarg;
          mi_charger_etat(optarg);
          break;
        case 'S': // --sauvegarde <rep>
          mi_repsauve = optarg;
          mi_sauv = calloc(1, sizeof(struct Mi_Sauvegarde_st));
          if (!mi_sauv)
            MI_FATALPRINTF("impossible de créer la sauvegarde (%s)",
                           strerror(errno));
          mi_sauvegarde_init(mi_sauv, mi_repsauve);
          break;
        case 'O': // --oublier <symb>
          if (!mi_sauv)
            MI_FATALPRINTF("option de --sauvegarde <répertoire> manquante avant --oubli %s", optarg);
          {
            const Mit_Symbole*sy = mi_trouver_symbole(optarg, NULL);
            if (!sy)
              MI_FATALPRINTF("Le symbole à oublier '%s' n'existe pas!", optarg);
            mi_sauvegarde_oublier(mi_sauv, sy);
          }
          break;
        case xtraopt_predefini: // --predefini <nom>
        {
          Mit_Symbole*sy = NULL;
          if (!optarg || !isalpha(optarg[0]) || strchr(optarg, '_') ||
              !mi_nom_licite_chaine(optarg) || !(sy=mi_creer_symbole_chaine(optarg, 0)))
            MI_FATALPRINTF("Mauvais nom %s de symbole predefini", optarg?optarg:"??");
          sy->mi_predef = true;
          printf("symbole prédéfini %s créé\n", mi_symbole_chaine(sy));
        }
        break;
        }
    }
}				// fin de mi_arguments_programme


// un tableau croissant de certains nombres premiers, obtenu par exemple avec
//     /usr/games/primes 3 2000000000  | awk '($1>p+p/9){print $1, ","; p=$1}'
static const unsigned mi_tableau_premiers[] =
{
  3, 5, 7, 11, 13, 17, 19, 23, 29, 37, 43, 53, 59, 67, 79, 89, 101, 113,
  127, 149, 167, 191, 223, 251, 281, 313, 349, 389, 433, 487, 547, 613,
  683, 761, 853, 953, 1061, 1181, 1319, 1471, 1637, 1823, 2027, 2267,
  2521, 2803, 3119, 3467, 3853, 4283, 4759, 5297, 5897, 6553, 7283,
  8093, 8999, 10007, 11119, 12373, 13751, 15287, 16987, 18899, 21001,
  23339, 25933, 28817, 32027, 35591, 39551, 43951, 48847, 54277, 60317,
  67021, 74471, 82757, 91957, 102181, 113537, 126173, 140197, 155777,
  173087, 192319, 213713, 237467, 263863, 293201, 325781, 361979, 402221,
  446921, 496579, 551767, 613097, 681221, 756919, 841063, 934517, 1038383,
  1153759, 1281961, 1424407, 1582697, 1758553, 1953949, 2171077, 2412323,
  2680367, 2978189, 3309107, 3676789, 4085339, 4539277, 5043653, 5604073,
  6226757, 6918619, 7687387, 8541551, 9490631, 10545167, 11716879,
  13018757, 14465291, 16072547, 17858389, 19842659, 22047401, 24497113,
  27219019, 30243359, 33603743, 37337497, 41486111, 46095719, 51217477,
  56908337, 63231499, 70257241, 78063641, 86737379, 96374881, 107083213,
  118981367, 132201521, 146890631, 163211821, 181346479, 201496157,
  223884629, 248760703, 276400823, 307112027, 341235667, 379150777,
  421278643, 468087391, 520097111, 577885681, 642095213, 713439127,
  792710159, 880789067, 978654533, 1087393949, 1208215531, 1342461719,
  1491624137, 1657360153, 1841511311, 0
};

unsigned
mi_nombre_premier_apres (unsigned i)
{
  const unsigned nbprem
    = sizeof (mi_tableau_premiers) / sizeof (mi_tableau_premiers[0]) - 1;
  unsigned ix = 0;
  if (i < mi_tableau_premiers[nbprem / 3])
    ix = 0;
  else if (i < mi_tableau_premiers[2 * nbprem / 3])
    ix = nbprem / 3;
  else
    ix = 2 * nbprem / 3;
  for (; ix < nbprem; ix++)
    {
      unsigned p = mi_tableau_premiers[ix];
      if (p > i)
        return p;
    }
  return 0;
}				// mi_nombre_premier_apres

unsigned
mi_nombre_premier_avant (unsigned i)
{
  const unsigned nbprem
    = sizeof (mi_tableau_premiers) / sizeof (mi_tableau_premiers[0]) - 1;
  unsigned d = 0;
  if (i < mi_tableau_premiers[nbprem / 3])
    d = nbprem / 3;
  else if (i < mi_tableau_premiers[2 * nbprem / 3])
    d = 2 * nbprem / 3;
  else
    d = nbprem;
  for (int k = (int)d; k >= 0; k--)
    {
      unsigned p = mi_tableau_premiers[k];
      if (p < i)
        return p;
    }
  return 0;
}				// fin mi_nombre_premier_avant



static void mi_initialiser_predefinis (void);

int
main (int argc, char **argv)
{
  mi_initialiser_predefinis ();
  mi_arguments_programme (argc, argv);
  if (mi_sauv)
    mi_sauvegarde_finir(mi_sauv);
}				// fin de main

/// declarer les prédéfinis
#define MI_TRAITER_PREDEFINI(Nom,Hash) Mit_Symbole*MI_PREDEFINI(Nom);
#include "_mi_predef.h"


void
mi_initialiser_predefinis (void)
{
#define MI_TRAITER_PREDEFINI(Nom,Hash) do {		\
    MI_PREDEFINI(Nom) =					\
      mi_creer_symbole_chaine(#Nom,0);			\
    if (MI_PREDEFINI(Nom)->mi_hash != (Hash))		\
      MI_FATALPRINTF("symbole predefini %s hash %u,"	\
		     " devrait être %u",		\
		     #Nom, MI_PREDEFINI(Nom)->mi_hash,	\
		     (unsigned)(Hash));			\
    MI_PREDEFINI(Nom)->mi_predef = true;		\
  } while(0);
#include "_mi_predef.h"
}


void
mi_emettre_notice_gplv3 (FILE * fichier, const char *prefixe,
                         const char *suffixe, const char *nomfich)
{
  time_t maintenant = 0;
  time (&maintenant);
  struct tm maintenanttm;
  memset (&maintenanttm, 0, sizeof (maintenanttm));
  localtime_r (&maintenant, &maintenanttm);
  if (!prefixe)
    prefixe = "";
  if (!suffixe)
    suffixe = "";
  fprintf (fichier, "%s *** generated file %s - DO NOT EDIT %s\n", prefixe,
           nomfich, suffixe);
  fprintf (fichier,
           "%s Copyright (C) %d Basile Starynkevitch. %s\n",
           prefixe, 1900 + maintenanttm.tm_year, suffixe);
  fprintf (fichier,
           "%s This generated file %s is part of Minil %s\n",
           prefixe, nomfich, suffixe);
  fprintf (fichier, "%s%s\n", prefixe, suffixe);
  fprintf (fichier,
           "%s Minil is free software; you can redistribute it and/or modify %s\n",
           prefixe, suffixe);
  fprintf (fichier,
           "%s it under the terms of the GNU General Public License as published by %s\n",
           prefixe, suffixe);
  fprintf (fichier,
           "%s the Free Software Foundation; either version 3, or (at your option) %s\n",
           prefixe, suffixe);
  fprintf (fichier, "%s any later version. %s\n", prefixe, suffixe);
  fprintf (fichier, "%s%s\n", prefixe, suffixe);
  fprintf (fichier,
           "%s  Minil is distributed in the hope that it will be useful, %s\n",
           prefixe, suffixe);
  fprintf (fichier,
           "%s  but WITHOUT ANY WARRANTY; without even the implied warranty of %s\n",
           prefixe, suffixe);
  fprintf (fichier,
           "%s  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the %s\n",
           prefixe, suffixe);
  fprintf (fichier, "%s  GNU General Public License for more details. %s\n",
           prefixe, suffixe);
  fprintf (fichier,
           "%s  You should have received a copy of the GNU General Public License %s\n",
           prefixe, suffixe);
  fprintf (fichier,
           "%s  along with Minil; see the file COPYING3.   If not see %s\n",
           prefixe, suffixe);
  fprintf (fichier, "%s  <http://www.gnu.org/licenses/>. %s\n", prefixe,
           suffixe);
  fprintf (fichier, "%s%s\n", prefixe, suffixe);
  fflush (fichier);
}				/* fin mi_emettre_notice_gplv3 */
