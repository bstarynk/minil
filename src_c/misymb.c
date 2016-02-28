// fichier misymb.c - gestion des symboles
/* la notice de copyright est légalement en anglais */

// (C) 2016 Basile Starynkevitch
//   this file misymb.c is part of Minil
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
  return mi_nom_licite_chaine (nom->mi_car);
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
  const Mit_Chaine *baq_nom;
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

// Trouver un symbole de nom et indice donnés
Mit_Symbole *
mi_trouver_symbole_nom (const Mit_Chaine *nom, unsigned ind)
{
  if (!nom || nom->mi_type != MiTy_Chaine)
    return NULL;
  return mi_trouver_symbole_chaine (nom->mi_car, ind);
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
  unsigned bas = 0, hau = mi_dicho_symb.dic_compte, mil = 0;
  assert (hau <= mi_dicho_symb.dic_taille);
  while (bas + 5 < hau)
    {
      mil = (bas + hau) / 2;
      const Mit_Chaine *nom = mi_dicho_symb.dic_table[mil].baq_nom;
      assert (nom != NULL && nom->mi_type == MiTy_Chaine);
      int cmp = strcmp (ch, nom->mi_car);
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
      int cmp = strcmp (ch, nom->mi_car);
      if (cmp == 0)
        return mi_trouver_symbole_baquet (mi_dicho_symb.dic_table + mil, ind);
    }
  return NULL;
}				// fin mi_trouver_symbole_chaine


Mit_Symbole*mi_trouver_symbole(const char*ch, const char**pfin)
{
  if (pfin) *pfin = NULL;
  if (!ch || !isascii(ch[0])) return NULL;
  const char*pe = strchr(ch, '_');
  if (!pe)
    {
      Mit_Symbole*sy= mi_trouver_symbole_chaine(ch, 0);
      if (pfin) *pfin = ch+strlen(ch);
      return sy;
    }
  // si ça rentre utiliser un tampon sur la pile
  char*fin = NULL;
  unsigned ind = strtol(pe+1, &fin, 10);
  if (fin == pe+1) return NULL;
  char tamp[80];
  char *chn = (pe-ch < (int) sizeof(tamp)-3)?tamp:malloc(2+pe-ch);
  if (!chn)
    MI_FATALPRINTF("pas de place pour tampon de %d octets (%s)",
                   (int)(pe-ch), strerror(errno));
  memset(chn, 0, 2+pe-ch);
  strncpy(chn, ch, pe-ch);
  Mit_Symbole*sy = mi_trouver_symbole_chaine(chn, ind);
  if (chn != tamp) free (chn);
  if (sy && pfin) *pfin = fin;
  return sy;
} /* fin mi_trouver_symbole */



// Créer un symbole de nom et indice donnés
Mit_Symbole *
mi_creer_symbole_nom (const Mit_Chaine *nom, unsigned ind)
{
  if (!nom || nom->mi_type != MiTy_Chaine)
    return NULL;
  if (!mi_nom_licite_chaine (nom->mi_car))
    return NULL;
  return mi_creer_symbole_chaine (nom->mi_car, ind);
}				// fin mi_creer_symbole_nom


static unsigned
mi_hashage_symbole_indice (const Mit_Chaine *nom, unsigned ind)
{
  assert (nom && nom->mi_type == MiTy_Chaine && nom->mi_hash > 0);
  unsigned h = nom->mi_hash ^ ind;
  if (!h)
    h = (nom->mi_hash % 1200697) + (ind % 1500827) + 3;
  assert (h != 0);
  return h;
}


static Mit_Symbole *
mi_allouer_symbole (const Mit_Chaine *nom, unsigned ind)
{
  assert (nom && nom->mi_type == MiTy_Chaine);
  Mit_Symbole *sy = mi_allouer_valeur (MiTy_Symbole, sizeof (Mit_Symbole));
  unsigned h = mi_hashage_symbole_indice (nom, ind);
  sy->mi_nom = nom;
  sy->mi_hash = h;
  return sy;
}				/* fin mi_allouer_symbole */


/// renvoie le baquet pour un nom donné, en l'insérant si besoin
static struct mi_baquet_symbole_st *
mi_inserer_baquet (const char *chnom)
{
  assert (chnom && mi_nom_licite_chaine (chnom));
  unsigned nbaq = mi_dicho_symb.dic_compte;
  int bas = 0, hau = nbaq, mil = 0;
  while (bas + 5 < hau)
    {
      mil = (bas + hau) / 2;
      struct mi_baquet_symbole_st *baqmil = mi_dicho_symb.dic_table + mil;
      assert (baqmil != NULL);
      const Mit_Chaine *milnom = baqmil->baq_nom;
      assert (milnom != NULL && milnom->mi_type == MiTy_Chaine);
      int cmp = strcmp (chnom, milnom->mi_car);
      if (cmp < 0)
        hau = mil;
      else if (cmp > 0)
        bas = mil;
      else
        return baqmil;
    }
  for (mil = bas; mil < hau; mil++)
    {
      struct mi_baquet_symbole_st *baqmil = mi_dicho_symb.dic_table + mil;
      assert (baqmil != NULL);
      const Mit_Chaine *milnom = baqmil->baq_nom;
      assert (milnom != NULL && milnom->mi_type == MiTy_Chaine);
      int cmp = strcmp (chnom, milnom->mi_car);
      if (!cmp)
        return baqmil;
      if (cmp < 0 && (mil + 1 >= hau
                      || strcmp (mi_dicho_symb.dic_table[mil + 1].baq_nom->
                                 mi_car, chnom) > 0))
        break;
    }
  // il faut insérer un nouveau baquet en position mil
  struct mi_baquet_symbole_st *nouvbaq = NULL;
  if (mi_dicho_symb.dic_taille <= nbaq + 2)
    {
      unsigned nouvtail = mi_nombre_premier_apres (5 * nbaq / 4 + 20);
      if (!nouvtail)
        MI_FATALPRINTF ("trop (%d) de baquets de symboles", nbaq);
      struct mi_baquet_symbole_st *nouvtab =
      calloc (nouvtail, sizeof (struct mi_baquet_symbole_st));
      if (!nouvtab)
        MI_FATALPRINTF ("impossible d'allouer %d baquets de symboles (%s)",
                        nouvtail, strerror (errno));
      if (mil > 0)
        memcpy (nouvtab, mi_dicho_symb.dic_table,
                mil * sizeof (struct mi_baquet_symbole_st));
      if ((int) nbaq > mil)
        memcpy (nouvtab + mil + 1, mi_dicho_symb.dic_table + mil,
                (nbaq - mil) * sizeof (struct mi_baquet_symbole_st));
      if (nbaq > 0)
        free (mi_dicho_symb.dic_table);
      mi_dicho_symb.dic_table = nouvtab;
      mi_dicho_symb.dic_taille = nouvtail;
      nouvbaq = nouvtab + mil;
    }
  else				// la table est assez grande pour l'insertion
    {
      for (int ix = (int)nbaq; ix >= mil; ix--)
        mi_dicho_symb.dic_table[ix + 1] = mi_dicho_symb.dic_table[ix];
      nouvbaq = mi_dicho_symb.dic_table + mil;
      memset (nouvbaq, 0, sizeof (struct mi_baquet_symbole_st));
    }
  assert (nouvbaq != NULL);
  nouvbaq->baq_nom = mi_creer_chaine (chnom);
  mi_dicho_symb.dic_compte++;
  return nouvbaq;
}				/* fin mi_inserer_baquet */


static int
mi_indice_symbole_baquet (struct mi_baquet_symbole_st *baq, unsigned ind)
{
  assert (ind > 0);
  assert (baq != NULL);
  unsigned h = mi_hashage_symbole_indice (baq->baq_nom, ind);
  assert (h != 0);
  struct mi_tabhashsymb_secondaire_st *bs = baq->baq_tabsec;
  assert (bs != NULL);
  unsigned t = bs->ths_taille;
  assert (bs->ths_compte < t);
  assert (t > 2);
  unsigned ideb = h % t;
  int pos = -1;
  for (unsigned ix = ideb; ix < t; ix++)
    {
      Mit_Symbole *sy = bs->ths_symboles[ix];
      if (sy == MI_TROU_SYMBOLE)
        {
          if (pos < 0)
            pos = (int) ix;
          continue;
        }
      else if (!sy)
        {
          if (pos < 0)
            pos = (int) ix;
          return pos;
        }
      assert (sy->mi_type == MiTy_Symbole);
      assert (sy->mi_nom == baq->baq_nom);
      if (sy->mi_indice == ind)
        return ix;
    }
  for (unsigned ix = 0; ix < ideb; ix++)
    {
      Mit_Symbole *sy = bs->ths_symboles[ix];
      if (sy == MI_TROU_SYMBOLE)
        {
          if (pos < 0)
            pos = (int) ix;
          continue;
        }
      else if (!sy)
        {
          if (pos < 0)
            pos = (int) ix;
          return pos;
        }
      assert (sy->mi_type == MiTy_Symbole);
      assert (sy->mi_nom == baq->baq_nom);
      if (sy->mi_indice == ind)
        return ix;
    }
  return pos;
}				// fin mi_indice_symbole_baquet



static Mit_Symbole *
mi_creer_symbole_baquet (struct mi_baquet_symbole_st *baq, unsigned ind)
{
  if (!baq)
    return NULL;
  if (!ind)
    {
      Mit_Symbole *sy = baq->baq_symbprim;
      if (!sy)
        sy = baq->baq_symbprim = mi_allouer_symbole (baq->baq_nom, 0);
      assert (sy != NULL && sy != MI_TROU_SYMBOLE);
      assert (sy->mi_nom == baq->baq_nom);
      assert (sy->mi_indice == 0);
      return sy;
    }
  struct mi_tabhashsymb_secondaire_st *th = baq->baq_tabsec;
  if (!th)
    {
      unsigned ta = 5;
      th =
        calloc (1,
                sizeof (struct mi_tabhashsymb_secondaire_st) +
                ta * sizeof (Mit_Symbole *));
      if (!th)
        MI_FATALPRINTF ("échec d'allocation de table secondaire (%s)",
                        strerror (errno));
      th->ths_taille = ta;
      baq->baq_tabsec = th;
    }
  unsigned t = th->ths_taille;
  if (5 * th->ths_compte + 2 >= 4 * t)
    {
      // faut agrandir le baquet
      unsigned nouvtail =
        mi_nombre_premier_apres (4 * th->ths_compte / 3 + t / 32 + 6);
      assert (nouvtail > t);
      struct mi_tabhashsymb_secondaire_st *nouvtab =	//
      calloc (1, sizeof (*nouvtab) + nouvtail * sizeof (Mit_Symbole *));
      if (!nouvtab)
        MI_FATALPRINTF
        ("échec de réallocation de table secondaire de taille %d (%s)",
         nouvtail, strerror (errno));
      nouvtab->ths_taille = nouvtail;
      nouvtab->ths_compte = 0;
      baq->baq_tabsec = nouvtab;
      for (unsigned ix = 0; ix < t; ix++)
        {
          Mit_Symbole *ancsy = th->ths_symboles[ix];
          if (!ancsy || ancsy == MI_TROU_SYMBOLE)
            continue;
          assert (ancsy->mi_type == MiTy_Symbole
                  && ancsy->mi_nom == baq->baq_nom && ancsy->mi_indice > 0);
          int pos = mi_indice_symbole_baquet (baq, ancsy->mi_indice);
          assert (pos >= 0 && pos < (int) nouvtail);
          nouvtab->ths_symboles[pos] = ancsy;
          nouvtab->ths_compte++;
        }
      free (th);
      t = nouvtail;
      th = nouvtab;
    }
  int posy = mi_indice_symbole_baquet (baq, ind);
  assert (posy >= 0 && posy < (int) t);
  Mit_Symbole *sy = th->ths_symboles[posy];
  if (sy == NULL || sy == MI_TROU_SYMBOLE)
    {
      sy = mi_allouer_symbole (baq->baq_nom, ind);
      th->ths_symboles[posy] = sy;
      th->ths_compte++;
    }
  return sy;
}				// fin mi_creer_symbole_baquet






Mit_Symbole *
mi_creer_symbole_chaine (const char *ch, unsigned ind)
{
  if (!mi_nom_licite_chaine (ch))
    return NULL;
  struct mi_baquet_symbole_st *baq = mi_inserer_baquet (ch);
  assert (baq != NULL);
  Mit_Symbole *sy = mi_creer_symbole_baquet (baq, ind);
  return sy;
}				// fin mi_creer_symbole_chaine


int
mi_cmp_symbole (const Mit_Symbole *sy1, const Mit_Symbole *sy2)
{
  if (sy1 == sy2)
    return 0;
  if (!sy1 || sy1->mi_type != MiTy_Symbole)
    return -1;
  if (!sy2 || sy2->mi_type != MiTy_Symbole)
    return 1;
  const Mit_Chaine *nom1 = sy1->mi_nom;
  const Mit_Chaine *nom2 = sy2->mi_nom;
  assert (nom1 && nom1->mi_type == MiTy_Chaine);
  assert (nom2 && nom2->mi_type == MiTy_Chaine);
  if (nom1 != nom2)
    {
      int cmp = strcmp (nom1->mi_car, nom2->mi_car);
      if (cmp < 0)
        return -1;
      else if (cmp > 0)
        return 1;
    }
  if (sy1->mi_indice < sy2->mi_indice)
    return -1;
  if (sy1->mi_indice > sy2->mi_indice)
    return +1;
  MI_FATALPRINTF
  ("symboles corrompus differents mais de même noms %s et indice %u",
   nom1->mi_car, sy1->mi_indice);
}

int
mi_cmp_symboleptr (const void *p1, const void *p2)
{
  assert (p1 != NULL);
  assert (p2 != NULL);
  return mi_cmp_symbole (*(const Mit_Symbole **) p1,
                         *(const Mit_Symbole **) p2);
}



/// itérer sur chaque symbole primaire
void
mi_iterer_symbole_primaire (mi_itersymb_sigt * f, void *client)
{
  if (!f)
    return;
  for (unsigned ix = 0; ix < mi_dicho_symb.dic_compte; ix++)
    {
      struct mi_baquet_symbole_st *baq = mi_dicho_symb.dic_table + ix;
      if (!baq->baq_symbprim)
        continue;
      if ((*f) (baq->baq_symbprim, client))
        return;
    }
}				/* fin mi_iterer_symbole_primaire */


/// itérer sur chaque symbole primaire ou secondaire de nom donné
void
mi_iterer_symbole_nomme (const char *ch, mi_itersymb_sigt * f, void *client)
{
  if (!f || !ch)
    return;
  if (!mi_nom_licite_chaine (ch))
    return;
  struct mi_baquet_symbole_st *baq = NULL;
  // recherche dichotomique
  unsigned bas = 0, hau = mi_dicho_symb.dic_compte, mil = 0;
  assert (hau <= mi_dicho_symb.dic_taille);
  while (bas + 5 < hau)
    {
      mil = (bas + hau) / 2;
      const Mit_Chaine *nom = mi_dicho_symb.dic_table[mil].baq_nom;
      assert (nom != NULL && nom->mi_type == MiTy_Chaine);
      int cmp = strcmp (ch, nom->mi_car);
      if (!cmp)
        {
          baq = mi_dicho_symb.dic_table + mil;
          break;
        }
    };
  if (!baq)
    for (mil = bas; mil < hau; mil++)
      {
        const Mit_Chaine *nom = mi_dicho_symb.dic_table[mil].baq_nom;
        assert (nom != NULL && nom->mi_type == MiTy_Chaine);
        int cmp = strcmp (ch, nom->mi_car);
        if (!cmp)
          {
            baq = mi_dicho_symb.dic_table + mil;
            break;
          }
      }
  if (baq->baq_symbprim)
    if ((*f) (baq->baq_symbprim, client))
      return;
  if (baq->baq_tabsec)
    {
      for (unsigned ix = 0; ix < baq->baq_tabsec->ths_taille; ix++)
        {
          Mit_Symbole *sy = baq->baq_tabsec->ths_symboles[ix];
          if (sy && sy != MI_TROU_SYMBOLE)
            {
              assert (sy->mi_type == MiTy_Symbole);
              if ((*f) (sy, client))
                return;
            }
        }
    }
}				// fin mi_iterer_symbole_nomme
