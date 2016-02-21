// fichier minil.h
#ifndef MINIL_INCLUDED_
#define MINIL_INCLUDED_
/* la notice de copyright GPL est l�galement en anglais */

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

/// les constantes d'empreinte dans le fichier g�n�r� _timestamp.c
extern const char minil_timestamp[];
extern const char minil_lastgitcommit[];
extern const char minil_checksum[];
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
  enum mi_typeval_en *miva_type;
  Mit_Entier *miva_ent;
  Mit_Double *miva_dbl;
  Mit_Chaine *miva_chn;
  Mit_Noeud *miva_noe;
  Mit_Symbole *miva_sym;
};
typedef union MiSt_Val_un Mit_Val;

// Une valeur enti�re a un type, une marque, et un nombre entier.
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

// Une valeur cha�ne a un type, une marque, et les octets de la cha�ne 
// (termin�s par l'octet nul).
// C'est une structure de taille "variable" se terminant par un membre flexible
// https://en.wikipedia.org/wiki/Flexible_array_member
struct MiSt_Chaine_st
{
  enum mi_typeval_en mi_type;
  bool mi_marq;
  char mi_str[];
};

// Une valeur noeud a un type, une marque, une connective, une arit�, des fils
struct MiSt_Noeud_st
{
  enum mi_typeval_en mi_type;
  bool mi_marq;
  Mit_Symbole *mi_conn;
  unsigned mi_arite;
  Mit_Val mi_fils[];
};

// Une association par table de hashage entre symboles et valeurs.
// Ce n'est pas une valeur, mais une donn�e interne.
struct Mi_Assoc_st;

// Un vecteur n'est pas une valeur, mais une donn�e interne.
struct Mi_Vecteur_st;

// Une valeur symbole a un type, une marque, une cha�ne nom, un indice, une association pour les attributs
// et un vecteur de composants
struct MiSt_Symbole_st
{
  enum mi_typeval_en mi_type;
  bool mi_marq;
  Mit_Chaine *mi_nom;
  unsigned mi_indice;
  struct Mi_Assoc_st *mi_attrs;
  struct Mi_Vecteur_st *mi_comps;
};

////////////////////// conversions s�res, car verifiantes
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

static inline Mit_Symbole *	// sans const, car on touchera l'int�rieur du symbole
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

// tester si une valeur chaine est licite pour un nom
bool mi_nom_licite (Mit_Chaine *nom);
// tester si une chaine C est licite
bool mi_nom_licite_chaine (const char *ch);
// Trouver un symbole de nom et indice donn�s
Mit_Symbole *mi_trouver_symbole_nom (const Mit_Chaine *nom, unsigned ind);

#endif /*MINIL_INCLUDED_ */
