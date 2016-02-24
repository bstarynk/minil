# fichier Makefile ---- lancer "make" pour compiler
# pour l'utilitaire `GNU make` https://www.gnu.org/software/make

# la commande de calcul d'empreinte (ou somme de contrôle)
MD5SUM= md5sum

# la commande pour supprimer les fichiers
RM= rm -vf

# le compilateur C
CC= gcc

# l'indenteur de code ASTYLE et ses options
ASTYLE= astyle
ASTYLEFLAGS= -s2 --style=gnu

## dépendance : GNU libunistring http://www.gnu.org/software/libunistring/
## dépendance : GNU readline http://www.gnu.org/software/readline
LDLIBS= -lunistring -lreadline -ljansson

# les options du compilateur ; on veut le standard C99 amÃliorÃ©par GCC
CFLAGS= -std=gnu99 $(OPTIMFLAGS) $(DIAGFLAGS) $(PREPROFLAGS)
# optimisation:
OPTIMFLAGS= -g -O
# diagnostics
DIAGFLAGS= -Wall -Wextra
# preprocesseur
PREPROFLAGS= -I /usr/local/include/

# les fichiers sources et objects
SRCFILES=$(wildcard mi*.c)
OBJFILES=$(patsubst %.c,%.o,$(SRCFILES))

## les cibles de la commande make, la première étant par défaut
.PHONY: all clean indent
all: minil

# le fichier d'horodatage
_timestamp.c:
	@date +'const char minil_timestamp[]="%c";' > _timestamp.tmp
	@(echo -n 'const char minil_lastgitcommit[]="' ; \
	   git log --format=oneline --abbrev=12 --abbrev-commit -q  \
	     | head -1 | tr -d '\n\r\f\"' ; \
	   echo '";') >> _timestamp.tmp
	@(echo -n 'const char minil_checksum[]="'; cat minil.h $(SRCFILES) \
	   | $(MD5SUM) | cut -d' ' -f1 | tr -d '\n\r\f\"\\' ; echo '";') >> _timestamp.tmp
	@mv _timestamp.tmp _timestamp.c


$(OBJFILES): minil.h

## pour nettoyer
clean:
	$(RM) $(OBJFILES) _timestamp.* *~ *% minil

# le programme minil
minil: $(OBJFILES)
	$(MAKE) _timestamp.o
	$(LINK.c) $(LINKFLAGS) -rdynamic $(OBJFILES) _timestamp.o $(LDLIBS) -o $@
	$(RM) _timestamp.*

## pour indenter
indent: .indent.pro
	for f in minil.h $(SRCFILES) ; do $(ASTYLE) $(ASTYLEFLAGS) $$f ; done
