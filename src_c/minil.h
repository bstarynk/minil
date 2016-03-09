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

#define _GNU_SOURCE
/* on a besoin de asprintf */
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
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistr.h>		// GNU libunistring
#include <readline/readline.h>	// GNU readline
#include <jansson.h>		// JSON parsing library
/// les constantes d'empreinte dans le fichier généré _timestamp.c
extern const char minil_timestamp[];
extern const char minil_lastgitcommit[];
extern const char minil_checksum[];

/// une macro, à la printf, pour les messages d'erreur fatale
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
  MiTy_Ensemble,
  MiTy_Tuple,
  MiTy_Symbole,
  MiTy__Dernier // doit toujours être en dernier
};

// Les structures correspondantes
typedef struct MiSt_Entier_st Mit_Entier;
typedef struct MiSt_Double_st Mit_Double;
typedef struct MiSt_Chaine_st Mit_Chaine;
typedef struct MiSt_Ensemble_st Mit_Ensemble;
typedef struct MiSt_Tuple_st Mit_Tuple;
typedef struct MiSt_Symbole_st Mit_Symbole;

// Une valeur est un pointeur, mais de plusieurs types possibles, donc une union.
// Toute valeur non-nulle commence par son type et sa marque de ramasse-miettes.
struct MiStValeurMarquee_st
{
  enum mi_typeval_en miva_type;
  bool miva_marq;
};

// l'union des valeurs
union MiSt_Val_un
{
  void *miva_ptr;
  struct MiStValeurMarquee_st *miva_vmrq;
  const enum mi_typeval_en *miva_type;
  const Mit_Entier *miva_ent;
  const Mit_Double *miva_dbl;
  const Mit_Chaine *miva_chn;
  const Mit_Ensemble *miva_ens;
  const Mit_Tuple *miva_tup;
  Mit_Symbole *miva_sym;
};
typedef union MiSt_Val_un Mit_Val;

#define MI_NILV ((Mit_Val){.miva_ptr=NULL})
#define MI_ENTIERV(E) ((Mit_Val){.miva_ent=(E)})
#define MI_DOUBLEV(D) ((Mit_Val){.miva_dbl=(D)})
#define MI_CHAINEV(C) ((Mit_Val){.miva_chn=(C)})
#define MI_ENSEMBLEV(E) ((Mit_Val){.miva_ens=(E)})
#define MI_TUPLEV(T) ((Mit_Val){.miva_tup=(T)})
#define MI_SYMBOLEV(S) ((Mit_Val){.miva_sym=(S)})

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
  unsigned mi_hash;
  unsigned mi_taille;
  unsigned mi_long;
  char mi_car[];
};

// Une valeur ensemble a un type, une marque, une taille, et les symboles en ordre croissant

struct MiSt_Ensemble_st
{
  enum mi_typeval_en mi_type;
  bool mi_marq;
  unsigned mi_hash;
  unsigned mi_taille;
  Mit_Symbole *mi_elements[];
};
// Une valeur ensemble a un type, une marque, une taille, et les symboles

struct MiSt_Tuple_st
{
  enum mi_typeval_en mi_type;
  bool mi_marq;
  unsigned mi_hash;
  unsigned mi_taille;
  Mit_Symbole *mi_composants[];
};
// Une association par table de hashage entre symboles et valeurs.
// Ce n'est pas une valeur, mais une donnée interne.
struct Mi_Assoc_st;
// Un vecteur n'est pas une valeur, mais une donnée interne.
struct Mi_Vecteur_st;

enum mi_type_charge_en
{
  MiCh_Rien,
  MiCh_Tampon = MiTy__Dernier,
  Mich_Fichier,
  Mich_Tampon,
  Mich_Assoc,
  Mich_Vecteur,
  Mich_Environ
};

struct MiCh_Tampon_st;
struct MiCh_Environ_st;

struct MiSt_Radical_st;

// Une valeur symbole a un type, une marque, un radical, un
// indice, une association pour les attributs et un vecteur de
// composants; elle a aussi un chargement qui n'est pas proprement une
// valeur, et qui est discriminée par mi_chatype
struct MiSt_Symbole_st
{
  enum mi_typeval_en mi_type;
  bool mi_marq;
  unsigned mi_hash;
  unsigned mi_indice;
  bool mi_predef;
  struct MiSt_Radical_st*mi_radical;
  struct Mi_Assoc_st *mi_attrs;
  struct Mi_Vecteur_st *mi_comps;
  enum mi_type_charge_en mi_chatype;
  union
  {
    void*mi_chaptr;
    struct MiCh_Tampon_st*mi_chatamp;
    FILE*mi_chafichier;
    struct Mi_Assoc_st* mi_chassoc;
    struct Mi_Vecteur_st* mi_chvect;
    struct MiCh_Environ_st* mi_chenv;
  };
};


int mi_cmp_symbole (const Mit_Symbole *sy1, const Mit_Symbole *sy2);
int mi_cmp_symboleptr (const void *, const void *);	// pour qsort

// aussi bien dans la table des symboles que dans les associations
// on a besoin d'un trou qui ne soit pas nil. Aucune fonction publique
// ne doit renvoyer ce trou...
#define MI_TROU_SYMBOLE (Mit_Symbole*)(-1)


//// renvoie le hashage qu'aurait un symbole de nom et indice donnés
unsigned mi_hashage_nom_indice(const char*nom, unsigned ind);

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

static inline const Mit_Tuple *
mi_en_tuple (const Mit_Val v)
{
  if (!v.miva_ptr || *v.miva_type != MiTy_Tuple)
    return NULL;
  return v.miva_tup;
}				// fin mi_en_tuple

static inline const Mit_Ensemble *
mi_en_ensemble (const Mit_Val v)
{
  if (!v.miva_ptr || *v.miva_type != MiTy_Ensemble)
    return NULL;
  return v.miva_ens;
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



/// arité d'un tuple
static inline unsigned
mi_arite_noeud (const Mit_Tuple*tu)
{
  if (!tu || tu->mi_type != MiTy_Tuple)
    return 0;
  return tu->mi_taille;
}


static inline unsigned
mi_cardinal_ensemble (const Mit_Ensemble * en)
{
  if (!en || en->mi_type != MiTy_Ensemble)
    return 0;
  return en->mi_taille;
}

static inline Mit_Symbole *
mi_ensemble_nieme (const Mit_Ensemble * en, int n)
{
  if (!en || en->mi_type != MiTy_Ensemble)
    return NULL;
  unsigned ca = en->mi_taille;
  if (n < (int) ca)
    n += (int) ca;
  if (n >= 0 && n < (int) ca)
    return en->mi_elements[n];
  return NULL;
}

static inline Mit_Symbole *
mi_tuple_nieme (const Mit_Tuple * tu, int n)
{
  if (!tu || tu->mi_type != MiTy_Tuple)
    return NULL;
  unsigned ta = tu->mi_taille;
  if (n < (int) ta)
    n += (int) ta;
  if (n >= 0 && n < (int) ta)
    return tu->mi_composants[n];
  return NULL;
}

static inline const Mit_Ensemble *
mi_vald_ensemble (const Mit_Val v)
{
  if (mi_vtype (v) == MiTy_Ensemble)
    return v.miva_ens;
  return NULL;
}

bool mi_ensemble_contient (const Mit_Ensemble * en, const Mit_Symbole *sy);

/// ensemble de hash, alloué sur la pile
struct Mi_EnsHash_st
{
  unsigned eh_magiq;
  unsigned eh_taille;
  unsigned eh_compte;
  const Mit_Symbole **eh_table;
};
void mi_enshash_initialiser (struct Mi_EnsHash_st *eh, unsigned nb);
void mi_enshash_reserver (struct Mi_EnsHash_st *eh, unsigned nb);
void mi_enshash_detruire (struct Mi_EnsHash_st *eh);
void mi_enshash_ajouter (struct Mi_EnsHash_st *eh, const Mit_Symbole *sy);
void mi_enshash_ajouter_valeur (struct Mi_EnsHash_st *eh, const Mit_Val va);
void mi_enshash_oter (struct Mi_EnsHash_st *eh, const Mit_Symbole *sy);
bool mi_enshash_contient (const struct Mi_EnsHash_st *eh, const Mit_Symbole *sy);

/// la fonction d'iteration renvoie true pour arrêter l'itération
typedef bool mi_itersymb_sigt (Mit_Symbole *sy, void *client);
void mi_enshash_iterer (struct Mi_EnsHash_st *eh, mi_itersymb_sigt * f,
                        void *client);

const Mit_Ensemble* mi_ensemble_vide(void);
const Mit_Ensemble *mi_creer_ensemble_enshash (struct Mi_EnsHash_st *eh);
const Mit_Ensemble *mi_creer_ensemble_symboles (unsigned nb,
    const Mit_Symbole **tab);
const Mit_Ensemble *mi_creer_ensemble_valeurs (unsigned nb,
    const Mit_Val *tabval);
const Mit_Ensemble *mi_creer_ensemble_varsym (unsigned nb, ...);
const Mit_Ensemble *mi_creer_ensemble_varval (unsigned nb, ...);
void mi_ensemble_iterer (const Mit_Ensemble * en, mi_itersymb_sigt * f,
                         void *client);




const Mit_Ensemble *mi_ensemble_union (const Mit_Ensemble * en1,
                                       const Mit_Ensemble * en2);
const Mit_Ensemble *mi_ensemble_intersection (const Mit_Ensemble * en1,
    const Mit_Ensemble * en2);

// un tuple contient des symboles non nuls
const Mit_Tuple*mi_tuple_vide(void);
const Mit_Tuple*mi_creer_tuple_symboles(unsigned nb,
                                        const Mit_Symbole**tabsym);
const Mit_Tuple*mi_creer_tuple_valeurs(unsigned nb,
                                       const Mit_Val*tabval);
// hash code d'une chaine
unsigned mi_hashage_chaine (const char *ch);
// tester si une valeur chaine est licite pour un nom
bool mi_nom_licite (const Mit_Chaine *nom);
// tester si une chaine C est licite
bool mi_nom_licite_chaine (const char *ch);
// Trouver un symbole de nom et indice donnés
Mit_Symbole *mi_trouver_symbole_nom (const Mit_Chaine *nom, unsigned ind);
Mit_Symbole *mi_trouver_symbole_chaine (const char *ch, unsigned ind);
// trouver un symbole à partir de la chaine le nommant, avec un
// éventuel indice commençant par un blanc souligné
// si pfin est non nul il est mis au caractère suivant le nom
Mit_Symbole *mi_trouver_symbole(const char*ch, const char**pfin);
// Creer (ou trouver, s'il existe déjà) un symbole de nom et indice donnés
Mit_Symbole *mi_creer_symbole_nom (const Mit_Chaine *nom, unsigned ind);
Mit_Symbole *mi_creer_symbole_chaine (const char *ch, unsigned ind);

/// itérer sur chaque symbole primaire
void mi_iterer_symbole_primaire (mi_itersymb_sigt * f, void *client);
/// itérer sur chaque symbole primaire ou secondaire de nom donné
void mi_iterer_symbole_nomme (const char *ch, mi_itersymb_sigt * f,
                              void *client);

const char *mi_symbole_chaine (const Mit_Symbole *sy);

const Mit_Chaine*mi_symbole_nom(const Mit_Symbole*sy);

static inline unsigned
mi_symbole_indice (const Mit_Symbole *sy)
{
  if (sy && sy->mi_type == MiTy_Symbole)
    return sy->mi_indice;
  return 0;
}

static inline const char *
mi_symbole_indice_ch (char tamp[16], const Mit_Symbole *sy)
{
  unsigned i = mi_symbole_indice (sy);
  if (i)
    snprintf (tamp, 16, "_%d", i);
  else
    tamp[0] = (char) 0;
  return tamp;
}

static inline unsigned
mi_hashage_symbole (const Mit_Symbole *sy)
{
  if (!sy || sy->mi_type != MiTy_Symbole)
    return 0;
}


//// le type abstrait des associations entre symbole et valeur -quelconque-
//// On distingue la valeur nulle de l'absence de valeur
// reserver de la place, ou bien allouer
struct Mi_Assoc_st *mi_assoc_reserver (struct Mi_Assoc_st *a, unsigned nb);
// mettre une entrée
struct Mi_Assoc_st *mi_assoc_mettre (struct Mi_Assoc_st *a,
                                     const Mit_Symbole *sy, const Mit_Val va);
// supprimer une entrée
struct Mi_Assoc_st *mi_assoc_enlever (struct Mi_Assoc_st *a,
                                      const Mit_Symbole *sy);
// chercher une valeur, donc renvoyer la valeur et un drapeau de présence
struct Mi_trouve_st
{
  Mit_Val t_val;
  bool t_pres;
};
struct Mi_trouve_st mi_assoc_chercher (const struct Mi_Assoc_st *a,
                                       const Mit_Symbole *sy);
/// la fonction d'iteration renvoie true pour arrêter l'itération
typedef bool mi_assoc_sigt (const Mit_Symbole *sy, const Mit_Val va,
                            void *client);
unsigned mi_assoc_compte (const struct Mi_Assoc_st *a);
void mi_assoc_iterer (const struct Mi_Assoc_st *a, mi_assoc_sigt * f,
                      void *client);


//// le type abstrait des vecteurs
struct Mi_Vecteur_st *mi_vecteur_reserver (struct Mi_Vecteur_st *v,
    unsigned nb);
struct Mi_Vecteur_st *mi_vecteur_ajouter (struct Mi_Vecteur_st *v,
    const Mit_Val va);
struct Mi_trouve_st mi_vecteur_comp (struct Mi_Vecteur_st *v, int rang);
void mi_vecteur_mettre (struct Mi_Vecteur_st *v, int rank, const Mit_Val va);
unsigned mi_vecteur_taille (const struct Mi_Vecteur_st *v);
/// la fonction d'iteration renvoie true pour arrêter l'itération
typedef bool mi_vect_sigt (const Mit_Val va, unsigned ix, void *client);
void mi_vecteur_iterer (const struct Mi_Vecteur_st *v, mi_vect_sigt * f,
                        void *client);


struct mi_cadre_appel_st
{
  unsigned mic_nmagic;
  unsigned mic_taille;
  const char*mic_func;
  struct mi_cadre_appel_st*mic_prec;
  Mit_Symbole*mic_symbenv;
  Mit_Val mic_val[];
};
void mi_ramasse_miettes(struct mi_cadre_appel_st*cap);

//// sérialisation en JSON
//

/// types  pour la sauvegarde
#define MI_QUEUE_LONG_ELEM 15
struct Mi_Queuesauve_st
{
  struct Mi_Queuesauve_st *q_suiv;
  const Mit_Symbole *q_symb[MI_QUEUE_LONG_ELEM];
};

struct Mi_Sauvegarde_st		/* à allouer sur la pile */
{
  unsigned sv_magiq;
  struct Mi_EnsHash_st sv_syoubli;
  struct Mi_EnsHash_st sv_syconnu;
  const char *sv_rep;		// repertoire de sauvegarde
  struct Mi_Queuesauve_st *sv_tet;	// tête des symboles à parcourir
  struct Mi_Queuesauve_st *sv_que;	// queue des symboles à parcourir
};
void mi_sauvegarde_init (struct Mi_Sauvegarde_st *sv, const char *dir);

void mi_sauvegarde_symbole (struct Mi_Sauvegarde_st *sv,
                            const Mit_Symbole *sy);
bool mi_sauvegarde_symbole_oublie (struct Mi_Sauvegarde_st *sv,
                                   const Mit_Symbole *sy);
bool mi_sauvegarde_symbole_connu (struct Mi_Sauvegarde_st *sv,
                                  const Mit_Symbole *sy);
void mi_sauvegarde_oublier (struct Mi_Sauvegarde_st *sv,
                            const Mit_Symbole *sy);
void mi_sauvegarde_finir (struct Mi_Sauvegarde_st *sv);



/// charger l'état depuis un répertoire
void mi_charger_etat(const char*rep);

// serialiser une valeur en JSON
json_t *mi_json_val (struct Mi_Sauvegarde_st *sv, const Mit_Val v);
// serialiser le contenu d'un symbole en JSON
json_t *mi_json_contenu_symbole (struct Mi_Sauvegarde_st *sv,
                                 const Mit_Symbole *sy);

// construire une valeur à partir d'un JSON
Mit_Val mi_val_json (const json_t *j);
// remplir un symbole à partir de son contenu
const Mit_Symbole* mi_remplir_symbole_json (const json_t *j);

/// emettre la notice de copyright GPLv3
void
mi_emettre_notice_gplv3 (FILE * fichier, const char *prefixe,
                         const char *suffixe, const char *nomfich);

#define MI_PREDEFINI(Nom) mipred_##Nom
#define MI_TRAITER_PREDEFINI(Nom,Hash) extern Mit_Symbole*MI_PREDEFINI(Nom);
#include "_mi_predef.h"
#endif /*MINIL_INCLUDED_ */
