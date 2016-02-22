// fichier minil.h
#ifndef MINIL_INCLUDED_
#define MINIL_INCLUDED_
/* la notice de copyright GPL est lÃ©galement en anglais */

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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <getopt.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <limits.h>

/// les constantes d'empreinte dans le fichier généré _timestamp.c
extern const char minil_timestamp[];
extern const char minil_lastgitcommit[];
extern const char minil_checksum[];

/// une macro, à la printf, pour les messages d'erreur fatale
#define MI_FATALPRINTF(Fmt,...) do { \
	fprintf (stderr, "%s:%d(%s) FATAL:" Fmt "\n", __FILE__, __LINE__, \
        __func__, ##__VA_ARGS__); fflush(NULL); \
	abort(); } while(0)

//////////////////////////////////////
// Les nombres premiers sont utiles, notamment pour les tables de hash
// Donne un nombre premier après ou bien avant l'argument, ou 0 en échec.
unsigned mi_nombre_premier_apres (unsigned i);
unsigned mi_nombre_premier_avant (unsigned i);
//////////////////////////////////////
// Les differents types de valeurs
enum mi_typeval_en
{
  MiTy_Nil,
  MiTy_Entier,
  MiTy_Double,
  MiTy_Chaine,
  MiTy_Noeud,
  MiTy_Symbole,
};

// Les structures correspondantes
typedef struct MiSt_Entier_st Mit_Entier;
typedef struct MiSt_Double_st Mit_Double;
typedef struct MiSt_Chaine_st Mit_Chaine;
typedef struct MiSt_Noeud_st Mit_Noeud;
typedef struct MiSt_Symbole_st Mit_Symbole;
// Une valeur est un pointeur, mais de plusieurs types possibles, donc une union.
// Toute valeur non-nulle commence par son type et sa marque de ramasse-miettes.
struct MiStValeurMarquee_st
{
  enum mi_typeval_en miva_type;
  bool miva_marq;
};
union MiSt_Val_un
{
  void *miva_ptr;
  struct MiStValeurMarquee_st *miva_vmrq;
  const enum mi_typeval_en *miva_type;
  const Mit_Entier *miva_ent;
  const Mit_Double *miva_dbl;
  const Mit_Chaine *miva_chn;
  const Mit_Noeud *miva_noe;
  Mit_Symbole *miva_sym;
};
typedef union MiSt_Val_un Mit_Val;

static inline enum mi_typeval_en
mi_vtype (const Mit_Val v)
{
  if (!v.miva_ptr)
    return MiTy_Nil;
  return *v.miva_type;
}				// fi mi_vtype

// Une valeur entière a un type, une marque, et un nombre entier.
struct MiSt_Entier_st
{
  enum mi_typeval_en mi_type;
  bool mi_marq;
  long mi_ent;
};

// Une valeur double a un type, une marque, et un nombre double.
struct MiSt_Double_st
{
  enum mi_typeval_en mi_type;
  bool mi_marq;
  double mi_dbl;
};

// Une valeur chaîne a un type, une marque, et les octets de la chaîne 
// (terminés par l'octet nul).
// C'est une structure de taille "variable" se terminant par un membre flexible
// https://en.wikipedia.org/wiki/Flexible_array_member
struct MiSt_Chaine_st
{
  enum mi_typeval_en mi_type;
  bool mi_marq;
  char mi_car[];
};

// Une valeur noeud a un type, une marque, une connective, une arité, des fils
// C'est la seule valeur composite....
struct MiSt_Noeud_st
{
  enum mi_typeval_en mi_type;
  bool mi_marq;
  Mit_Symbole *mi_conn;
  unsigned mi_arite;
  Mit_Val mi_fils[];
};
// Une association par table de hashage entre symboles et valeurs.
// Ce n'est pas une valeur, mais une donnée interne.
struct Mi_Assoc_st;
// Un vecteur n'est pas une valeur, mais une donnée interne.
struct Mi_Vecteur_st;
// Une valeur symbole a un type, une marque, une chaîne nom, un indice, 
// une association pour les attributs
// et un vecteur de composants
struct MiSt_Symbole_st
{
  enum mi_typeval_en mi_type;
  bool mi_marq;
  const Mit_Chaine *mi_nom;
  unsigned mi_indice;
  struct Mi_Assoc_st *mi_attrs;
  struct Mi_Vecteur_st *mi_comps;
};
////////////////////// conversions sûres, car verifiantes
static inline const Mit_Entier *
mi_en_entier (const Mit_Val v)
{
  if (!v.miva_ptr || *v.miva_type != MiTy_Entier)
    return NULL;
  return v.miva_ent;
}				// fin mi_en_entier

static inline const Mit_Double *
mi_en_double (const Mit_Val v)
{
  if (!v.miva_ptr || *v.miva_type != MiTy_Double)
    return NULL;
  return v.miva_dbl;
}				// fin mi_en_double

static inline const Mit_Chaine *
mi_en_chaine (const Mit_Val v)
{
  if (!v.miva_ptr || *v.miva_type != MiTy_Chaine)
    return NULL;
  return v.miva_chn;
}				// fin mi_en_chaine

static inline Mit_Symbole *	// sans const, car on touchera l'intérieur du symbole
mi_en_symbole (const Mit_Val v)
{
  if (!v.miva_ptr || *v.miva_type != MiTy_Symbole)
    return NULL;
  return v.miva_sym;
}				// fin mi_en_symbole

static inline const Mit_Noeud *
mi_en_noeud (const Mit_Val v)
{
  if (!v.miva_ptr || *v.miva_type != MiTy_Noeud)
    return NULL;
  return v.miva_noe;
}				// fin mi_en_noeud


/// mis à vrai quand faut lancer un ramasse miettes, ne doit pas être
/// modifié ailleurs...
extern bool mi_faut_ramiet;
/// allocation de bas niveau d'une valeur, utilisée par les routines de création
void *mi_allouer_valeur (enum mi_typeval_en typv, size_t tail);

/// création de chaine
const Mit_Chaine *mi_creer_chaine (const char *ch);
/// création à la printf
const Mit_Chaine *mi_creer_chaine_printf (const char *fmt, ...)
  __attribute__ ((format (printf, 1, 2)));
/// accès à la chaîne, ou bien NULL
static inline const char *
mi_val_chaine (const Mit_Val v)
{
  if (mi_vtype (v) == MiTy_Chaine)
    return v.miva_chn->mi_car;
  else
    return NULL;
}

/// accès à la chaine avec valeur par défaut
static inline const char *
mi_vald_chaine (const Mit_Val v, const char *def)
{
  const char *ch = mi_val_chaine (v);
  return ch ? ch : def;
}

/// création d'entier
const Mit_Entier *mi_creer_entier (long l);
/// accès à l'entier ou une valeur par défaut
static inline long
mi_vald_entier (const Mit_Val v, long def)
{
  if (mi_vtype (v) == MiTy_Entier)
    return v.miva_ent->mi_ent;
  return def;
}

/// création d'un double
const Mit_Double *mi_creer_double (double d);
/// accès au double ou une valeur par défaut
static inline double
mi_vald_double (const Mit_Val v, double def)
{
  if (mi_vtype (v) == MiTy_Double)
    return v.miva_dbl->mi_dbl;
  return def;
}

/// création d'un noeud d'arité donnée
const Mit_Noeud *mi_creer_noeud (const Mit_Symbole *consymb, unsigned arite,
				 const Mit_Val fils[]);
const Mit_Noeud *mi_creer_noeud_va (const Mit_Symbole *consymb,
				    unsigned arite, ...);

// hash code d'une chaine
unsigned mi_hashage_chaine (const char *ch);
// tester si une valeur chaine est licite pour un nom
bool mi_nom_licite (Mit_Chaine *nom);
// tester si une chaine C est licite
bool mi_nom_licite_chaine (const char *ch);
// Trouver un symbole de nom et indice donnés
Mit_Symbole *mi_trouver_symbole_nom (const Mit_Chaine *nom, unsigned ind);
Mit_Symbole *mi_trouver_symbole_chaine (const char *ch, unsigned ind);
// Creer (ou trouver, s'il existe déjà) un symbole de nom et indice donnés
Mit_Symbole *mi_creer_symbole_nom (const Mit_Chaine *nom, unsigned ind);
Mit_Symbole *mi_creer_symbole_chaine (const char *ch, unsigned ind);
#endif /*MINIL_INCLUDED_ */
