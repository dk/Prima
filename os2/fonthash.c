/*-
 * Copyright (c) 1997-2002 The Protein Laboratory, University of Copenhagen
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Id$
 */
/* Created by Anton Berezin <tobez@plab.ku.dk> */
#include "os2/os2guts.h"

#define FONTHASH_SIZE 563
#define FONTIDHASH_SIZE 23

typedef struct _FontInfo {
   Font font;
   int  vectored;
   FONTMETRICS fm;
} FontInfo;

typedef struct _FontHashNode
{
   struct _FontHashNode *next;
   struct _FontHashNode *next2;
   Font key;
   FontInfo value;
} FontHashNode, *PFontHashNode;

typedef struct _FontHash
{
   PFontHashNode buckets[ FONTHASH_SIZE];
} FontHash, *PFontHash;

static FontHash fontHash;
static FontHash fontHashBySize;

static unsigned long
elf_hash( const char *name, int size)
{
   unsigned long   h = 0, g;

   while ( size)
   {
      h = ( h << 4) + *name++;
      if (( g = h & 0xF0000000))
         h ^= g >> 24;
      h &= ~g;
      size--;
   }
   while ( *name)
   {
      h = ( h << 4) + *name++;
      if (( g = h & 0xF0000000))
         h ^= g >> 24;
      h &= ~g;
   }
   return h;
}

static unsigned long
elf_hash_by_size( const Font *f)
{
   unsigned long   h = 0, g;
   char *name = (char *)&(f-> width);
   int size = (char *)(&(f-> name)) - (char *)name;

   while ( size)
   {
      h = ( h << 4) + *name++;
      if (( g = h & 0xF0000000))
         h ^= g >> 24;
      h &= ~g;
      size--;
   }
   while ( *name)
   {
      h = ( h << 4) + *name++;
      if (( g = h & 0xF0000000))
         h ^= g >> 24;
      h &= ~g;
   }
   name = (char *)&(f-> size);
   size = sizeof( f-> size);
   while ( size)
   {
      h = ( h << 4) + *name++;
      if (( g = h & 0xF0000000))
         h ^= g >> 24;
      h &= ~g;
      size--;
   }
   return h;
}

static PFontHashNode
find_node( const PFont font, Bool bySize)
{
   unsigned long i;
   PFontHashNode node;
   int sz;

   if ( font == nil) return nil;
   if (bySize) {
      sz = (char *)(&(font-> name)) - (char *)&(font-> width);
      i = elf_hash_by_size( font) % FONTHASH_SIZE;
   } else {
      sz = (char *)(&(font-> name)) - (char *)font;
      i = elf_hash((const char *)font, sz) % FONTHASH_SIZE;
   }
   if (bySize)
      node = fontHashBySize. buckets[ i];
   else
      node = fontHash. buckets[ i];
   if ( bySize) {
      while ( node != nil)
      {
         if (( memcmp( &(font-> width), &(node-> key. width), sz) == 0) &&
             ( strcmp( font-> name, node-> key. name) == 0 ) &&
             (font-> size == node-> key. size))
            return node;
         node = node-> next2;
      }
   } else {
      while ( node != nil)
      {
         if (( memcmp( font, &(node-> key), sz) == 0) && ( strcmp( font-> name, node-> key. name) == 0 ))
            return node;
         node = node-> next;
      }
   }
   return nil;
}

Bool
add_font_to_hash( const PFont key, const PFont font, int vectored, PFONTMETRICS fm, Bool addSizeEntry)
{
   PFontHashNode node;
   unsigned long i;
   unsigned long j;
   int sz;

//   if ( find_node( key) != nil)
//      return false ;
   node = malloc( sizeof( FontHashNode));
   if ( node == nil) return false;
   memcpy( &(node-> key), key, sizeof( Font));
   memcpy( &(node-> value. font), font, sizeof( Font));
   node-> value. vectored = vectored;
   memcpy( &(node-> value. fm), fm, sizeof( FONTMETRICS));
   sz = (char *)(&(key-> name)) - (char *)key;
   i = elf_hash((const char *)key, sz) % FONTHASH_SIZE;
   node-> next = fontHash. buckets[ i];
   fontHash. buckets[ i] = node;
   if ( addSizeEntry) {
      j = elf_hash_by_size( key) % FONTHASH_SIZE;
      node-> next2 = fontHashBySize. buckets[ j];
      fontHashBySize. buckets[ j] = node;
   }
   return true;
}

Bool
get_font_from_hash( PFont font, int *vectored, PFONTMETRICS fm, Bool bySize)
{
   PFontHashNode node = find_node( font, bySize);
   if ( node == nil) return false;
   *font = node-> value. font;
   *vectored = node-> value. vectored;
   if ( fm) memcpy( fm, &(node-> value. fm), sizeof(FONTMETRICS));
   return true;
}

Bool
create_font_hash( void)
{
   memset ( &fontHash, 0, sizeof( FontHash));
   memset ( &fontHashBySize, 0, sizeof( FontHash));
   return true;
}

Bool
destroy_font_hash( void)
{
   PFontHashNode node, node2;
   int i;
   for ( i = 0; i < FONTHASH_SIZE; i++)
   {
      node = fontHash. buckets[ i];
      while ( node)
      {
         node2 = node;
         node = node-> next;
         free( node2);
      }
   }
   create_font_hash();     // just in case...
   return true;
}

// Font id hash

typedef struct _FontIdHashNode
{
   struct _FontIdHashNode *next;
   Font key;
   int value;
   SIZEF size;
   int vectored;
} FontIdHashNode, *PFontIdHashNode;

typedef struct _FontIdHash
{
   PFontIdHashNode buckets[ FONTIDHASH_SIZE];
   PFontIdHashNode ids[ 256];
} FontIdHash, *PFontIdHash;

void *
create_fontid_hash( void)
{
   PFontIdHash hash = malloc( sizeof( FontIdHash));
   if ( hash) memset( hash, 0, sizeof( FontIdHash));
   return hash;
}

void
destroy_fontid_hash( void *_hash)
{
   PFontIdHash hash = _hash;
   int i;

   if ( hash == nil) return;
   for ( i = 0; i < 256; i++)
   {
      free( hash-> ids[ i]);
   }
   free( hash);
}

static PFontIdHashNode
find_id_node( void *_hash, const PFont font)
{
   PFontIdHash hash = _hash;
   PFontIdHashNode node;
   int sz;
   unsigned long i;

   if ( font == nil) return nil;
   sz = (char *)(&(font-> name)) - (char *)font;
   i = elf_hash((const char *)font, sz) % FONTIDHASH_SIZE;
   node = hash-> buckets[ i];
   while ( node != nil)
   {
      if (( memcmp( font, &(node-> key), sz) == 0) && ( strcmp( font-> name, node-> key. name) == 0 ))
         return node;
      node = node-> next;
   }
   return nil;
}

int
get_fontid_from_hash( void *_hash, const PFont font, SIZEF *sz, int *vectored)
{
   PFontIdHash hash = _hash;
   PFontIdHashNode node;
   Font f = *font;
   f. direction = 0;
   node = find_id_node( hash, &f);
   if ( node == nil) return 0;
   *sz = node-> size;
   *vectored = node-> vectored;
   return node-> value;
}

void
add_fontid_to_hash( void *_hash, int id, const PFont font, const SIZEF *sz, int vectored)
{
   PFontIdHash hash = _hash;
   PFontIdHashNode node, node2;
   int size;
   unsigned long i;

   if ( hash == nil || font == nil || id < 1 || id > 254) return;
   size = (char *)(&(font-> name)) - (char *)font;
   if ( hash-> ids[ id])
   {
      // remove existing one...
      i = elf_hash((const char *)&(hash-> ids[ id]-> key), size) % FONTIDHASH_SIZE;
      node = hash-> buckets[ i];  node2 = nil;
      while ( node && node != hash-> ids[ id])
      {
         node2 = node;
         node = node-> next;
      }
      if ( node)
      {
         if ( node2)
            node2-> next = node-> next;
         else
            hash-> buckets[ i] = node-> next;
         free( node);
      }
      hash-> ids[ id] = nil;
   }
   i = elf_hash((const char *)font, size) % FONTIDHASH_SIZE;
   node = malloc( sizeof( FontHashNode));
   if ( node == nil) return;
   memcpy( &(node-> key), font, sizeof( Font));
   node-> key. direction = 0;
   node-> value = id;
   node-> size = *sz;
   node-> vectored = vectored;
   node-> next = hash-> buckets[ i];
   hash-> buckets[ i] = node;
   hash-> ids[ id] = node;
}

