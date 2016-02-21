// fichier misymb.c - gestion des symboles
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


// tester si une valeur chaine est licite pour un nom
bool
mi_nom_licite (Mit_Chaine *nom)
{
  if (!nom || nom->mi_type != MiTy_Chaine)
    return false;
  return mi_nom_licite_chaine (nom->mi_str);
}				// fin mi_nom_licite

// tester si une chaine C est licite
bool
mi_nom_licite_chaine (const char *ch)
{
  if (!ch)
    return false;
  if (!isalpha (ch[0]))
    return false;
  for (const char *pc = ch + 1; *pc; pc++)
    if (!isalnum (*pc))
      return false;
  return true;
}				// fin mi_nom_licite_chaine

// calcul du hash d'une chaine, généralement non nul
unsigned
mi_hashage_chaine (const char *ch)
{
  unsigned h1 = 0, h2 = 0;
  unsigned rk = 0;
  if (!ch || !ch[0])
    return 0;
  /// lest constantes multiplicatives sont des nombres premiers
  for (const char *pc = ch; *pc; pc++, rk++)
    {
      if (rk % 2)
	h1 = (h1 * 463) ^ (((unsigned char *) pc)[0] * 2027 + 5 * rk);
      else
	h2 = (h2 * 1021 + 13 * rk) ^ (((unsigned char *) pc)[0] * 1049);
    }
  unsigned h = (5 * h1) ^ (17 * h2);
  if (!h)
    h = (h1 % 10039) + (h2 % 20047) + (rk % 30059) + 7;
  return h;
}				// fin mi_hashage_chaine


/****
 *      Organisation de la table des symboles
 *      =====================================
 *
 * Les symboles (primaires) sans indices sont toujours préservés.
 * Mais les symboles d'indices non-nuls (symboles secondaires) peuvent 
 * être récupérés par le ramasse-miettes.
 *
 * On a donc une table dichotomique de baquets. Chaque baquet contient 
 * le nom commun de ses symboles, son symbole primaire, et une table de 
 * hashage de symboles secondaires
 *
 ****/
struct mi_tabhash_second_st;
struct mi_baquet_symbole_st
{
  Mit_Chaine *baq_nom;
  Mit_Symbole *baq_symbprim;
  struct mi_tabhashsymb_secondaire_st *baq_tabsec;
};
// la table dichotomique de baquets
static struct
{
  unsigned dic_taille;
  unsigned dic_compte;
  struct mi_baquet_symbole_st *dic_table;	// taille allouée == dic_taille
} mi_dicho_symb;


// la table de hashage dans un baquet
struct mi_tabhashsymb_secondaire_st
{
  unsigned ths_taille;		// generalement un nombre premier
  unsigned ths_compte;
  Mit_Symbole *ths_symboles[];	// taille allouée == ths_taille
};
// dans la table ci-dessus, un emplacement de symbole inoccupé peut être nil ou bien
#define MI_TROU_SYMBOLE (Mit_Symbole*)(-1)

// Trouver un symbole de nom et indice donnés
Mit_Symbole *
mi_trouver_symbole_nom (const Mit_Chaine *nom, unsigned ind)
{
  if (!nom || nom->mi_type != MiTy_Chaine)
    return NULL;
  return mi_trouver_symbole_chaine (nom->mi_str, ind);
}				// fin mi_trouver_symbole_nom

static Mit_Symbole *
mi_trouver_symbole_baquet (struct mi_baquet_symbole_st *baq, unsigned ind)
{
  if (!baq)
    return NULL;
  if (!ind)
    return baq->baq_symbprim;
  struct mi_tabhashsymb_secondaire_st *th = baq->baq_tabsec;
  unsigned t = th->ths_taille;
  assert (t > 2);
  unsigned d = ind % t;
  for (unsigned i = d; i < t; i++)
    {
      Mit_Symbole *syc = th->ths_symboles[i];
      if (syc == NULL)
	return NULL;
      if (syc == MI_TROU_SYMBOLE)
	continue;
      assert (syc->mi_type == MiTy_Symbole);
      if (syc->mi_indice == ind)
	return syc;
    }
  for (unsigned i = 0; i < d; i++)
    {
      Mit_Symbole *syc = th->ths_symboles[i];
      if (syc == NULL)
	return NULL;
      if (syc == MI_TROU_SYMBOLE)
	continue;
      assert (syc->mi_type == MiTy_Symbole);
      if (syc->mi_indice == ind)
	return syc;
    }
  return NULL;
}				// fin mi_trouver_symbole_baquet


Mit_Symbole *
mi_trouver_symbole_chaine (const char *ch, unsigned ind)
{
  if (!mi_nom_licite_chaine (ch))
    return NULL;
  // recherche dichotomique
  unsigned bas = 0, hau = mi_dicho_symb.dic_taille, mil = 0;
  while (bas + 5 < hau)
    {
      mil = (bas + hau) / 2;
      const Mit_Chaine *nom = mi_dicho_symb.dic_table[mil].baq_nom;
      assert (nom != NULL && nom->mi_type == MiTy_Chaine);
      int cmp = strcmp (ch, nom->mi_str);
      if (cmp == 0)
	return mi_trouver_symbole_baquet (mi_dicho_symb.dic_table + mil, ind);
      else if (cmp < 0)
	hau = mil;
      else
	bas = mil;
    }
  for (mil = bas; mil < hau; mil++)
    {
      const Mit_Chaine *nom = mi_dicho_symb.dic_table[mil].baq_nom;
      assert (nom != NULL && nom->mi_type == MiTy_Chaine);
      int cmp = strcmp (ch, nom->mi_str);
      if (cmp == 0)
	return mi_trouver_symbole_baquet (mi_dicho_symb.dic_table + mil, ind);
    }
  return NULL;
}				// fin mi_trouver_symbole_chaine
