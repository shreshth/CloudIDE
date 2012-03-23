/*--------------------------------------------------------------------*/
/* dynarray.c                                                         */
/* Author: Bob Dondero                                                */
/*--------------------------------------------------------------------*/

#include "dynarray.h"
#include <assert.h>
#include <stdlib.h>

enum {MIN_PHYS_LENGTH = 2};
enum {GROWTH_FACTOR = 2};

/*--------------------------------------------------------------------*/

/* A DynArray consists of an array, along with its logical and
   physical lengths. */

struct DynArray
{
   int iLength;
   /* The number of elements in the DynArray from the client's
      point of view. */

   int iPhysLength;
   /* The number of elements in the array that underlies the
      DynArray. */

   const void **ppvArray;
   /* The array that underlies the DynArray. */
};

/*--------------------------------------------------------------------*/

#ifndef NDEBUG
static int DynArray_isValid(DynArray_T oDynArray)

/* Check the invariants of oDynArray.  Return 1 (TRUE) iff oDynArray
   is in a valid state. */

{
   if (oDynArray->iLength < 0) return 0;
   if (oDynArray->iPhysLength < MIN_PHYS_LENGTH) return 0;
   if (oDynArray->iLength > oDynArray->iPhysLength) return 0;
   if (oDynArray->ppvArray == NULL) return 0;
   return 1;
}
#endif

/*--------------------------------------------------------------------*/

DynArray_T DynArray_new(int iLength)

/* Return a new DynArray_T object whose length is iLength, or
   NULL if insufficient memory is available. */

{
   DynArray_T oDynArray;

   assert(iLength >= 0);

   oDynArray = (struct DynArray*)malloc(sizeof(struct DynArray));
   if (oDynArray == NULL)
      return NULL;

   oDynArray->iLength = iLength;
   if (iLength > MIN_PHYS_LENGTH)
      oDynArray->iPhysLength = iLength;
   else
      oDynArray->iPhysLength = MIN_PHYS_LENGTH;

   oDynArray->ppvArray =
      (const void**)calloc((size_t)oDynArray->iPhysLength,
                            sizeof(void*));
   if (oDynArray->ppvArray == NULL)
   {
      free(oDynArray);
      return NULL;
   }

   return oDynArray;
}

/*--------------------------------------------------------------------*/

void DynArray_free(DynArray_T oDynArray)

/* Free oDynArray. */

{
   assert(oDynArray != NULL);
   assert(DynArray_isValid(oDynArray));

   free(oDynArray->ppvArray);
   free(oDynArray);
}

/*--------------------------------------------------------------------*/

int DynArray_getLength(DynArray_T oDynArray)

/* Return the length of oDynArray. */

{
   assert(oDynArray != NULL);
   assert(DynArray_isValid(oDynArray));

   return oDynArray->iLength;
}

/*--------------------------------------------------------------------*/

void *DynArray_get(DynArray_T oDynArray, int iIndex)

/* Return the iIndex'th element of oDynArray. */

{
   assert(oDynArray != NULL);
   assert(iIndex >= 0);
   assert(iIndex < oDynArray->iLength);
   assert(DynArray_isValid(oDynArray));

   return (void*)(oDynArray->ppvArray)[iIndex];
}

/*--------------------------------------------------------------------*/

void *DynArray_set(DynArray_T oDynArray, int iIndex,
                  const void *pvElement)

/* Assign pvElement to the iIndex'th element of oDynArray.  Return the
   old element. */

{
   const void *pvOldElement;

   assert(oDynArray != NULL);
   assert(iIndex >= 0);
   assert(iIndex < oDynArray->iLength);
   assert(DynArray_isValid(oDynArray));

   pvOldElement = oDynArray->ppvArray[iIndex];
   oDynArray->ppvArray[iIndex] = pvElement;

   assert(DynArray_isValid(oDynArray));

   return (void*)pvOldElement;
}

/*--------------------------------------------------------------------*/

static int DynArray_grow(DynArray_T oDynArray)

/* Increase the physical length of oDynArray.  Return 1 (TRUE) if
   successful and 0 (FALSE) if insufficient memory is available. */

{
   int iNewLength;
   const void **ppvNewArray;

   assert(oDynArray != NULL);

   iNewLength = oDynArray->iPhysLength * GROWTH_FACTOR;

   ppvNewArray = (const void**)
      realloc(oDynArray->ppvArray, sizeof(void*) * iNewLength);
   if (ppvNewArray == NULL)
      return 0;

   oDynArray->iPhysLength = iNewLength;
   oDynArray->ppvArray = ppvNewArray;
   return 1;
}

/*--------------------------------------------------------------------*/

int DynArray_add(DynArray_T oDynArray, const void *pvElement)

/* Add pvElement to the end of oDynArray, thus incrementing its length.
   Return 1 (TRUE) if successful, or 0 (FALSE) if insufficient memory
   is available. */

{
   assert(oDynArray != NULL);
   assert(DynArray_isValid(oDynArray));

   if (oDynArray->iLength == oDynArray->iPhysLength)
      if (! DynArray_grow(oDynArray))
         return 0;

   oDynArray->ppvArray[oDynArray->iLength] = pvElement;
   oDynArray->iLength++;

   assert(DynArray_isValid(oDynArray));

   return 1;
}

/*--------------------------------------------------------------------*/

int DynArray_addAt(DynArray_T oDynArray, int iIndex,
   const void *pvElement)

/* Add pvElement to oDynArray such that it is the iIndex'th element.
   Return 1 (TRUE) if successful, or 0 (FALSE) if insufficient memory
   is available. */

{
   int i;

   assert(oDynArray != NULL);
   assert(iIndex >= 0);
   assert(iIndex <= oDynArray->iLength);
   assert(DynArray_isValid(oDynArray));

   if (oDynArray->iLength == oDynArray->iPhysLength)
      if (! DynArray_grow(oDynArray))
         return 0;

   for (i = oDynArray->iLength; i > iIndex; i--)
      oDynArray->ppvArray[i] = oDynArray->ppvArray[i-1];

   oDynArray->ppvArray[iIndex] = pvElement;
   oDynArray->iLength++;

   assert(DynArray_isValid(oDynArray));

   return 1;
}

/*--------------------------------------------------------------------*/

void *DynArray_removeAt(DynArray_T oDynArray, int iIndex)

/* Remove and return the iIndex'th element of oDynArray. */

{
   const void *pvOldElement;
   int i;

   assert(oDynArray != NULL);
   assert(iIndex >= 0);
   assert(iIndex < oDynArray->iLength);
   assert(DynArray_isValid(oDynArray));

   pvOldElement = oDynArray->ppvArray[iIndex];

   oDynArray->iLength--;

   for (i = iIndex; i < oDynArray->iLength; i++)
      oDynArray->ppvArray[i] = oDynArray->ppvArray[i+1];

   assert(DynArray_isValid(oDynArray));

   return (void*)pvOldElement;
}

/*--------------------------------------------------------------------*/

void DynArray_toArray(DynArray_T oDynArray, void **ppvArray)

/* Fill ppvArray with the elements of oDynArray.  ppvArray should point
   to an area of memory that is large enough to hold all elements of
   oDynArray. */

{
   int i;

   assert(oDynArray != NULL);
   assert(ppvArray != NULL);
   assert(DynArray_isValid(oDynArray));

   for (i = 0; i < oDynArray->iLength; i++)
      ppvArray[i] = (void*)oDynArray->ppvArray[i];
}

/*--------------------------------------------------------------------*/

void DynArray_map(DynArray_T oDynArray,
   void (*pfApply)(void *pvElement, void *pvExtra),
   const void *pvExtra)

/* Apply function *pfApply to each element of oDynArray, passing
   pvExtra as an extra argument.  That is, for each element pvElement of
   oDynArray, call (*pfApply)(pvElement, pvExtra). */

{
   int i;

   assert(oDynArray != NULL);
   assert(pfApply != NULL);
   assert(DynArray_isValid(oDynArray));

   for (i = 0; i < oDynArray->iLength; i++)
      (*pfApply)((void*)oDynArray->ppvArray[i], (void*)pvExtra);
}

/*--------------------------------------------------------------------*/

static void DynArray_swap(const void *ppvArray[], int iOne, int iTwo)

/* Swap ppvArray[iOne] and ppvArray[iTwo]. */

{
   const void *pvTemp;
   pvTemp = ppvArray[iOne];
   ppvArray[iOne] = ppvArray[iTwo];
   ppvArray[iTwo] = pvTemp;
}

/*--------------------------------------------------------------------*/

static int DynArray_partition(const void *ppvArray[],
   int iLeft, int iRight,
   int (*pfCompare)(const void *pvElement1, const void *pvElement2))

/* Divide ppvArray[iLeft...iRight] into two partitions so elements
   in the first partition are <= elements in the second partition.
   Return the index of the element that marks the partition
   boundary. The sort order is determined by *pfCompare. */

/* This function is a variation of the partition() function from the
   book "Algorithms in C" by Robert Sedgewick. */

{
   int iFirst = iLeft-1;
   int iLast = iRight;

   while (1)
   {
      while ((*pfCompare)(ppvArray[++iFirst], ppvArray[iRight]) < 0)
         ;
      while ((*pfCompare)(ppvArray[iRight], ppvArray[--iLast]) < 0)
         if (iLast == iLeft)
            break;
      if (iFirst >= iLast)
         break;
      DynArray_swap(ppvArray, iFirst, iLast);
   }
   DynArray_swap(ppvArray, iFirst, iRight);
   return iFirst;
}

/*--------------------------------------------------------------------*/

static void DynArray_quicksort(const void *ppvArray[],
   int iLeft, int iRight,
   int (*pfCompare)(const void *pvElement1, const void *pvElement2))

/* Sort ppvArray[iLeft...iRight] in ascending order, as determined
   by *pfCompare. */

/* This function is a variation of the quicksort() function from the
   book "Algorithms in C" by Robert Sedgewick. */

{
   int iMid;
   if (iRight > iLeft)
   {
      iMid = DynArray_partition(ppvArray, iLeft, iRight, pfCompare);
      DynArray_quicksort(ppvArray, iLeft, iMid - 1, pfCompare);
      DynArray_quicksort(ppvArray, iMid + 1, iRight, pfCompare);
   }
}

/*--------------------------------------------------------------------*/

void DynArray_sort(DynArray_T oDynArray,
   int (*pfCompare)(const void *pvElement1, const void *pvElement2))

/* Sort oDynArray in the order determined by *pfCompare.
   *pfCompare should return <0, 0, or >0 depending upon whether
   *pvElement1 is less than, equal to, or greater than *pvElement2,
   respectively. */

{
   assert(oDynArray != NULL);
   assert(pfCompare != NULL);
   assert(DynArray_isValid(oDynArray));

   DynArray_quicksort(oDynArray->ppvArray, 0, oDynArray->iLength-1,
      pfCompare);

   assert(DynArray_isValid(oDynArray));
}

/*--------------------------------------------------------------------*/

int DynArray_search(DynArray_T oDynArray, void *pvSoughtElement,
   int (*pfCompare)(const void *pvElement1, const void *pvElement2))

/* Linear search oDynArray for *pvSoughtElement using *pfCompare to
   determine equality.  Return the index at which *pvSoughtElement
   is found, or -1 if there is no such index.
   *pfCompare should return 0 if *pvElement1 is equal to pvElement2,
   and non-0 otherwise. */

{
   int i;

   assert(oDynArray != NULL);
   assert(pfCompare != NULL);
   assert(DynArray_isValid(oDynArray));

   for (i = 0; i < oDynArray->iLength; i++)
      if ((*pfCompare)(oDynArray->ppvArray[i], pvSoughtElement) == 0)
         return i;
   return -1;
}

/*--------------------------------------------------------------------*/

int DynArray_bsearch(DynArray_T oDynArray, void *pvSoughtElement,
   int (*pfCompare)(const void *pvElement1, const void *pvElement2))

/* Binary search oDynArray for *pvSoughtElement using *pfCompare to
   determine equality.  Return the index at which *pvSoughtElement
   is found, or -1 if there is no such index.
   *pfCompare should return <0, 0, or >0 depending upon whether
   *pvElement1 is less than, equal to, or greater than *pvElement2,
   respectively.  oDynArray should be sorted as determined by
   *pfCompare. */

{
   int iLeft;
   int iRight;
   int iMid;
   int iCompare;

   assert(oDynArray != NULL);
   assert(pfCompare != NULL);
   assert(DynArray_isValid(oDynArray));

   iLeft = 0;
   iRight = oDynArray->iLength - 1;

   while (iLeft <= iRight)
   {
      iMid = (iLeft + iRight) / 2;
      iCompare =
         (*pfCompare)(pvSoughtElement, oDynArray->ppvArray[iMid]);
      if (iCompare == 0)
         return iMid;
      if (iCompare < 0)
         iRight = iMid - 1;
      else
         iLeft = iMid + 1;
   }

   return -1;
}


