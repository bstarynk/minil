// fichier miprog.c - le programme principal
/* la notice de copyright est legalement en anglais */

// (C) 2016 Basile Starynkevitch
//   this file minil.h is part of Minil
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


/// pour getopt_long, décrit les arguments de programme
const struct option minil_options[] = {
  {"--help", no_argument, NULL, 'h'},
  {"--version", no_argument, NULL, 'V'},
  {NULL, 0, NULL, 0}
};

/// affiche l'usage, doit correspondre à minil_options
static void
mi_usage (const char *nomprog)
{
  printf ("usage de %s, un mini interprète\n", nomprog);
  printf (" --help | -h #message d'aide\n");
  printf (" --version | -V #donne la version\n");
}

static void
mi_arguments_programme (int argc, char **argv)
{
  int ch = 0;
  while ((ch = getopt_long (argc, argv, "hV", minil_options, NULL)) > 0)
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
	}
    }
}

int
main (int argc, char **argv)
{
  mi_arguments_programme (argc, argv);
}
