// fichier minil.h
#ifndef MINIL_INCLUDED_
#define MINIL_INCLUDED_
/* la notice de copyright GPL est légalement en anglais */

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

/// les constantes d'empreinte dans le fichier généré _timestamp.c
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

// Une valeur entière a un type, une marque, et un nombre entier.
struct MiSt_Entier_st
{
  enum mi_typeval_en mi_type;
  bool miva_marq;
  long mi_ent;
};

// Une valeur double a un type, une marque, et un nombre double.
struct MiSt_Double_st
{
  enum mi_typeval_en mi_type;
  bool miva_marq;
  double mi_dbl;
};

// Une valeur chaîne a un type, une marque, et les octets de la chaîne 
// (terminés par l'octet nul).
// C'est une structure de taille "variable" se terminant par un membre flexible
// https://en.wikipedia.org/wiki/Flexible_array_member
struct MiSt_Chaine_st
{
  enum mi_typeval_en mi_type;
  bool miva_marq;
  char mi_str[];
};

// Une valeur noeud a un type, une marque, une connective, une arité, des fils
struct MiSt_Noeud_st
{
  enum mi_typeval_en mi_type;
  bool miva_marq;
  Mit_Symbole *mi_conn;
  unsigned mi_arite;
  Mit_Val mi_fils[];
};
#endif /*MINIL_INCLUDED_ */
